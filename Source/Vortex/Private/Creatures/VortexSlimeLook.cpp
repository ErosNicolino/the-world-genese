#include "Creatures/VortexSlimeLook.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Creatures/VortexMonster.h"
#include "Creatures/VortexMonsterSpecies.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

namespace
{
    // os dados primitivos que o material lê (Tools/ImportSlime.py)
    constexpr int32 DataSquash = 0, DataStretch = 1, DataWobble = 2, DataDir = 3, DataHeight = 7, DataBlink = 8, DataLean = 9, DataMood = 11, DataOpen = 12, DataTongue = 13, DataAngry = 14;
}

UVortexSlimeLook::UVortexSlimeLook()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

const TCHAR* UVortexSlimeLook::KindOf(FName Species)
{
    if (Species == TEXT("SlimeFire")) return TEXT("Fogo");
    if (Species == TEXT("SlimePoison")) return TEXT("Veneno");
    if (Species == TEXT("SlimeIron")) return TEXT("Ferro");
    return TEXT("Normal");
}

void UVortexSlimeLook::Build(AVortexMonster* Monster, float HalfHeight, float Scale)
{
    if (!Monster) return;
    UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Vortex/Creatures/SlimeNew/SM_Slime/StaticMeshes/SM_Slime.SM_Slime"));
    if (!Mesh) return;
    if (!Body)
    {
        Body = NewObject<UStaticMeshComponent>(Monster, TEXT("SlimeBody"));
        Body->SetupAttachment(Monster->GetCapsuleComponent());
        Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Body->SetCanEverAffectNavigation(false);
        Body->bAffectDistanceFieldLighting = false;
        Body->RegisterComponent();
    }
    Body->SetStaticMesh(Mesh);
    const FString Kind = KindOf(Monster->GetAssetName());
    for (int32 Slot = 0; Slot < Body->GetNumMaterials(); ++Slot)
    {
        const FString SlotName = Body->GetMaterialSlotNames().IsValidIndex(Slot) ? Body->GetMaterialSlotNames()[Slot].ToString() : FString();
        const FString Name = FString::Printf(TEXT("MI_Slime_%s_%s"), *Kind, SlotName.Contains(TEXT("Body")) ? TEXT("Body") : TEXT("Core"));
        if (UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Creatures/SlimeNew/%s.%s"), *Name, *Name)))
            Body->SetMaterial(Slot, Material);
    }
    // o fundo da gota (a origem da malha) no chão da cápsula; a malha olha para +X, a frente do monstro
    BaseLocation = FVector(0.f, 0.f, -HalfHeight);
    Body->SetRelativeLocation(BaseLocation);
    Body->SetRelativeRotation(FRotator::ZeroRotator);
    Body->SetRelativeScale3D(FVector(Scale));
    BodyHeight = Mesh->GetBoundingBox().Max.Z * Scale;
    Body->SetCustomPrimitiveDataFloat(DataHeight, BodyHeight);
    // fogo: faíscas subindo; veneno: fumaça escura (a aura de cura tinha cruzes e parecia cura)
    const TCHAR* Fx = Kind == TEXT("Fogo") ? TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Aura_Fire.NS_Free_Spells_Aura_Fire") : nullptr;
    // veneno: bolhas roxas que sobem do corpo e estouram (feitas aqui; as auras prontas pareciam cura)
    if (Bubbles) { Bubbles->DestroyComponent(); Bubbles = nullptr; }
    if (Kind == TEXT("Veneno"))
    {
        Bubbles = NewObject<UInstancedStaticMeshComponent>(Monster, TEXT("PoisonBubbles"));
        Bubbles->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
        Bubbles->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Creatures/SlimeNew/MI_Slime_Veneno_Bubble.MI_Slime_Veneno_Bubble")));
        Bubbles->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Bubbles->SetCastShadow(false);
        Bubbles->SetupAttachment(Body);
        Bubbles->RegisterComponent();
        Bubbles->SetCustomPrimitiveDataFloat(DataHeight, BodyHeight);
        for (int32 i = 0; i < 7; ++i) Bubbles->AddInstance(FTransform(FQuat::Identity, FVector::ZeroVector, FVector(.001f)));
    }
    if (Aura) { Aura->DestroyComponent(); Aura = nullptr; }
    if (Fx)
        if (UNiagaraSystem* System = LoadObject<UNiagaraSystem>(nullptr, Fx))
        {
            Aura = UNiagaraFunctionLibrary::SpawnSystemAttached(System, Body, NAME_None, FVector(0.f, 0.f, BodyHeight * .15f), FRotator::ZeroRotator,
                FVector(.45f * Scale), EAttachLocation::KeepRelativeOffset, false, ENCPoolMethod::None);
            // pequena: umas faíscas/fumaça subindo da gota (a do tamanho da aura cobria a gota inteira)
            if (Aura) Aura->SetRelativeScale3D(FVector(.2f));
        }
}

void UVortexSlimeLook::TickFace(AVortexMonster* Monster, float Dt, float Now, float Speed, bool bAir)
{
    if (!Body) return;
    // o humor de agora: contente parado (às vezes mostra a língua), sério andando, bravo perseguindo e atacando
    // (sobrancelhas), "o" no ar, a boca aberta no bote, careta apanhando e boca mole morto
    const bool bDead = Monster->IsCreatureDead();
    const bool bAttack = ActClip == TEXT("Attack") || ActClip == TEXT("Leap");
    if (ActClip == TEXT("Hit") && ActAge < .05f) HitFace = .6f;
    HitFace = FMath::Max(0.f, HitFace - Dt);
    float WantMood = 1.f, WantOpen = 0.f, WantTongue = 0.f, WantAngry = 0.f, WantSquint = 0.f;
    if (Speed > 150.f) { WantMood = .1f; WantAngry = .7f; }
    else if (Speed > 30.f) WantMood = .6f;
    else if (Now > NextBleh)
    {
        BlehUntil = Now + .9f;
        NextBleh = Now + FMath::FRandRange(5.f, 10.f);
    }
    if (Now < BlehUntil && Speed <= 30.f) { WantOpen = .25f; WantTongue = 1.f; WantMood = 1.f; WantSquint = .5f; }
    if (bAir) { WantMood = 0.f; WantOpen = .55f; WantTongue = 0.f; }
    if (bAttack) { WantMood = -1.f; WantAngry = 1.f; WantOpen = ActAge < .2f ? .15f : 1.f; WantTongue = 0.f; }
    if (HitFace > 0.f) { WantMood = -.8f; WantOpen = .35f; WantSquint = 1.f; WantAngry = 0.f; WantTongue = 0.f; }
    if (bDead) { WantMood = -.5f; WantOpen = .3f; WantTongue = .6f; WantAngry = 0.f; WantSquint = 0.f; }
    Mood = FMath::FInterpTo(Mood, WantMood, Dt, 10.f);
    MouthOpen = FMath::FInterpTo(MouthOpen, WantOpen, Dt, 14.f);
    Tongue = FMath::FInterpTo(Tongue, WantTongue, Dt, 10.f);
    Angry = FMath::FInterpTo(Angry, WantAngry, Dt, 8.f);
    Squint = FMath::FInterpTo(Squint, WantSquint, Dt, 14.f);
    Body->SetCustomPrimitiveDataFloat(DataMood, Mood);
    Body->SetCustomPrimitiveDataFloat(DataOpen, MouthOpen);
    Body->SetCustomPrimitiveDataFloat(DataTongue, Tongue);
    Body->SetCustomPrimitiveDataFloat(DataAngry, Angry);
}

void UVortexSlimeLook::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    AVortexMonster* Monster = Cast<AVortexMonster>(GetOwner());
    if (!Monster || !Body) return;
    const float Dt = FMath::Min(DeltaTime, 1.f / 20.f);
    const float Now = GetWorld()->GetTimeSeconds();
    if (Monster->GetActSerial() != LastSerial)
    {
        LastSerial = Monster->GetActSerial();
        ActClip = Monster->GetActClip();
        ActAge = 0.f;
        if (ActClip == TEXT("Hit"))
        {
            // apanhou: afunda, chacoalha e o corpo é empurrado para um lado
            SquashSpeed += 3.2f; Wobble = 7.f;
            LeanSpeed += FVector2D(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f)).GetSafeNormal() * 260.f;
        }
    }
    if (Seed < 0.f) Seed = FMath::FRandRange(0.f, 2.f * PI);
    ActAge += Dt;
    const FVortexSpeciesDef* D = Monster->Def();
    const FVector Velocity = Monster->GetVelocity();
    const float Speed = Velocity.Size2D();
    const bool bAir = Monster->GetCharacterMovement() && Monster->GetCharacterMovement()->IsFalling();
    // 03/11 (o usuário: "ele tá muito duro, até o golpe; estilo Slime Rancher, bem amoeba"): respira em dois ritmos,
    // cada gota no seu tempo; as molas abaixo são moles e passam do ponto (a gelatina chacoalha depois de tudo)
    float SquashGoal = .1f * FMath::Sin(Now * 2.1f + Seed) + .035f * FMath::Sin(Now * 3.7f + Seed * 2.f);
    FVector2D LeanGoal = FVector2D(-Velocity.X, -Velocity.Y) * .05f;   // andando, o alto fica um pouco para trás
    float StretchGoal = 0.f;
    float Lift = 0.f;
    if (Monster->IsCreatureDead())
    {
        // derrete: achata, alarga e afunda
        Melt = FMath::Min(1.f, Melt + Dt / 1.2f);
        SquashGoal = .85f * Melt;
        Lift = -BodyHeight * .15f * Melt;
    }
    else if (bAir)
    {
        SquashGoal = Velocity.Z > 0.f ? -.3f : -.12f;       // no ar: subindo, alto e fino; caindo, já se prepara
    }
    else if (Speed > 30.f)
    {
        // aos pulinhos: a fase anda com a distância; amassa quando encosta no chão
        Hop += Dt * Speed / 95.f * PI;
        const float Up = FMath::Abs(FMath::Sin(Hop));
        Lift = Up * BodyHeight * .3f;
        SquashGoal = Up > .2f ? -.3f * Up : .42f * (1.f - Up / .2f);
    }
    // golpes: o salto (encolhe antes de sair) e o bote de gelatina até o alvo
    const FVortexMonsterMove* Move = nullptr;
    if (D) for (const FVortexMonsterMove& M : D->Moves) if (M.Clip == ActClip) { Move = &M; break; }
    if (Move && !Monster->IsCreatureDead())
    {
        const float Impact = Move->Impact;
        const FVector2D Ahead(Monster->GetActorForwardVector().GetSafeNormal2D());
        if (Move->Kind == EVortexMoveKind::Lunge && ActAge < FMath::Max(.05f, Move->LungeFrom)) { SquashGoal = .45f; LeanGoal = -Ahead * 10.f; }
        if (Move->Kind == EVortexMoveKind::Strike || Move->Kind == EVortexMoveKind::Area)
        {
            // o bote de gelatina: encolhe e pende para trás (preparo), dispara o braço que passa um pouco do alcance,
            // segura, e a mola mole puxa de volta chacoalhando
            const float Reach = FMath::Max(40.f, Move->Reach + (D ? D->Radius * .5f : 0.f) - BodyHeight * .45f);
            const float Ready = Impact * .5f;
            if (ActAge < Ready) { SquashGoal = .38f; LeanGoal = -Ahead * 14.f; }
            else if (ActAge < Impact)
            {
                const float U = (ActAge - Ready) / FMath::Max(.03f, Impact - Ready);
                const float Back = 1.f + 2.2f * FMath::Pow(U - 1.f, 3.f) + 1.2f * FMath::Pow(U - 1.f, 2.f);   // sai com folga (passa e volta)
                StretchGoal = Reach * Back;
                SquashGoal = -.05f;
                LeanGoal = Ahead * 12.f;
            }
            else if (ActAge < Impact + .16f) { StretchGoal = Reach; LeanGoal = Ahead * 8.f; Wobble = FMath::Max(Wobble, 2.5f); }
            if (Move->Kind == EVortexMoveKind::Area) StretchGoal = 0.f;
        }
    }
    // pousou: amassa forte
    if (bWasAir && !bAir) { SquashSpeed += 2.6f; Wobble = FMath::Max(Wobble, 6.f); }
    bWasAir = bAir;
    // molas moles (a gelatina passa do ponto e volta umas vezes); antes 180/9, quase sem balanço
    const bool bStretching = StretchGoal > Stretch;
    SquashSpeed += ((SquashGoal - Squash) * 62.f - SquashSpeed * 3.1f) * Dt;
    Squash = FMath::Clamp(Squash + SquashSpeed * Dt, -.45f, Monster->IsCreatureDead() ? .9f : .48f);   // mais que isso vira panqueca
    if (bStretching) { Stretch = FMath::FInterpTo(Stretch, StretchGoal, Dt, 30.f); StretchSpeed = 0.f; }
    else
    {
        StretchSpeed += ((StretchGoal - Stretch) * 75.f - StretchSpeed * 3.4f) * Dt;
        Stretch = FMath::Max(-15.f, Stretch + StretchSpeed * Dt);
        if (FMath::Abs(Stretch) > 8.f) Wobble = FMath::Max(Wobble, 4.f);
    }
    Wobble = FMath::Max(0.f, Wobble - Dt * 2.4f);
    // a inércia: quando a gota arranca, freia ou vira, o alto fica para trás e depois balança até parar
    const FVector2D Velocity2D(Velocity.X, Velocity.Y);
    FVector2D Accel = (Velocity2D - LastVelocity) / Dt;
    LastVelocity = Velocity2D;
    if (Accel.Size() > 4000.f) Accel = Accel.GetSafeNormal() * 4000.f;   // um pulo de posição pela rede não vira um tranco
    if (Monster->IsCreatureDead()) LeanGoal = FVector2D::ZeroVector;
    LeanSpeed += ((LeanGoal - Lean) * 42.f - LeanSpeed * 2.5f - Accel * .07f) * Dt;
    Lean += LeanSpeed * Dt;
    if (Lean.Size() > 46.f) Lean = Lean.GetSafeNormal() * 46.f;
    Body->SetRelativeLocation(BaseLocation + FVector(0.f, 0.f, Lift));
    Body->SetCustomPrimitiveDataFloat(DataSquash, Squash);
    Body->SetCustomPrimitiveDataFloat(DataStretch, FMath::Max(0.f, Stretch));
    Body->SetCustomPrimitiveDataFloat(DataWobble, Wobble + FMath::Min(1.5f, Speed / 200.f));
    const FVector Forward = Monster->GetActorForwardVector().GetSafeNormal2D();
    Body->SetCustomPrimitiveDataVector3(DataDir, FVector(Forward.X, Forward.Y, -.08f));
    // pisca a cada 2,5-5 s (0,14 s de olho fechado); morto, de olho fechado; apanhando, aperta os olhos
    if (Now >= NextBlink) { BlinkStart = Now; NextBlink = Now + FMath::FRandRange(2.5f, 5.f); }
    TickFace(Monster, Dt, Now, Speed, bAir);
    const float Blink = Monster->IsCreatureDead() ? .9f
        : FMath::Max(FMath::Clamp(1.f - FMath::Abs(Now - BlinkStart - .07f) / .07f, 0.f, 1.f) * .9f, Squint * .6f);
    Body->SetCustomPrimitiveDataFloat(DataBlink, Blink);
    Body->SetCustomPrimitiveDataFloat(DataLean, Lean.X);
    Body->SetCustomPrimitiveDataFloat(DataLean + 1, Lean.Y);
    Body->SetVisibility(Melt < 1.f || !Monster->IsCreatureDead());
    if (Aura) Aura->SetVisibility(!Monster->IsCreatureDead());
    if (Bubbles)
    {
        // cada bolha nasce num ponto do alto da gota, sobe, cresce e estoura (ciclo de 1,6 s, cada uma num tempo)
        TArray<FTransform> Each;
        // no espaço do corpo (que já sobe e desce com o pulo)
        const float Top = BodyHeight * (1.f - FMath::Max(0.f, Squash)) / FMath::Max(.01f, Body->GetRelativeScale3D().Z);
        for (int32 i = 0; i < Bubbles->GetInstanceCount(); ++i)
        {
            const float T = FMath::Fmod(Now / 1.6f + i * .37f, 1.f);
            const float Angle = (i * 2.4f + FMath::FloorToFloat(Now / 1.6f + i * .37f) * 1.7f);
            const FVector From(FMath::Cos(Angle) * 14.f, FMath::Sin(Angle) * 14.f, Top * .85f);
            const float Grow = T < .85f ? FMath::Lerp(.04f, .11f, T / .85f) : .11f * (1.f - (T - .85f) / .15f) * 1.6f;
            Each.Add(FTransform(FQuat::Identity, From + FVector(0.f, 0.f, T * 38.f), FVector(Monster->IsCreatureDead() ? .001f : Grow)));
        }
        Bubbles->BatchUpdateInstancesTransforms(0, Each, false, true);
    }
}
