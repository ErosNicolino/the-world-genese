#include "World/VortexTownLife.h"

#include "Animation/AnimSequence.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "World/VortexTerrainData.h"
#include "World/VortexTerrainStreamer.h"
#include "World/VortexTown.h"
#include "World/VortexWorldHooks.h"
#include "World/VortexWorldLayout.h"

namespace
{
    template <typename T> T* Load(const FString& Path) { return LoadObject<T>(nullptr, *Path); }
    // os .glb do Cube World Kit (limpos no Tools/Blender/PrepTownAnimals.py) olham para -Y
    constexpr float ImportScale = 1.f;
    // 12/11 (o usuário: "os animais andando de costas"): nos modelos do Cube World a cabeça de uns aponta para um lado e a
    // de outros para o outro (medido no Blender: porco, ovelha e guaxinim ao contrário do gato, das galinhas e do cachorro)
    float MeshYaw(FName Kind) { return Kind == TEXT("Pig") || Kind == TEXT("Sheep") || Kind == TEXT("Raccoon") ? 90.f : -90.f; }
    USoundBase* Sfx(const TCHAR* Name) { return Load<USoundBase>(FString::Printf(TEXT("/Game/Vortex/Audio/Sfx/%s.%s"), Name, Name)); }
    // o relógio do jogo em horas (como AVortexGameState::HourOfClock)
    float HourNow(const UWorld* World)
    {
        float Clock = 0.f;
        int32 Day = 0;
        if (!VortexWorldHooks::DayClock(World, Clock, Day)) return 12.f;
        using namespace VortexWorldLayout;
        return Clock < DaySeconds ? 6.f + 12.f * Clock / DaySeconds : FMath::Fmod(18.f + 12.f * (Clock - DaySeconds) / NightSeconds, 24.f);
    }
    bool Between(float Hour, float A, float B) { return A <= B ? Hour >= A && Hour < B : Hour >= A || Hour < B; }
}

AVortexTownLife::AVortexTownLife()
{
    PrimaryActorTick.bCanEverTick = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

float AVortexTownLife::GroundAt(const FVector2D& At) const
{
    const TSharedPtr<FVortexTerrainData> Terrain = AVortexTerrainStreamer::SharedData();
    return Terrain ? Terrain->GroundAt(At.X, At.Y) : 0.f;
}

void AVortexTownLife::Init(const TSharedPtr<FJsonObject>& Plan)
{
    Near = NewObject<USoundAttenuation>(this);
    Near->Attenuation.AttenuationShapeExtents = FVector(200.f, 0.f, 0.f);
    Near->Attenuation.FalloffDistance = 1800.f;
    Far = NewObject<USoundAttenuation>(this);
    Far->Attenuation.AttenuationShapeExtents = FVector(600.f, 0.f, 0.f);
    Far->Attenuation.FalloffDistance = 5000.f;
    // ---- os tipos: (nome, escala, passo, corrida, distância da fuga, intervalo dos sons, sons, animações parado/andando/correndo/comendo)
    struct FSpec { const TCHAR* Name; float Scale, Walk, Run, Flee, Sound; TArray<const TCHAR*> Sounds; const TCHAR* Anims[4]; };
    const FSpec Specs[] = {
        { TEXT("Chicken"), .36f, 45.f, 260.f, 250.f, 9.f, { TEXT("anim_hen_1"), TEXT("anim_hen_2"), TEXT("anim_hen_3"), TEXT("anim_hen_4") }, { TEXT("Idle"), TEXT("Run"), TEXT("Run"), TEXT("Idle_Peck") } },
        { TEXT("Chick"), .2f, 40.f, 220.f, 200.f, 14.f, { TEXT("anim_hen_2"), TEXT("anim_hen_3") }, { TEXT("Idle"), TEXT("Run"), TEXT("Run"), TEXT("Idle_Peck") } },
        { TEXT("Cat"), .21f, 55.f, 320.f, 220.f, 30.f, { TEXT("anim_cat_1"), TEXT("anim_cat_2"), TEXT("anim_cat_3") }, { TEXT("Idle"), TEXT("Walk"), TEXT("Run"), TEXT("Idle_Eating") } },
        { TEXT("Dog"), .32f, 95.f, 330.f, 0.f, 40.f, { TEXT("anim_dog_1"), TEXT("anim_dog_2"), TEXT("anim_dog_3") }, { TEXT("Idle"), TEXT("Walk"), TEXT("Run"), TEXT("Idle_Eating") } },
        { TEXT("Sheep"), .55f, 45.f, 160.f, 0.f, 26.f, { TEXT("anim_sheep_1"), TEXT("anim_sheep_2"), TEXT("anim_sheep_3") }, { TEXT("Idle"), TEXT("Walk"), TEXT("Run"), TEXT("Idle_Eating") } },
        { TEXT("Pig"), .45f, 45.f, 180.f, 0.f, 30.f, { TEXT("anim_pig_1") }, { TEXT("Idle"), TEXT("Walk"), TEXT("Run"), TEXT("Idle_Eating") } },
    };
    TMap<FName, int32> KindOf;
    TMap<FName, USkeletalMesh*> MeshOf;
    for (const FSpec& Spec : Specs)
    {
        const FString Folder = FString::Printf(TEXT("/Game/Vortex/Town/Animals/%s/"), Spec.Name);
        USkeletalMesh* Mesh = Load<USkeletalMesh>(FString::Printf(TEXT("%sSK_%s.SK_%s"), *Folder, Spec.Name, Spec.Name));
        if (!Mesh) { UE_LOG(LogTemp, Warning, TEXT("VORTEX_TOWN_LIFE sem o bicho %s (Tools/ImportTownLife.py)"), Spec.Name); continue; }
        FKind& Kind = Kinds.AddDefaulted_GetRef();
        Kind.Name = Spec.Name;
        Kind.Scale = Spec.Scale; Kind.Walk = Spec.Walk; Kind.Run = Spec.Run; Kind.Flee = Spec.Flee; Kind.Sound = Spec.Sound;
        for (const TCHAR* Name : Spec.Sounds) if (USoundBase* Sound = Sfx(Name)) { Kind.Sounds.Add(Sound); Keep.Add(Sound); }
        for (int32 i = 0; i < 4; ++i)
        {
            Kind.Anims[i] = Load<UAnimSequence>(FString::Printf(TEXT("%sA_%s_%s.A_%s_%s"), *Folder, Spec.Name, Spec.Anims[i], Spec.Name, Spec.Anims[i]));
            if (Kind.Anims[i]) Keep.Add(Kind.Anims[i]);
        }
        KindOf.Add(Spec.Name, Kinds.Num() - 1);
        MeshOf.Add(Spec.Name, Mesh);
        Keep.Add(Mesh);
    }
    const TArray<TSharedPtr<FJsonValue>>* List = nullptr;
    if (Plan->TryGetArrayField(TEXT("animals"), List))
        for (const TSharedPtr<FJsonValue>& Value : *List)
        {
            const TArray<TSharedPtr<FJsonValue>>& E = Value->AsArray();
            if (E.Num() < 4) continue;
            const FName Name(*E[0]->AsString());
            const int32* Kind = KindOf.Find(Name);
            if (!Kind) continue;
            FAnimal& A = Animals.AddDefaulted_GetRef();
            A.Kind = *Kind;
            A.Home = A.At = A.Target = FVector2D(E[1]->AsNumber(), E[2]->AsNumber()) * 100.f;
            A.Radius = E[3]->AsNumber() * 100.f;
            A.Yaw = FMath::FRandRange(0.f, 360.f);
            A.Timer = FMath::FRandRange(0.f, 4.f);
            A.SoundIn = FMath::FRandRange(2.f, Kinds[A.Kind].Sound);
            USkeletalMeshComponent* Mesh = NewObject<USkeletalMeshComponent>(this);
            Mesh->SetSkeletalMesh(MeshOf[Name]);
            Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Mesh->SetCastShadow(true);
            Mesh->bCastDynamicShadow = true;
            Mesh->SetCachedMaxDrawDistance(6000.f);
            Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
            Mesh->bEnableUpdateRateOptimizations = true;
            Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
            Mesh->SetupAttachment(GetRootComponent());
            Mesh->RegisterComponent();
            Mesh->SetWorldScale3D(FVector(Kinds[A.Kind].Scale * ImportScale));
            Mesh->SetWorldLocationAndRotation(FVector(A.At, GroundAt(A.At)), FRotator(0.f, A.Yaw + MeshYaw(Kinds[A.Kind].Name), 0.f));
            A.Mesh = Mesh;
            SetAct(A, EAct::Idle);
        }
    // ---- os lugares com som (as construções com nome da vila)
    FVector2D Market = FVector2D::ZeroVector;
    int32 Stalls = 0;
    for (const TSharedPtr<FJsonValue>& Value : Plan->GetArrayField(TEXT("pieces")))
    {
        const TSharedPtr<FJsonObject> P = Value->AsObject();
        const FString Model = P->GetStringField(TEXT("model")), Label = P->GetStringField(TEXT("label"));
        const FVector2D At = FVector2D(P->GetNumberField(TEXT("x")), P->GetNumberField(TEXT("z"))) * 100.f;
        if (Model.StartsWith(TEXT("vila_barraca"))) { Market += At; ++Stalls; }
        const FName Kind = Label == TEXT("FORJA DE BRAM") ? FName(TEXT("forge")) : Label.StartsWith(TEXT("TAVERNA")) ? FName(TEXT("tavern"))
            : Label == TEXT("CAPELA") ? FName(TEXT("bell")) : Label == TEXT("ESTÁBULO") ? FName(TEXT("stable")) : NAME_None;
        if (!Kind.IsNone()) Places.Add({ Kind, FVector(At, GroundAt(At) + 200.f) });
    }
    if (Stalls) { const FVector2D At = Market / Stalls; Places.Add({ TEXT("market"), FVector(At, GroundAt(At) + 150.f) }); }
    for (FPlace& Place : Places)
    {
        const TCHAR* Loop = Place.Kind == TEXT("forge") ? TEXT("town_fire") : Place.Kind == TEXT("tavern") ? TEXT("town_tavern")
            : Place.Kind == TEXT("market") ? TEXT("amb_town") : nullptr;
        USoundBase* Sound = Loop ? (Place.Kind == TEXT("market") ? Load<USoundBase>(TEXT("/Game/Vortex/Audio/amb_town.amb_town")) : Sfx(Loop)) : nullptr;
        if (!Sound) continue;
        UAudioComponent* Audio = NewObject<UAudioComponent>(this);
        Audio->SetSound(Sound);
        Audio->bAutoActivate = false;
        Audio->AttenuationSettings = Place.Kind == TEXT("market") ? Far : Near;
        Audio->SetupAttachment(GetRootComponent());
        Audio->SetWorldLocation(Place.At);
        Audio->RegisterComponent();
        Audio->SetVolumeMultiplier(.001f);
        Audio->Play(FMath::FRandRange(0.f, 5.f));
        Place.Loop = Audio;
    }
    for (const TCHAR* Name : { TEXT("anim_rooster"), TEXT("town_anvil_1"), TEXT("town_anvil_2"), TEXT("town_bell"), TEXT("anim_horse") })
        if (USoundBase* Sound = Sfx(Name)) Keep.Add(Sound);
    UE_LOG(LogTemp, Display, TEXT("VORTEX_TOWN_LIFE %d bichos (%d tipos) e %d lugares com som"), Animals.Num(), Kinds.Num(), Places.Num());
    for (int32 i = 0; i < FMath::Min(3, Animals.Num()); ++i)
        if (const USkeletalMeshComponent* Mesh = Animals[i].Mesh.Get())
            UE_LOG(LogTemp, Display, TEXT("VORTEX_TOWN_LIFE %s em %s, escala %.2f, tamanho %s"), *Kinds[Animals[i].Kind].Name.ToString(),
                *Mesh->GetComponentLocation().ToString(), Mesh->GetComponentScale().X, *Mesh->Bounds.BoxExtent.ToString());
}

void AVortexTownLife::SetAct(FAnimal& A, EAct Act)
{
    A.Act = Act;
    USkeletalMeshComponent* Mesh = A.Mesh.Get();
    UAnimSequence* Anim = Kinds[A.Kind].Anims[int32(Act)];
    if (!Mesh || !Anim) return;
    Mesh->PlayAnimation(Anim, true);
    // a galinha não tem passo: a corrida bem devagar
    const bool bSlowRun = Act == EAct::Walk && Kinds[A.Kind].Anims[1] == Kinds[A.Kind].Anims[2];
    Mesh->SetPlayRate(bSlowRun ? .45f : 1.f);
    Mesh->SetPosition(FMath::FRandRange(0.f, Anim->GetPlayLength()), false);
}

void AVortexTownLife::PickTarget(FAnimal& A, const FVector2D& Away)
{
    const bool bDog = Kinds[A.Kind].Name == TEXT("Dog");
    for (int32 Try = 0; Try < 8; ++Try)
    {
        FVector2D To = Away.IsZero() ? A.Home + FVector2D(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f)) * A.Radius
            : A.At + Away * FMath::FRandRange(250.f, 400.f);
        // fugindo, pode sair um pouco da área (até 1,6x o raio)
        if (FVector2D::Distance(To, A.Home) > A.Radius * (Away.IsZero() ? 1.f : 1.6f))
            To = A.Home + (To - A.Home).GetSafeNormal() * A.Radius * (Away.IsZero() ? 1.f : 1.6f);
        const int32 Kind = AVortexTown::MapKindAt(To.X, To.Y);
        // cachorro: só nas pedras da rua; os outros: nunca dentro de uma construção
        if ((bDog && Kind != 1) || Kind == 2) continue;
        A.Target = To;
        return;
    }
    A.Target = A.At;
}

void AVortexTownLife::TickPlaces(float DeltaSeconds, float Hour, const FVector& Eye)
{
    for (FPlace& Place : Places)
    {
        const float Distance = FVector::Dist(Place.At, Eye);
        if (UAudioComponent* Loop = Place.Loop.Get())
        {
            float Volume = 0.f;
            if (Place.Kind == TEXT("forge")) Volume = Between(Hour, 6.5f, 19.f) ? .8f : .25f;
            else if (Place.Kind == TEXT("tavern")) Volume = Between(Hour, 11.5f, 14.f) || Between(Hour, 17.f, 1.f) ? .9f : .25f;
            else if (Place.Kind == TEXT("market")) Volume = Between(Hour, 7.f, 18.5f) ? .7f : 0.f;
            Loop->SetVolumeMultiplier(FMath::Max(.001f, Volume));
        }
        if (Distance > 6000.f && Place.Kind != TEXT("bell")) continue;
        Place.Timer -= DeltaSeconds;
        if (Place.Kind == TEXT("forge") && Between(Hour, 7.f, 17.f) && Place.Timer <= 0.f)
        {
            // o Bram bate em séries de 3 a 5 e descansa
            if (--Place.Rings > 0) Place.Timer = FMath::FRandRange(.55f, .75f);
            else { Place.Rings = FMath::RandRange(3, 5); Place.Timer = FMath::FRandRange(3.f, 7.f); }
            if (USoundBase* Hit = Sfx(FMath::RandBool() ? TEXT("town_anvil_1") : TEXT("town_anvil_2")))
                UGameplayStatics::PlaySoundAtLocation(this, Hit, Place.At, .55f, FMath::FRandRange(.92f, 1.08f), 0.f, Near);
        }
        else if (Place.Kind == TEXT("stable") && Place.Timer <= 0.f)
        {
            Place.Timer = FMath::FRandRange(20.f, 45.f);
            if (USoundBase* Horse = Sfx(TEXT("anim_horse"))) UGameplayStatics::PlaySoundAtLocation(this, Horse, Place.At, .6f, FMath::FRandRange(.9f, 1.1f), 0.f, Near);
        }
        else if (Place.Kind == TEXT("bell"))
        {
            // o sino às 6, 12 e 18 h: 3 badaladas
            for (const float Ring : { 6.f, 12.f, 18.f })
                if (LastHour >= 0.f && LastHour < Ring && Hour >= Ring && Hour - LastHour < 1.f) { Place.Rings = 3; Place.Timer = 0.f; }
            if (Place.Rings > 0 && Place.Timer <= 0.f)
            {
                --Place.Rings;
                Place.Timer = 2.6f;
                if (USoundBase* Bell = Sfx(TEXT("town_bell"))) UGameplayStatics::PlaySoundAtLocation(this, Bell, Place.At + FVector(0.f, 0.f, 800.f), .8f, 1.f, 0.f, Far);
            }
        }
    }
}

void AVortexTownLife::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const APlayerController* Viewer = GetWorld()->GetFirstPlayerController();
    if (!Viewer || !Viewer->PlayerCameraManager) return;
    const FVector Eye = Viewer->PlayerCameraManager->GetCameraLocation();
    const FVector Hero = Viewer->GetPawn() ? Viewer->GetPawn()->GetActorLocation() : Eye;
    const float Hour = HourNow(GetWorld());
    const bool bNight = !Between(Hour, 6.f, 19.f), bDawn = Between(Hour, 5.5f, 7.5f);
    TickPlaces(DeltaSeconds, Hour, Eye);
    LastHour = Hour;
    for (FAnimal& A : Animals)
    {
        USkeletalMeshComponent* Mesh = A.Mesh.Get();
        if (!Mesh) continue;
        const FKind& Kind = Kinds[A.Kind];
        const float ToEye = FVector2D::Distance(A.At, FVector2D(Eye));
        if (ToEye > 5000.f) continue;   // longe: parado (e sem animar, fora da tela)
        // sons, só com alguém perto
        A.SoundIn -= DeltaSeconds;
        if (A.SoundIn <= 0.f)
        {
            const bool bDog = Kind.Name == TEXT("Dog");
            A.SoundIn = FMath::FRandRange(.6f, 1.4f) * Kind.Sound * (bDog && bNight ? .5f : 1.f);
            const float Reach = bDog ? 3500.f : 2500.f;
            if (ToEye < Reach && Kind.Sounds.Num())
            {
                USoundBase* Sound = Kind.Sounds[FMath::RandHelper(Kind.Sounds.Num())];
                // de manhã, uma galinha de vez em quando é o galo
                if (Kind.Name == TEXT("Chicken") && bDawn && FMath::FRand() < .25f) Sound = Sfx(TEXT("anim_rooster"));
                const float Pitch = Kind.Name == TEXT("Chick") ? 1.6f : FMath::FRandRange(.92f, 1.08f);
                UGameplayStatics::PlaySoundAtLocation(this, Sound, Mesh->GetComponentLocation() + FVector(0.f, 0.f, 30.f), .7f, Pitch, 0.f, Near);
            }
        }
        // fugir de quem chega perto (o herói)
        const FVector2D FromHero = A.At - FVector2D(Hero);
        if (Kind.Flee > 0.f && FromHero.Size() < Kind.Flee && A.Act != EAct::Run)
        {
            PickTarget(A, FromHero.GetSafeNormal());
            SetAct(A, EAct::Run);
            A.Timer = 1.8f;
        }
        A.Timer -= DeltaSeconds;
        if (A.Act == EAct::Walk || A.Act == EAct::Run)
        {
            const FVector2D To = A.Target - A.At;
            const float Speed = A.Act == EAct::Run ? Kind.Run : Kind.Walk;
            if (To.Size() < 20.f || A.Timer <= 0.f)
            {
                SetAct(A, FMath::FRand() < .55f ? EAct::Eat : EAct::Idle);
                A.Timer = FMath::FRandRange(3.f, 9.f);
            }
            else
            {
                A.At += To.GetSafeNormal() * FMath::Min(Speed * DeltaSeconds, float(To.Size()));
                A.Yaw = FMath::FixedTurn(A.Yaw, FMath::RadiansToDegrees(FMath::Atan2(To.Y, To.X)), 360.f * DeltaSeconds);
            }
        }
        else if (A.Timer <= 0.f)
        {
            PickTarget(A, FVector2D::ZeroVector);
            SetAct(A, EAct::Walk);
            A.Timer = 8.f;
        }
        Mesh->SetWorldLocationAndRotation(FVector(A.At, GroundAt(A.At)), FRotator(0.f, A.Yaw + MeshYaw(Kinds[A.Kind].Name), 0.f));
    }
}
