#include "World/VortexWorldFeatures.h"

#include "Framework/VortexProbe.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "Combat/VortexHitFx.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Rpg/VortexQuestLog.h"
#include "Creatures/VortexMonster.h"
#include "World/VortexTerrainData.h"
#include "World/VortexTerrainStreamer.h"

namespace
{
    constexpr float FeatureWaterM = 2.05f;

    FLinearColor FeatureHex(const TCHAR* Code, float Darken = 0.f, float Lighten = 0.f)
    {
        FLinearColor C = FLinearColor(FColor::FromHex(Code));   // valores sRGB 0..1, como o Color do Godot
        C = C * (1.f - Darken);
        C = C + (FLinearColor::White - C) * Lighten;
        return FLinearColor::FromSRGBColor(FLinearColor(C.R, C.G, C.B).ToFColor(false));
    }

    /** Blocos em coordenadas do Godot (metros, Y para cima) num nó com posição e giro em Y. */
    struct FBoxes
    {
        FVector Node = FVector::ZeroVector;   // Godot (x, y, z)
        float Turn = 0.f;                      // giro em Y (rad, Godot)
        TArray<FVector> Vertices, Normals;
        TArray<int32> Triangles;
        TArray<FLinearColor> Colors;

        FVector ToUnreal(const FVector& L) const
        {
            const float C = FMath::Cos(Turn), S = FMath::Sin(Turn);
            const float X = Node.X + L.X * C + L.Z * S, Z = Node.Z - L.X * S + L.Z * C;
            return FVector(X, Z, Node.Y + L.Y) * 100.f;
        }
        void Box(const FVector& Center, const FVector& Size, const FLinearColor& Color)
        {
            const FVector H = Size * .5f;
            static const int32 Faces[6][4] = { {0,1,3,2}, {4,6,7,5}, {0,4,5,1}, {2,3,7,6}, {0,2,6,4}, {1,5,7,3} };
            FVector Corner[8];
            for (int32 i = 0; i < 8; ++i)
                Corner[i] = ToUnreal(Center + FVector((i & 4) ? H.X : -H.X, (i & 2) ? H.Y : -H.Y, (i & 1) ? H.Z : -H.Z));
            const FVector Mid = ToUnreal(Center);
            for (const auto& F : Faces)
            {
                const FVector P[4] = { Corner[F[0]], Corner[F[1]], Corner[F[2]], Corner[F[3]] };
                const FVector Out = ((P[0] + P[1] + P[2] + P[3]) * .25f - Mid).GetSafeNormal();
                const int32 Base = Vertices.Num();
                for (const FVector& V : P) { Vertices.Add(V); Normals.Add(Out); Colors.Add(Color); }
                // a frente da Unreal: o giro contrário ao da normal geométrica
                const bool bFlip = FVector::DotProduct(FVector::CrossProduct(P[1] - P[0], P[2] - P[0]), Out) > 0.f;
                if (bFlip) Triangles.Append({ Base, Base + 2, Base + 1, Base, Base + 3, Base + 2 });
                else Triangles.Append({ Base, Base + 1, Base + 2, Base, Base + 2, Base + 3 });
            }
        }
    };

    /** O baú (TreasureChest.Make do Godot) no ponto C do nó de M, de frente para -Z; a tampa vai em LidBoxes
     *  (em coordenadas da dobradiça, que sai em LidHinge, girada LidTurn). Raro: vinho e ouro, com gemas. */
    void ChestBoxes(FBoxes& M, FBoxes& LidBoxes, const FVector& C, bool bRare, FVector& LidHinge, float& LidTurn, bool bBoxes = true)
    {
        const FLinearColor Wood = FeatureHex(bRare ? TEXT("7a2a2a") : TEXT("6e4a2a")), PlankC = FeatureHex(bRare ? TEXT("7a2a2a") : TEXT("6e4a2a"), 0.f, .12f);
        const FLinearColor Band = FeatureHex(bRare ? TEXT("d9aa45") : TEXT("4a4d52")), BandHi = FeatureHex(bRare ? TEXT("f5d77a") : TEXT("8f969e")), Lock = FeatureHex(TEXT("d9aa45"));
        // corpo (girado 180°: a frente para a porta, -Z da cabana); sem bBoxes só a dobradiça (o baú em blocos)
        auto ChestBox = [&](FBoxes& Into, FVector P, const FVector& S, const FLinearColor& Col) { if (!bBoxes) return; P.X = -P.X; P.Z = -P.Z; Into.Box(C + P, S, Col); };
        ChestBox(M, FVector(0.f, .26f, 0.f), FVector(1.f, .5f, .66f), Wood);
        for (int32 i = 0; i < 4; ++i) ChestBox(M, FVector(-.375f + i * .25f, .26f, -.335f), FVector(.22f, .46f, .02f), PlankC);
        for (const float X : { -.42f, .42f }) ChestBox(M, FVector(X, .26f, 0.f), FVector(.07f, .52f, .7f), Band);
        ChestBox(M, FVector(0.f, .02f, 0.f), FVector(1.04f, .05f, .7f), Band * .8f);
        for (const float X : { -.5f, .5f }) for (const float Z : { -.33f, .33f }) ChestBox(M, FVector(X, .26f, Z), FVector(.06f, .5f, .06f), BandHi);
        ChestBox(M, FVector(0.f, .42f, -.345f), FVector(.16f, .16f, .04f), Lock);
        if (bRare) for (const float X : { -.25f, .25f }) ChestBox(M, FVector(X, .3f, -.345f), FVector(.06f, .06f, .03f), FeatureHex(TEXT("4ad0ff")));
        // a tampa é uma peça à parte (gira na dobradiça de trás)
        LidHinge = M.ToUnreal(C + FVector(0.f, .51f, -.33f));
        LidTurn = M.Turn;
        LidBoxes.Node = FVector::ZeroVector;
        LidBoxes.Turn = 0.f;
        auto LidBox = [&](const FVector& P, const FVector& S, const FLinearColor& Col) { if (bBoxes) LidBoxes.Box(FVector(P.X, P.Y, -P.Z), S, Col); };
        LidBox(FVector(0.f, .1f, -.33f), FVector(1.f, .2f, .66f), PlankC);
        LidBox(FVector(0.f, .22f, -.33f), FVector(.94f, .06f, .5f), Wood);
        for (const float X : { -.42f, .42f }) LidBox(FVector(X, .14f, -.33f), FVector(.07f, .22f, .7f), Band);
        LidBox(FVector(0.f, .02f, -.67f), FVector(.16f, .1f, .03f), Lock);
    }
}

AVortexWorldFeatures::AVortexWorldFeatures()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = .5f;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

AVortexWorldFeatures* AVortexWorldFeatures::Find(const UWorld* World)
{
    if (!World) return nullptr;
    if (TActorIterator<AVortexWorldFeatures> It(const_cast<UWorld*>(World)); It) return *It;
    return nullptr;
}

void AVortexWorldFeatures::Tick(float DeltaSeconds)
{
    VORTEX_PROBE("PontesCabanas");
    Super::Tick(DeltaSeconds);
    const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
    if (!Data || !Data->HasWorld()) return;
    TArray<FVector2D> Players;
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        if (const APawn* Pawn = It->Get() ? It->Get()->GetPawn() : nullptr) Players.Add(FVector2D(Pawn->GetActorLocation()));
    if (Players.IsEmpty()) return;
    const TArray<FVortexTerrainData::FFeature>& Features = Data->Features();
    int32 Budget = 2;   // no máximo duas montagens por vez
    for (int32 i = 0; i < Features.Num(); ++i)
    {
        float Nearest = 1e12f;
        for (const FVector2D& P : Players) Nearest = FMath::Min(Nearest, FVector2D::Distance(P, Features[i].At));
        if (Built.Contains(i)) { if (Nearest > DropRange) DropFeature(i); }
        else if (Nearest < BuildRange && Budget > 0) { BuildFeature(i); --Budget; }
    }
    // tampas: a do herói desta máquina abre (animada na hora em que ele abre)
    const APlayerController* Local = GetWorld()->GetFirstPlayerController();
    const APawn* Hero = Local ? Local->GetPawn() : nullptr;
    const UVortexQuestLog* Log = Hero ? Hero->FindComponentByClass<UVortexQuestLog>() : nullptr;
    bool bOpening = false;
    for (FChest& Chest : Chests)
    {
        USceneComponent* Lid = Chest.Lid.Get();
        if (!Lid) continue;
        float Angle = Log && Log->HasFlag(Chest.Id) ? 100.f : 0.f;
        if (Chest.OpenAge >= 0.f)
        {
            // 02/11: 0,7 s, passando um pouco do ponto e assentando (a tampa fica aberta: o diário marca o baú)
            Chest.OpenAge += DeltaSeconds;
            const float U = FMath::Clamp(Chest.OpenAge / .7f, 0.f, 1.f);
            Angle = 100.f * (1.f - FMath::Pow(1.f - U, 3.f)) + 14.f * FMath::Sin(U * PI) * (1.f - U);
            bOpening = true;
            if (U >= 1.f) Chest.OpenAge = -1.f;
        }
        Lid->SetRelativeRotation(FRotator(0.f, Chest.LidYaw, -Angle));
    }
    // abrindo: todo quadro; parado: de meio em meio segundo (as montagens não precisam de mais)
    SetActorTickInterval(bOpening ? 0.f : .5f);
}

void AVortexWorldFeatures::PlayOpen(FName Id)
{
    for (FChest& Chest : Chests)
    {
        if (Chest.Id != Id) continue;
        Chest.OpenAge = 0.f;
        // o brilho do tesouro saindo
        if (UNiagaraSystem* Glow = VortexHitFx::System(TEXT("/Game/FreeParticle_SoftTofu/Niagara/NS_Sparkling_Glow.NS_Sparkling_Glow")))
            UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), Glow, Chest.Location + FVector(0.f, 0.f, 40.f));
    }
    SetActorTickInterval(0.f);
}

bool AVortexWorldFeatures::AddChestModel(FChest& Chest, const FVector& Floor, float Yaw)
{
    // o baú em blocos (Tools/ImportChest.py): o corpo no chão (origem no meio do fundo) e a tampa na dobradiça
    const TCHAR* Suffix = Chest.bRare ? TEXT("_Rare") : TEXT("");
    UStaticMesh* BodyMesh = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Props/Chest/Chest_Body%s/StaticMeshes/Chest_Body%s.Chest_Body%s"), Suffix, Suffix, Suffix));
    UStaticMesh* LidMesh = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Props/Chest/Chest_Lid%s/StaticMeshes/Chest_Lid%s.Chest_Lid%s"), Suffix, Suffix, Suffix));
    if (!BodyMesh || !LidMesh) return false;
    UStaticMeshComponent* Body = NewObject<UStaticMeshComponent>(this);
    Body->SetupAttachment(RootComponent);
    Body->SetUsingAbsoluteLocation(true);
    Body->SetUsingAbsoluteRotation(true);
    Body->SetStaticMesh(BodyMesh);
    Body->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
    Body->RegisterComponent();
    Body->SetWorldLocationAndRotation(Floor, FRotator(0.f, Yaw, 0.f));
    UStaticMeshComponent* Lid = NewObject<UStaticMeshComponent>(this);
    Lid->SetupAttachment(Body);
    Lid->SetStaticMesh(LidMesh);
    Lid->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Lid->RegisterComponent();
    // a dobradiça: a borda de trás, em cima do corpo (a frente do baú é o +Y dele)
    Lid->SetRelativeLocation(FVector(0.f, -32.5f, 47.5f));
    Chest.Body = Body;
    Chest.Lid = Lid;
    Chest.LidYaw = 0.f;
    return true;
}

const AVortexWorldFeatures::FChest* AVortexWorldFeatures::NearestChest(const FVector& At, float Range, const UVortexQuestLog* Log) const
{
    const FChest* Best = nullptr;
    float BestDistance = Range;
    for (const FChest& Chest : Chests)
    {
        if (Log && Log->HasFlag(Chest.Id)) continue;
        const float D = FVector::Dist(Chest.Location, At);
        if (D < BestDistance && FMath::Abs(Chest.Location.Z - At.Z) < 250.f) { Best = &Chest; BestDistance = D; }
    }
    return Best;
}

void AVortexWorldFeatures::DropFeature(int32 Index)
{
    if (TObjectPtr<UProceduralMeshComponent>* Mesh = Built.Find(Index))
    {
        if (*Mesh) (*Mesh)->DestroyComponent();
        Built.Remove(Index);
    }
    for (int32 i = Chests.Num() - 1; i >= 0; --i)
        if (Chests[i].Feature == Index)
        {
            if (USceneComponent* Lid = Chests[i].Lid.Get()) Lid->DestroyComponent();
            if (USceneComponent* Body = Chests[i].Body.Get()) Body->DestroyComponent();   // o baú em blocos tem o corpo à parte
            Chests.RemoveAt(i);
        }
}

void AVortexWorldFeatures::BuildFeature(int32 Index)
{
    const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
    const FVortexTerrainData::FFeature& F = Data->Features()[Index];
    if (!Material) Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Materials/M_FarTerrain.M_FarTerrain"));
    auto Ground = [&](float X, float Z) { return Data->GroundAt(X * 100.f, Z * 100.f) / 100.f; };
    const FVector2D At = F.At / 100.f;   // Godot (x, z) em metros
    FBoxes M;
    FBoxes LidBoxes;
    FVector LidHinge = FVector::ZeroVector;
    float LidTurn = 0.f;
    bool bChestModel = false;
    FVector ChestFloor = FVector::ZeroVector;
    float ChestYaw = 0.f;
    if (F.bBridge)
    {
        // ---- ponte (BuildBridge): tábuas em arco, vigas, postes, corrimão e pilares no rio
        const FVector2D Dir(FMath::Cos(F.Radians), -FMath::Sin(F.Radians));
        const float Half = F.Length / 200.f;
        auto EndY = [&](float Sign)
        {
            for (float T = Half - 2.f; T < Half + 10.f; T += .5f)
            {
                const FVector2D P = At + Dir * T * Sign;
                const float G = Ground(P.X, P.Y);
                if (G > FeatureWaterM + .25f) return G;
            }
            return FeatureWaterM + .4f;
        };
        const float Ya = EndY(-1.f), Yb = EndY(1.f), BaseY = FMath::Min(Ya, Yb);
        const float Peak = FMath::Max(FMath::Max(Ya, Yb), FeatureWaterM + 1.f) + .45f;
        M.Node = FVector(At.X, BaseY, At.Y);
        M.Turn = FMath::Atan2(Dir.X, Dir.Y) - UE_HALF_PI;
        const float Length = Half * 2.f;
        auto Deck = [&](float X)
        {
            const float U = (X + Half) / Length;
            const float Lin = FMath::Lerp(Ya, Yb, U) - BaseY;
            const float Arch = (Peak - BaseY - FMath::Max(Lin, 0.f)) * FMath::Sin(U * UE_PI);
            return Lin + FMath::Max(0.f, Arch);
        };
        const FLinearColor Dark = FeatureHex(TEXT("5a3f28")), Mid = FeatureHex(TEXT("7a5838")), Light = FeatureHex(TEXT("98764b")), Iron = FeatureHex(TEXT("4a4d52")), Plank = FeatureHex(TEXT("8a6a44"));
        const float W = 2.6f;
        const int32 Planks = FMath::CeilToInt(Length / .42f);
        for (int32 i = 0; i <= Planks; ++i)
        {
            const float X = -Half + i * .42f, Y = Deck(X);
            M.Box(FVector(X, Y, 0.f), FVector(.38f, .14f, W), i % 3 == 0 ? Light : i % 3 == 1 ? Mid : Plank);
            if (i % 4 == 0)
            {
                M.Box(FVector(X, Y + .08f, -W * .4f), FVector(.06f, .03f, .06f), Iron);
                M.Box(FVector(X, Y + .08f, W * .4f), FVector(.06f, .03f, .06f), Iron);
            }
        }
        for (float X = -Half; X <= Half; X += .42f)
        {
            const float Y = Deck(X);
            M.Box(FVector(X, Y - .16f, -W * .42f), FVector(.44f, .18f, .2f), Dark);
            M.Box(FVector(X, Y - .16f, W * .42f), FVector(.44f, .18f, .2f), Dark);
        }
        for (float X = -Half + .3f; X <= Half - .2f; X += 2.1f)
        {
            const float Y = Deck(X);
            for (const float Z : { -W * .5f + .08f, W * .5f - .08f })
            {
                M.Box(FVector(X, Y + .55f, Z), FVector(.18f, 1.f, .18f), Dark);
                M.Box(FVector(X, Y + 1.08f, Z), FVector(.24f, .08f, .24f), Light);
            }
        }
        for (float X = -Half; X < Half; X += .42f)
        {
            const float Y = Deck(X) + .95f;
            M.Box(FVector(X + .21f, Y, -W * .5f + .08f), FVector(.44f, .1f, .1f), Mid);
            M.Box(FVector(X + .21f, Y, W * .5f - .08f), FVector(.44f, .1f, .1f), Mid);
        }
        for (const float U : { -.25f, .25f })
        {
            const float X = U * Length, Top = Deck(X) - .2f;
            const float Bottom = Ground(At.X + Dir.X * X, At.Y + Dir.Y * X) - BaseY - .3f;
            if (Top - Bottom < .3f) continue;
            for (const float Z : { -W * .35f, W * .35f }) M.Box(FVector(X, (Top + Bottom) * .5f, Z), FVector(.32f, Top - Bottom, .32f), Dark);
        }
    }
    else
    {
        // ---- cabana abandonada (BuildCabin): alicerce, troncos, telhado com buraco, móveis e o baú
        FRandomStream Rng(F.Index * 7919 + 17);
        // o chão mais baixo dos cantos; no morro, sobe até 80 cm (o alicerce de pedra cobre o vão) para o
        // chão do morro não aparecer pelo assoalho
        float Low = Ground(At.X, At.Y), High = Low;
        for (const FVector2D D : { FVector2D(2.8f, 2.2f), FVector2D(-2.8f, 2.2f), FVector2D(2.8f, -2.2f), FVector2D(-2.8f, -2.2f), FVector2D(1.4f, 0.f), FVector2D(-1.4f, 0.f) })
        {
            const float G = Ground(At.X + D.X, At.Y + D.Y);
            Low = FMath::Min(Low, G); High = FMath::Max(High, G);
        }
        const float Y = Low + FMath::Min(.8f, High - Low);
        M.Node = FVector(At.X, Y, At.Y);
        M.Turn = F.Radians;
        static const TCHAR* Logs[] = { TEXT("6e5034"), TEXT("7b5a3b"), TEXT("5f452d"), TEXT("86643f") };
        const FLinearColor Moss = FeatureHex(TEXT("4f7a3a")), Stone = FeatureHex(TEXT("7c7a72")), Dark = FeatureHex(TEXT("3a2a1c"));
        constexpr float W = 5.2f, D = 4.2f, Hh = 2.4f;
        M.Box(FVector(0.f, -.4f, 0.f), FVector(W + .3f, 1.f, D + .3f), Stone);
        for (float X = -W / 2.f + .25f; X < W / 2.f; X += .5f)
            M.Box(FVector(X, .14f, 0.f), FVector(.46f, .12f, D - .1f), FeatureHex(Logs[int32((X + 9.f) * 3.f) % 4], .15f));
        for (int32 Row = 0; Row < 7; ++Row)
        {
            const float Yy = .35f + Row * .32f;
            const FLinearColor Col = FeatureHex(Logs[(Row + F.Index) % 4]);
            const bool bBroken = Row >= 5 && Rng.FRand() < .35f;
            M.Box(FVector(0.f, Yy, D / 2.f), FVector(W, .3f, .3f), Col);
            if (!bBroken) M.Box(FVector(-W / 2.f, Yy, 0.f), FVector(.3f, .3f, D), Col);
            const bool bWindow = Row == 3 || Row == 4;
            M.Box(FVector(W / 2.f, Yy, bWindow ? D / 4.f + .3f : 0.f), FVector(.3f, .3f, bWindow ? D / 2.f - .6f : D), Col);
            if (bWindow) M.Box(FVector(W / 2.f, Yy, -D / 2.f + .5f), FVector(.3f, .3f, 1.f), Col);
            if (Row < 6)
            {
                M.Box(FVector(-W / 2.f + 1.1f, Yy, -D / 2.f), FVector(2.2f, .3f, .3f), Col);
                M.Box(FVector(W / 2.f - 1.5f, Yy, -D / 2.f), FVector(1.8f, .3f, .3f), Col);
            }
            else M.Box(FVector(0.f, Yy, -D / 2.f), FVector(W, .3f, .3f), Col);
            if (Rng.FRand() < .3f) M.Box(FVector(-W / 2.f - .02f, Yy + .1f, Rng.FRand() * D - D / 2.f), FVector(.32f, .12f, .5f), Moss);
        }
        for (const float Sx : { -1.f, 1.f }) for (const float Sz : { -1.f, 1.f })
            M.Box(FVector(Sx * W / 2.f, Hh / 2.f + .2f, Sz * D / 2.f), FVector(.42f, Hh, .42f), FeatureHex(TEXT("3a2a1c"), 0.f, .15f));
        const int32 Hole = Rng.RandRange(3, 6);
        for (int32 S = 0; S < 8; ++S)
        {
            const float T = S / 7.f, Yy = Hh + .3f + FMath::Min(S, 7 - S) * .26f, Zz = -D / 2.f - .4f + T * (D + .8f);
            for (float X = -W / 2.f - .4f; X < W / 2.f + .4f; X += .6f)
            {
                if (S == Hole && X > -.8f && X < 1.4f) continue;
                M.Box(FVector(X + .3f, Yy, Zz), FVector(.58f, .14f, .62f), (S + int32(X * 2.f)) % 5 == 0 ? Moss : FeatureHex(S % 2 == 0 ? TEXT("5a3a24") : TEXT("4a2e1c")));
            }
        }
        M.Box(FVector(-1.4f, .6f, .8f), FVector(1.2f, .1f, .8f), FeatureHex(TEXT("7b5a3b")));
        M.Box(FVector(-1.9f, .38f, .5f), FVector(.1f, .5f, .1f), Dark);
        M.Box(FVector(-.9f, .3f, 1.1f), FVector(.1f, .4f, .1f), Dark);
        M.Box(FVector(1.8f, .6f, -1.2f), FVector(.7f, .9f, .7f), FeatureHex(TEXT("6e4a2a")));
        M.Box(FVector(1.8f, .4f, -1.2f), FVector(.74f, .08f, .74f), FeatureHex(TEXT("4a4d52")));
        M.Box(FVector(1.8f, .85f, -1.2f), FVector(.74f, .08f, .74f), FeatureHex(TEXT("4a4d52")));
        M.Box(FVector(-2.2f, Hh - .1f, 1.7f), FVector(.5f, .5f, .05f), FeatureHex(TEXT("d9d9d9")));
        // o baú no fundo (TreasureChest.Make), virado para a porta; raro em 15%
        const bool bRare = Rng.FRand() < .15f;
        const FVector C(.4f, .2f, D / 2.f - .75f);
        // 02/11: o baú em blocos (sem as caixas no corpo da cabana), se ele foi importado
        bChestModel = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Vortex/Props/Chest/Chest_Body/StaticMeshes/Chest_Body.Chest_Body")) != nullptr;
        ChestBoxes(M, LidBoxes, C, bRare, LidHinge, LidTurn, !bChestModel);
        ChestFloor = M.ToUnreal(C);
        ChestYaw = -FMath::RadiansToDegrees(LidTurn);
        FChest Chest;
        Chest.Id = FName(*FString::Printf(TEXT("chest:%d"), F.Index));
        Chest.Feature = Index;
        Chest.Region = F.Region;
        Chest.bRare = bRare;
        Chest.Location = M.ToUnreal(C + FVector(0.f, .3f, 0.f));
        Chests.Add(Chest);
    }
    UProceduralMeshComponent* Mesh = NewObject<UProceduralMeshComponent>(this);
    Mesh->SetupAttachment(RootComponent);
    Mesh->SetUsingAbsoluteLocation(true);
    Mesh->SetUsingAbsoluteRotation(true);
    Mesh->bUseComplexAsSimpleCollision = true;
    Mesh->RegisterComponent();
    Mesh->CreateMeshSection_LinearColor(0, M.Vertices, M.Triangles, M.Normals, {}, M.Colors, {}, true);
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Mesh->SetCollisionResponseToAllChannels(ECR_Block);
    if (Material) Mesh->SetMaterial(0, Material);
    Built.Add(Index, Mesh);
    if (!F.bBridge && bChestModel && Chests.Num() && Chests.Last().Feature == Index)
    {
        if (!AddChestModel(Chests.Last(), ChestFloor, ChestYaw)) UE_LOG(LogTemp, Warning, TEXT("VORTEX_CHEST sem o modelo em blocos"));
    }
    else if (!F.bBridge && LidBoxes.Vertices.Num())
    {
        UProceduralMeshComponent* Lid = NewObject<UProceduralMeshComponent>(this);
        Lid->SetupAttachment(Mesh);
        Lid->SetUsingAbsoluteLocation(false);
        Lid->RegisterComponent();
        // a peça em coordenadas da dobradiça (o eixo X da tampa na direção da cabana)
        Lid->SetWorldLocationAndRotation(LidHinge, FRotator(0.f, -FMath::RadiansToDegrees(LidTurn), 0.f));
        Lid->CreateMeshSection_LinearColor(0, LidBoxes.Vertices, LidBoxes.Triangles, LidBoxes.Normals, {}, LidBoxes.Colors, {}, false);
        if (Material) Lid->SetMaterial(0, Material);
        Chests.Last().Lid = Lid;
        Chests.Last().LidYaw = -FMath::RadiansToDegrees(LidTurn);
    }
}

void AVortexWorldFeatures::AddCampChest(FName Id, const FVector& Ground, float Yaw, int32 Region, bool bRare, bool bGuarded)
{
    if (Chests.ContainsByPredicate([Id](const FChest& C) { return C.Id == Id; })) return;
    if (!Material) Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Materials/M_FarTerrain.M_FarTerrain"));
    FBoxes M, LidBoxes;
    M.Node = FVector(Ground.X / 100.f, Ground.Z / 100.f, Ground.Y / 100.f);
    M.Turn = FMath::DegreesToRadians(Yaw);
    FVector LidHinge = FVector::ZeroVector;
    float LidTurn = 0.f;
    // 02/11: o baú em blocos, se ele foi importado (senão as caixas de antes)
    {
        FBoxes Probe, ProbeLid;
        Probe.Node = M.Node;
        Probe.Turn = M.Turn;
        ChestBoxes(Probe, ProbeLid, FVector::ZeroVector, bRare, LidHinge, LidTurn, false);
        FChest Chest;
        Chest.Id = Id;
        Chest.Region = Region;
        Chest.bRare = bRare;
        Chest.bGuarded = bGuarded;
        Chest.Location = M.ToUnreal(FVector(0.f, .3f, 0.f));
        if (AddChestModel(Chest, M.ToUnreal(FVector::ZeroVector), -FMath::RadiansToDegrees(LidTurn)))
        {
            Chests.Add(Chest);
            return;
        }
    }
    ChestBoxes(M, LidBoxes, FVector::ZeroVector, bRare, LidHinge, LidTurn);
    UProceduralMeshComponent* Body = NewObject<UProceduralMeshComponent>(this);
    Body->SetupAttachment(RootComponent);
    Body->SetUsingAbsoluteLocation(true);
    Body->SetUsingAbsoluteRotation(true);
    Body->bUseComplexAsSimpleCollision = true;
    Body->RegisterComponent();
    Body->CreateMeshSection_LinearColor(0, M.Vertices, M.Triangles, M.Normals, {}, M.Colors, {}, true);
    Body->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Body->SetCollisionResponseToAllChannels(ECR_Block);
    if (Material) Body->SetMaterial(0, Material);
    UProceduralMeshComponent* Lid = NewObject<UProceduralMeshComponent>(this);
    Lid->SetupAttachment(Body);
    Lid->RegisterComponent();
    Lid->SetWorldLocationAndRotation(LidHinge, FRotator(0.f, -FMath::RadiansToDegrees(LidTurn), 0.f));
    Lid->CreateMeshSection_LinearColor(0, LidBoxes.Vertices, LidBoxes.Triangles, LidBoxes.Normals, {}, LidBoxes.Colors, {}, false);
    if (Material) Lid->SetMaterial(0, Material);
    FChest Chest;
    Chest.Id = Id;
    Chest.Region = Region;
    Chest.bRare = bRare;
    Chest.bGuarded = bGuarded;
    Chest.Location = M.ToUnreal(FVector(0.f, .3f, 0.f));
    Chest.Lid = Lid;
    Chest.Body = Body;
    Chest.LidYaw = -FMath::RadiansToDegrees(LidTurn);
    Chests.Add(Chest);
}

void AVortexWorldFeatures::RemoveChest(FName Id)
{
    for (int32 i = Chests.Num() - 1; i >= 0; --i)
    {
        if (Chests[i].Id != Id) continue;
        if (USceneComponent* Lid = Chests[i].Lid.Get()) Lid->DestroyComponent();
        if (USceneComponent* Body = Chests[i].Body.Get()) Body->DestroyComponent();
        Chests.RemoveAt(i);
    }
}

int32 AVortexWorldFeatures::GuardsAlive(const UWorld* World, const FVector& At)
{
    int32 Alive = 0;
    if (!World) return 0;
    for (TActorIterator<AActor> It(const_cast<UWorld*>(World)); It; ++It)
    {
        const IVortexCreature* Creature = Cast<IVortexCreature>(*It);
        if (Creature && !Creature->IsCreatureDead() && !Creature->IsPassive() && FVector::Dist2D(It->GetActorLocation(), At) < 2200.f) ++Alive;
    }
    return Alive;
}