#include "World/VortexFairies.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/PoseableMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Texture.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "World/VortexDayNightSubsystem.h"
#include "World/VortexTerrainData.h"
#include "World/VortexTerrainStreamer.h"

namespace
{
    const TCHAR* Root = TEXT("/Game/Vortex/Creatures/Fada/");
    const TCHAR* HairColors[] = { TEXT("rosa"), TEXT("lilas"), TEXT("menta"), TEXT("loiro"), TEXT("ruivo"), TEXT("azul") };
    constexpr int32 Styles = 6;
}

AVortexFairies::AVortexFairies()
{
    PrimaryActorTick.bCanEverTick = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

FVector AVortexFairies::CameraLocation(FVector* Forward) const
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

void AVortexFairies::BeginPlay()
{
    Super::BeginPlay();
    if (GetNetMode() == NM_DedicatedServer) { Destroy(); return; }
    // os ajustes (só números)
    FString Text;
    TSharedPtr<FJsonObject> Json;
    if (FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir() / TEXT("Data/Creatures/Fada/fadas.json")))
        && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json) && Json.IsValid())
        for (const auto& Pair : Json->Values)
        {
            double Number = 0.0;
            if (Pair.Value.IsValid() && Pair.Value->TryGetNumber(Number)) Tune.Add(FString(Pair.Key), float(Number));
        }
    for (int32 i = 0; i < Styles; ++i)
        if (USkeletalMesh* M = LoadObject<USkeletalMesh>(nullptr, *FString::Printf(TEXT("%sSK_Fada_%d.SK_Fada_%d"), Root, i, i)))
        {
            Meshes.Add(M);
            Keep.Add(M);
        }
    // as paletas de cabelo já no começo (a tela de carregamento cobre): carregar uma no meio do jogo travava o quadro
    for (int32 i = 0; i < Meshes.Num(); ++i)
        for (const TCHAR* Color : HairColors)
            if (UTexture* Palette = LoadObject<UTexture>(nullptr, *FString::Printf(TEXT("%sT_Fada_%d_%s.T_Fada_%d_%s"), Root, i, Color, i, Color))) Keep.Add(Palette);
    if (Meshes.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("VORTEX_FAIRIES sem as malhas (Tools/ImportFairy.py)"));
        Destroy();
        return;
    }
    WingBones = { TEXT("asa_cima_E"), TEXT("asa_cima_D"), TEXT("asa_baixo_E"), TEXT("asa_baixo_D") };
    Fairies.SetNum(FMath::Clamp(int32(T(TEXT("quantas"), 4.f)), 0, 16));
    for (FFairy& F : Fairies)
    {
        F.Mesh = NewObject<UPoseableMeshComponent>(this);
        F.Mesh->SetupAttachment(GetRootComponent());
        F.Mesh->SetUsingAbsoluteLocation(true);
        F.Mesh->SetUsingAbsoluteRotation(true);
        F.Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        // sem sombra: com 9 cm a fada se cobria inteira com a própria sombra (ficava preta), e no chão ela nem aparece
        F.Mesh->SetCastShadow(false);
        F.Mesh->SetCanEverAffectNavigation(false);
        F.Mesh->SetVisibility(false);
        F.Mesh->RegisterComponent();
    }
    // a pose de descanso das asas e para onde a malha olha (o rosto): a frente sai dos braços e do "para cima"
    if (!Fairies.IsEmpty())
    {
        UPoseableMeshComponent* Probe = Fairies[0].Mesh;
        Probe->SetSkinnedAssetAndUpdate(Meshes[0]);
        for (const FName& Bone : WingBones) Rest.Add(Bone, Probe->GetBoneTransformByName(Bone, EBoneSpaces::ComponentSpace));
        const FVector Right = Probe->GetBoneLocationByName(TEXT("braco_E"), EBoneSpaces::ComponentSpace) - Probe->GetBoneLocationByName(TEXT("braco_D"), EBoneSpaces::ComponentSpace);
        const FVector Front = FVector::CrossProduct(Right, FVector::UpVector).GetSafeNormal2D();
        YawFix = -FMath::RadiansToDegrees(FMath::Atan2(Front.Y, Front.X)) + T(TEXT("ajusteYaw"), 0.f);
    }
    Rng.Initialize(int32(FPlatformTime::Cycles() & 0x7fffffff));
    UE_LOG(LogTemp, Display, TEXT("VORTEX_FAIRIES %d fadas, %d malhas, giro %.0f"), Fairies.Num(), Meshes.Num(), YawFix);
}

void AVortexFairies::Dress(FFairy& F, int32 Style)
{
    if (Style < 0 || Style >= Meshes.Num()) Style = Rng.RandRange(0, Meshes.Num() - 1);
    F.Mesh->SetSkinnedAssetAndUpdate(Meshes[Style]);
    // 11/11: sem limpar, o material da vez anterior (já uma cópia) virava a base da nova: a cadeia crescia a cada volta
    F.Mesh->EmptyOverrideMaterials();
    const TCHAR* Color = HairColors[Rng.RandRange(0, UE_ARRAY_COUNT(HairColors) - 1)];
    UTexture* Palette = LoadObject<UTexture>(nullptr, *FString::Printf(TEXT("%sT_Fada_%d_%s.T_Fada_%d_%s"), Root, Style, Color, Style, Color));
    F.Wing = F.Face = nullptr;
    F.Frame = -1;
    for (int32 i = 0; i < F.Mesh->GetNumMaterials(); ++i)
    {
        UMaterialInterface* M = F.Mesh->GetMaterial(i);
        if (!M) continue;
        UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(M, this);
        if (M->GetName().Contains(TEXT("Asa"))) F.Wing = D;
        else
        {
            // o brilho próprio do corpo e do rosto (fadas.json, brilhoCorpo): na sombra, sem ele, ela ficava preta
            D->SetScalarParameterValue(TEXT("BrilhoProprio"), T(TEXT("brilhoCorpo"), 3.f));
            if (M->GetName().Contains(TEXT("Rosto"))) F.Face = D;
            else if (Palette) D->SetTextureParameterValue(TEXT("Base"), Palette);
        }
        F.Mesh->SetMaterial(i, D);
    }
    F.Mesh->SetWorldScale3D(FVector(T(TEXT("escala"), 1.f) * Rng.FRandRange(.92f, 1.08f)));
}

bool AVortexFairies::FindSpot(const FVector& Camera, const FVector& Forward, bool bBehind, FVector& Out)
{
    const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
    if (!Data) return false;
    for (int32 Try = 0; Try < 24; ++Try)
    {
        float Angle = Rng.FRand() * UE_TWO_PI;
        if (bBehind) Angle = FMath::Atan2(-Forward.Y, -Forward.X) + Rng.FRandRange(-1.3f, 1.3f);
        const float D = Rng.FRandRange(T(TEXT("perto"), 900.f), T(TEXT("longe"), 2600.f));
        const float X = Camera.X + FMath::Cos(Angle) * D, Y = Camera.Y + FMath::Sin(Angle) * D;
        const float Ground = Data->GroundAt(X, Y);
        float Surface = 0.f;
        if (Ground <= -299.f || Data->IsWaterAt(X, Y, Surface)) continue;
        Out = FVector(X, Y, Ground);
        return true;
    }
    return false;
}

void AVortexFairies::Spawn(FFairy& F, const FVector& Camera, const FVector& Forward, bool bFirst)
{
    FVector Ground;
    if (!FindSpot(Camera, Forward, !bFirst, Ground)) { F.bActive = false; F.Mesh->SetVisibility(false); return; }
    Dress(F);
    F.Home = Ground;
    F.Pos = Ground + FVector(0.f, 0.f, Rng.FRandRange(T(TEXT("alturaMin"), 70.f), T(TEXT("alturaMax"), 230.f)));
    F.Target = F.Pos;
    F.Vel = FVector::ZeroVector;
    F.Phase = Rng.FRand() * UE_TWO_PI;
    F.Pause = Rng.FRandRange(.2f, 1.5f);
    F.Flee = 0.f;
    F.Yaw = Rng.FRand() * 360.f;
    F.bActive = true;
    F.Mesh->SetVisibility(true);
}

void AVortexFairies::Animate(FFairy& F, float Speed)
{
    // as asas giram em volta do "para cima" na raiz (abrem e fecham como um livro), o par de baixo atrasado
    const float Open = FMath::DegreesToRadians(T(TEXT("aberturaAsa"), 55.f));
    const float Lag = T(TEXT("atrasoAsaBaixo"), .25f) * UE_TWO_PI;
    for (const FName& Bone : WingBones)
    {
        const FTransform* R = Rest.Find(Bone);
        if (!R) continue;
        const FString Name = Bone.ToString();
        const float Side = Name.EndsWith(TEXT("_D")) ? 1.f : -1.f;
        const bool bLow = Name.Contains(TEXT("baixo"));
        const float A = Open * (.15f + .85f * FMath::Sin(F.Phase - (bLow ? Lag : 0.f)));
        // um pouco de torção junto (o "oito" das asas de inseto)
        const FQuat Sweep(FVector::UpVector, Side * A);
        const FQuat Twist(R->GetRotation().GetAxisX(), .25f * FMath::Cos(F.Phase - (bLow ? Lag : 0.f)));
        F.Mesh->SetBoneTransformByName(Bone, FTransform(Sweep * Twist * R->GetRotation(), R->GetLocation(), R->GetScale3D()), EBoneSpaces::ComponentSpace);
    }
    // o corpo: sobe e desce com as batidas, inclina para a frente quando voa rápido
    const float Bob = 2.5f * FMath::Sin(F.Phase * .5f) * F.Mesh->GetComponentScale().Z;
    const float Pitch = -FMath::Clamp(Speed / FMath::Max(1.f, T(TEXT("velocidade"), 130.f)) * 14.f, 0.f, 30.f);
    F.Mesh->SetWorldLocationAndRotation(F.Pos + FVector(0.f, 0.f, Bob), FRotator(Pitch, F.Yaw + YawFix, 0.f));
}

void AVortexFairies::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const float Dt = FMath::Min(DeltaSeconds, .1f);
    Clock += Dt;
    const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
    if (Fairies.IsEmpty()) return;
    if (!Data)
    {
        // sem terreno (mapas de teste): só as postas à mão batem as asas
        for (FFairy& F : Fairies)
            if (F.bActive)
            {
                F.Phase = FMath::Fmod(F.Phase + UE_TWO_PI * T(TEXT("batidasParada"), 7.f) * Dt, UE_TWO_PI * 2.f);
                Animate(F, 0.f);
            }
        return;
    }
    const UVortexDayNightSubsystem* DayNight = GetWorld()->GetSubsystem<UVortexDayNightSubsystem>();
    Daylight = DayNight && DayNight->IsActive() ? DayNight->GetDaylight() : 1.f;
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
    const float FleeD = T(TEXT("fugaDistancia"), 450.f), Cruise = T(TEXT("velocidade"), 130.f);
    for (FFairy& F : Fairies)
    {
        if (F.bHeld)
        {
            F.Phase = FMath::Fmod(F.Phase + UE_TWO_PI * T(TEXT("batidasParada"), 7.f) * Dt, UE_TWO_PI * 2.f);
            Animate(F, 0.f);
            Expression(F, Dt);
            continue;
        }
        if (!F.bActive || FVector::Dist2D(F.Pos, Camera) > T(TEXT("some"), 6000.f))
        {
            Spawn(F, Camera, Forward, !F.bActive && Clock < 1.f);
            if (!F.bActive) continue;
        }
        // foge de quem chegou perto: para longe e para cima, rápido
        if (F.Flee <= 0.f)
            for (const FVector& S : Scarers)
                if (FVector::Dist(S, F.Pos) < FleeD)
                {
                    F.Flee = T(TEXT("fugaTempo"), 3.5f);
                    FVector Away = (F.Pos - S).GetSafeNormal2D();
                    if (Away.IsNearlyZero()) Away = FVector(1.f, 0.f, 0.f);
                    F.Vel = (Away + FVector(0.f, 0.f, .55f)).GetSafeNormal() * T(TEXT("fugaVelocidade"), 650.f);
                    break;
                }
        const float Ground = Data->GroundAt(F.Pos.X, F.Pos.Y);
        if (F.Flee > 0.f)
        {
            F.Flee -= Dt;
            F.Vel.Z += 60.f * Dt;
            if (F.Flee <= 0.f)
            {
                // passou o susto: longe da vista volta em outro lugar; perto, faz ali a casa nova
                if (FVector::Dist2D(F.Pos, Camera) > T(TEXT("longe"), 2600.f)) { F.bActive = false; F.Mesh->SetVisibility(false); continue; }
                F.Home = FVector(F.Pos.X, F.Pos.Y, Ground);
                F.Target = F.Pos;
            }
        }
        else if (F.Pause > 0.f)
        {
            // parada no ar
            F.Pause -= Dt;
            F.Vel = FMath::Lerp(F.Vel, FVector::ZeroVector, FMath::Min(1.f, Dt * 3.f));
        }
        else
        {
            if (FVector::Dist(F.Pos, F.Target) < 40.f)
            {
                const float A = Rng.FRand() * UE_TWO_PI, R = Rng.FRandRange(.3f, 1.f) * T(TEXT("voltaRaio"), 320.f);
                const float X = F.Home.X + FMath::Cos(A) * R, Y = F.Home.Y + FMath::Sin(A) * R;
                F.Target = FVector(X, Y, Data->GroundAt(X, Y) + Rng.FRandRange(T(TEXT("alturaMin"), 70.f), T(TEXT("alturaMax"), 230.f)));
                if (Rng.FRand() < .45f)
                {
                    F.Pause = Rng.FRandRange(.6f, 2.2f);
                    if (Rng.FRand() < .4f) F.Happy = F.Pause;   // às vezes para no ar sorrindo
                }
            }
            const FVector Want = (F.Target - F.Pos).GetSafeNormal() * Cruise;
            F.Vel = FMath::Lerp(F.Vel, Want, FMath::Min(1.f, Dt * 1.6f));
        }
        F.Pos += F.Vel * Dt;
        F.Pos.Z = FMath::Max(F.Pos.Z, Ground + 35.f);
        const float Speed = F.Vel.Size2D();
        if (Speed > 15.f) F.Yaw = FMath::FixedTurn(F.Yaw, FMath::RadiansToDegrees(FMath::Atan2(F.Vel.Y, F.Vel.X)), 360.f * Dt);
        const float Rate = (F.Pause > 0.f && F.Flee <= 0.f ? T(TEXT("batidasParada"), 7.f) : T(TEXT("batidas"), 11.f)) * (F.Flee > 0.f ? 1.6f : 1.f);
        F.Phase = FMath::Fmod(F.Phase + UE_TWO_PI * Rate * Dt, UE_TWO_PI * 2.f);
        Animate(F, Speed);
        Expression(F, Dt);
        if (F.Wing) F.Wing->SetScalarParameterValue(TEXT("Brilho"), FMath::Lerp(T(TEXT("brilhoNoite"), 2.5f), T(TEXT("brilhoDia"), .8f), Daylight));
    }
}

void AVortexFairies::Expression(FFairy& F, float Dt)
{
    // como o rosto do herói: um quadro de cada vez; pisca sozinha, sorri em algumas paradas, assusta quando foge
    F.Blink -= Dt;
    F.Happy = FMath::Max(0.f, F.Happy - Dt);
    if (F.Blink < -.13f) F.Blink = Rng.FRandRange(2.f, 5.f);
    const int32 Want = F.Flee > 0.f ? 3 : F.Blink < 0.f ? 1 : F.Happy > 0.f ? 2 : 0;
    if (Want != F.Frame && F.Face)
    {
        F.Frame = Want;
        F.Face->SetScalarParameterValue(TEXT("Quadro"), float(Want));
    }
}

int32 AVortexFairies::Visible() const { int32 N = 0; for (const FFairy& F : Fairies) N += F.bActive; return N; }
int32 AVortexFairies::Fleeing() const { int32 N = 0; for (const FFairy& F : Fairies) N += F.bActive && F.Flee > 0.f; return N; }

bool AVortexFairies::Summon(const FVector& At)
{
    if (Fairies.IsEmpty() || Meshes.IsEmpty()) return false;
    const FVector Camera = CameraLocation();
    FFairy* Pick = &Fairies[0];
    for (FFairy& F : Fairies)
        if (!F.bHeld && (!F.bActive || FVector::DistSquared(F.Pos, Camera) > FVector::DistSquared(Pick->Pos, Camera))) Pick = &F;
    Dress(*Pick);
    Pick->bActive = true;
    Pick->bHeld = false;
    Pick->Mesh->SetVisibility(true);
    Pick->Home = Pick->Target = Pick->Pos = At;
    Pick->Vel = FVector::ZeroVector;
    Pick->Pause = 1.f;
    Pick->Flee = 0.f;
    return true;
}

void AVortexFairies::DebugPlace(const FVector& At, float Yaw, int32 Style)
{
    if (Fairies.IsEmpty() || Meshes.IsEmpty()) return;
    FFairy& F = Fairies[0];
    F.Mesh->EmptyOverrideMaterials();
    Dress(F, Style);
    F.Yaw = Yaw;
    F.bActive = true;
    F.Mesh->SetVisibility(true);
    F.Home = F.Target = F.Pos = At;
    F.Vel = FVector::ZeroVector;
    F.Pause = 1000.f;
    F.Flee = 0.f;
    F.bHeld = true;
}
