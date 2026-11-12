#include "World/VortexMist.h"

#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/VortexPlayerSettings.h"
#include "Framework/VortexProbe.h"
#include "GameFramework/PlayerController.h"
#include "World/VortexTerrainData.h"
#include "World/VortexTerrainStreamer.h"
#include "World/VortexTown.h"

AVortexMist::AVortexMist()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = .5f;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void AVortexMist::EndPlay(const EEndPlayReason::Type Reason)
{
    for (TPair<FIntPoint, TWeakObjectPtr<AActor>>& Pair : Patches) if (AActor* Fog = Pair.Value.Get()) Fog->Destroy();
    Patches.Reset();
    Super::EndPlay(Reason);
}

bool AVortexMist::Roll(const FIntPoint& Key, FTransform& Out) const
{
    const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
    if (!Data) return false;
    FRandomStream Rng(HashCombine(GetTypeHash(Key.X * 92821), GetTypeHash(Key.Y - 4513)));
    const float X = (Key.X + .15f + .7f * Rng.FRand()) * Cell, Y = (Key.Y + .15f + .7f * Rng.FRand()) * Cell;
    const float Ground = Data->QuickGround(X, Y);
    if (Ground <= -299.f || !Data->InMap(X, Y) || AVortexTown::Contains(FVector(X, Y, Ground))) return false;
    // vale: o chão mais baixo que a média em volta; água perto também conta
    float Around = 0.f;
    bool bWater = Ground < Data->WaterLevel + 150.f;
    for (int32 i = 0; i < 6; ++i)
    {
        const float A = i * UE_TWO_PI / 6.f;
        const float H = Data->QuickGround(X + FMath::Cos(A) * 4000.f, Y + FMath::Sin(A) * 4000.f);
        Around += H / 6.f;
        bWater |= H < Data->WaterLevel;
    }
    const bool bValley = Ground < Around - 150.f;
    const float Chance = bWater ? .8f : bValley ? .65f : .3f;
    if (Rng.FRand() > Chance) return false;
    // a mancha: larga e baixa, um pouco girada; sobre a água fica rente à lâmina
    const float Size = Rng.FRandRange(32.f, 55.f);
    const float Height = Rng.FRandRange(8.f, 13.f);
    const float Base = bWater ? FMath::Max(Ground, Data->WaterSurface) : Ground;
    Out = FTransform(FRotator(0.f, Rng.FRandRange(0.f, 180.f), 0.f), FVector(X, Y, Base + Height * 40.f), FVector(Size, Size * Rng.FRandRange(.6f, 1.f), Height));
    return true;
}

void AVortexMist::Tick(float DeltaSeconds)
{
    VORTEX_PROBE("Neblina");
    Super::Tick(DeltaSeconds);
    if (!bLoaded)
    {
        bLoaded = true;
        FogClass = LoadClass<AActor>(nullptr, TEXT("/Game/FogArea/Blueprints/BP_FogArea.BP_FogArea_C"));
        if (!FogClass) UE_LOG(LogTemp, Warning, TEXT("VORTEX_MIST sem /Game/FogArea/Blueprints/BP_FogArea"));
    }
    const UVortexPlayerSettings* Options = UVortexPlayerSettings::Get();
    // a parte volumétrica da névoa da cena (as manchas precisam dela; com os shaders ela engrossa). 02/11: a volumétrica
    // não respeita o começo nem o corte da névoa normal (a Unreal avisa: "does not support StartDistance, FogMaxOpacity
    // and FogCutoffDistance") e, a partir da câmera por 220 m, cobria o céu inteiro com os shaders. Agora começa perto
    // de onde a névoa normal começa, entra suave e vai só por uma faixa de ~80 m; mais densa com os shaders
    // só a primeira névoa da cena (o clang do Linux recusa um for que sai sempre na 1ª volta)
    if (TActorIterator<AExponentialHeightFog> It(GetWorld()); It)
    {
        UExponentialHeightFogComponent* Haze = It->GetComponent();
        const float Extinction = Options && Options->bShaderVolumetricFog ? .3f : .1f;
        const float Start = FMath::Max(0.f, Haze->StartDistance * .6f);
        if (!Haze->bEnableVolumetricFog || !FMath::IsNearlyEqual(Haze->VolumetricFogExtinctionScale, Extinction) ||
            !FMath::IsNearlyEqual(Haze->VolumetricFogStartDistance, Start))
        {
            Haze->SetVolumetricFog(true);
            Haze->SetVolumetricFogExtinctionScale(Extinction);
            Haze->SetVolumetricFogStartDistance(Start);
            Haze->SetVolumetricFogNearFadeInDistance(1500.f);
            Haze->SetVolumetricFogDistance(Start + 8000.f);
        }
    }
    // desligada nas opções: tira as manchas
    if (Options && !Options->bValleyMist)
    {
        for (TPair<FIntPoint, TWeakObjectPtr<AActor>>& Pair : Patches) if (AActor* Fog = Pair.Value.Get()) Fog->Destroy();
        Patches.Reset();
        Empty.Reset();
        return;
    }
    const APlayerController* Player = GetWorld()->GetFirstPlayerController();
    const APawn* Pawn = Player ? Player->GetPawn() : nullptr;
    if (!FogClass || !Pawn || !AVortexTerrainStreamer::SharedData()) return;
    const FVector2D Here(Pawn->GetActorLocation());
    // as que ficaram longe somem
    for (auto It = Patches.CreateIterator(); It; ++It)
    {
        const FVector2D Center((It.Key().X + .5f) * Cell, (It.Key().Y + .5f) * Cell);
        if (FVector2D::Distance(Center, Here) < Reach + Cell) continue;
        if (AActor* Fog = It.Value().Get()) Fog->Destroy();
        It.RemoveCurrent();
    }
    for (auto It = Empty.CreateIterator(); It; ++It)
        if (FVector2D::Distance(FVector2D((It->X + .5f) * Cell, (It->Y + .5f) * Cell), Here) > Reach + Cell * 2.f) It.RemoveCurrent();
    // as de perto aparecem (no máximo duas por vez, para não pesar num quadro)
    const int32 Span = FMath::CeilToInt(Reach / Cell);
    const FIntPoint Home(FMath::FloorToInt(Here.X / Cell), FMath::FloorToInt(Here.Y / Cell));
    int32 Spawned = 0;
    for (int32 DY = -Span; DY <= Span && Spawned < 2; ++DY)
    {
        for (int32 DX = -Span; DX <= Span && Spawned < 2; ++DX)
        {
            const FIntPoint Key = Home + FIntPoint(DX, DY);
            if (Patches.Contains(Key) || Empty.Contains(Key)) continue;
            if (FVector2D::Distance(FVector2D((Key.X + .5f) * Cell, (Key.Y + .5f) * Cell), Here) > Reach) continue;
            FTransform Pose;
            if (!Roll(Key, Pose)) { Empty.Add(Key); continue; }
            FActorSpawnParameters Params;
            Params.ObjectFlags |= RF_Transient;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            if (AActor* Fog = GetWorld()->SpawnActor<AActor>(FogClass, Pose, Params))
            {
                Fog->SetActorEnableCollision(false);
                Patches.Add(Key, Fog);
                UE_LOG(LogTemp, Verbose, TEXT("VORTEX_MIST mancha em (%.0f, %.0f, %.0f) escala %s"), Pose.GetLocation().X, Pose.GetLocation().Y, Pose.GetLocation().Z, *Pose.GetScale3D().ToString());
                ++Spawned;
            }
        }
    }
}
