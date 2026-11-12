#include "World/VortexPopulationSubsystem.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Creatures/VortexNpc.h"
#include "Creatures/VortexNpcRoutine.h"
#include "Framework/VortexCharacter.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Persistence/VortexWorldSave.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "TimerManager.h"
#include "World/VortexTerrainStreamer.h"

bool UVortexPopulationSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    const UWorld* World = Cast<UWorld>(Outer);
    return World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

void UVortexPopulationSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    // -VortexNoPopulation: sem NPCs (comparação de custo)
    if (InWorld.GetNetMode() == NM_Client || FParse::Param(FCommandLine::Get(), TEXT("VortexNoPopulation"))) return;
    bool bHasTerrain = false;
    if (TActorIterator<AVortexTerrainStreamer> It(&InWorld); It) { bHasTerrain = true; }
    if (!bHasTerrain || !LoadPosts()) return;
    bActive = true;
    InWorld.GetTimerManager().SetTimer(UpdateHandle, this, &UVortexPopulationSubsystem::Update, UpdateInterval, true, UpdateInterval);
    UE_LOG(LogTemp, Display, TEXT("Vortex population: %d postos, %d no protótipo"), Records.Num(), InPrototypeCount());
}

void UVortexPopulationSubsystem::Deinitialize()
{
    if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(UpdateHandle);
    Super::Deinitialize();
}

bool UVortexPopulationSubsystem::LoadPosts()
{
    const FString Path = FPaths::Combine(FPaths::ProjectDir(), TEXT("Data/Population/npc_posts.json"));
    FString Text;
    TSharedPtr<FJsonObject> Root;
    if (!FFileHelper::LoadFileToString(Text, *Path) ||
        !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Vortex population: não foi possível ler %s"), *Path);
        return false;
    }
    const TSharedPtr<FVortexTerrainData> Terrain = AVortexTerrainStreamer::SharedData();
    const auto ReadPoint = [](const TSharedPtr<FJsonValue>& Value, FVector2D& Out)
    {
        const TArray<TSharedPtr<FJsonValue>>* Pair = nullptr;
        if (!Value.IsValid() || !Value->TryGetArray(Pair) || Pair->Num() != 2) return false;
        Out = FVector2D((*Pair)[0]->AsNumber(), (*Pair)[1]->AsNumber());
        return true;
    };
    const TArray<TSharedPtr<FJsonValue>>* Npcs = nullptr;
    if (!Root->TryGetArrayField(TEXT("npcs"), Npcs)) return false;
    for (const TSharedPtr<FJsonValue>& Value : *Npcs)
    {
        const TSharedPtr<FJsonObject> Npc = Value->AsObject();
        FRecord Record;
        FString Id;
        if (!Npc.IsValid() || !Npc->TryGetStringField(TEXT("id"), Id) || Id.IsEmpty() ||
            !ReadPoint(Npc->TryGetField(TEXT("post_cm")), Record.Post)) continue;
        Record.Id = FName(*Id);
        if (Records.ContainsByPredicate([&Record](const FRecord& Other) { return Other.Id == Record.Id; })) continue;
        Npc->TryGetStringField(TEXT("name"), Record.Name);
        Npc->TryGetStringField(TEXT("title"), Record.Title);
        double Yaw = 0.0;
        Npc->TryGetNumberField(TEXT("yaw"), Yaw);
        Record.Yaw = float(Yaw);
        const TArray<TSharedPtr<FJsonValue>>* Patrol = nullptr;
        if (Npc->TryGetArrayField(TEXT("patrol_cm"), Patrol))
        {
            for (const TSharedPtr<FJsonValue>& Point : *Patrol)
            {
                FVector2D At;
                if (ReadPoint(Point, At)) Record.Patrol.Add(At);
            }
        }
        // aparência: raça, aparência, peças e cores (o catálogo completa o que faltar)
        const TSharedPtr<FJsonObject>* LookJson = nullptr;
        if (Npc->TryGetObjectField(TEXT("look"), LookJson) && LookJson && (*LookJson)->Values.Num() > 0)
        {
            FString Race = TEXT("humano"), Appearance = TEXT("male");
            (*LookJson)->TryGetStringField(TEXT("race"), Race);
            (*LookJson)->TryGetStringField(TEXT("appearance"), Appearance);
            Record.Look.Race = FName(*Race);
            Record.Look.Appearance = FName(*Appearance.ToLower());
            const TSharedPtr<FJsonObject>* Parts = nullptr;
            if ((*LookJson)->TryGetObjectField(TEXT("parts"), Parts))
                for (const TPair<FString, TSharedPtr<FJsonValue>> Part : (*Parts)->Values) Record.Look.Pieces.Add(FName(*Part.Key), FName(*Part.Value->AsString()));
            const TSharedPtr<FJsonObject>* Colors = nullptr;
            if ((*LookJson)->TryGetObjectField(TEXT("colors"), Colors))
                for (const TPair<FString, TSharedPtr<FJsonValue>> Color : (*Colors)->Values) Record.Look.Colors.Add(FName(*Color.Key), FColor::FromHex(Color.Value->AsString()));
            Record.bHasLook = true;
        }
        FString Pet;
        if (Npc->TryGetStringField(TEXT("pet"), Pet) && !Pet.IsEmpty()) Record.Pet = FName(*Pet);
        // postos fora da região exportada (Godot: dunas e Verdalume) ficam só como estado
        Record.bInPrototype = Terrain && Terrain->ContainsWorldXY(Record.Post.X, Record.Post.Y) &&
            Terrain->GroundAt(Record.Post.X, Record.Post.Y) > -299.f;
        Record.State.NpcId = Record.Id;
        Record.State.Location = FVector(Record.Post, 0.f);
        Record.State.Yaw = Record.Yaw;
        Records.Add(MoveTemp(Record));
    }
    // 08/11: os passantes (adultos e crianças sorteados, VortexNpcRoutine): o posto é a porta da casa deles
    if (!FParse::Param(FCommandLine::Get(), TEXT("VortexNoRoutines")))
        for (const VortexNpcRoutine::FExtra& Extra : VortexNpcRoutine::Extras())
        {
            FRecord Record;
            Record.Id = Extra.Id;
            Record.Name = Extra.Name;
            Record.Title = Extra.Title;
            Record.Post = Extra.Home;
            Record.Look = Extra.Look;
            Record.bHasLook = true;
            Record.bChild = Extra.bChild;
            Record.bInPrototype = Terrain && Terrain->ContainsWorldXY(Record.Post.X, Record.Post.Y) && Terrain->GroundAt(Record.Post.X, Record.Post.Y) > -299.f;
            Record.State.NpcId = Record.Id;
            Record.State.Location = FVector(Record.Post, 0.f);
            Records.Add(MoveTemp(Record));
        }
    return Records.Num() > 0;
}

int32 UVortexPopulationSubsystem::InPrototypeCount() const
{
    int32 Count = 0;
    for (const FRecord& Record : Records) Count += Record.bInPrototype ? 1 : 0;
    return Count;
}

int32 UVortexPopulationSubsystem::MaterializedCount() const
{
    int32 Count = 0;
    for (const FRecord& Record : Records) Count += Record.Actor.IsValid() ? 1 : 0;
    return Count;
}

const FVortexNpcState* UVortexPopulationSubsystem::FindState(FName Id) const
{
    const FRecord* Record = Records.FindByPredicate([Id](const FRecord& R) { return R.Id == Id; });
    return Record ? &Record->State : nullptr;
}

AVortexNpc* UVortexPopulationSubsystem::FindActor(FName Id) const
{
    const FRecord* Record = Records.FindByPredicate([Id](const FRecord& R) { return R.Id == Id; });
    return Record ? Record->Actor.Get() : nullptr;
}

void UVortexPopulationSubsystem::Capture(FRecord& Record)
{
    const AVortexNpc* Npc = Record.Actor.Get();
    if (!Npc) return;
    Record.State.Location = Npc->GetActorLocation();
    Record.State.Yaw = Npc->GetActorRotation().Yaw;
    Record.State.PatrolIndex = Npc->GetPatrolIndex();
    Record.State.bPlaced = true;
}

void UVortexPopulationSubsystem::WriteTo(UVortexWorldSave& Save) const
{
    Save.NpcStates.Reset();
    for (const FRecord& Record : Records)
    {
        FVortexNpcState State = Record.State;
        if (const AVortexNpc* Npc = Record.Actor.Get())
        {
            // o ator vivo é a verdade mais recente
            State.Location = Npc->GetActorLocation();
            State.Yaw = Npc->GetActorRotation().Yaw;
            State.PatrolIndex = Npc->GetPatrolIndex();
            State.bPlaced = true;
        }
        Save.NpcStates.Add(State);
    }
    Save.NpcStates.Append(UnknownStates);
}

void UVortexPopulationSubsystem::ReadFrom(const UVortexWorldSave& Save)
{
    UnknownStates.Reset();
    for (const FVortexNpcState& State : Save.NpcStates)
    {
        FRecord* Record = Records.FindByPredicate([&State](const FRecord& R) { return R.Id == State.NpcId; });
        if (!Record) { UnknownStates.Add(State); continue; }
        // um ator já em cena continua; o estado lido vale para a próxima aparição
        if (Record->Actor.IsValid()) continue;
        Record->State = State;
        // 06/11 (a vila nova): o jogo salvo guardava o lugar de antes (os NPCs ficavam dentro das casas novas). Quem
        // não patrulha fica sempre no posto; quem patrulha volta ao posto se o salvo estiver longe da patrulha dele
        bool bNear = false;
        for (const FVector2D& Point : Record->Patrol) bNear |= FVector2D::Distance(FVector2D(State.Location), Point) < 1500.f;
        if (Record->Patrol.IsEmpty() || !bNear)
        {
            Record->State.Location = FVector(Record->Post, 0.f);
            Record->State.Yaw = Record->Yaw;
            Record->State.PatrolIndex = 0;
        }
    }
}

bool UVortexPopulationSubsystem::Materialize(FRecord& Record, AVortexTerrainStreamer* Streamer)
{
    FVector2D At(Record.State.Location);
    float Ground = 0.f;
    if (!Streamer->GroundReadyAt(At.X, At.Y, Ground)) return false;
    // 08/11: com rotina, aparece onde a hora manda (dormindo na porta de casa, sentado no banco, no posto...)
    bool bAtGoal = false;
    static const bool bNoRoutines = FParse::Param(FCommandLine::Get(), TEXT("VortexNoRoutines"));
    VortexNpcRoutine::FGoal Now;
    if (!bNoRoutines && VortexNpcRoutine::Plan(Record.Id, Record.Post, Record.Yaw, VortexNpcRoutine::HourOf(GetWorld()), 0, Now) && !Now.bPatrol)
    {
        float GoalGround = 0.f;
        const FVector2D Spot = Now.bSit ? Now.Stand : Now.At;
        if (Streamer->GroundReadyAt(Spot.X, Spot.Y, GoalGround)) { At = Spot; Ground = GoalGround; bAtGoal = true; Record.State.Yaw = Now.bFaceYaw ? Now.Yaw : Record.State.Yaw; }
    }
    const double Start = FPlatformTime::Seconds();
    const FTransform Pose(FRotator(0.f, Record.State.Yaw, 0.f),
        // 06/11: 30 cm a mais (os lojistas da vila nova ficam no piso das lojas, 20 cm acima da terra): cai no piso
        FVector(At.X, At.Y, Ground + AVortexCharacter::HeroCapsuleHalfHeight + 32.f));
    AVortexNpc* Npc = GetWorld()->SpawnActorDeferred<AVortexNpc>(AVortexNpc::StaticClass(), Pose, nullptr, nullptr,
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
    if (!Npc) return false;
    Npc->InitNpc(Record.Id, Record.Name, Record.Patrol, Record.State.PatrolIndex, Record.State.Yaw);
    if (Record.bHasLook) Npc->InitLook(Record.Look, Record.Pet);
    Npc->InitRoutine(Record.Post, Record.Yaw, Record.bChild, bAtGoal);
    Npc->FinishSpawning(Pose);
    Record.Actor = Npc;
    ++Record.State.Materializations;
    ++Spawned;
    SpawnMsMax = FMath::Max(SpawnMsMax, (FPlatformTime::Seconds() - Start) * 1000.0);
    return true;
}

void UVortexPopulationSubsystem::Update()
{
    const double Start = FPlatformTime::Seconds();
    // 11/11 (auditoria de FPS): com as sombras, os moradores eram metade dos triângulos da praça; a mais de 25 m da câmera
    // a sombra deles não se nota (nesta máquina: a sombra é só desenho)
    if (const APlayerController* Viewer = GetWorld()->GetFirstPlayerController(); Viewer && Viewer->PlayerCameraManager)
    {
        const FVector Eye = Viewer->PlayerCameraManager->GetCameraLocation();
        for (TActorIterator<AVortexNpc> It(GetWorld()); It; ++It)
        {
            const bool bNear = FVector::DistSquared(Eye, It->GetActorLocation()) < FMath::Square(2500.f);
            TInlineComponentArray<USkeletalMeshComponent*> Meshes(*It);
            for (USkeletalMeshComponent* Mesh : Meshes)
                if (Mesh->IsVisible() && Mesh->CastShadow != bNear) Mesh->SetCastShadow(bNear);
        }
    }
    AVortexTerrainStreamer* Streamer = nullptr;
    if (TActorIterator<AVortexTerrainStreamer> It(GetWorld()); It) { Streamer = *It; }
    if (!Streamer || !Streamer->IsReady()) return;
    TArray<FVector2D> Players;
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (const APawn* Pawn = It->Get() ? It->Get()->GetPawn() : nullptr) Players.Add(FVector2D(Pawn->GetActorLocation()));
    }
    const auto Nearest = [&Players](const FVector2D& At)
    {
        float Best = TNumericLimits<float>::Max();
        for (const FVector2D& P : Players) Best = FMath::Min(Best, float(FVector2D::Distance(P, At)));
        return Best;
    };
    int32 Spawns = 0;
    for (FRecord& Record : Records)
    {
        if (Record.Actor.IsValid())
        {
            Capture(Record);
            if (Nearest(FVector2D(Record.State.Location)) > DematerializeRadius)
            {
                Record.Actor->Destroy();
                Record.Actor.Reset();
                ++Despawned;
            }
            continue;
        }
        if (!Record.bInPrototype || Spawns >= SpawnsPerUpdate) continue;
        if (Nearest(FVector2D(Record.State.Location)) < MaterializeRadius && Materialize(Record, Streamer)) ++Spawns;
    }
    const double Ms = (FPlatformTime::Seconds() - Start) * 1000.0;
    UpdateMsMax = FMath::Max(UpdateMsMax, Ms);
    UpdateMsTotal += Ms;
    ++Updates;
}
