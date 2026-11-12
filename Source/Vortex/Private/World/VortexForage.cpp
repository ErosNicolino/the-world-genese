#include "World/VortexForage.h"

#include "Framework/VortexProbe.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/VortexCharacter.h"
#include "GameFramework/GameStateBase.h"
#include "World/VortexCaldera.h"
#include "Framework/VortexGameState.h"
#include "World/VortexTerrainData.h"
#include "World/VortexTerrainStreamer.h"
#include "World/VortexTown.h"

bool UVortexForageSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    const UWorld* World = Cast<UWorld>(Outer);
    return World && World->IsGameWorld();
}

const TArray<FVortexForageSpot>& UVortexForageSubsystem::ChunkSpots(const FIntPoint& Chunk)
{
    if (const TArray<FVortexForageSpot>* Found = Chunks.Find(Chunk)) return *Found;
    // continente: espera o terreno montar as plantas do pedaço (em outra thread) para não travar aqui
    static const TArray<FVortexForageSpot> None;
    const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
    if (Data && !Data->IsScattered(Chunk)) return None;
    TArray<FVortexForageSpot>& Spots = Chunks.Add(Chunk);
    if (!Data || !Data->HasChunk(Chunk)) return Spots;
    FRandomStream Rng(int32(HashCombineFast(GetTypeHash(Chunk.X * 7919), GetTypeHash(Chunk.Y * 104729))));
    const float Size = Data->ChunkSize();
    TArray<FVector2D> Trees;
    if (const TArray<FVortexTerrainProp>* Props = Data->PropsInChunk(Chunk))
        for (const FVortexTerrainProp& Prop : *Props) if (Data->IsTree(Prop.Kind)) Trees.Add(FVector2D(Prop.Location));
    auto Add = [&](FName Kind, FName Item, const TCHAR* Name, const FVector2D& P, float Scale)
    {
        const float Ground = Data->GroundAt(P.X, P.Y);
        float Surface = 0.f;
        if (Ground < Data->WaterLevel + 15.f || Ground < -299.f || Data->IsWaterAt(P.X, P.Y, Surface)) return;
        const FVector At(P.X, P.Y, Ground);
        if (AVortexTown::Contains(At) || FVector::Dist2D(At, FVector(UVortexCalderaSubsystem::CenterX, UVortexCalderaSubsystem::CenterY, 0.f)) < UVortexCalderaSubsystem::ZoneInner) return;
        for (const FVortexForageSpot& Other : Spots) if (FVector::Dist2D(Other.At, At) < 150.f) return;
        FVortexForageSpot Spot;
        Spot.Kind = Kind;
        Spot.Item = Item;
        Spot.Name = Name;
        Spot.At = At;
        Spot.Scale = Scale;
        Spot.Yaw = Rng.FRandRange(0.f, 360.f);
        Spot.Id = FName(*FString::Printf(TEXT("forage:%s:%d:%d"), *Kind.ToString(), FMath::RoundToInt(P.X / 10.f), FMath::RoundToInt(P.Y / 10.f)));
        Spots.Add(Spot);
    };
    // cogumelos ao pé das árvores (uma em quatro), a 1,6-2,6 m do tronco
    for (const FVector2D& Tree : Trees)
    {
        if (Rng.FRand() > .25f) continue;
        const float A = Rng.FRandRange(0.f, 2.f * UE_PI), R = Rng.FRandRange(160.f, 260.f);
        Add(Rng.FRand() < .5f ? FName(TEXT("mushroom_red")) : FName(TEXT("mushroom_brown")), TEXT("mushroom_cap"), TEXT("Cogumelo"),
            Tree + FVector2D(FMath::Cos(A), FMath::Sin(A)) * R, Rng.FRandRange(.8f, 1.1f));
    }
    // moitas de amora no campo aberto (longe das árvores); metade frutas, metade ervas, pela posição
    const int32 Bushes = Rng.FRand() < .45f ? (Rng.FRand() < .3f ? 2 : 1) : 0;
    for (int32 i = 0; i < Bushes; ++i)
    {
        const FVector2D P(Chunk.X * Size + Rng.FRandRange(100.f, Size - 100.f), Chunk.Y * Size + Rng.FRandRange(100.f, Size - 100.f));
        bool bClear = true;
        for (const FVector2D& Tree : Trees) bClear &= FVector2D::Distance(Tree, P) > 220.f;
        if (!bClear) continue;
        const bool bFruit = (((FMath::RoundToInt(P.X * .03f) * 73856093) ^ (FMath::RoundToInt(P.Y * .03f) * 19349663)) & 1) == 0;
        Add(TEXT("bush_berry"), bFruit ? FName(TEXT("frutas_silvestres")) : FName(TEXT("healing_herb")), bFruit ? TEXT("Frutas silvestres") : TEXT("Ervas curativas"), P, Rng.FRandRange(.9f, 1.2f));
    }
    return Spots;
}

bool UVortexForageSubsystem::IsAvailable(FName Id) const
{
    const AGameStateBase* State = GetWorld() ? GetWorld()->GetGameState() : nullptr;
    const double Now = State ? State->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
    if (const double* Until = TakenUntil.Find(Id); Until && Now < *Until) return false;
    // a lista do servidor (quem entrou depois da colheita também sabe)
    if (const AVortexGameState* Shared = Cast<AVortexGameState>(State))
        for (int32 i = 0; i < Shared->TakenForage.Num(); ++i)
            if (Shared->TakenForage[i] == Id && Shared->TakenForageUntil.IsValidIndex(i) && Now < Shared->TakenForageUntil[i]) return false;
    return true;
}

void UVortexForageSubsystem::MarkTaken(FName Id)
{
    const AGameStateBase* State = GetWorld() ? GetWorld()->GetGameState() : nullptr;
    const double Now = State ? State->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
    TakenUntil.Add(Id, Now + Respawn);
    bDirty = true;
    if (AVortexGameState* Shared = Cast<AVortexGameState>(GetWorld()->GetGameState())) Shared->MarkForage(Id, float(Now + Respawn));
}

const FVortexForageSpot* UVortexForageSubsystem::Find(FName Id, const FVector& Near)
{
    const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
    if (!Data) return nullptr;
    const float Size = Data->ChunkSize();
    const FIntPoint C(FMath::FloorToInt(Near.X / Size), FMath::FloorToInt(Near.Y / Size));
    for (int32 dx = -1; dx <= 1; ++dx) for (int32 dy = -1; dy <= 1; ++dy)
        for (const FVortexForageSpot& Spot : ChunkSpots(C + FIntPoint(dx, dy))) if (Spot.Id == Id) return &Spot;
    return nullptr;
}

void UVortexForageSubsystem::Nearby(const FVector& Where, float Radius, TArray<const FVortexForageSpot*>& Out)
{
    const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
    if (!Data) return;
    const float Size = Data->ChunkSize();
    const int32 Span = FMath::CeilToInt(Radius / Size);
    const FIntPoint C(FMath::FloorToInt(Where.X / Size), FMath::FloorToInt(Where.Y / Size));
    for (int32 dx = -Span; dx <= Span; ++dx) for (int32 dy = -Span; dy <= Span; ++dy)
        for (const FVortexForageSpot& Spot : ChunkSpots(C + FIntPoint(dx, dy)))
            if (FVector::Dist2D(Spot.At, Where) < Radius && IsAvailable(Spot.Id)) Out.Add(&Spot);
}

const FVortexForageSpot* UVortexForageSubsystem::Nearest(const FVector& Where, float Range)
{
    TArray<const FVortexForageSpot*> Near;
    Nearby(Where, Range, Near);
    const FVortexForageSpot* Best = nullptr;
    float BestDistance = Range;
    for (const FVortexForageSpot* Spot : Near)
    {
        const float D = FVector::Dist2D(Spot->At, Where);
        if (D < BestDistance && FMath::Abs(Spot->At.Z - Where.Z) < 250.f) { Best = Spot; BestDistance = D; }
    }
    return Best;
}

void UVortexForageSubsystem::Tick(float DeltaTime)
{
    VORTEX_PROBE("Coletaveis");
    UWorld* World = GetWorld();
    if (!World || World->GetNetMode() == NM_DedicatedServer || !AVortexTerrainStreamer::SharedData()) return;
    Clock += DeltaTime;
    if (Clock < .5f && !bDirty) return;   // colhido agora: some na hora
    Clock = 0.f;
    bDirty = false;
    // desenha os de pé até 60 m de quem joga (a moita colhida some; volta sozinha no tempo)
    APlayerController* Local = World->GetFirstPlayerController();
    const APawn* Pawn = Local ? Local->GetPawn() : nullptr;
    if (!Pawn) return;
    if (!Field.IsValid())
    {
        FActorSpawnParameters Params;
        Params.ObjectFlags |= RF_Transient;
        Field = World->SpawnActor<AVortexForageField>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
    }
    TArray<const FVortexForageSpot*> Spots;
    Nearby(Pawn->GetActorLocation(), 6000.f, Spots);
    if (Field.IsValid()) Field->Show(Spots);
}

// ---- desenho -------------------------------------------------------------------------------------

AVortexForageField::AVortexForageField()
{
    PrimaryActorTick.bCanEverTick = false;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

UInstancedStaticMeshComponent* AVortexForageField::KindMesh(FName Kind)
{
    if (TObjectPtr<UInstancedStaticMeshComponent>* Found = Kinds.Find(Kind)) return *Found;
    UInstancedStaticMeshComponent* Mesh = NewObject<UInstancedStaticMeshComponent>(this);
    Mesh->SetupAttachment(RootComponent);
    const FString K = Kind.ToString();
    Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Props/%s/%s/StaticMeshes/%s.%s"), *K, *K, *K, *K)));
    if (UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Props/%s/MI_%s.MI_%s"), *K, *K, *K))) Mesh->SetMaterial(0, Material);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->SetUsingAbsoluteLocation(true);
    Mesh->SetUsingAbsoluteRotation(true);
    Mesh->RegisterComponent();
    Kinds.Add(Kind, Mesh);
    return Mesh;
}

void AVortexForageField::Show(const TArray<const FVortexForageSpot*>& Spots)
{
    TMap<FName, TArray<FTransform>> ByKind;
    for (const FVortexForageSpot* Spot : Spots)
        ByKind.FindOrAdd(Spot->Kind).Add(FTransform(FRotator(0.f, Spot->Yaw, 0.f), Spot->At, FVector(Spot->Scale)));
    for (const TPair<FName, TObjectPtr<UInstancedStaticMeshComponent>>& Pair : Kinds) if (!ByKind.Contains(Pair.Key)) Pair.Value->ClearInstances();
    for (const TPair<FName, TArray<FTransform>>& Pair : ByKind)
    {
        UInstancedStaticMeshComponent* Mesh = KindMesh(Pair.Key);
        if (Mesh->GetInstanceCount() == Pair.Value.Num())
        {
            // o mesmo conjunto (o mais comum): só atualiza se mudou algum
            bool bSame = true;
            for (int32 i = 0; i < Pair.Value.Num() && bSame; ++i)
            {
                FTransform Old;
                Mesh->GetInstanceTransform(i, Old, true);
                bSame = Old.GetLocation().Equals(Pair.Value[i].GetLocation(), 1.f);
            }
            if (bSame) continue;
        }
        Mesh->ClearInstances();
        Mesh->AddInstances(Pair.Value, false, true);
    }
}
