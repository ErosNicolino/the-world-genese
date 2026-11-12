#include "World/VortexForestLife.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "MeshDescription.h"
#include "Sound/SoundBase.h"
#include "StaticMeshAttributes.h"
#include "World/VortexDayNightSubsystem.h"
#include "World/VortexTerrainStreamer.h"
#include "World/VortexTown.h"

AVortexForestLife::AVortexForestLife()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;   // depois da câmera andar
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AVortexForestLife::MakeLoop(FLoop& Loop, const TCHAR* Path)
{
    USoundBase* Sound = LoadObject<USoundBase>(nullptr, Path);
    if (!Sound) return;
    Loop.Audio = NewObject<UAudioComponent>(this);
    Loop.Audio->SetSound(Sound);
    Loop.Audio->bAutoActivate = false;
    Loop.Audio->bAllowSpatialization = false;   // ambiente: em volta, não num ponto
    Loop.Audio->bIsUISound = false;
    Loop.Audio->RegisterComponent();
    Loop.Audio->SetVolumeMultiplier(0.001f);
    Loop.Audio->Play(FMath::FRand() * 10.f);   // cada loop começa num ponto diferente
    Keep.Add(Loop.Audio);
}

// como o Ambience.cs: o volume anda em linha reta até o alvo, 0,5 por segundo
void AVortexForestLife::Mix(FLoop& Loop, float Target, float DeltaSeconds)
{
    if (!Loop.Audio) return;
    Loop.Volume = FMath::FInterpConstantTo(Loop.Volume, Target, DeltaSeconds, .5f);
    Loop.Audio->SetVolumeMultiplier(FMath::Max(Loop.Volume * SoundLevel, 0.001f));
}

UStaticMesh* AVortexForestLife::BuildMesh(const TArray<FVector3f>& Positions, const TArray<FVector3f>& Normals,
    const TArray<FVector2f>& UV0, const TArray<FVector2f>& UV1, const TArray<int32>& Triangles, float Reach)
{
    FMeshDescription Description;
    FStaticMeshAttributes Attributes(Description);
    Attributes.Register();
    TVertexAttributesRef<FVector3f> Points = Attributes.GetVertexPositions();
    TVertexInstanceAttributesRef<FVector3f> InstanceNormals = Attributes.GetVertexInstanceNormals();
    TVertexInstanceAttributesRef<FVector2f> UVs = Attributes.GetVertexInstanceUVs();
    UVs.SetNumChannels(2);
    const FPolygonGroupID Group = Description.CreatePolygonGroup();
    Attributes.GetPolygonGroupMaterialSlotNames()[Group] = TEXT("Life");
    TArray<FVertexInstanceID> Instances;
    for (int32 i = 0; i < Positions.Num(); ++i)
    {
        const FVertexID Vertex = Description.CreateVertex();
        Points[Vertex] = Positions[i];
        const FVertexInstanceID Instance = Description.CreateVertexInstance(Vertex);
        InstanceNormals[Instance] = Normals[i];
        UVs.Set(Instance, 0, UV0[i]);
        UVs.Set(Instance, 1, UV1[i]);
        Instances.Add(Instance);
    }
    for (int32 i = 0; i + 2 < Triangles.Num(); i += 3)
    {
        Description.CreateTriangle(Group, { Instances[Triangles[i]], Instances[Triangles[i + 1]], Instances[Triangles[i + 2]] });
    }
    UStaticMesh* Mesh = NewObject<UStaticMesh>(this);
    Mesh->GetStaticMaterials().Add(FStaticMaterial(nullptr, TEXT("Life")));
    UStaticMesh::FBuildMeshDescriptionsParams Params;
    Params.bFastBuild = true;
    Params.bBuildSimpleCollision = false;
    Params.bCommitMeshDescription = false;
    Mesh->BuildFromMeshDescriptions({ &Description }, Params);
    // malha feita no código: sem a densidade de UV, o streaming de texturas dispara um ensure (no pacote de medição o
    // jogo travava nele)
    for (FStaticMaterial& Slot : Mesh->GetStaticMaterials()) Slot.UVChannelData = FMeshUVChannelInfo(1.f);
    // o voo é feito no material: a caixa de cada cópia cresce até onde o bicho pode ir
    Mesh->SetPositiveBoundsExtension(FVector(Reach));
    Mesh->SetNegativeBoundsExtension(FVector(Reach));
    Mesh->CalculateExtendedBounds();
    Keep.Add(Mesh);
    return Mesh;
}

// Borboleta de ~12 cm: duas asas (quadrados do corpo para fora) e um corpo fino. UV0 = posição na
// asa (0 no corpo .. 1 na ponta, frente/trás), UV1 = (quanto a asa se afasta do corpo em cm, corpo).
UStaticMesh* AVortexForestLife::BuildButterfly()
{
    TArray<FVector3f> P, N;
    TArray<FVector2f> U0, U1;
    TArray<int32> T;
    const float Span = 6.f, Half = 4.f;
    for (const float Side : { -1.f, 1.f })
    {
        const int32 Base = P.Num();
        const FVector3f Corners[] = { { -Half, 0.f, 0.f }, { Half, 0.f, 0.f }, { Half, Side * Span, 0.f }, { -Half, Side * Span, 0.f } };
        for (const FVector3f& C : Corners)
        {
            P.Add(C);
            N.Add(FVector3f::UpVector);
            U0.Add(FVector2f(FMath::Abs(C.Y) / Span, (C.X + Half) / (2.f * Half)));
            U1.Add(FVector2f(C.Y, 0.f));
        }
        T.Append({ Base, Base + 1, Base + 2, Base, Base + 2, Base + 3 });
    }
    // corpo: uma tirinha em pé ao longo do eixo
    const int32 Base = P.Num();
    const FVector3f Body[] = { { -3.f, 0.f, -.4f }, { 3.f, 0.f, -.4f }, { 3.f, 0.f, .5f }, { -3.f, 0.f, .5f } };
    for (const FVector3f& C : Body)
    {
        P.Add(C);
        N.Add(FVector3f(0.f, 1.f, 0.f));
        U0.Add(FVector2f(0.f, .5f));
        U1.Add(FVector2f(0.f, 1.f));
    }
    T.Append({ Base, Base + 1, Base + 2, Base, Base + 2, Base + 3 });
    return BuildMesh(P, N, U0, U1, T, 260.f);
}

// Vaga-lume: um octaedro de 3,5 cm que brilha (o bloom faz o halo).
UStaticMesh* AVortexForestLife::BuildFirefly()
{
    TArray<FVector3f> P, N;
    TArray<FVector2f> U0, U1;
    TArray<int32> T;
    const float R = 3.5f;
    const FVector3f Axis[] = { { R, 0, 0 }, { 0, R, 0 }, { -R, 0, 0 }, { 0, -R, 0 } };
    for (int32 i = 0; i < 4; ++i)
    {
        for (const float Z : { R, -R })
        {
            const FVector3f A = Axis[i], B = Axis[(i + 1) % 4], C(0.f, 0.f, Z);
            const FVector3f Normal = ((A + B + C) / 3.f).GetSafeNormal();
            const int32 Base = P.Num();
            P.Append({ A, B, C });
            N.Append({ Normal, Normal, Normal });
            U0.Append({ FVector2f::ZeroVector, FVector2f::ZeroVector, FVector2f::ZeroVector });
            U1.Append({ FVector2f::ZeroVector, FVector2f::ZeroVector, FVector2f::ZeroVector });
            if (Z > 0.f) T.Append({ Base, Base + 2, Base + 1 });
            else T.Append({ Base, Base + 1, Base + 2 });
        }
    }
    return BuildMesh(P, N, U0, U1, T, 220.f);
}

// Semente de dente-de-leão: três plaquinhas cruzadas de 3 cm com um disco macio (no material).
UStaticMesh* AVortexForestLife::BuildSeed()
{
    TArray<FVector3f> P, N;
    TArray<FVector2f> U0, U1;
    TArray<int32> T;
    const float H = 1.5f;
    for (int32 i = 0; i < 3; ++i)
    {
        const float Yaw = i * PI / 3.f;
        const FVector3f Right(FMath::Cos(Yaw) * H, FMath::Sin(Yaw) * H, 0.f);
        const FVector3f Up = i == 2 ? FVector3f(0.f, 0.f, 0.f) : FVector3f(0.f, 0.f, H);
        const FVector3f Side = i == 2 ? FVector3f(-FMath::Sin(Yaw) * H, FMath::Cos(Yaw) * H, 0.f) : Up;
        const FVector3f Normal = FVector3f::CrossProduct(Right, Side).GetSafeNormal();
        const int32 Base = P.Num();
        P.Append({ -Right - Side, Right - Side, Right + Side, -Right + Side });
        N.Append({ Normal, Normal, Normal, Normal });
        U0.Append({ FVector2f(0.f, 0.f), FVector2f(1.f, 0.f), FVector2f(1.f, 1.f), FVector2f(0.f, 1.f) });
        U1.Append({ FVector2f::ZeroVector, FVector2f::ZeroVector, FVector2f::ZeroVector, FVector2f::ZeroVector });
        T.Append({ Base, Base + 1, Base + 2, Base, Base + 2, Base + 3 });
    }
    return BuildMesh(P, N, U0, U1, T, 900.f);
}

FVector AVortexForestLife::CameraLocation() const
{
    const APlayerController* Controller = GetWorld()->GetFirstPlayerController();
    if (Controller && Controller->PlayerCameraManager) return Controller->PlayerCameraManager->GetCameraLocation();
    return FVector::ZeroVector;
}

// Um ponto sobre o chão, longe da água, entre MinDistance e o raio da espécie.
bool AVortexForestLife::PlaceHome(const FKind& Kind, const FVector& Center, float MinDistance, FVector& Out)
{
    const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
    if (!Data) return false;
    for (int32 Try = 0; Try < 6; ++Try)
    {
        const float Angle = Rng.FRand() * UE_TWO_PI;
        const float Distance = FMath::Sqrt(FMath::Lerp(FMath::Square(MinDistance / Kind.Radius), 1.f, Rng.FRand())) * Kind.Radius;
        const float X = Center.X + FMath::Cos(Angle) * Distance, Y = Center.Y + FMath::Sin(Angle) * Distance;
        const float Ground = Data->GroundAt(X, Y);
        float Surface = 0.f;
        if (Ground <= -299.f || Data->IsWaterAt(X, Y, Surface)) continue;
        Out = FVector(X, Y, Ground + Rng.FRandRange(Kind.MinHeight, Kind.MaxHeight));
        return true;
    }
    return false;
}

void AVortexForestLife::SetupKind(FKind& Kind, UStaticMesh* Mesh, const TCHAR* MaterialPath, int32 Count, const FVector& Center)
{
    UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, MaterialPath);
    if (!Mesh || !Base) return;
    Kind.Instances = NewObject<UInstancedStaticMeshComponent>(this);
    Kind.Instances->SetupAttachment(RootComponent);
    Kind.Instances->SetStaticMesh(Mesh);
    Kind.Material = UMaterialInstanceDynamic::Create(Base, this);
    Kind.Material->SetScalarParameterValue(TEXT("Fade"), 0.f);
    Kind.Instances->SetMaterial(0, Kind.Material);
    Kind.Instances->SetMobility(EComponentMobility::Movable);
    Kind.Instances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Kind.Instances->SetCastShadow(false);
    Kind.Instances->bAffectDynamicIndirectLighting = false;
    Kind.Instances->bAffectDistanceFieldLighting = false;
    Kind.Instances->SetVisibleInRayTracing(false);
    Kind.Instances->bVisibleInReflectionCaptures = false;
    Kind.Instances->bVisibleInRealTimeSkyCaptures = false;
    Kind.Instances->SetCullDistances(0, FMath::RoundToInt(Kind.Radius + 600.f));
    Kind.Instances->RegisterComponent();
    Keep.Add(Kind.Instances);
    Keep.Add(Kind.Material);
    TArray<FTransform> Transforms;
    for (int32 i = 0; i < Count; ++i)
    {
        FVector Home = Center;
        PlaceHome(Kind, Center, 0.f, Home);
        Kind.Homes.Add(Home);
        Transforms.Add(FTransform(FRotator(0.f, Rng.FRand() * 360.f, 0.f), Home, FVector(Rng.FRandRange(Kind.Scale.X, Kind.Scale.Y))));
    }
    Kind.Instances->AddInstances(Transforms, false, true);
    Kind.Instances->SetVisibility(false);
}

void AVortexForestLife::BeginPlay()
{
    Super::BeginPlay();
    MakeLoop(Day, TEXT("/Game/Vortex/Audio/amb_day.amb_day"));
    MakeLoop(Night, TEXT("/Game/Vortex/Audio/amb_night.amb_night"));
    MakeLoop(Wind, TEXT("/Game/Vortex/Audio/amb_wind.amb_wind"));
    // o rio gravado (amb_water); sem ele, o riacho do pacote antigo
    if (LoadObject<USoundBase>(nullptr, TEXT("/Game/Vortex/Audio/amb_water.amb_water"))) MakeLoop(Water, TEXT("/Game/Vortex/Audio/amb_water.amb_water"));
    else MakeLoop(Water, TEXT("/Game/Free_Sounds_Pack/wav/Ambient_Water_Stream_Calm_3.Ambient_Water_Stream_Calm_3"));
    const FVector Center = CameraLocation();
    Butterflies.Radius = 1400.f; Butterflies.MinHeight = 40.f; Butterflies.MaxHeight = 160.f; Butterflies.Scale = FVector2f(1.3f, 1.8f);
    Seeds.Radius = 1800.f; Seeds.MinHeight = 60.f; Seeds.MaxHeight = 260.f; Seeds.Scale = FVector2f(1.1f, 1.5f);
    Fireflies.Radius = 1800.f; Fireflies.MinHeight = 25.f; Fireflies.MaxHeight = 170.f; Fireflies.bNight = true;
    // -VortexLifeRadius=0.3: todos bem perto (fotos de teste)
    float RadiusScale = 1.f;
    if (FParse::Value(FCommandLine::Get(), TEXT("VortexLifeRadius="), RadiusScale))
    {
        for (FKind* Kind : { &Butterflies, &Seeds, &Fireflies }) Kind->Radius *= RadiusScale;
    }
    SetupKind(Butterflies, BuildButterfly(), TEXT("/Game/Vortex/Life/M_Butterfly.M_Butterfly"), 20, Center);
    SetupKind(Seeds, BuildSeed(), TEXT("/Game/Vortex/Life/M_Seed.M_Seed"), 90, Center);
    SetupKind(Fireflies, BuildFirefly(), TEXT("/Game/Vortex/Life/M_Firefly.M_Firefly"), 90, Center);
    // sementes não giram: o vento delas está no material, na direção do vento da grama (26,6 graus)
    if (Seeds.Instances)
    {
        for (int32 i = 0; i < Seeds.Homes.Num(); ++i)
        {
            Seeds.Instances->UpdateInstanceTransform(i, FTransform(FRotator::ZeroRotator, Seeds.Homes[i], FVector(Rng.FRandRange(Seeds.Scale.X, Seeds.Scale.Y))), true, false, true);
        }
        Seeds.Instances->MarkRenderStateDirty();
    }
}

// Repõe quem ficou longe da câmera numa faixa mais afastada (a mudança fica fora da vista de perto).
void AVortexForestLife::Refresh(FKind& Kind, const FVector& Center, float Fade, bool bReplace)
{
    if (!Kind.Instances) return;
    if (FMath::Abs(Fade - Kind.Fade) > .005f || (Fade == 0.f) != (Kind.Fade == 0.f))
    {
        Kind.Material->SetScalarParameterValue(TEXT("Fade"), Fade);
        Kind.Instances->SetVisibility(Fade > 0.f);
        Kind.Fade = Fade;
    }
    if (!bReplace || Fade <= 0.f) return;
    bool bChanged = false;
    const float Far = FMath::Square(Kind.Radius * 1.1f);
    // salto grande (teleporte, início): espalha de novo pelo círculo todo
    int32 FarCount = 0;
    for (const FVector& Home : Kind.Homes) FarCount += FVector::DistSquared2D(Home, Center) > Far;
    const float MinDistance = FarCount * 2 > Kind.Homes.Num() ? 0.f : Kind.Radius * .7f;
    for (int32 i = 0; i < Kind.Homes.Num(); ++i)
    {
        if (FVector::DistSquared2D(Kind.Homes[i], Center) <= Far) continue;
        FVector Home;
        if (!PlaceHome(Kind, Center, MinDistance, Home)) continue;
        Kind.Homes[i] = Home;
        FTransform Transform;
        Kind.Instances->GetInstanceTransform(i, Transform, true);
        Transform.SetLocation(Home);
        Kind.Instances->UpdateInstanceTransform(i, Transform, true, false, true);
        bChanged = true;
    }
    if (bChanged) Kind.Instances->MarkRenderStateDirty();
}

// Distância até a água mais perto (cm), olhando uma grade de 2,5 m até 25 m em volta.
float AVortexForestLife::NearestWater(const FVector& Center) const
{
    const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
    if (!Data) return 1.e6f;
    float Best = 1.e6f;
    const float Step = 250.f;
    for (int32 Ix = -10; Ix <= 10; ++Ix)
    for (int32 Iy = -10; Iy <= 10; ++Iy)
    {
        const float D = FMath::Sqrt(float(Ix * Ix + Iy * Iy)) * Step;
        float Surface = 0.f;
        if (D < Best && Data->IsWaterAt(Center.X + Ix * Step, Center.Y + Iy * Step, Surface)) Best = D;
    }
    return Best;
}

void AVortexForestLife::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const UVortexDayNightSubsystem* DayNight = GetWorld()->GetSubsystem<UVortexDayNightSubsystem>();
    Daylight = DayNight && DayNight->IsActive() ? DayNight->GetDaylight() : 1.f;
    const FVector Center = CameraLocation();
    CheckTime -= DeltaSeconds;
    if (CheckTime <= 0.f)
    {
        CheckTime = .5f;
        WaterNear = 1.f - FMath::SmoothStep(300.f, 2200.f, NearestWater(Center));
        // o som do lugar: um de cada vez; onde ele manda, os passarinhos do dia ficam mais baixos
        const int32 Place = PlaceIndex();
        static const float Volumes[PlaceCount] = { .55f, .5f, .4f, .5f, .45f, .5f, .45f, .45f, .4f };
        static const float Birds[PlaceCount] = { .3f, .2f, .5f, .25f, .25f, .7f, .35f, .9f, .5f };
        for (int32 i = 0; i < PlaceCount; ++i) PlaceTarget[i] = i == Place ? Volumes[i] : 0.f;
        DayScale = Place == INDEX_NONE ? 1.f : Birds[Place];
    }
    for (int32 i = 0; i < PlaceCount; ++i) MixPlace(i, DeltaSeconds);
    // volumes do Ambience.cs (ainda sem cidade: vento no máximo do campo)
    Mix(Day, .55f * Daylight * DayScale, DeltaSeconds);
    Mix(Night, .5f * (1.f - Daylight), DeltaSeconds);
    Mix(Wind, .4f, DeltaSeconds);
    Mix(Water, .7f * WaterNear, DeltaSeconds);

    PlaceTime -= DeltaSeconds;
    const bool bReplace = PlaceTime <= 0.f;
    if (bReplace) PlaceTime = .25f;
    const float DayFade = FMath::SmoothStep(.55f, .85f, Daylight);
    Refresh(Butterflies, Center, DayFade, bReplace);
    Refresh(Seeds, Center, DayFade, bReplace);
    Refresh(Fireflies, Center, 1.f - FMath::SmoothStep(.15f, .5f, Daylight), bReplace);
}

// 0 mar, 1 Brasafenda (vulcão), 2 Raikor (tempestade), 3 Nivaluz (neve), 4 Solcânion (deserto), 5 Lodonegro (pântano),
// 6 Morthúria (assombrado), 7 Verdalume (élfico), 8 cidade; INDEX_NONE: Valdora e Aurivale (só pássaros, vento e água)
int32 AVortexForestLife::PlaceIndex() const
{
    const FVector Center = CameraLocation();
    if (AVortexTown::Contains(Center)) return 8;
    const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
    if (!Data || !Data->HasWorld()) return INDEX_NONE;
    int32 Region = Data->RegionAt(Center.X, Center.Y);
    // o mapa das regiões marca lagos e rios como mar: só é mar sem terra a 60 m
    for (int32 Ring = 1; Ring <= 4 && Region == 0; ++Ring)
        for (int32 A = 0; A < 8 && Region == 0; ++A)
            Region = Data->RegionAt(Center.X + FMath::Cos(A * UE_PI / 4.f) * Ring * 1500.f, Center.Y + FMath::Sin(A * UE_PI / 4.f) * Ring * 1500.f);
    switch (Region)
    {
    case 0: return 0;
    case 2: return 1;
    case 3: return 2;
    case 4: return 3;
    case 5: return 4;
    case 7: return 5;
    case 8: return 6;
    case 9: return 7;
    default: return INDEX_NONE;
    }
}

void AVortexForestLife::MixPlace(int32 Index, float DeltaSeconds)
{
    static const TCHAR* Sounds[PlaceCount] = { TEXT("amb_ocean"), TEXT("amb_volcano"), TEXT("amb_storm"), TEXT("amb_snow"), TEXT("amb_desert"),
        TEXT("amb_swamp"), TEXT("amb_haunted"), TEXT("amb_elven"), TEXT("amb_town") };
    FLoop& Loop = Places[Index];
    const float Target = PlaceTarget[Index];
    if (!Loop.Audio)
    {
        if (Target <= 0.f) return;
        MakeLoop(Loop, *FString::Printf(TEXT("/Game/Vortex/Audio/%s.%s"), Sounds[Index], Sounds[Index]));
        if (!Loop.Audio) { PlaceTarget[Index] = 0.f; return; }
    }
    if (Target > 0.f && !Loop.Audio->IsPlaying()) Loop.Audio->Play(FMath::FRand() * 10.f);
    Mix(Loop, Target, DeltaSeconds);
    // sumiu de vez: para de tocar (cada laço tocando custa, mesmo sem volume)
    if (Target <= 0.f && Loop.Volume <= .002f && Loop.Audio->IsPlaying()) Loop.Audio->Stop();
}

int32 AVortexForestLife::VisibleCritters() const
{
    int32 Count = 0;
    for (const FKind* Kind : { &Butterflies, &Seeds, &Fireflies })
    {
        if (Kind->Instances && Kind->Fade > 0.f) Count += Kind->Instances->GetInstanceCount();
    }
    return Count;
}

FString AVortexForestLife::DescribeSound() const
{
    FString Place = TEXT("nenhum");
    for (int32 i = 0; i < PlaceCount; ++i) if (PlaceTarget[i] > 0.f) Place = FString::Printf(TEXT("%d(%.2f)"), i, Places[i].Volume);
    return FString::Printf(TEXT("dia=%.2f noite=%.2f vento=%.2f agua=%.2f lugar=%s borboletas=%.2f vagalumes=%.2f"),
        Day.Volume, Night.Volume, Wind.Volume, Water.Volume, *Place, Butterflies.Fade, Fireflies.Fade);
}
