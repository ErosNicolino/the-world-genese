#include "World/VortexFarTerrain.h"

#include "Framework/VortexProbe.h"
#include "Async/Async.h"
#include "Async/ParallelFor.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "ProceduralMeshComponent.h"
#include "World/VortexTerrainData.h"
#include "World/VortexTerrainStreamer.h"

AVortexFarTerrain::AVortexFarTerrain()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = .25f;
    Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Far"));
    SetRootComponent(Mesh);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->SetCastShadow(false);
    Mesh->bAffectDistanceFieldLighting = false;
    Mesh->bAffectDynamicIndirectLighting = false;
    Mesh->SetUsingAbsoluteLocation(true);
}

void AVortexFarTerrain::EndPlay(const EEndPlayReason::Type Reason)
{
    if (Job.IsValid()) Job.Wait();
    Super::EndPlay(Reason);
}

TSharedPtr<AVortexFarTerrain::FFarTile> AVortexFarTerrain::BuildTile(FIntPoint Key)
{
    const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
    TSharedPtr<FFarTile> Out = MakeShared<FFarTile>();
    Out->Key = Key;
    if (!Data) return Out;
    // uma borda a mais de cada lado só para as normais saírem iguais nos dois blocos vizinhos
    const int32 N = TilePoints + 1, P = N + 2;
    const FVector2D Origin(Key.X * TileWorld() - Step, Key.Y * TileWorld() - Step);
    const FLinearColor Sea = FLinearColor::FromSRGBColor(FColor(40, 96, 128)), Ocean = FLinearColor::FromSRGBColor(FColor(18, 52, 92));
    TArray<FVector> Padded;
    TArray<FLinearColor> PaddedColors;
    Padded.SetNumUninitialized(P * P);
    PaddedColors.SetNumUninitialized(P * P);
    for (int32 J = 0; J < P; ++J) for (int32 I = 0; I < P; ++I)
    {
        const float X = Origin.X + I * Step, Y = Origin.Y + J * Step;
        // a menor altura em volta (meio passo): a malha fica sempre por baixo de vales e rios
        float Low = Data->QuickGround(X, Y);
        for (const FVector2D D : { FVector2D(.5f, 0.f), FVector2D(-.5f, 0.f), FVector2D(0.f, .5f), FVector2D(0.f, -.5f) })
            Low = FMath::Min(Low, Data->QuickGround(X + D.X * Step, Y + D.Y * Step));
        float Height;
        FColor Ground;
        Data->SampleCell(FMath::FloorToInt(X / Data->CellSize), FMath::FloorToInt(Y / Data->CellSize), Height, Ground);
        FLinearColor Color;
        float Z;
        if (Low < Data->WaterLevel)
        {
            // água: a lâmina, mais escura no fundo
            Z = Data->WaterSurface - 40.f;
            Color = FMath::Lerp(Sea, Ocean, FMath::Clamp((Data->WaterLevel - Low) / 300.f, 0.f, 1.f));
        }
        else
        {
            Z = Low - Sink;
            Color = FLinearColor::FromSRGBColor(Ground);
        }
        Padded[J * P + I] = FVector(X, Y, Z);
        PaddedColors[J * P + I] = Color;
    }
    Out->Vertices.SetNumUninitialized(N * N);
    Out->Normals.SetNumUninitialized(N * N);
    Out->Colors.SetNumUninitialized(N * N);
    for (int32 J = 0; J < N; ++J) for (int32 I = 0; I < N; ++I)
    {
        auto At = [&](int32 A, int32 B) { return Padded[(B + 1) * P + A + 1]; };
        const FVector Dx = At(I + 1, J) - At(I - 1, J), Dy = At(I, J + 1) - At(I, J - 1);
        const FVector Cross = FVector::CrossProduct(Dx, Dy);
        Out->Vertices[J * N + I] = At(I, J);
        Out->Colors[J * N + I] = PaddedColors[(J + 1) * P + I + 1];
        Out->Normals[J * N + I] = Cross.GetSafeNormal() * (Cross.Z < 0.f ? -1.f : 1.f);
    }
    return Out;
}

void AVortexFarTerrain::Upload(FFarTile& Tile, const FVector2D& HoleCenter)
{
    // triângulos fora do buraco (lá o terreno de verdade está montado)
    const int32 N = TilePoints + 1;
    const float Hole2 = Hole * Hole;
    TArray<int32> Triangles;
    Triangles.Reserve(TilePoints * TilePoints * 6);
    bool bCut = false;
    for (int32 J = 0; J < N - 1; ++J) for (int32 I = 0; I < N - 1; ++I)
    {
        const int32 A = J * N + I, B = A + 1, C = A + N, D = C + 1;
        const FVector2D Mid = (FVector2D(Tile.Vertices[A]) + FVector2D(Tile.Vertices[D])) * .5f;
        if (FVector2D::DistSquared(Mid, HoleCenter) < Hole2) { bCut = true; continue; }
        // frente para cima (o mesmo giro do topo das células)
        Triangles.Append({ A, C, B, B, C, D });
    }
    Tile.bHoled = bCut;
    if (Tile.Section < 0) Tile.Section = FreeSections.Num() ? FreeSections.Pop() : NextSection++;
    Mesh->CreateMeshSection_LinearColor(Tile.Section, Tile.Vertices, Triangles, Tile.Normals, {}, Tile.Colors, {}, false);
    if (Material) Mesh->SetMaterial(Tile.Section, Material);
}

void AVortexFarTerrain::TuneFog()
{
    // com o horizonte de verdade, a névoa pode afinar: mata nítida mais longe e as montanhas azuladas atrás
    if (bFogTuned) return;
    for (TActorIterator<AExponentialHeightFog> It(GetWorld()); It; ++It)
    {
        UExponentialHeightFogComponent* Fog = It->GetComponent();
        // névoa densa que sempre esconde a borda do mundo sendo montado (pedido do jogador, 23/10): limpa até
        // 60 m, fechada antes da borda do terreno detalhado (~108 m) e das árvores que nascem (150 m), no máximo
        // 98,5% (as montanhas distantes ficam só em silhueta). Antes (0,09, a partir de 65 m, e só depois de o
        // horizonte ficar pronto) fechava pouco e, de cima de um morro (a névoa afinava rápido com a altura),
        // dava para ver as árvores e o chão se formando
        float Density = .45f, Start = 6000.f;
        FParse::Value(FCommandLine::Get(), TEXT("VortexFarFogDensity="), Density);
        FParse::Value(FCommandLine::Get(), TEXT("VortexFarFogStart="), Start);
        Fog->SetFogDensity(Density);
        Fog->SetStartDistance(Start);
        // quase igual em qualquer altura (era 0,12: no alto dos morros a névoa sumia)
        Fog->SetFogHeightFalloff(.025f);
        Fog->SetFogMaxOpacity(.985f);
        Fog->SetFogCutoffDistance(Radius + 20000.f);   // a névoa vale até o fim do relevo distante; o céu segue limpo
        UE_LOG(LogTemp, Display, TEXT("VORTEX_FOG %s densidade=%.3f inicio=%.0f queda=%.3f altura=%.0f"), *It->GetName(), Density, Start, Fog->FogHeightFalloff, Fog->GetComponentLocation().Z);
        bFogTuned = true;
        // (todas: com mais de uma no mapa, a Unreal desenha a que ela escolher)
    }
}

void AVortexFarTerrain::Tick(float DeltaSeconds)
{
    VORTEX_PROBE("Horizonte");
    Super::Tick(DeltaSeconds);
    // a névoa densa vale desde o primeiro quadro: esperar o horizonte ficar pronto deixava a névoa fina do mapa
    // justamente enquanto o mundo estava sendo montado
    if (!bFogTuned) TuneFog();
    UWorld* World = GetWorld();
    const APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
    const APawn* Pawn = Player ? Player->GetPawn() : nullptr;
    if (!Pawn) return;
    if (!Material) Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Materials/M_FarTerrain.M_FarTerrain"));
    if (Job.IsValid() && Job.IsReady())
    {
        Ready.Append(Job.Consume());
        Job = {};
    }
    const FVector2D Here(Pawn->GetActorLocation());
    const float Tile = TileWorld();
    const FIntPoint Home(FMath::FloorToInt(Here.X / Tile), FMath::FloorToInt(Here.Y / Tile));
    const int32 Span = FMath::CeilToInt(Radius / Tile);
    auto InRange = [&](const FIntPoint& Key, float Extra)
    {
        // o ponto do bloco mais perto de quem joga
        const FVector2D Near(FMath::Clamp(Here.X, Key.X * Tile, (Key.X + 1) * Tile), FMath::Clamp(Here.Y, Key.Y * Tile, (Key.Y + 1) * Tile));
        return FVector2D::Distance(Near, Here) < Radius + Extra;
    };
    // blocos montados viram seção: tudo de uma vez na entrada, depois poucos por vez
    const int32 Budget = bBuilt ? 4 : 1000;
    for (int32 Count = 0; Ready.Num() && Count < Budget; ++Count)
    {
        const TSharedPtr<FFarTile> Built = Ready[0];
        Ready.RemoveAt(0);
        if (!InRange(Built->Key, Tile)) continue;
        Tiles.Add(Built->Key, Built);
        Upload(*Built, Here);
    }
    if (!bBuilt && Tiles.Num() && !Ready.Num() && !Job.IsValid())
    {
        bBuilt = true;
        HoleAt = Here;
    }
    // o buraco segue o herói: só os blocos que ele toca (agora ou antes) são refeitos
    if (bBuilt && FVector2D::Distance(Here, HoleAt) > 2000.f)
    {
        HoleAt = Here;
        const float Reach = Hole + Step * 2.f;
        for (TPair<FIntPoint, TSharedPtr<FFarTile>>& Pair : Tiles)
        {
            const FIntPoint K = Pair.Key;
            const FVector2D Near(FMath::Clamp(Here.X, K.X * Tile, (K.X + 1) * Tile), FMath::Clamp(Here.Y, K.Y * Tile, (K.Y + 1) * Tile));
            if (Pair.Value->bHoled || FVector2D::Distance(Near, Here) < Reach) Upload(*Pair.Value, Here);
        }
    }
    // longe demais: some (a seção volta para a fila)
    for (auto It = Tiles.CreateIterator(); It; ++It)
    {
        if (InRange(It.Key(), Tile * 1.5f)) continue;
        Mesh->ClearMeshSection(It.Value()->Section);
        FreeSections.Add(It.Value()->Section);
        It.RemoveCurrent();
    }
    // os que faltam são montados em outra thread
    if (!Job.IsValid() && Ready.Num() == 0)
    {
        TArray<FIntPoint> Missing;
        for (int32 DY = -Span; DY <= Span; ++DY) for (int32 DX = -Span; DX <= Span; ++DX)
        {
            const FIntPoint Key = Home + FIntPoint(DX, DY);
            if (!Tiles.Contains(Key) && InRange(Key, 0.f)) Missing.Add(Key);
        }
        if (Missing.Num())
        {
            Job = Async(EAsyncExecution::ThreadPool, [Missing]()
            {
                TArray<TSharedPtr<FFarTile>> Out;
                Out.SetNum(Missing.Num());
                ParallelFor(Missing.Num(), [&](int32 I) { Out[I] = BuildTile(Missing[I]); });
                return Out;
            });
        }
    }

}
