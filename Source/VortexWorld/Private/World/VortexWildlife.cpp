#include "World/VortexWildlife.h"
#include "World/VortexWorldHooks.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "World/VortexDayNightSubsystem.h"
#include "World/VortexTerrainData.h"
#include "World/VortexTerrainStreamer.h"

namespace
{
    // ---- malhas de blocos com cor no vértice (alfa 0 = brilha no M_LifeBlocks)
    struct FLifeMesh
    {
        TArray<FVector3f> P, N;
        TArray<FVector4f> C;
        TArray<int32> T;
        void Box(const FVector3f& Center, const FVector3f& Size, const FLinearColor& Color)
        {
            const FVector3f H = Size * .5f;
            static const FVector3f Dirs[6] = { FVector3f(1, 0, 0), FVector3f(-1, 0, 0), FVector3f(0, 1, 0), FVector3f(0, -1, 0), FVector3f(0, 0, 1), FVector3f(0, 0, -1) };
            for (const FVector3f& Nrm : Dirs)
            {
                const FVector3f A = FMath::Abs(Nrm.Z) > .5f ? FVector3f(1, 0, 0) : FVector3f(0, 0, 1);
                const FVector3f B = FVector3f::CrossProduct(Nrm, A);
                const FVector3f Mid = Center + Nrm * H;
                const FVector3f EA = A * H, EB = B * H;
                const int32 Base = P.Num();
                const FVector3f Q[4] = { Mid - EA - EB, Mid + EA - EB, Mid + EA + EB, Mid - EA + EB };
                for (const FVector3f& V : Q) { P.Add(V); N.Add(Nrm); C.Add(FVector4f(Color.R, Color.G, Color.B, Color.A)); }
                // a Unreal desenha a frente no sentido horário
                const bool bFlip = FVector3f::DotProduct(FVector3f::CrossProduct(Q[1] - Q[0], Q[2] - Q[0]), Nrm) > 0.f;
                if (bFlip) T.Append({ Base, Base + 2, Base + 1, Base, Base + 3, Base + 2 });
                else T.Append({ Base, Base + 1, Base + 2, Base, Base + 2, Base + 3 });
            }
        }
    };

    UStaticMesh* BuildMesh(UObject* Outer, const FLifeMesh& M)
    {
        FMeshDescription Description;
        FStaticMeshAttributes Attributes(Description);
        Attributes.Register();
        TVertexAttributesRef<FVector3f> Points = Attributes.GetVertexPositions();
        TVertexInstanceAttributesRef<FVector3f> Normals = Attributes.GetVertexInstanceNormals();
        TVertexInstanceAttributesRef<FVector4f> Colors = Attributes.GetVertexInstanceColors();
        TVertexInstanceAttributesRef<FVector2f> UVs = Attributes.GetVertexInstanceUVs();
        UVs.SetNumChannels(1);
        const FPolygonGroupID Group = Description.CreatePolygonGroup();
        Attributes.GetPolygonGroupMaterialSlotNames()[Group] = TEXT("Life");
        TArray<FVertexInstanceID> Instances;
        for (int32 i = 0; i < M.P.Num(); ++i)
        {
            const FVertexID Vertex = Description.CreateVertex();
            Points[Vertex] = M.P[i];
            const FVertexInstanceID Instance = Description.CreateVertexInstance(Vertex);
            Normals[Instance] = M.N[i];
            Colors[Instance] = M.C[i];
            UVs.Set(Instance, 0, FVector2f::ZeroVector);
            Instances.Add(Instance);
        }
        for (int32 i = 0; i + 2 < M.T.Num(); i += 3)
            Description.CreateTriangle(Group, { Instances[M.T[i]], Instances[M.T[i + 1]], Instances[M.T[i + 2]] });
        UStaticMesh* Mesh = NewObject<UStaticMesh>(Outer);
        Mesh->GetStaticMaterials().Add(FStaticMaterial(nullptr, TEXT("Life")));
        UStaticMesh::FBuildMeshDescriptionsParams Params;
        Params.bFastBuild = true;
        Params.bBuildSimpleCollision = false;
        Params.bCommitMeshDescription = false;
        Mesh->BuildFromMeshDescriptions({ &Description }, Params);
        // malha feita no código: sem a densidade de UV, o streaming de texturas dispara um ensure (no pacote de medição o
        // jogo travava nele)
        for (FStaticMaterial& Slot : Mesh->GetStaticMaterials()) Slot.UVChannelData = FMeshUVChannelInfo(1.f);
        return Mesh;
    }

    // a montagem da malha grava a cor do vértice em sRGB e o material lê o byte como se fosse linear: para a cor
    // sair como no código, ela entra "linear duas vezes" (sem isso tudo saía lavado, bege-claro)
    FLinearColor Hex(const TCHAR* Code, float Alpha = 1.f)
    {
        const FLinearColor C = FLinearColor(FColor::FromHex(Code));
        return FLinearColor(FMath::Pow(C.R, 2.2f), FMath::Pow(C.G, 2.2f), FMath::Pow(C.B, 2.2f), Alpha);
    }

    // ---- as espécies
    struct FBirdKind
    {
        const TCHAR* Name;
        const TCHAR* Body; const TCHAR* Belly; const TCHAR* Wing; const TCHAR* Tip; const TCHAR* Beak; const TCHAR* Tail;
        float Size, Span, FlapHz;
        bool bGlowTail, bHeron, bSoar, bNight, bForkTail;
        const TCHAR* Call;
    };
    const FBirdKind& BirdKind(int32 Kind)
    {
        static const FBirdKind Kinds[AVortexWildlife::BirdKinds] = {
            { TEXT("Pardal"),                   TEXT("8a6a48"), TEXT("e8dcc0"), TEXT("6a4e34"), TEXT("3a2a1e"), TEXT("e0b060"), TEXT("5a4430"), 1.25f, 15.f, 11.f, false, false, false, false, false, TEXT("bird_chirp") },
            { TEXT("Tiziu-de-brasa"),           TEXT("d8401e"), TEXT("ffa040"), TEXT("2a1a1a"), TEXT("ff7020"), TEXT("2a2020"), TEXT("ff8a20"), 1.2f, 14.f, 12.f, true, false, false, false, false, TEXT("bird_chirp") },
            { TEXT("Andorinha-da-tempestade"),  TEXT("243a6a"), TEXT("eef2f8"), TEXT("1a2848"), TEXT("101828"), TEXT("202020"), TEXT("1a2848"), 1.25f, 18.f, 9.f, false, false, false, false, true, TEXT("bird_chirp") },
            { TEXT("Pardal-da-neve"),           TEXT("e8eef4"), TEXT("ffffff"), TEXT("aab8c8"), TEXT("4a5868"), TEXT("303840"), TEXT("8a98a8"), 1.25f, 15.f, 11.f, false, false, false, false, false, TEXT("bird_chirp") },
            { TEXT("Abutre"),                   TEXT("4a3628"), TEXT("6a5040"), TEXT("3a2a20"), TEXT("1e1610"), TEXT("e8c890"), TEXT("3a2a20"), 2.8f, 30.f, 3.f, false, false, true, false, false, TEXT("") },
            { TEXT("Garça"),                    TEXT("f4f4f0"), TEXT("ffffff"), TEXT("e0e4e8"), TEXT("7a8a98"), TEXT("f0c030"), TEXT("dcdcdc"), 1.9f, 26.f, 4.f, false, true, true, false, false, TEXT("") },
            { TEXT("Corvo"),                    TEXT("1c1c24"), TEXT("2a2a34"), TEXT("181c2c"), TEXT("0c0c12"), TEXT("1a1a1a"), TEXT("14141c"), 1.7f, 20.f, 6.f, false, false, false, true, false, TEXT("bird_crow") },
            { TEXT("Pássaro-lume"),             TEXT("2aa89a"), TEXT("e8fff8"), TEXT("1a7a70"), TEXT("6affe8"), TEXT("f0e060"), TEXT("6affe0"), 1.25f, 15.f, 11.f, true, false, false, false, true, TEXT("bird_chirp") },
            { TEXT("Gaivota"),                  TEXT("f4f6f8"), TEXT("ffffff"), TEXT("a8b0b8"), TEXT("202428"), TEXT("f0c030"), TEXT("e8ecf0"), 1.9f, 26.f, 4.5f, false, false, true, false, false, TEXT("bird_gull") },
        };
        return Kinds[FMath::Clamp(Kind, 0, int32(AVortexWildlife::BirdKinds) - 1)];
    }

    struct FFishKind { const TCHAR* Name; const TCHAR* Back; const TCHAR* Belly; const TCHAR* Fin; const TCHAR* Stripe; float Size; bool bGlowFin, bWhiskers; };
    const FFishKind& FishKind(int32 Kind)
    {
        static const FFishKind Kinds[AVortexWildlife::FishKinds] = {
            { TEXT("Truta"),            TEXT("5a6a3a"), TEXT("e8e0c8"), TEXT("6a7048"), TEXT("e88a8a"), 2.2f, false, false },
            { TEXT("Carpa-dourada"),    TEXT("ff7a1e"), TEXT("ffffff"), TEXT("ffb070"), TEXT("ffffff"), 2.6f, false, false },
            { TEXT("Peixe-gelo"),       TEXT("9ad0f0"), TEXT("ffffff"), TEXT("d0f0ff"), TEXT("5aa8d8"), 2.1f, false, false },
            { TEXT("Bagre-do-lodo"),    TEXT("3a3040"), TEXT("8a7a70"), TEXT("2a2030"), TEXT("5a4a50"), 2.8f, false, true },
            { TEXT("Peixe-lume"),       TEXT("1a7a80"), TEXT("c8fff0"), TEXT("6affe8"), TEXT("6affe8"), 2.1f, true, false },
            { TEXT("Peixe-brasa"),      TEXT("2a1a1a"), TEXT("d84020"), TEXT("ff7a20"), TEXT("ff9a30"), 2.2f, true, false },
            { TEXT("Sardinha"),         TEXT("5a7a98"), TEXT("e8f0f8"), TEXT("a8b8c8"), TEXT("c8d8e8"), 1.5f, false, false },
            { TEXT("Peixe dourado"),    TEXT("ffc828"), TEXT("fff0a0"), TEXT("ffe060"), TEXT("ffffff"), 2.5f, true, false },
        };
        return Kinds[FMath::Clamp(Kind, 0, int32(AVortexWildlife::FishKinds) - 1)];
    }

    // pássaro de ~30 cm (x Size: o pardal fica com ~40 cm, para aparecer no meio do capim) olhando para +X; as asas são malhas à parte, com o ombro na origem
    UStaticMesh* BuildBirdBody(UObject* Outer, const FBirdKind& K)
    {
        FLifeMesh M;
        const FLinearColor Black = Hex(TEXT("101010"));
        if (K.bHeron)
        {
            M.Box({ 0, 0, 0 }, { 22, 10, 11 }, Hex(K.Body));
            M.Box({ 12, 0, 9 }, { 4, 4, 14 }, Hex(K.Body));
            M.Box({ 15, 0, 17 }, { 8, 5, 6 }, Hex(K.Body));
            M.Box({ 15, 0, 20.5f }, { 6, 1, 1.5f }, Hex(K.Tip));
            M.Box({ 23, 0, 16.5f }, { 10, 1.6f, 1.6f }, Hex(K.Beak));
            M.Box({ 17, 2.6f, 18 }, { 1.4f, .4f, 1.4f }, Black);
            M.Box({ 17, -2.6f, 18 }, { 1.4f, .4f, 1.4f }, Black);
            M.Box({ -14, 0, 1 }, { 8, 7, 2 }, Hex(K.Tail));
            for (const float S : { -1.f, 1.f }) M.Box({ 0, S * 2.5f, -16 }, { 1.6f, 1.6f, 22 }, Hex(TEXT("5a5040")));
            return BuildMesh(Outer, M);
        }
        M.Box({ 0, 0, 0 }, { 18, 10, 10 }, Hex(K.Body));
        M.Box({ 2, 0, -3 }, { 14, 9.4f, 5 }, Hex(K.Belly));
        M.Box({ 11, 0, 4 }, { 9, 8, 8 }, Hex(K.Body));
        M.Box({ 17, 0, 3 }, { 5, 2.6f, 2.2f }, Hex(K.Beak));
        M.Box({ 14, 4.1f, 5.5f }, { 1.8f, .4f, 1.8f }, Black);
        M.Box({ 14, -4.1f, 5.5f }, { 1.8f, .4f, 1.8f }, Black);
        // abutre: cabeça pelada rosada e gola
        if (K.bSoar && K.Span >= 30.f) { M.Box({ 12, 0, 4.5f }, { 8, 7, 7 }, Hex(TEXT("d89a8a"))); M.Box({ 7, 0, 3 }, { 3, 11, 9 }, Hex(TEXT("e8dccc"))); }
        const FLinearColor Tail = Hex(K.Tail, K.bGlowTail ? 0.f : 1.f);
        if (K.bForkTail)
        {
            M.Box({ -12, 2, 1 }, { 14, 2.4f, 1.6f }, Tail);
            M.Box({ -12, -2, 1 }, { 14, 2.4f, 1.6f }, Tail);
        }
        else M.Box({ -13, 0, 1 }, { 10, 7, 2 }, Tail);
        for (const float S : { -1.f, 1.f }) M.Box({ 2, S * 2.f, -7.5f }, { 1.2f, 1.2f, 5 }, Hex(TEXT("c09060")));
        return BuildMesh(Outer, M);
    }

    UStaticMesh* BuildBirdWing(UObject* Outer, const FBirdKind& K, float Side)
    {
        FLifeMesh M;
        const float Span = K.Span, Inner = Span * .6f;
        M.Box({ -1.f, Side * Inner * .5f, 0 }, { 12, Inner, 1.6f }, Hex(K.Wing));
        M.Box({ -3.f, Side * (Inner + (Span - Inner) * .5f), 0 }, { 10, Span - Inner, 1.4f }, Hex(K.Tip, K.bGlowTail ? .5f : 1.f));
        return BuildMesh(Outer, M);
    }

    // peixe de ~20 cm (x Size: a truta fica com ~45 cm, para aparecer debaixo da água) olhando para +X; a cauda é à parte, com a junta na origem
    UStaticMesh* BuildFishBody(UObject* Outer, const FFishKind& K)
    {
        FLifeMesh M;
        const float FinGlow = K.bGlowFin ? 0.f : 1.f;
        M.Box({ 0, 0, .6f }, { 16, 5, 6 }, Hex(K.Back));
        M.Box({ .5f, 0, -2.6f }, { 14, 4.6f, 3 }, Hex(K.Belly));
        M.Box({ 9, 0, 0 }, { 5, 4.4f, 6 }, Hex(K.Back));
        M.Box({ 10, 2.3f, 1.2f }, { 1.4f, .4f, 1.4f }, Hex(TEXT("101010")));
        M.Box({ 10, -2.3f, 1.2f }, { 1.4f, .4f, 1.4f }, Hex(TEXT("101010")));
        M.Box({ 0, 2.55f, .2f }, { 12, .3f, 1.4f }, Hex(K.Stripe));
        M.Box({ 0, -2.55f, .2f }, { 12, .3f, 1.4f }, Hex(K.Stripe));
        M.Box({ -1, 0, 4.8f }, { 7, 1, 3 }, Hex(K.Fin, FinGlow));
        M.Box({ 2, 2.8f, -2 }, { 3, 1.4f, .8f }, Hex(K.Fin, FinGlow));
        M.Box({ 2, -2.8f, -2 }, { 3, 1.4f, .8f }, Hex(K.Fin, FinGlow));
        if (K.bWhiskers) for (const float S : { -1.f, 1.f }) M.Box({ 12.5f, S * 2.f, -1.5f }, { 5, .4f, .4f }, Hex(TEXT("2a2020")));
        return BuildMesh(Outer, M);
    }

    UStaticMesh* BuildFishTail(UObject* Outer, const FFishKind& K)
    {
        FLifeMesh M;
        M.Box({ -2, 0, 0 }, { 4, 2.4f, 4 }, Hex(K.Back));
        M.Box({ -6, 0, 0 }, { 5, 1, 9 }, Hex(K.Fin, K.bGlowFin ? 0.f : 1.f));
        return BuildMesh(Outer, M);
    }

    UStaticMesh* BuildPad(UObject* Outer)
    {
        // vitória-régia: disco de blocos com o corte em V
        FLifeMesh M;
        const FLinearColor Green = Hex(TEXT("3e8a3a")), Dark = Hex(TEXT("2e6a2c"));
        for (int32 x = -3; x <= 3; ++x)
            for (int32 y = -3; y <= 3; ++y)
            {
                if (x * x + y * y > 10) continue;
                if (x > 0 && FMath::Abs(y) <= x / 2) continue;   // o corte
                M.Box({ x * 7.f, y * 7.f, 0 }, { 7, 7, 1.2f }, ((x + y) & 1) ? Green : Dark);
            }
        return BuildMesh(Outer, M);
    }

    UStaticMesh* BuildPadFlower(UObject* Outer, bool bGlow)
    {
        FLifeMesh M;
        const FLinearColor Petal = bGlow ? Hex(TEXT("aaffe8"), .3f) : Hex(TEXT("f4a0c8")), Inner = Hex(TEXT("ffe060"), bGlow ? 0.f : 1.f);
        for (int32 i = 0; i < 6; ++i)
        {
            const float A = i * UE_TWO_PI / 6.f;
            M.Box({ FMath::Cos(A) * 4.f, FMath::Sin(A) * 4.f, 3 }, { 4, 4, 4 }, Petal);
        }
        M.Box({ 0, 0, 4.5f }, { 3.5f, 3.5f, 3 }, Inner);
        return BuildMesh(Outer, M);
    }

    UStaticMesh* BuildWeed(UObject* Outer, const TCHAR* A, const TCHAR* B, float Alpha)
    {
        // alga de 100 cm (a escala Z estica): blocos em zigue-zague subindo, com folhinhas
        FLifeMesh M;
        for (int32 i = 0; i < 8; ++i)
        {
            const float Z = i * 12.5f + 6.f;
            const float X = FMath::Sin(i * .9f) * 3.f;
            M.Box({ X, 0, Z }, { 3, 3, 13 }, Hex(i & 1 ? A : B, Alpha));
            if (i % 2 == 0) M.Box({ X + 3.f, 0, Z + 3.f }, { 5, 1.2f, 3 }, Hex(B, Alpha));
        }
        return BuildMesh(Outer, M);
    }

    UStaticMesh* BuildDot(UObject* Outer, const TCHAR* Color, float Alpha)
    {
        FLifeMesh M;
        M.Box({ 0, 0, 0 }, { 2, 2, 2 }, Hex(Color, Alpha));
        return BuildMesh(Outer, M);
    }

    uint32 CellHash(int32 X, int32 Y, uint32 Salt) { return HashCombineFast(HashCombineFast(GetTypeHash(X), GetTypeHash(Y * 92821)), Salt); }

    /** A região do lugar; na água (o mapa das regiões marca lagos e rios como mar) a da terra em volta, até 60 m. */
    int32 LandRegion(const FVortexTerrainData& Data, float X, float Y)
    {
        if (const int32 Region = Data.RegionAt(X, Y)) return Region;
        for (int32 Ring = 1; Ring <= 4; ++Ring)
            for (int32 A = 0; A < 8; ++A)
            {
                const float Angle = A * UE_TWO_PI / 8.f;
                if (const int32 Region = Data.RegionAt(X + FMath::Cos(Angle) * Ring * 1500.f, Y + FMath::Sin(Angle) * Ring * 1500.f)) return Region;
            }
        return 0;
    }

    bool IsMushroom(const FString& Kind, int32& Look)
    {
        if (Kind == TEXT("mushroom_brown")) { Look = 0; return true; }
        if (Kind == TEXT("mushroom_red")) { Look = 1; return true; }
        if (Kind == TEXT("glowcap")) { Look = 2; return true; }
        if (Kind == TEXT("swamp_shroom")) { Look = 3; return true; }
        return false;
    }
}

FString AVortexWildlife::BirdName(int32 Kind) { return BirdKind(Kind).Name; }
FString AVortexWildlife::FishName(int32 Kind) { return FishKind(Kind).Name; }

AVortexWildlife::AVortexWildlife()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;   // depois da câmera andar
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

AVortexWildlife* AVortexWildlife::Find(const UWorld* World)
{
    if (!World) return nullptr;
    if (TActorIterator<AVortexWildlife> It(const_cast<UWorld*>(World)); It) return *It;
    return nullptr;
}

AVortexWildlife::FDraw AVortexWildlife::MakeDraw(UStaticMesh* Mesh, float Cull, bool bShadow)
{
    FDraw Draw;
    if (!Mesh) return Draw;
    Keep.Add(Mesh);
    UInstancedStaticMeshComponent* Ism = NewObject<UInstancedStaticMeshComponent>(this);
    Ism->SetupAttachment(RootComponent);
    Ism->SetStaticMesh(Mesh);
    if (Material) Ism->SetMaterial(0, Material);
    Ism->SetMobility(EComponentMobility::Movable);
    Ism->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Ism->SetCastShadow(bShadow);
    Ism->SetCanEverAffectNavigation(false);
    Ism->bAffectDistanceFieldLighting = false;
    Ism->SetVisibleInRayTracing(false);
    Ism->bVisibleInReflectionCaptures = false;
    Ism->SetCullDistances(0, FMath::RoundToInt(Cull));
    Ism->RegisterComponent();
    Keep.Add(Ism);
    Draw.Ism = Ism;
    return Draw;
}

void AVortexWildlife::Flush(FDraw& Draw, const TArray<FTransform>& Transforms)
{
    UInstancedStaticMeshComponent* Ism = Draw.Ism;
    if (!Ism) return;
    if (Transforms.Num() != Draw.Count)
    {
        Ism->ClearInstances();
        if (Transforms.Num()) Ism->AddInstances(Transforms, false, true);
        Draw.Count = Transforms.Num();
        return;
    }
    if (Transforms.Num()) Ism->BatchUpdateInstancesTransforms(0, Transforms, true, true, true);
}

FVector AVortexWildlife::CameraLocation(FVector* Forward) const
{
    const APlayerController* Controller = GetWorld()->GetFirstPlayerController();
    if (Controller && Controller->PlayerCameraManager)
    {
        if (Forward) *Forward = Controller->PlayerCameraManager->GetCameraRotation().Vector().GetSafeNormal2D();
        return Controller->PlayerCameraManager->GetCameraLocation();
    }
    if (Forward) *Forward = FVector::ForwardVector;
    return FVector::ZeroVector;
}

void AVortexWildlife::BeginPlay()
{
    Super::BeginPlay();
    if (GetNetMode() == NM_DedicatedServer) { Destroy(); return; }
    Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Life/M_LifeBlocks.M_LifeBlocks"));
    for (int32 Kind = 0; Kind < BirdKinds; ++Kind)
    {
        const FBirdKind& K = BirdKind(Kind);
        BirdBody.Add(MakeDraw(BuildBirdBody(this, K), 30000.f, true));
        BirdWingL.Add(MakeDraw(BuildBirdWing(this, K, -1.f), 30000.f, true));
        BirdWingR.Add(MakeDraw(BuildBirdWing(this, K, 1.f), 30000.f, true));
    }
    for (int32 Kind = 0; Kind < FishKinds; ++Kind)
    {
        FishBody.Add(MakeDraw(BuildFishBody(this, FishKind(Kind)), 6000.f, false));
        FishTail.Add(MakeDraw(BuildFishTail(this, FishKind(Kind)), 6000.f, false));
    }
    Pads = MakeDraw(BuildPad(this), 7000.f, false);
    PadFlowers = MakeDraw(BuildPadFlower(this, false), 7000.f, false);
    PadGlowFlowers = MakeDraw(BuildPadFlower(this, true), 7000.f, false);
    WeedGreen = MakeDraw(BuildWeed(this, TEXT("3a7a3a"), TEXT("5aa04a"), 1.f), 5000.f, false);
    WeedDark = MakeDraw(BuildWeed(this, TEXT("3a4a2a"), TEXT("5a5a30"), 1.f), 5000.f, false);
    WeedRed = MakeDraw(BuildWeed(this, TEXT("8a2a2a"), TEXT("c04a30"), 1.f), 5000.f, false);
    WeedGlow = MakeDraw(BuildWeed(this, TEXT("1a7a70"), TEXT("6affe0"), .45f), 5000.f, false);
    // esporos: marrom, vermelho (pólen claro), aceso (azul) e do pântano (verde aceso); respingo branco
    PuffDraw.Add(MakeDraw(BuildDot(this, TEXT("c8a878"), 1.f), 5000.f, false));
    PuffDraw.Add(MakeDraw(BuildDot(this, TEXT("fff0b0"), .7f), 5000.f, false));
    PuffDraw.Add(MakeDraw(BuildDot(this, TEXT("6ad8ff"), 0.f), 5000.f, false));
    PuffDraw.Add(MakeDraw(BuildDot(this, TEXT("9aff5a"), 0.f), 5000.f, false));
    PuffDraw.Add(MakeDraw(BuildDot(this, TEXT("e8f8ff"), .6f), 5000.f, false));
    Flocks.SetNum(6);     // 0-2 no céu, 3-5 no chão
    Schools.SetNum(4);
    Rng.Initialize(int32(FPlatformTime::Cycles() & 0x7fffffff));
}

// ---- onde ficam --------------------------------------------------------------------------------------

uint8 AVortexWildlife::BirdFor(const FVector& At, bool bSky) const
{
    const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
    const int32 Region = Data ? LandRegion(*Data, At.X, At.Y) : 1;
    float Surface = 0.f;
    const bool bWater = Data && Data->IsWaterAt(At.X, At.Y, Surface);
    if (Region == 0 || (bSky && bWater && Rng.FRand() < .6f)) return Gull;
    switch (Region)
    {
    case 2: return EmberFinch;
    case 3: return StormSwallow;
    case 4: return SnowSparrow;
    case 5: return bSky ? Vulture : Sparrow;
    case 7: return bSky ? Crow : Heron;
    case 8: return Crow;
    case 9: return Glowbird;
    default: return Sparrow;
    }
}

uint8 AVortexWildlife::FishFor(const FVector& At) const
{
    const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
    switch (Data ? LandRegion(*Data, At.X, At.Y) : 1)
    {
    case 0: return Sardine;
    case 2: return EmberFish;
    case 4: return IceFish;
    case 5: return Koi;
    case 6: return Rng.FRand() < .5f ? Koi : Trout;
    case 7: case 8: return MudCatfish;
    case 9: return Glowfish;
    default: return Rng.FRand() < .3f ? Koi : Trout;
    }
}

bool AVortexWildlife::FindLand(const FVector& Camera, const FVector& Forward, float MinD, float MaxD, bool bBehind, FVector& Out, bool bShallowOk) const
{
    const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
    if (!Data) return false;
    for (int32 Try = 0; Try < 24; ++Try)
    {
        float Angle = Rng.FRand() * UE_TWO_PI;
        // atrás da câmera: quem chega não aparece do nada na frente
        if (bBehind) Angle = FMath::Atan2(-Forward.Y, -Forward.X) + Rng.FRandRange(-1.4f, 1.4f);
        const float D = Rng.FRandRange(MinD, MaxD);
        const float X = Camera.X + FMath::Cos(Angle) * D, Y = Camera.Y + FMath::Sin(Angle) * D;
        const float Ground = Data->GroundAt(X, Y);
        if (Ground <= -299.f) continue;
        float Surface = 0.f;
        if (Data->IsWaterAt(X, Y, Surface))
        {
            if (!bShallowOk || Surface - Ground > 45.f) continue;
            Out = FVector(X, Y, Surface);
            return true;
        }
        if (bShallowOk) continue;   // a garça procura a água rasa
        Out = FVector(X, Y, Ground);
        return true;
    }
    return false;
}

bool AVortexWildlife::FindWater(const FVector& Camera, float MinD, float MaxD, float MinDepth, FVector& Out, float& Surface, float& Depth) const
{
    const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
    if (!Data) return false;
    // lagoas pequenas ocupam pouco do círculo: muitas tentativas (só contas, sem montar nada)
    for (int32 Try = 0; Try < 60; ++Try)
    {
        const float Angle = Rng.FRand() * UE_TWO_PI, D = Rng.FRandRange(MinD, MaxD);
        const float X = Camera.X + FMath::Cos(Angle) * D, Y = Camera.Y + FMath::Sin(Angle) * D;
        if (!Data->IsWaterAt(X, Y, Surface)) continue;
        Depth = Surface - Data->GroundAt(X, Y);
        if (Depth < MinDepth) continue;
        Out = FVector(X, Y, Surface);
        return true;
    }
    return false;
}

// ---- pássaros ----------------------------------------------------------------------------------------

void AVortexWildlife::SpawnFlock(int32 Index, const FVector& Camera, const FVector& Forward, bool bSky, bool bFirst)
{
    FFlock& F = Flocks[Index];
    Birds.RemoveAll([Index](const FBird& B) { return B.Group == Index; });
    F = FFlock();
    F.bSky = bSky;
    FVector At;
    if (bSky)
    {
        const float Angle = Rng.FRand() * UE_TWO_PI, D = bFirst ? Rng.FRandRange(3000.f, 9000.f) : Rng.FRandRange(9000.f, 12000.f);
        At = Camera + FVector(FMath::Cos(Angle) * D, FMath::Sin(Angle) * D, 0.f);
        F.Kind = BirdFor(At, true);
    }
    else
    {
        // chão seco; onde quase tudo é água (pântano, lagoas) vale a água rasa, com garças
        const bool bLand = FindLand(Camera, Forward, bFirst ? 1500.f : 2600.f, 4200.f, !bFirst, At, false);
        if (!bLand && !FindLand(Camera, Forward, bFirst ? 1500.f : 2600.f, 4500.f, !bFirst, At, true)) { F.Timer = 3.f; return; }
        F.Kind = bLand ? BirdFor(At, false) : uint8(Heron);
        // garça: na água rasa por perto (senão vira pardal no capim)
        if (bLand && F.Kind == Heron && !FindLand(Camera, Forward, bFirst ? 1500.f : 2600.f, 4500.f, !bFirst, At, true)) F.Kind = Sparrow;
    }
    const FBirdKind& K = BirdKind(F.Kind);
    if (IsNight() && !K.bNight) { F.Timer = 5.f; return; }   // de noite só os corvos
    F.bActive = true;
    F.Center = At;
    const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
    const float Ground = Data ? FMath::Max(Data->GroundAt(At.X, At.Y), Data->WaterSurface) : At.Z;
    F.Height = bSky ? (K.bSoar ? Rng.FRandRange(4000.f, 6500.f) : Rng.FRandRange(1600.f, 3200.f)) : 0.f;
    if (bSky) F.Center.Z = Ground + F.Height;
    F.Radius = bSky ? (K.bSoar ? Rng.FRandRange(2200.f, 3500.f) : Rng.FRandRange(900.f, 1800.f)) : 260.f;
    F.Spin = (Rng.FRand() < .5f ? -1.f : 1.f) * (K.bSoar ? 380.f : 650.f) / F.Radius;
    F.Drift = FVector(Rng.FRandRange(-1.f, 1.f), Rng.FRandRange(-1.f, 1.f), 0.f).GetSafeNormal() * Rng.FRandRange(60.f, 160.f);
    F.NextCall = Rng.FRandRange(2.f, 7.f);
    const int32 Count = bSky ? (K.bSoar ? Rng.RandRange(2, 4) : Rng.RandRange(6, 10)) : (K.bSoar ? Rng.RandRange(1, 2) : Rng.RandRange(3, 6));
    for (int32 i = 0; i < Count; ++i)
    {
        FBird B;
        B.Kind = F.Kind;
        B.Group = Index;
        B.Mode = bSky ? 0 : 1;
        B.Slot = i * UE_TWO_PI / Count + Rng.FRandRange(-.25f, .25f);
        B.Flap = Rng.FRand() * 10.f;
        B.Yaw = Rng.FRand() * 360.f;
        B.Timer = Rng.FRandRange(.3f, 2.f);
        if (bSky) B.Pos = F.Center + FVector(FMath::Cos(B.Slot) * F.Radius, FMath::Sin(B.Slot) * F.Radius, Rng.FRandRange(-200.f, 200.f));
        else
        {
            const FVector2D Off = FVector2D(Rng.FRandRange(-1.f, 1.f), Rng.FRandRange(-1.f, 1.f)) * F.Radius;
            B.Pos = FVector(At.X + Off.X, At.Y + Off.Y, 0.f);
            float Surface = 0.f;
            B.Pos.Z = Data && Data->IsWaterAt(B.Pos.X, B.Pos.Y, Surface) ? Surface : (Data ? Data->GroundAt(B.Pos.X, B.Pos.Y) : At.Z);
            B.HopFrom = B.HopTo = B.Pos;
        }
        Birds.Add(B);
    }
}

void AVortexWildlife::UpdateBirds(float Dt, const FVector& Camera, const FVector& Forward)
{
    const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
    for (int32 Index = 0; Index < Flocks.Num(); ++Index)
    {
        FFlock& F = Flocks[Index];
        const bool bSky = Index < 3;
        if (!F.bActive)
        {
            F.Timer -= Dt;
            if (F.Timer <= 0.f) SpawnFlock(Index, Camera, Forward, bSky, false);
            continue;
        }
        const FBirdKind& K = BirdKind(F.Kind);
        if (bSky)
        {
            F.Angle += F.Spin * Dt;
            F.Center += F.Drift * Dt;
            // longe demais: some lá longe e volta outro bando do outro lado
            if (FVector::Dist2D(F.Center, Camera) > 16000.f || (IsNight() && !K.bNight)) { F.bActive = false; F.Timer = Rng.FRandRange(1.f, 6.f); Birds.RemoveAll([Index](const FBird& B) { return B.Group == Index; }); continue; }
        }
        else
        {
            // pousados: alguém chegou perto? todos levantam voo juntos
            if (!F.bFled)
            {
                const float Scare = K.bHeron ? 900.f : 650.f;
                bool bScared = FVector::Dist2D(Camera, F.Center) < 350.f || (IsNight() && !K.bNight);
                for (const FVector& P : Scarers) if (FVector::Dist2D(P, F.Center) < Scare + F.Radius && FMath::Abs(P.Z - F.Center.Z) < 600.f) { bScared = true; break; }
                if (bScared)
                {
                    F.bFled = true;
                    F.Timer = 7.f;
                    VortexWorldHooks::PlaySound(this, TEXT("bird_flutter"), F.Center + FVector(0.f, 0.f, 60.f), K.bHeron ? .5f : .8f, .12f);
                    for (FBird& B : Birds)
                    {
                        if (B.Group != Index) continue;
                        FVector Away = B.Pos - Camera;
                        for (const FVector& P : Scarers) if (FVector::Dist2D(P, B.Pos) < 1200.f) Away = B.Pos - P;
                        Away = Away.GetSafeNormal2D();
                        if (Away.IsNearlyZero()) Away = FVector(1.f, 0.f, 0.f);
                        B.Mode = 2;
                        B.Vel = (Away.RotateAngleAxis(Rng.FRandRange(-35.f, 35.f), FVector::UpVector) * Rng.FRandRange(350.f, 520.f)) + FVector(0.f, 0.f, Rng.FRandRange(380.f, 560.f));
                        B.Timer = Rng.FRandRange(0.f, .25f);   // uns saem um pouquinho depois dos outros
                    }
                }
                else if ((F.NextCall -= Dt) <= 0.f)
                {
                    F.NextCall = Rng.FRandRange(3.f, 8.f);
                    if (*K.Call && FVector::Dist(Camera, F.Center) < 3000.f)
                    {
                        const FString Call = FString(K.Call) == TEXT("bird_chirp") ? FString::Printf(TEXT("bird_chirp_%d"), Rng.RandRange(1, 3)) : FString(K.Call);
                        VortexWorldHooks::PlaySound(this, FName(*Call), F.Center + FVector(0.f, 0.f, 30.f), .45f, .15f);
                    }
                }
            }
            else if ((F.Timer -= Dt) <= 0.f || FVector::Dist2D(Camera, F.Center) > 9000.f)
            {
                // voaram para longe: pousam de novo noutro canto, fora da vista
                F.bActive = false;
                F.Timer = Rng.FRandRange(2.f, 6.f);
                Birds.RemoveAll([Index](const FBird& B) { return B.Group == Index; });
                continue;
            }
            // pousados longe demais (a câmera andou): voltam para perto
            if (!F.bFled && FVector::Dist2D(Camera, F.Center) > 6500.f) { F.bActive = false; F.Timer = 1.f; Birds.RemoveAll([Index](const FBird& B) { return B.Group == Index; }); continue; }
        }
        if (bSky && (F.NextCall -= Dt) <= 0.f)
        {
            F.NextCall = Rng.FRandRange(5.f, 12.f);
            if (*K.Call && FString(K.Call) != TEXT("bird_chirp") && FVector::Dist(Camera, F.Center) < 5000.f) VortexWorldHooks::PlaySound(this, K.Call, F.Center, .5f, .15f);
        }
    }
    for (FBird& B : Birds)
    {
        const FFlock& F = Flocks[B.Group];
        const FBirdKind& K = BirdKind(B.Kind);
        const float Scale = K.Size;
        if (B.Mode == 0)
        {
            const FVector Target = F.Center + FVector(FMath::Cos(F.Angle + B.Slot) * F.Radius, FMath::Sin(F.Angle + B.Slot) * F.Radius, FMath::Sin(Clock * .6f + B.Slot * 2.f) * 180.f);
            const FVector Want = (Target - B.Pos) * 1.4f;
            B.Vel = FMath::VInterpTo(B.Vel, Want.GetClampedToMaxSize(K.bSoar ? 700.f : 1100.f), Dt, 2.5f);
            B.Pos += B.Vel * Dt;
        }
        else if (B.Mode == 2)
        {
            if (B.Timer > 0.f) { B.Timer -= Dt; continue; }
            B.Vel.Z = FMath::Min(B.Vel.Z + 120.f * Dt, 700.f);
            B.Vel += B.Vel.GetSafeNormal2D() * 120.f * Dt;
            B.Pos += B.Vel * Dt;
        }
        else
        {
            // no chão: pula para outro ponto, bica, olha em volta
            B.Timer -= Dt;
            if (B.HopT < 1.f)
            {
                B.HopT = FMath::Min(1.f, B.HopT + Dt / (K.bHeron ? 1.2f : .28f));
                B.Pos = FMath::Lerp(B.HopFrom, B.HopTo, B.HopT) + FVector(0.f, 0.f, K.bHeron ? 0.f : FMath::Sin(B.HopT * PI) * 14.f * Scale);
            }
            else if (B.Timer <= 0.f)
            {
                const float Roll = Rng.FRand();
                if (Roll < .45f)
                {
                    const FVector2D Off = FVector2D(Rng.FRandRange(-1.f, 1.f), Rng.FRandRange(-1.f, 1.f)) * F.Radius;
                    FVector To(F.Center.X + Off.X, F.Center.Y + Off.Y, B.Pos.Z);
                    if ((To - B.Pos).Size2D() > 120.f * Scale) To = B.Pos + (To - B.Pos).GetSafeNormal2D() * 120.f * Scale;
                    float Surface = 0.f;
                    To.Z = Data && Data->IsWaterAt(To.X, To.Y, Surface) ? Surface : (Data ? Data->GroundAt(To.X, To.Y) : B.Pos.Z);
                    if (!K.bHeron && Data && Data->IsWaterAt(To.X, To.Y, Surface)) To = B.Pos;   // passarinho não pula na água
                    B.HopFrom = B.Pos;
                    B.HopTo = To;
                    B.HopT = 0.f;
                    if (!(To - B.Pos).IsNearlyZero()) B.Yaw = (To - B.Pos).Rotation().Yaw;
                    B.Timer = Rng.FRandRange(.4f, 1.6f);
                }
                else if (Roll < .8f) { B.Peck = .35f; B.Timer = Rng.FRandRange(.5f, 1.2f); }
                else { B.Yaw += Rng.FRandRange(-90.f, 90.f); B.Timer = Rng.FRandRange(.6f, 2.f); }
            }
            B.Peck = FMath::Max(0.f, B.Peck - Dt);
        }
        // asas: batendo (no ar), planando (os grandes) ou dobradas (no chão)
        const bool bAir = B.Mode != 1;
        const float Hz = B.Mode == 2 ? K.FlapHz * 1.4f : K.FlapHz;
        const bool bGlide = bAir && B.Mode == 0 && (K.bSoar ? FMath::Sin(Clock * .4f + B.Slot) > -.7f : FMath::Sin(Clock * .9f + B.Slot * 3.f) > .35f);
        if (bAir && !bGlide) B.Flap += Dt * Hz * UE_TWO_PI;
        if (bAir && !B.Vel.IsNearlyZero(1.f))
        {
            const float NewYaw = B.Vel.Rotation().Yaw;
            const float Turn = FMath::FindDeltaAngleDegrees(B.Yaw, NewYaw) / FMath::Max(Dt, .001f);
            B.Roll = FMath::FInterpTo(B.Roll, FMath::Clamp(Turn * .35f, -40.f, 40.f), Dt, 4.f);
            B.Yaw = NewYaw;
        }
    }
}

// ---- peixes ------------------------------------------------------------------------------------------

void AVortexWildlife::SpawnSchool(int32 Index, const FVector& Camera, bool bFirst)
{
    FSchool& S = Schools[Index];
    Fish.RemoveAll([Index](const FFishy& F) { return F.School == Index; });
    S = FSchool();
    FVector At;
    float Surface = 0.f, Depth = 0.f;
    if (!FindWater(Camera, bFirst ? 0.f : 2000.f, 3800.f, 70.f, At, Surface, Depth)) { S.Retarget = 3.f; return; }
    S.bActive = true;
    S.Surface = Surface;
    S.Depth = Depth;
    S.Center = S.Target = FVector(At.X, At.Y, Surface - FMath::Clamp(Depth * .3f, 28.f, 60.f));
    S.NextJump = Rng.FRandRange(4.f, 12.f);
    const uint8 Kind = FishFor(At);
    const int32 Count = Kind == Sardine ? Rng.RandRange(8, 14) : Rng.RandRange(3, 6);
    const bool bGold = Rng.FRand() < .05f;
    for (int32 i = 0; i < Count; ++i)
    {
        FFishy F;
        F.Kind = bGold && i == 0 ? GoldFish : Kind;
        F.School = Index;
        F.Offset = FVector(Rng.FRandRange(-1.f, 1.f), Rng.FRandRange(-1.f, 1.f), Rng.FRandRange(-.3f, .3f)) * FVector(220.f, 220.f, 40.f);
        F.Pos = S.Center + F.Offset;
        F.Phase = Rng.FRand() * 10.f;
        F.Yaw = Rng.FRand() * 360.f;
        Fish.Add(F);
    }
}

void AVortexWildlife::UpdateFish(float Dt, const FVector& Camera)
{
    const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
    if (!Data) return;
    for (int32 Index = 0; Index < Schools.Num(); ++Index)
    {
        FSchool& S = Schools[Index];
        if (!S.bActive)
        {
            S.Retarget -= Dt;
            if (S.Retarget <= 0.f) SpawnSchool(Index, Camera, false);
            continue;
        }
        if (FVector::Dist2D(Camera, S.Center) > 5500.f) { S.bActive = false; S.Retarget = 1.f; Fish.RemoveAll([Index](const FFishy& F) { return F.School == Index; }); continue; }
        S.Rush = FMath::Max(0.f, S.Rush - Dt);
        // alguém perto da água: o cardume dispara para o outro lado
        for (const FVector& P : Scarers)
        {
            if (S.Rush > 0.f || FVector::Dist2D(P, S.Center) > 520.f || P.Z > S.Surface + 400.f) continue;
            const FVector Away = (S.Center - P).GetSafeNormal2D();
            for (const float Turn : { 0.f, 40.f, -40.f, 80.f, -80.f })
            {
                const FVector To = S.Center + Away.RotateAngleAxis(Turn, FVector::UpVector) * 800.f;
                float Surface = 0.f;
                if (Data->IsWaterAt(To.X, To.Y, Surface) && Surface - Data->GroundAt(To.X, To.Y) > 50.f) { S.Target = FVector(To.X, To.Y, S.Center.Z); S.Rush = 1.6f; break; }
            }
        }
        S.Retarget -= Dt;
        if (S.Retarget <= 0.f && S.Rush <= 0.f)
        {
            S.Retarget = Rng.FRandRange(3.f, 7.f);
            const float Angle = Rng.FRand() * UE_TWO_PI;
            const FVector To = S.Center + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * Rng.FRandRange(200.f, 600.f);
            float Surface = 0.f;
            if (Data->IsWaterAt(To.X, To.Y, Surface) && Surface - Data->GroundAt(To.X, To.Y) > 55.f) S.Target = FVector(To.X, To.Y, S.Center.Z);
        }
        const float Speed = S.Rush > 0.f ? 480.f : 70.f;
        S.Center += (S.Target - S.Center).GetClampedToMaxSize(Speed * Dt);
        // um pulo de vez em quando
        S.NextJump -= Dt;
        if (S.NextJump <= 0.f && S.Rush <= 0.f)
        {
            S.NextJump = Rng.FRandRange(6.f, 15.f);
            TArray<int32> Mine;
            for (int32 i = 0; i < Fish.Num(); ++i) if (Fish[i].School == Index && Fish[i].JumpT < 0.f) Mine.Add(i);
            if (Mine.Num())
            {
                FFishy& F = Fish[Mine[Rng.RandRange(0, Mine.Num() - 1)]];
                F.JumpT = 0.f;
                F.JumpFrom = F.Pos;
                const FVector Dir = FRotator(0.f, F.Yaw, 0.f).Vector();
                F.JumpTo = FVector(F.Pos.X, F.Pos.Y, S.Surface - 20.f) + Dir * 170.f;
                Emit(4, FVector(F.Pos.X, F.Pos.Y, S.Surface + 4.f), 8, 260.f, 110.f, .7f, 2.2f);
            }
        }
    }
    for (FFishy& F : Fish)
    {
        const FSchool& S = Schools[F.School];
        const FFishKind& K = FishKind(F.Kind);
        if (F.JumpT >= 0.f)
        {
            // salto em arco: sai da água, gira e cai de cabeça
            F.JumpT += Dt / .85f;
            const float T = FMath::Min(F.JumpT, 1.f);
            F.Pos = FMath::Lerp(F.JumpFrom, F.JumpTo, T) + FVector(0.f, 0.f, FMath::Sin(T * PI) * (S.Surface + 70.f - F.JumpFrom.Z));
            F.Phase += Dt * 22.f;
            if (F.JumpT >= 1.f)
            {
                F.JumpT = -1.f;
                Emit(4, FVector(F.Pos.X, F.Pos.Y, S.Surface + 4.f), 12, 320.f, 140.f, .8f, 2.6f);
                if (FVector::Dist(Camera, F.Pos) < 3500.f) VortexWorldHooks::PlaySound(this, TEXT("fish_plop"), F.Pos, .55f, .2f);
            }
            continue;
        }
        const FVector Wobble(FMath::Sin(Clock * .7f + F.Phase) * 60.f, FMath::Cos(Clock * .5f + F.Phase * 1.3f) * 60.f, FMath::Sin(Clock * .9f + F.Phase) * 12.f);
        FVector Want = S.Center + F.Offset + Wobble;
        Want.Z = FMath::Min(Want.Z, S.Surface - 10.f * K.Size);
        const FVector Step = (Want - F.Pos);
        const float Max = (S.Rush > 0.f ? 560.f : 140.f) * Dt;
        const FVector Move = Step.GetClampedToMaxSize(Max);
        if (Move.Size2D() > .05f) F.Yaw = FMath::FixedTurn(F.Yaw, Move.Rotation().Yaw, (S.Rush > 0.f ? 720.f : 240.f) * Dt);
        F.Pos += Move;
        F.Phase += Dt * (S.Rush > 0.f ? 24.f : 7.f + Move.Size() / FMath::Max(Dt, .001f) * .03f);
    }
}

// ---- plantas da água ---------------------------------------------------------------------------------

void AVortexWildlife::UpdatePlants(const FVector& Camera)
{
    if (FVector::Dist2D(Camera, PlantsCenter) < 900.f) return;
    const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
    if (!Data) return;
    PlantsCenter = Camera;
    TArray<FTransform> PadT, FlowerT, GlowFlowerT, Green, Dark, Red, Glow;
    const float Step = 300.f, Radius = 4500.f;
    const int32 Cx = FMath::FloorToInt(Camera.X / Step), Cy = FMath::FloorToInt(Camera.Y / Step), Reach = FMath::CeilToInt(Radius / Step);
    for (int32 dx = -Reach; dx <= Reach; ++dx)
        for (int32 dy = -Reach; dy <= Reach; ++dy)
        {
            if (dx * dx + dy * dy > Reach * Reach) continue;
            const int32 Gx = Cx + dx, Gy = Cy + dy;
            const uint32 H = CellHash(Gx, Gy, 0x51ed);
            const float X = (Gx + ((H >> 4) % 100) / 100.f) * Step, Y = (Gy + ((H >> 12) % 100) / 100.f) * Step;
            float Surface = 0.f;
            if (!Data->IsWaterAt(X, Y, Surface)) continue;
            const float Depth = Surface - Data->GroundAt(X, Y);
            const int32 Region = LandRegion(*Data, X, Y);
            const uint32 Roll = H % 100;
            // vitórias-régias: água parada e rasa (no pântano bem mais); nada no gelo, na brasa e no mar
            const bool bPadRegion = Region != 0 && Region != 2 && Region != 4;
            const uint32 PadChance = Region == 7 ? 55 : Region == 9 ? 35 : 20;
            if (bPadRegion && Depth > 15.f && Depth < 170.f && Roll < PadChance)
            {
                const int32 Count = 1 + (H >> 20) % 3;
                for (int32 i = 0; i < Count; ++i)
                {
                    const uint32 Hi = CellHash(Gx, Gy, 0x9a1 + i);
                    const FVector At(X + (int32((Hi >> 3) % 120) - 60), Y + (int32((Hi >> 9) % 120) - 60), Surface + 2.5f);
                    const float S = .8f + ((Hi >> 15) % 50) / 100.f;
                    PadT.Add(FTransform(FRotator(0.f, float(Hi % 360), 0.f), At, FVector(S, S, 1.f)));
                    if ((Hi >> 23) % 3 == 0) (Region == 9 ? GlowFlowerT : FlowerT).Add(FTransform(FRotator(0.f, float(Hi % 360), 0.f), At + FVector(0.f, 0.f, .5f), FVector(S)));
                }
            }
            // algas no fundo
            const uint32 WeedChance = Region == 0 ? 40 : 28;
            if (Depth > 50.f && Depth < 450.f && ((H >> 8) % 100) < WeedChance)
            {
                const int32 Count = 2 + (H >> 24) % 3;
                for (int32 i = 0; i < Count; ++i)
                {
                    const uint32 Hi = CellHash(Gx, Gy, 0x77e + i);
                    const float Ground = Surface - Depth;
                    const float Tall = FMath::Min(Depth - 25.f, 60.f + float((Hi >> 5) % 120)) / 100.f;
                    if (Tall <= .25f) continue;
                    const FVector At(X + (int32((Hi >> 3) % 160) - 80), Y + (int32((Hi >> 11) % 160) - 80), Ground);
                    const FTransform T(FRotator(float((Hi >> 17) % 10) - 5.f, float(Hi % 360), 0.f), At, FVector(1.f, 1.f, Tall));
                    (Region == 9 ? Glow : Region == 2 ? Red : (Region == 7 || Region == 8) ? Dark : Green).Add(T);
                }
            }
        }
    Flush(Pads, PadT);
    Flush(PadFlowers, FlowerT);
    Flush(PadGlowFlowers, GlowFlowerT);
    Flush(WeedGreen, Green);
    Flush(WeedDark, Dark);
    Flush(WeedRed, Red);
    Flush(WeedGlow, Glow);
    PadCount = PadT.Num();
    PadSpots.Reset();
    for (const FTransform& T : PadT) PadSpots.Add(T.GetLocation());
    WeedCount = Green.Num() + Dark.Num() + Red.Num() + Glow.Num();
}

// ---- esporos e respingos -----------------------------------------------------------------------------

void AVortexWildlife::Emit(int32 Look, const FVector& At, int32 Count, float Up, float Out, float Life, float Size)
{
    for (int32 i = 0; i < Count && Particles.Num() < 400; ++i)
    {
        FParticle P;
        P.Look = Look;
        P.Pos = At + FVector(Rng.FRandRange(-8.f, 8.f), Rng.FRandRange(-8.f, 8.f), Rng.FRandRange(0.f, 10.f));
        const float A = Rng.FRand() * UE_TWO_PI;
        P.Vel = FVector(FMath::Cos(A), FMath::Sin(A), 0.f) * Rng.FRandRange(.3f, 1.f) * Out + FVector(0.f, 0.f, Rng.FRandRange(.5f, 1.f) * Up);
        P.MaxLife = P.Life = Life * Rng.FRandRange(.7f, 1.2f);
        P.Size = Size * Rng.FRandRange(.6f, 1.2f);
        Particles.Add(P);
    }
}

void AVortexWildlife::DebugPuff(const FVector& At, bool bGlow) { Emit(bGlow ? 2 : 0, At, 18, 90.f, 70.f, 2.2f, 1.8f); }

void AVortexWildlife::UpdateSpores(float Dt)
{
    // o herói passando correndo em cima de um cogumelo: nuvem de esporos (cada cogumelo, uma vez a cada 4 s)
    SporeTimer -= Dt;
    const APlayerController* Controller = GetWorld()->GetFirstPlayerController();
    const APawn* Hero = Controller ? Controller->GetPawn() : nullptr;
    const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
    if (SporeTimer <= 0.f && Hero && Data && Hero->GetVelocity().Size2D() > 120.f)
    {
        SporeTimer = .15f;
        const FVector At = Hero->GetActorLocation();
        const double Now = GetWorld()->GetTimeSeconds();
        const float Size = Data->ChunkSize();
        const FIntPoint Chunk(FMath::FloorToInt(At.X / Size), FMath::FloorToInt(At.Y / Size));
        for (int32 dx = -1; dx <= 1; ++dx)
            for (int32 dy = -1; dy <= 1; ++dy)
            {
                const FIntPoint C = Chunk + FIntPoint(dx, dy);
                if (!Data->IsScattered(C)) continue;
                const TArray<FVortexTerrainProp>* Props = Data->PropsInChunk(C);
                if (!Props) continue;
                for (const FVortexTerrainProp& Prop : *Props)
                {
                    int32 Look = 0;
                    if (FVector::Dist2D(Prop.Location, At) > 110.f * FMath::Max(1.f, Prop.Scale) || !Data->Kinds.IsValidIndex(Prop.Kind) || !IsMushroom(Data->Kinds[Prop.Kind], Look)) continue;
                    const FIntVector Key(FMath::RoundToInt(Prop.Location.X), FMath::RoundToInt(Prop.Location.Y), 0);
                    if (const double* Last = PuffedAt.Find(Key); Last && Now - *Last < 4.0) continue;
                    PuffedAt.Add(Key, Now);
                    Emit(Look, Prop.Location + FVector(0.f, 0.f, 25.f * Prop.Scale), 16, 85.f, 60.f, 2.4f, 1.7f);
                }
            }
        if (PuffedAt.Num() > 200) PuffedAt.Reset();
    }
    for (int32 i = Particles.Num() - 1; i >= 0; --i)
    {
        FParticle& P = Particles[i];
        P.Life -= Dt;
        if (P.Life <= 0.f) { Particles.RemoveAtSwap(i); continue; }
        // respingo cai; esporo flutua e se espalha devagar
        if (P.Look == 4) P.Vel.Z -= 980.f * Dt;
        else { P.Vel *= FMath::Max(0.f, 1.f - 1.6f * Dt); P.Vel.Z += 6.f * Dt; P.Vel += FVector(FMath::Sin(Clock * 2.f + i), FMath::Cos(Clock * 1.7f + i), 0.f) * 12.f * Dt; }
        P.Pos += P.Vel * Dt;
    }
}

// ---- desenho -----------------------------------------------------------------------------------------

void AVortexWildlife::DrawAll()
{
    TArray<TArray<FTransform>> Body, WingL, WingR;
    Body.SetNum(BirdKinds); WingL.SetNum(BirdKinds); WingR.SetNum(BirdKinds);
    for (const FBird& B : Birds)
    {
        const FBirdKind& K = BirdKind(B.Kind);
        const float S = K.Size;
        const bool bAir = B.Mode != 1;
        float Pitch = 0.f;
        if (bAir) Pitch = FMath::Clamp(B.Vel.Rotation().Pitch, -30.f, 40.f);
        else if (B.Peck > 0.f) Pitch = -35.f * FMath::Sin(B.Peck / .35f * PI);
        const FRotator Rot(Pitch, B.Yaw, bAir ? B.Roll : 0.f);
        // no chão o corpo fica em cima das pernas
        const float Lift = bAir ? 0.f : (K.bHeron ? 27.f : 10.f) * S;
        const FTransform Pose(Rot, B.Pos + FVector(0.f, 0.f, Lift), FVector(S));
        Body[B.Kind].Add(Pose);
        float Flap = 0.f, Fold = 0.f;
        if (bAir) Flap = FMath::Sin(B.Flap) * (K.bSoar ? 28.f : 55.f) + 8.f;
        else Fold = 82.f;
        const FVector Shoulder(1.f, 4.5f, 2.f);
        WingR[B.Kind].Add(FTransform(FRotator(0.f, Fold, -Flap), FVector(Shoulder.X, Shoulder.Y, Shoulder.Z)) * Pose);
        WingL[B.Kind].Add(FTransform(FRotator(0.f, -Fold, Flap), FVector(Shoulder.X, -Shoulder.Y, Shoulder.Z)) * Pose);
    }
    for (int32 k = 0; k < BirdKinds; ++k) { Flush(BirdBody[k], Body[k]); Flush(BirdWingL[k], WingL[k]); Flush(BirdWingR[k], WingR[k]); }
    TArray<TArray<FTransform>> FBody, FTail;
    FBody.SetNum(FishKinds); FTail.SetNum(FishKinds);
    for (const FFishy& F : Fish)
    {
        const FFishKind& K = FishKind(F.Kind);
        float Pitch = 0.f;
        if (F.JumpT >= 0.f) Pitch = FMath::Lerp(55.f, -70.f, FMath::Clamp(F.JumpT, 0.f, 1.f));
        const FTransform Pose(FRotator(Pitch, F.Yaw + FMath::Sin(F.Phase + 1.f) * 6.f, 0.f), F.Pos, FVector(K.Size));
        FBody[F.Kind].Add(Pose);
        FTail[F.Kind].Add(FTransform(FRotator(0.f, FMath::Sin(F.Phase) * 28.f, 0.f), FVector(-8.f, 0.f, .5f)) * Pose);
    }
    for (int32 k = 0; k < FishKinds; ++k) { Flush(FishBody[k], FBody[k]); Flush(FishTail[k], FTail[k]); }
    TArray<TArray<FTransform>> Dots;
    Dots.SetNum(PuffDraw.Num());
    for (const FParticle& P : Particles)
    {
        const float Fade = FMath::Clamp(P.Life / P.MaxLife * 2.f, 0.f, 1.f);
        Dots[FMath::Clamp(P.Look, 0, PuffDraw.Num() - 1)].Add(FTransform(FRotator(Clock * 90.f, Clock * 70.f, 0.f), P.Pos, FVector(P.Size * Fade)));
    }
    for (int32 k = 0; k < PuffDraw.Num(); ++k) Flush(PuffDraw[k], Dots[k]);
}

void AVortexWildlife::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const float Dt = FMath::Min(DeltaSeconds, .1f);
    Clock += Dt;
    const UVortexDayNightSubsystem* DayNight = GetWorld()->GetSubsystem<UVortexDayNightSubsystem>();
    Daylight = DayNight && DayNight->IsActive() ? DayNight->GetDaylight() : 1.f;
    if (!AVortexTerrainStreamer::SharedData()) return;
    FVector Forward;
    const FVector Camera = CameraLocation(&Forward);
    ScareTimer -= Dt;
    if (ScareTimer <= 0.f)
    {
        ScareTimer = .2f;
        Scarers.Reset();
        for (TActorIterator<APawn> It(GetWorld()); It; ++It)
            if (FVector::Dist2D(It->GetActorLocation(), Camera) < 8000.f) Scarers.Add(It->GetActorLocation());
    }
    // primeiro quadro: espalha tudo já em volta
    if (!bSeeded)
    {
        bSeeded = true;
        for (int32 i = 0; i < Flocks.Num(); ++i) SpawnFlock(i, Camera, Forward, i < 3, true);
        for (int32 i = 0; i < Schools.Num(); ++i) SpawnSchool(i, Camera, true);
    }
    UpdateBirds(Dt, Camera, Forward);
    UpdateFish(Dt, Camera);
    UpdatePlants(Camera);
    UpdateSpores(Dt);
    DrawAll();
}

// ---- testes ------------------------------------------------------------------------------------------

int32 AVortexWildlife::SkyBirds() const { int32 N = 0; for (const FBird& B : Birds) N += B.Mode == 0; return N; }
int32 AVortexWildlife::GroundBirds() const { int32 N = 0; for (const FBird& B : Birds) N += B.Mode == 1; return N; }
int32 AVortexWildlife::FleeingBirds() const { int32 N = 0; for (const FBird& B : Birds) N += B.Mode == 2; return N; }
int32 AVortexWildlife::JumpingFish() const { int32 N = 0; for (const FFishy& F : Fish) N += F.JumpT >= 0.f; return N; }

FVector AVortexWildlife::NearestGroundFlock(const FVector& From) const
{
    FVector Best = FVector::ZeroVector;
    float BestD = 1.e12f;
    for (int32 i = 3; i < Flocks.Num(); ++i)
        if (Flocks[i].bActive && !Flocks[i].bFled && FVector::DistSquared2D(Flocks[i].Center, From) < BestD) { BestD = FVector::DistSquared2D(Flocks[i].Center, From); Best = Flocks[i].Center; }
    return Best;
}

FVector AVortexWildlife::NearestPad(const FVector& From) const
{
    FVector Best = FVector::ZeroVector;
    float BestD = 1.e12f;
    for (const FVector& P : PadSpots) if (FVector::DistSquared2D(P, From) < BestD) { BestD = FVector::DistSquared2D(P, From); Best = P; }
    return Best;
}

FVector AVortexWildlife::NearestSchool(const FVector& From) const
{
    FVector Best = FVector::ZeroVector;
    float BestD = 1.e12f;
    for (const FSchool& S : Schools)
        if (S.bActive && FVector::DistSquared2D(S.Center, From) < BestD) { BestD = FVector::DistSquared2D(S.Center, From); Best = S.Center; }
    return Best;
}

void AVortexWildlife::DebugSpawnNear(const FVector& At)
{
    FVector Forward;
    CameraLocation(&Forward);
    // só o primeiro de cada tipo, bem perto do ponto
    const float Saved = Daylight;
    Daylight = 1.f;
    for (int32 Try = 0; Try < 6 && !Flocks[3].bActive; ++Try) SpawnFlock(3, At, Forward, false, true);
    Daylight = Saved;
    for (int32 Try = 0; Try < 6 && !Schools[0].bActive; ++Try) SpawnSchool(0, At, true);
    PlantsCenter = FVector(1.e9f);
}

FString AVortexWildlife::Describe() const
{
    TArray<FString> Kinds;
    for (const FFlock& F : Flocks) if (F.bActive) Kinds.AddUnique(BirdName(F.Kind));
    for (const FFishy& F : Fish) Kinds.AddUnique(FishName(F.Kind));
    return FString::Printf(TEXT("ceu=%d chao=%d fugindo=%d peixes=%d pulando=%d vitorias=%d algas=%d esporos=%d especies=[%s]"),
        SkyBirds(), GroundBirds(), FleeingBirds(), Fish.Num(), JumpingFish(), PadCount, WeedCount, Particles.Num(), *FString::Join(Kinds, TEXT(", ")));
}
