#include "Animation/VortexAnimInstance.h"
#include "Creatures/VortexNpc.h"
#include "Combat/VortexSwords.h"
#include "Misc/PackageName.h"
#include "Engine/SkeletalMesh.h"

#include "Animation/AnimSequence.h"
#include "AnimationRuntime.h"
#include "Animation/AnimationPoseData.h"
#include "Framework/VortexCharacter.h"
#include "Framework/VortexCharacterMovementComponent.h"
#include "Combat/VortexWeaponComponent.h"
#include "Combat/VortexWeaponDefinition.h"
#include "Combat/VortexClassSkills.h"
#include "Rpg/VortexHeroSheet.h"
#include "Combat/VortexAttributes.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "ReferenceSkeleton.h"

namespace
{
    /** O osso do herói antigo, ou o do Mixamo no mesmo papel (o herói do Mixamo como veio: mixamorig:Spine...). */
    int32 RoleBone(const FReferenceSkeleton& Reference, const TCHAR* Old, const TCHAR* Mixamo)
    {
        for (const TCHAR* Prefix : { TEXT("mixamorig:"), TEXT("mixamorig_") })
            if (const int32 Bone = Reference.FindBoneIndex(FName(*(FString(Prefix) + Mixamo))); Bone != INDEX_NONE) return Bone;
        return Reference.FindBoneIndex(Old);
    }
}

bool UVortexAnimInstance::Fits(const UAnimSequence* Sequence) const
{
    // os clipes do aventureiro (armas, habilidades, beber) não servem no esqueleto do Mixamo: tocavam por cima ossos
    // que não existem e entortavam o corpo
    const USkeletalMeshComponent* MeshComponent = GetSkelMeshComponent();
    const USkeletalMesh* Mesh = MeshComponent ? MeshComponent->GetSkeletalMeshAsset() : nullptr;
    return Sequence && (!Mesh || Sequence->GetSkeleton() == Mesh->GetSkeleton());
}

void UVortexAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();
    Play(TEXT("Idle"), true, 1.f, 0.f);
}

UAnimSequence* UVortexAnimInstance::Clip(FName Name)
{
    // trocou de herói (outra raça, outros clipes): a lista guardada não vale mais
    const USkeletalMeshComponent* MeshComponent = GetSkelMeshComponent();
    if (const USkeletalMesh* Mesh = MeshComponent ? MeshComponent->GetSkeletalMeshAsset() : nullptr; Mesh != ClipsMesh.Get())
    {
        Clips.Reset();
        ClipsMesh = Mesh;
    }
    if (TObjectPtr<UAnimSequence>* Found = Clips.Find(Name))
    {
        return *Found;
    }
    const FString N = Name.ToString();
    // herói novo: as animações do Mixamo feitas no corpo da raça (Tools/Blender/MixamoToHero.py -> MX_<raça>_<clipe>),
    // quando existem. As do redirecionador da Unreal (A_<raça>_*) ficam de fora: quebravam a corrida e afinavam a cintura
    UAnimSequence* Sequence = nullptr;
    if (const USkeletalMeshComponent* Owner = GetSkelMeshComponent())
        if (const USkeletalMesh* Mesh = Owner->GetSkeletalMeshAsset(); Mesh && Mesh->GetName().StartsWith(TEXT("Hero_")))
        {
            const FString Race = Mesh->GetName().RightChop(5).Replace(TEXT("_FP"), TEXT(""));
            const FString Package = FString::Printf(TEXT("/Game/Vortex/Characters/Heroes/Anims/%s/MX_%s_%s"), *Race, *Race, *N);
            if (FPackageName::DoesPackageExist(Package))
                Sequence = LoadObject<UAnimSequence>(nullptr, *FString::Printf(TEXT("%s.MX_%s_%s"), *Package, *Race, *N));
        }
    const FString Path = FString::Printf(
        TEXT("/Game/Vortex/Characters/Adventurer/Adventurer_Skinned/SkeletalMeshes/Adventurer_Skinned%s.Adventurer_Skinned%s"), *N, *N);
    if (!Sequence) Sequence = LoadObject<UAnimSequence>(nullptr, *Path);
    if (!Fits(Sequence)) Sequence = nullptr;
    Clips.Add(Name, Sequence);
    return Sequence;
}

UAnimSequence* UVortexAnimInstance::RaceAttack(const UVortexWeaponDefinition* Definition, int32 Index)
{
    // golpes do Mixamo da raça por arma (Tools/Blender/MixamoToHero.py: Atk_<arma>_<n>[_Full] e Arco_Release), com o
    // instante do golpe no meio da janela de dano de cada golpe da arma
    static const TMap<FName, FString> Prefixes = {
        { TEXT("espada_de_cinzas"), TEXT("Atk_Espada_") }, { TEXT("martelo_de_bram"), TEXT("Atk_Martelo_") },
        { TEXT("adagas_gemeas"), TEXT("Atk_Adagas_") }, { TEXT("cajado_de_ilsa"), TEXT("Atk_Cajado_") }, { TEXT("arco_de_betula"), TEXT("Arco") } };
    const FString* Prefix = Definition ? Prefixes.Find(Definition->WeaponId) : nullptr;
    if (!Prefix || !Clip(TEXT("Jog"))) return nullptr;   // só quem tem os clipes do Mixamo
    if (*Prefix == TEXT("Arco")) return Index < 0 ? Clip(TEXT("Arco_Draw")) : Clip(TEXT("Arco_Release"));
    if (Index < 0) return nullptr;
    const FString Name = *Prefix + FString::FromInt(Index % 3 + 1);
    UAnimSequence* Sequence = Clip(FName(*Name));
    return Sequence ? Sequence : Clip(FName(*(Name + TEXT("_Full"))));
}

float UVortexAnimInstance::ClipLength(FName Name)
{
    const UAnimSequence* Sequence = Clip(Name);
    return Sequence ? Sequence->GetPlayLength() / FMath::Max(.1f, Sequence->RateScale) : 0.f;
}

void UVortexAnimInstance::Play(FName Name, bool bLoop, float NewRate, float InBlend, UAnimSequence* Override)
{
    UAnimSequence* Next = Override ? Override : Clip(Name);
    // as animações redirecionadas trazem a taxa que casa o passo com a velocidade (RateScale, medido na raiz)
    if (Next) NewRate *= Next->RateScale;
    if (Name == CurrentName)
    {
        Rate = NewRate;
        bCurrentLoops = bLoop;
        return;
    }
    if (!Next)
    {
        return;
    }
    Previous = Current;
    PreviousTime = CurrentTime;
    bPreviousLoops = bCurrentLoops;
    PreviousRate = Rate;
    Rate = NewRate;
    Current = Next;
    CurrentName = Name;
    CurrentTime = 0.f;
    bCurrentLoops = bLoop;
    StateTime = 0.f;
    Blend = InBlend;
    BlendAlpha = (Previous && InBlend > 0.f) ? 0.f : 1.f;
}

void UVortexAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);
    const AVortexCharacter* Character = Cast<AVortexCharacter>(TryGetPawnOwner());
    const UVortexCharacterMovementComponent* Movement = Character ? Character->GetVortexMovement() : nullptr;
    if (Movement)
    {
        float Speed = FVector(Movement->Velocity.X, Movement->Velocity.Y, 0.f).Size();
#if !UE_BUILD_SHIPPING
        if (DebugSpeed > 0.f) Speed = DebugSpeed;
#endif
        const bool bFalling = Movement->IsFalling();
        if (Speed > 35.f || bFalling || !Character->GetEmote().IsNone()) IdleFor = 0.f;
        // correndo (Shift, acima da velocidade de andar): inclina o tronco (a disparada do Mixamo já vem inclinada)
        Lean = FMath::FInterpTo(Lean, !bFalling && !Movement->IsSwimmingInWater() && !Character->IsMounted() && Speed > 560.f && !Clip(TEXT("Sprint")) ? 1.f : 0.f, DeltaSeconds, 6.f);
#if !UE_BUILD_SHIPPING
        if (bDebugLean) Lean = 1.f;
#endif
        UAnimSequence* WantedHold = nullptr;
        if (!Character->IsDead() && !Movement->IsDodging() && !Character->IsMounted() && !Movement->IsSwimmingInWater() && Character->GetEmote().IsNone())
        {
            const UVortexWeaponComponent* HoldWeapon = Character->GetWeapon();
            const UVortexWeaponDefinition* HoldDef = HoldWeapon ? HoldWeapon->GetDefinition() : nullptr;
            UAnimSequence* Hold = HoldDef && Fits(HoldDef->StanceIdle.Get()) ? HoldDef->StanceIdle.Get() : nullptr;
            if (Hold && Hold->GetName().Contains(TEXT("_Hold"))) WantedHold = Hold;
        }
        if (bWasFalling && !bFalling)
        {
            LandTime = 0.25f;
        }
        bWasFalling = bFalling;
        LandTime = FMath::Max(0.f, LandTime - DeltaSeconds);
        if (Character->IsDead())
        {
            if (CurrentName != TEXT("Death")) Play(TEXT("Death"), false, 1.f, .15f);
        }
        else if (Movement->IsDodging())
        {
            static const FName DodgeClips[] = { TEXT("Dodge"), TEXT("Dodge_Back"), TEXT("Dodge_Left"), TEXT("Dodge_Right") };
            Play(DodgeClips[FMath::Min<int32>(Movement->DodgeVisualDirection, 3)], false, 1.f, 0.12f);
        }
        else if (Character->IsMounted())
        {
            // montado (clipe Ride de Tools/AddSwimClips.py; sem ele, parado)
            Play(Clip(TEXT("Ride")) ? FName(TEXT("Ride")) : FName(TEXT("Idle")), true, FMath::Clamp(.8f + Speed / 900.f, .8f, 1.6f), .2f);
        }
        else if (Movement->IsSwimmingInWater())
        {
            // crawl nadando, boiar parado (clipes de Tools/AddSwimClips.py; sem eles, o andar antigo)
            if (Clip(TEXT("Swim"))) Speed > 40.f ? Play(TEXT("Swim"), true, FMath::Clamp(Speed / 260.f, 0.7f, 1.4f), .25f) : Play(TEXT("Swim_Idle"), true, 1.f, .3f);
            else Speed > 40.f ? Play(TEXT("Walk"), true, FMath::Clamp(Speed / 240.f, 0.6f, 1.2f)) : Play(TEXT("Idle"), true, 1.f);
        }
        else if (bFalling)
        {
            const bool bStarting = CurrentName == TEXT("Jump_Start") && StateTime < 0.25f;
            if (!bStarting)
            {
                Movement->Velocity.Z > 200.f && CurrentName != TEXT("Jump_Loop") && CurrentName != TEXT("Jump_Start")
                    ? Play(TEXT("Jump_Start"), false, 1.f, 0.08f)
                    : Play(TEXT("Jump_Loop"), true, 1.f);
            }
        }
        else if (LandTime > 0.f && Speed < 300.f)
        {
            Play(TEXT("Jump_Land"), false, 1.f, 0.08f);
        }
        else if (const FName Emote = Character->GetEmote(); !Emote.IsNone() && Clip(Emote))
        {
            // roda de emotes (B): as danças repetem; os gestos tocam uma vez (o servidor encerra no fim)
            Play(Emote, Emote.ToString().StartsWith(TEXT("Danca")), 1.f, .25f);
        }
        else
        {
            // postura da arma na mão (lança, cajado, arco...): os clipes da família, se houver
            const UVortexWeaponComponent* HeldWeapon = Character->GetWeapon();
            const UVortexWeaponDefinition* Held = HeldWeapon ? HeldWeapon->GetDefinition() : nullptr;
            UAnimSequence* StanceRun = Held && Fits(Held->StanceRun.Get()) ? Held->StanceRun.Get() : nullptr;
            UAnimSequence* StanceWalk = Held && Fits(Held->StanceWalk.Get()) ? Held->StanceWalk.Get() : nullptr;
            UAnimSequence* StanceIdle = Held && Fits(Held->StanceIdle.Get()) ? Held->StanceIdle.Get() : nullptr;
            // famílias refeitas (clipe *_Hold): andar e correr normais, o tronco e os braços seguram a arma
            if (StanceIdle && StanceIdle->GetName().Contains(TEXT("_Hold"))) StanceRun = StanceWalk = StanceIdle = nullptr;
            // herói novo (clipes do Mixamo da raça): andar devagar, trote, a corrida no passo normal (460) e a disparada no
            // Shift; a taxa vem do RateScale de cada clipe (passo perto do chão)
            const bool bRaceClips = !StanceRun && Clip(TEXT("Jog")) != nullptr;
            // 01/11: arrancar (do parado direto para a corrida) e frear (parou de correr): clipes feitos de Idle e Run
            // (Tools/Blender/MixamoToHero.py make_derived); tocam uma vez e passam para o laço
            const bool bWasRunning = CurrentName == TEXT("Run") || CurrentName == TEXT("Sprint") || CurrentName == TEXT("RunStart") || CurrentName == TEXT("RunArmed");
            const bool bWasStill = CurrentName == TEXT("Idle") || CurrentName == TEXT("ArmStretching") || CurrentName == TEXT("RunStop");
            // (a velocidade cresce aos poucos: vale a velocidade que vai alcançar, a de correr, e não a do quadro)
            if (bRaceClips && Speed > 35.f && Movement->GetMaxSpeed() > 220.f && Clip(TEXT("RunStart")) && (bWasStill || (CurrentName == TEXT("RunStart") && StateTime < ClipLength(TEXT("RunStart")))))
            {
                Play(TEXT("RunStart"), false, 1.f, .1f);
            }
            else if (bRaceClips && Speed <= 35.f && Clip(TEXT("RunStop")) && (bWasRunning || (CurrentName == TEXT("RunStop") && StateTime < ClipLength(TEXT("RunStop")))))
            {
                IdleFor = 0.f;
                Play(TEXT("RunStop"), false, 1.f, .12f);
            }
            else if (bRaceClips && Speed > 35.f)
            {
                // 01/11: o passo casa com o chão. Cada clipe sabe a própria velocidade (RateScale = base / velocidade do
                // clipe, Tools/ImportMixamoHeroes.py) no tamanho do modelo; o herói aparece escalado (o anão a 70%), então
                // o passo de verdade é essa velocidade vezes a escala. Escolhe o clipe que precisa da taxa mais perto do
                // natural (um pouco acima de 1) e toca na taxa exata, até 2,6x (o anão de perna curta dá passinhos rápidos)
                // a intenção escolhe o grupo: andando (Caps Lock) o clipe de andar (ou o trote, se o passo não alcança);
                // correndo e na disparada (Shift), entre a corrida lenta e a rápida, a que toca mais perto de 1,3x
                static const TMap<FName, float> Base = { { TEXT("Walk"), 150.f }, { TEXT("Jog"), 300.f }, { TEXT("Run"), 460.f }, { TEXT("Sprint"), 620.f },
                    { TEXT("RunArmed"), 460.f } };
                // 02/11: com a arma, a disparada também entra (a corrida armada a 1,3x dava passinhos curtos e rápidos)
                static const FName WalkGroup[] = { TEXT("Walk"), TEXT("Jog") }, RunGroup[] = { TEXT("Run"), TEXT("Sprint") }, ArmedGroup[] = { TEXT("RunArmed"), TEXT("Sprint") };
                // arma de mão (não o arco): a corrida segurando a arma (Sword And Shield Run do Mixamo)
                const UVortexWeaponComponent* RunWeapon = Character->GetWeapon();
                const UVortexWeaponDefinition* RunDef = RunWeapon ? RunWeapon->GetDefinition() : nullptr;
                const bool bArmedRun = RunDef && !RunDef->WeaponId.ToString().StartsWith(TEXT("arco")) && Clip(TEXT("RunArmed"));
                const bool bStroll = Movement->bWantsToWalk && !Movement->IsSprinting();
#if !UE_BUILD_SHIPPING
                const bool bWalkIntent = DebugSpeed > 0.f ? Speed <= 220.f : bStroll || Speed <= 160.f;
#else
                const bool bWalkIntent = bStroll || Speed <= 160.f;
#endif
                const float Scale = FMath::Max(.1f, GetSkelMeshComponent()->GetComponentScale().Z);
                auto Need = [&](FName Name)
                {
                    const UAnimSequence* Sequence = Clip(Name);
                    return Sequence && Sequence->RateScale > 0.f ? Speed / FMath::Max(1.f, Base.FindRef(Name) / Sequence->RateScale * Scale) : -1.f;
                };
                FName Best;
                float BestRate = 1.f;
                if (bWalkIntent)
                {
                    float Lowest = BIG_NUMBER;
                    for (const FName& Name : WalkGroup)
                    {
                        const float N = Need(Name);
                        if (N <= 0.f) continue;
                        if (N <= 2.4f) { Best = Name; BestRate = N; break; }
                        if (N < Lowest) { Lowest = N; Best = Name; BestRate = N; }
                    }
                }
                else
                {
                    float BestCost = BIG_NUMBER;
                    // 06/11 (o usuário: "sem arma na mão volta a animação frenética"): na corrida normal o custo escolhia
                    // a disparada (braços rápidos) porque o chão a mais (FootRun 1,5) pede uma taxa perto da dela. A
                    // disparada só no Shift; sem Shift, só a corrida (a de mãos vazias ou a armada)
                    static const FName RunOnly[] = { TEXT("Run") }, ArmedOnly[] = { TEXT("RunArmed") };
                    const bool bSprintIntent = Movement->IsSprinting();
                    for (const FName& Name : bSprintIntent ? (bArmedRun ? TArrayView<const FName>(ArmedGroup) : TArrayView<const FName>(RunGroup))
                        : (bArmedRun ? TArrayView<const FName>(ArmedOnly) : TArrayView<const FName>(RunOnly)))
                    {
                        const float N = Need(Name);
                        if (N <= 0.f) continue;
                        // sem pular de clipe a cada quadro perto da troca: o atual leva vantagem
                        // 02/11: perto do ritmo natural (1,05x): a 1,3x os passos ficavam curtos e não cobriam o chão
                        const float Cost = FMath::Abs(FMath::Loge(N / 1.05f)) - (Name == CurrentName ? .12f : 0.f);
                        if (Cost < BestCost) { BestCost = Cost; Best = Name; BestRate = N; }
                    }
                }
                // 03/11: correndo, o clipe toca 18% mais devagar que o chão pede: o passo parece mais longo (o usuário:
                // "passos mais longos, a animação mais lenta"); andando, no ritmo certo
                // 05/11: 1,37 correndo e 1,16 andando (eram 1,18 e 1): a animação um pouco mais lenta, junto com o chão a
                // mais (FootRun/FootWalk no VortexCharacterMovementComponent): passadas mais longas
                // 10/11 (o usuário: "a corrida ainda muito rápida, frenética; animação mais lenta e a pisada maior"): 1,7
                // correndo (a corrida a ~0,73x no mesmo chão)
                const float Stride = bWalkIntent ? 1.16f : 1.7f;
                if (const UAnimSequence* Sequence = Clip(Best))
                    Play(Best, true, FMath::Clamp(BestRate / Stride, .6f, 2.6f) / Sequence->RateScale, BlendTime);
            }
            else if (Speed > 420.f)
            {
                Play(StanceRun ? FName(*(TEXT("Stance_") + StanceRun->GetName())) : FName(TEXT("Run")), true, FMath::Clamp(Speed / 620.f, 0.65f, 1.5f), BlendTime, StanceRun);
            }
            else if (Speed > 35.f)
            {
                Play(StanceWalk ? FName(*(TEXT("Stance_") + StanceWalk->GetName())) : FName(TEXT("Walk")), true, FMath::Clamp(Speed / 240.f, 0.6f, 1.6f), BlendTime, StanceWalk);
            }
            else
            {
                // parado muito tempo (14 s): espreguiça uma vez e volta (o clipe da raça, se tiver)
                IdleFor += DeltaSeconds;
                const float Stretch = StanceIdle ? 0.f : ClipLength(TEXT("ArmStretching"));
                if (Stretch > 0.f && IdleFor > 14.f && IdleFor < 14.f + Stretch) Play(TEXT("ArmStretching"), false, 1.f, .4f);
                else
                {
                    if (Stretch > 0.f && IdleFor >= 14.f + Stretch) IdleFor = 0.f;
                    Play(StanceIdle ? FName(*(TEXT("Stance_") + StanceIdle->GetName())) : FName(TEXT("Idle")), true, 1.f, .4f, StanceIdle);
                }
            }
        }

        // beber poção: o clipe do braço esquerdo pelo relógio da bebida
        const UVortexHeroSheet* DrinkSheet = Character->GetSheet();
        if (DrinkSheet && DrinkSheet->IsDrinking() && !Character->IsDead())
        {
            if (!DrinkPose || !Fits(DrinkPose))
            {
                // o herói do Mixamo bebe com o clipe dele (Beber); os outros, com o do aventureiro
                DrinkPose = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Vortex/Weapons/VxClips/Adventurer_VortexClips/SkeletalMeshes/Adventurer_VortexClipsVx_Drink_LeftArm.Adventurer_VortexClipsVx_Drink_LeftArm"));
                if (!Fits(DrinkPose)) DrinkPose = Clip(TEXT("Beber"));
            }
            DrinkTime = DrinkSheet->DrinkElapsed();
            DrinkAlpha = FMath::Min(1.f, DrinkAlpha + DeltaSeconds / .1f);
        }
        else
        {
            DrinkAlpha = FMath::Max(0.f, DrinkAlpha - DeltaSeconds / .2f);
            DrinkTime += DeltaSeconds;
        }
        // arma na mão andando e correndo (heróis do Mixamo): o braço da arma firme, na pose do parado
        {
            const UVortexWeaponComponent* Armed = Character->GetWeapon();
            const UVortexWeaponDefinition* ArmedDef = Armed ? Armed->GetDefinition() : nullptr;
            const bool bMixamo = Clip(TEXT("Jog")) && GetSkelMeshComponent()->GetBoneIndex(TEXT("mixamorig_RightArm")) != INDEX_NONE;
            // (na corrida armada o clipe já segura a arma)
            const bool bWant = bMixamo && ArmedDef && Speed > 35.f && CurrentName != TEXT("RunArmed") && !Movement->IsFalling() && !Movement->IsDodging() && !Character->IsMounted()
                && !Movement->IsSwimmingInWater() && Character->GetEmote().IsNone() && !Character->IsDead();
            if (bWant)
            {
                SteadyPose = Clip(TEXT("Idle"));
                bSteadyLeft = ArmedDef->WeaponId.ToString().StartsWith(TEXT("arco"));
            }
            SteadyAlpha = FMath::FInterpTo(SteadyAlpha, bWant ? .75f : 0.f, DeltaSeconds, 8.f);
        }
        // pose de segurar: entra e sai devagar (some na esquiva, no nado, montado e caído)
        if (WantedHold && WantedHold != HoldPose) { HoldPose = WantedHold; HoldAlpha = FMath::Min(HoldAlpha, .3f); }
        HoldAlpha = WantedHold ? FMath::Min(1.f, HoldAlpha + DeltaSeconds / .2f) : FMath::Max(0.f, HoldAlpha - DeltaSeconds / .15f);
        if (HoldAlpha <= 0.f) HoldPose = nullptr;
        HoldTime += DeltaSeconds;

        UAnimSequence* DesiredUpperBody = nullptr;
        FName DesiredName;
        float DesiredTime = 0.f;
        bool bDesiredLoops = false;
        if (!Movement->IsDodging())
        {
            const UVortexWeaponComponent* Weapon = Character->GetWeapon();
            const UVortexWeaponDefinition* Definition = Weapon ? Weapon->GetDefinition() : nullptr;
            float CastTime = 0.f;
            UAnimSequence* CastClip = Character->GetClassSkills() ? Character->GetClassSkills()->CastAnimation(CastTime) : nullptr;
            // as poses de habilidade são do aventureiro: o herói do Mixamo usa o golpe mais forte da arma
            if (CastClip && !Fits(CastClip)) CastClip = RaceAttack(Definition, 2);
            if (CastClip)
            {
                // habilidade de classe: o golpe/pose da arma na mão por um instante
                DesiredUpperBody = CastClip;
                DesiredTime = CastTime;
                DesiredName = FName(*FString::Printf(TEXT("ClassCast_%s"), *CastClip->GetName()));
            }
            else if (Definition && Weapon->IsGuarding() && Fits(Definition->GuardAnimation))
            {
                DesiredUpperBody = Definition->GuardAnimation;
                DesiredName = TEXT("SwordGuard");
                bDesiredLoops = true;
            }
            else if (Definition && Definition->Swings.Num() &&
                (Weapon->GetAttackState().SwingIndex == VortexSpecialPose::Hold || Weapon->GetAttackState().SwingIndex == VortexSpecialPose::Release))
            {
                // especial carregado: segurando, o clipe de carga (ou o golpe mais forte parado no
                // começo); soltando, o clipe do especial (ou o golpe seguindo dali)
                const FVortexAttackState& Attack = Weapon->GetAttackState();
                const FVortexWeaponSwing& Strong = Definition->Swings.Last();
                const AGameStateBase* State = Character->GetWorld()->GetGameState();
                const float Now = State ? State->GetServerWorldTimeSeconds() : Character->GetWorld()->GetTimeSeconds();
                const float Since = FMath::Max(0.f, Now - Attack.ServerStartTime);
                const float Windup = Strong.ActiveStart * .8f;
                // 07/11: as espadas com poder próprio (Data/Rpg/swords.json) usam o clipe do secundário da arma de origem
                VortexSwords::OnSpecialPose(Character, Attack.Serial);
                UAnimSequence* SwordHold = VortexSwords::SpecialClip(Character, Definition, true);
                UAnimSequence* SwordClip = VortexSwords::SpecialClip(Character, Definition, false);
                if (Attack.SwingIndex == VortexSpecialPose::Hold && SwordHold)
                {
                    DesiredUpperBody = SwordHold;
                    DesiredTime = Since;
                    bDesiredLoops = !SwordHold->GetName().Contains(TEXT("_Draw"));
                }
                else if (Attack.SwingIndex != VortexSpecialPose::Hold && SwordClip)
                {
                    DesiredUpperBody = Since < SwordClip->GetPlayLength() ? SwordClip : nullptr;
                    DesiredTime = Since;
                    if (DesiredUpperBody && !Fits(DesiredUpperBody)) { DesiredUpperBody = RaceAttack(Definition, Definition->Swings.Num() - 1); DesiredTime = Since; }
                }
                else if (Attack.SwingIndex == VortexSpecialPose::Hold)
                {
                    UAnimSequence* RaceHold = Definition->SecondaryHoldClip ? RaceAttack(Definition, -1) : nullptr;
                    DesiredUpperBody = RaceHold ? RaceHold : Definition->SecondaryHoldClip ? Definition->SecondaryHoldClip.Get() : Strong.Animation.Get();
                    if (!Fits(DesiredUpperBody)) DesiredUpperBody = RaceAttack(Definition, Definition->Swings.Num() - 1);
                    DesiredTime = Definition->SecondaryHoldClip ? Since : FMath::Min(Since, Windup);
                    // puxar a corda do arco fica no fim (sem recomeçar); o resto da carga fica em laço
                    bDesiredLoops = Definition->SecondaryHoldClip != nullptr && !Definition->SecondaryHoldClip->GetName().Contains(TEXT("_Draw"));
                }
                else if (Since < (Definition->SecondaryClip ? Definition->SecondaryClip->GetPlayLength() : Strong.Duration))
                {
                    DesiredUpperBody = Definition->SecondaryClip ? Definition->SecondaryClip.Get() : Strong.Animation.Get();
                    DesiredTime = Definition->SecondaryClip ? Since : Windup + Since;
                    if (!Fits(DesiredUpperBody)) { DesiredUpperBody = RaceAttack(Definition, Definition->Swings.Num() - 1); DesiredTime = Since; }
                }
                DesiredName = FName(*FString::Printf(TEXT("Special_%d_%d"), Attack.SwingIndex, Attack.Serial));
            }
            else if (Definition && Definition->Swings.IsValidIndex(Weapon->GetAttackState().SwingIndex))
            {
                const FVortexAttackState& Attack = Weapon->GetAttackState();
                const FVortexWeaponSwing& Swing = Definition->Swings[Attack.SwingIndex];
                const AGameStateBase* State = Character->GetWorld()->GetGameState();
                const float Now = State ? State->GetServerWorldTimeSeconds() : Character->GetWorld()->GetTimeSeconds();
                DesiredTime = FMath::Max(0.f, Now - Attack.ServerStartTime) * Weapon->GetAttackSpeed();
                if (DesiredTime < Swing.Duration + .1f)
                {
                    // clipes reaproveitados de outras armas tocam mais rápido (ClipSpeed do Godot)
                    DesiredTime *= FMath::Max(.1f, Swing.PlayRate);
                }
                if (DesiredTime < Swing.Duration * FMath::Max(.1f, Swing.PlayRate) + .1f)
                {
                    UAnimSequence* RaceSwing = RaceAttack(Definition, Attack.SwingIndex);
                    DesiredUpperBody = RaceSwing ? RaceSwing : Fits(Swing.Animation.Get()) ? Swing.Animation.Get() : nullptr;
                    DesiredName = FName(*FString::Printf(TEXT("SwordAttack_%d_%d"), Attack.SwingIndex, Attack.Serial));
                }
            }
        }
        // trocou de arma: saca a nova; levou dano: o tronco dá um tranco (clipes da raça, quando não está golpeando)
        const UVortexWeaponComponent* NowWeapon = Character->GetWeapon();
        const UVortexWeaponDefinition* NowDefinition = NowWeapon ? NowWeapon->GetDefinition() : nullptr;
        if (NowDefinition != LastDefinition.Get())
        {
            DrawAge = LastDefinition.IsValid() ? 0.f : 99.f;
            LastDefinition = NowDefinition;
        }
        DrawAge += DeltaSeconds;
        const float Health = Character->GetVitals() ? Character->GetVitals()->GetHealth() : 0.f;
        if (LastHealth >= 0.f && Health < LastHealth - .5f && Health > 0.f) HitAge = 0.f;
        LastHealth = Health;
        HitAge += DeltaSeconds;
        if (!DesiredUpperBody && !Movement->IsDodging())
        {
            if (const float DrawLength = ClipLength(TEXT("Sacar")); DrawLength > 0.f && DrawAge < DrawLength)
            {
                DesiredUpperBody = Clip(TEXT("Sacar"));
                DesiredTime = DrawAge * Clip(TEXT("Sacar"))->RateScale;
                DesiredName = TEXT("Sacar");
            }
            else if (const float HitLength = ClipLength(TEXT("Hit")); HitLength > 0.f && HitAge < HitLength)
            {
                DesiredUpperBody = Clip(TEXT("Hit"));
                DesiredTime = HitAge;
                DesiredName = TEXT("Hit");
            }
        }
        // o arco parado vai de corpo inteiro: o clipe do Mixamo põe o arqueiro de lado (quadril a -91°) com o braço do arco
        // apontando para a frente, para a mira; andando, só o tronco (com o giro do quadril do clipe, em Evaluate)
        if (DesiredUpperBody) bUpperBodyFull = DesiredUpperBody->GetName().Contains(TEXT("_Full")) ||
            (DesiredUpperBody->GetName().Contains(TEXT("_Arco_")) && Speed < 60.f && !bFalling);
        if (DesiredUpperBody)
        {
            if (DesiredName != UpperBodyName)
            {
                PreviousUpperBody = UpperBodyOverlay;
                PreviousUpperBodyTime = UpperBodyTime;
                bPreviousUpperBodyLoops = bUpperBodyLoops;
                UpperBodyName = DesiredName;
                UpperBodyOverlay = DesiredUpperBody;
                UpperBodyCrossfadeAlpha = PreviousUpperBody ? 0.f : 1.f;
                if (!PreviousUpperBody) UpperBodyAlpha = 0.f;
                UpperBodyTime = DesiredTime;
            }
            else UpperBodyTime = bDesiredLoops ? UpperBodyTime + DeltaSeconds : DesiredTime;
            bUpperBodyLoops = bDesiredLoops;
            UpperBodyAlpha = FMath::Min(1.f, UpperBodyAlpha + DeltaSeconds / .06f);
        }
        else
        {
            UpperBodyAlpha = FMath::Max(0.f, UpperBodyAlpha - DeltaSeconds / .22f);
            UpperBodyTime += DeltaSeconds;
            if (UpperBodyAlpha <= 0.f) { UpperBodyOverlay = nullptr; PreviousUpperBody = nullptr; UpperBodyName = NAME_None; }
        }
        PreviousUpperBodyTime += DeltaSeconds;
        UpperBodyCrossfadeAlpha = FMath::Min(1.f, UpperBodyCrossfadeAlpha + DeltaSeconds / .06f);
        if (UpperBodyCrossfadeAlpha >= 1.f) PreviousUpperBody = nullptr;
        if (Movement->IsDodging())
        {
            UpperBodyOverlay = nullptr;
            PreviousUpperBody = nullptr;
            UpperBodyName = NAME_None;
            UpperBodyAlpha = 0.f;
        }
    }
    else if (const ACharacter* Other = Cast<ACharacter>(TryGetPawnOwner()))
    {
        // NPCs (Fase 14) usam o mesmo aventureiro: só andar, correr, parar e cair, sem esquiva,
        // nado nem arma
        const UCharacterMovementComponent* OtherMovement = Other->GetCharacterMovement();
        const float Speed = OtherMovement ? FVector(OtherMovement->Velocity.X, OtherMovement->Velocity.Y, 0.f).Size() : 0.f;
        // 08/11: a rotina dos moradores (AVortexNpc): o gesto da vez (uma vez), sentado, o ofício em laço parado
        const AVortexNpc* Npc = Cast<AVortexNpc>(Other);
        if (Npc && Npc->GetGestureSeq() != NpcGestureSeq)
        {
            NpcGestureSeq = Npc->GetGestureSeq();
            NpcGestureLeft = Clip(Npc->GetGesture()) ? ClipLength(Npc->GetGesture()) : 0.f;
        }
        NpcGestureLeft = FMath::Max(0.f, NpcGestureLeft - DeltaSeconds);
        if (OtherMovement && OtherMovement->IsFalling() && !(Npc && Npc->IsSitting())) Play(TEXT("Jump_Loop"), true, 1.f);
        else if (Npc && Npc->IsSitting()) Play(TEXT("SentadoBanco"), true, 1.f, .35f);
        else if (Speed > 420.f) Play(TEXT("Run"), true, FMath::Clamp(Speed / 620.f, 0.65f, 1.5f));
        else if (Speed > 35.f) Play(TEXT("Walk"), true, FMath::Clamp(Speed / 240.f, 0.6f, 1.6f));
        else if (NpcGestureLeft > 0.f) Play(Npc->GetGesture(), false, 1.f, .25f);
        else if (Npc && !Npc->GetActivity().IsNone() && Clip(Npc->GetActivity())) Play(Npc->GetActivity(), true, 1.f, .4f);
        else Play(TEXT("Idle"), true, 1.f);
    }
    StateTime += DeltaSeconds;
    CurrentTime += DeltaSeconds * Rate;
    PreviousTime += DeltaSeconds * PreviousRate;
    BlendAlpha = Blend > 0.f ? FMath::Min(1.f, BlendAlpha + DeltaSeconds / Blend) : 1.f;
    if (BlendAlpha >= 1.f)
    {
        Previous = nullptr;
    }
}

FAnimInstanceProxy* UVortexAnimInstance::CreateAnimInstanceProxy()
{
    return new FVortexAnimInstanceProxy(this);
}

void FVortexAnimInstanceProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
    FAnimInstanceProxy::PreUpdate(InAnimInstance, DeltaSeconds);
    const UVortexAnimInstance* Instance = CastChecked<UVortexAnimInstance>(InAnimInstance);
    Current = Instance->Current;
    Previous = Instance->Previous;
    CurrentTime = Instance->CurrentTime;
    PreviousTime = Instance->PreviousTime;
    bCurrentLoops = Instance->bCurrentLoops;
    bPreviousLoops = Instance->bPreviousLoops;
    Alpha = Instance->BlendAlpha;
    UpperBody = Instance->UpperBodyOverlay;
    HoldPose = Instance->HoldPose;
    HoldTime = Instance->HoldTime;
    HoldAlpha = Instance->HoldAlpha;
    bUpperBodyFull = Instance->bUpperBodyFull;
    DrinkPose = Instance->DrinkPose;
    DrinkTime = Instance->DrinkTime;
    DrinkAlpha = Instance->DrinkAlpha;
    SteadyPose = Instance->SteadyPose;
    SteadyAlpha = Instance->SteadyAlpha;
    bSteadyLeft = Instance->bSteadyLeft;
    PreviousUpperBody = Instance->PreviousUpperBody;
    UpperBodyTime = Instance->UpperBodyTime;
    UpperBodyAlpha = Instance->UpperBodyAlpha;
    PreviousUpperBodyTime = Instance->PreviousUpperBodyTime;
    UpperBodyCrossfadeAlpha = Instance->UpperBodyCrossfadeAlpha;
    bUpperBodyLoops = Instance->bUpperBodyLoops;
    bPreviousUpperBodyLoops = Instance->bPreviousUpperBodyLoops;
    Body = Instance->Body;
    // braços abertos e ombro empurrado: só corrigem os clipes do aventureiro (os braços entravam no corpo). Os do
    // Mixamo (MX_*) já foram feitos no corpo da raça; por cima deles, deslocavam os ombros
    const auto FromMixamo = [](const UAnimSequence* Sequence) { return Sequence && Sequence->GetName().StartsWith(TEXT("MX_")); };
    const float AdventurerArms = !FromMixamo(Current) ? 1.f : FMath::Max(UpperBody && !FromMixamo(UpperBody) ? Instance->UpperBodyAlpha : 0.f,
        HoldPose && !FromMixamo(HoldPose) ? Instance->HoldAlpha : 0.f);
    Body.ArmSpread *= AdventurerArms;
    Body.ShoulderOut *= AdventurerArms;
    bHideHead = Instance->bHideHead;
    bHideArms = Instance->bHideArms;
    bArmsOnly = Instance->bArmsOnly;
    Lean = Instance->Lean;
}

// Proporção da raça. Os ossos do aventureiro ficam alinhados ao corpo: o eixo de comprimento de
// cada um é o da posição do filho (a perna aponta para a canela). As pernas encolhem no
// comprimento e engrossam nas outras direções, e o quadril desce o mesmo tanto, para os pés
// continuarem no chão; o tronco alarga, os braços engrossam e a cabeça cresce por igual.
void FVortexAnimInstanceProxy::ApplyBody(FPoseContext& Output) const
{
    const bool bPlain = FMath::IsNearlyEqual(Body.Legs, 1.f) && FMath::IsNearlyEqual(Body.LegThick, 1.f) && FMath::IsNearlyEqual(Body.Torso, 1.f) &&
        FMath::IsNearlyEqual(Body.Arms, 1.f) && FMath::IsNearlyEqual(Body.Head, 1.f);
    if (bPlain) return;
    const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
    const FReferenceSkeleton& Reference = Bones.GetReferenceSkeleton();
    auto Find = [&](const TCHAR* Name) -> FCompactPoseBoneIndex
    {
        const int32 Mesh = Reference.FindBoneIndex(Name);
        return Mesh == INDEX_NONE ? FCompactPoseBoneIndex(INDEX_NONE) : Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(Mesh));
    };
    // eixo do comprimento: o maior componente da posição do filho em descanso
    auto Axis = [&](const TCHAR* Child) -> int32
    {
        const int32 Mesh = Reference.FindBoneIndex(Child);
        if (Mesh == INDEX_NONE) return 1;
        const FVector T = Reference.GetRefBonePose()[Mesh].GetTranslation().GetAbs();
        return T.X >= T.Y && T.X >= T.Z ? 0 : T.Y >= T.Z ? 1 : 2;
    };
    auto Shape = [](int32 LengthAxis, float Length, float Thick)
    {
        FVector S(Thick);
        S[LengthAxis] = Length;
        return S;
    };
    auto Scale = [&](FCompactPoseBoneIndex Bone, const FVector& S)
    {
        if (Bone.GetInt() == INDEX_NONE) return;
        Output.Pose[Bone].SetScale3D(Output.Pose[Bone].GetScale3D() * S);
    };
    // pernas
    const int32 LegAxis = Axis(TEXT("Shin_L"));
    float LegLength = 0.f;
    for (const TCHAR* Name : { TEXT("Shin_L"), TEXT("Foot_L") })
    {
        const int32 Mesh = Reference.FindBoneIndex(Name);
        if (Mesh != INDEX_NONE) LegLength += Reference.GetRefBonePose()[Mesh].GetTranslation().Size();
    }
    Scale(Find(TEXT("Thigh_L")), Shape(LegAxis, Body.Legs, Body.LegThick));
    Scale(Find(TEXT("Thigh_R")), Shape(LegAxis, Body.Legs, Body.LegThick));
    const FCompactPoseBoneIndex Hips = Find(TEXT("Hips"));
    if (Hips.GetInt() != INDEX_NONE && LegLength > 0.f)
    {
        FVector T = Output.Pose[Hips].GetTranslation();
        const int32 Up = Axis(TEXT("Hips"));
        T[Up] -= FMath::Sign(T[Up]) * (1.f - Body.Legs) * LegLength;
        Output.Pose[Hips].SetTranslation(T);
    }
    // tronco e braços
    const int32 TorsoAxis = Axis(TEXT("Head"));
    Scale(Find(TEXT("Torso")), Shape(TorsoAxis, 1.f, Body.Torso));
    // os braços herdam a largura do tronco: corrige para ficarem só "Arms" mais grossos
    const float ArmFix = Body.Arms / FMath::Max(Body.Torso, .01f);
    const int32 ArmAxis = Axis(TEXT("Forearm_L"));
    Scale(Find(TEXT("UpperArm_L")), Shape(ArmAxis, 1.f, ArmFix));
    Scale(Find(TEXT("UpperArm_R")), Shape(ArmAxis, 1.f, ArmFix));
    // cabeça (desfaz a largura do tronco, que ela também herda)
    const FVector HeadFix = Shape(TorsoAxis, 1.f, 1.f / FMath::Max(Body.Torso, .01f)) * Body.Head;
    Scale(Find(TEXT("Head")), HeadFix);
}

namespace
{
    void Sample(UAnimSequence* Sequence, float Time, bool bLoop, FPoseContext& Pose)
    {
        const float Length = Sequence->GetPlayLength();
        const float T = bLoop && Length > 0.f ? FMath::Fmod(Time, Length) : FMath::Min(Time, Length);
        FAnimationPoseData PoseData(Pose);
        Sequence->GetAnimationPose(PoseData, FAnimExtractContext(static_cast<double>(T), false, FDeltaTimeRecord(), bLoop));
    }
}

void FVortexAnimInstanceProxy::ApplyPosture(FPoseContext& Output) const
{
    if (FMath::IsNearlyZero(Body.ArmSpread) && FMath::IsNearlyZero(Body.ShoulderOut) && Lean < .01f) return;
    const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
    const FReferenceSkeleton& Reference = Bones.GetReferenceSkeleton();
    auto Find = [&](const TCHAR* Name)
    {
        const int32 Mesh = Reference.FindBoneIndex(Name);
        return Mesh == INDEX_NONE ? FCompactPoseBoneIndex(INDEX_NONE) : Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(Mesh));
    };
    // rotação do osso no espaço do componente (subindo pelos pais)
    auto ComponentRotation = [&](FCompactPoseBoneIndex Bone)
    {
        FQuat Q = FQuat::Identity;
        for (FCompactPoseBoneIndex B = Bone; B.GetInt() != INDEX_NONE; B = Bones.GetParentBoneIndex(B)) Q = Output.Pose[B].GetRotation() * Q;
        return Q;
    };
    // gira o osso em volta de um eixo do componente (o modelo olha para +Y; +X é o lado esquerdo do herói)
    auto Turn = [&](FCompactPoseBoneIndex Bone, const FVector& Axis, float Degrees)
    {
        if (Bone.GetInt() == INDEX_NONE || FMath::IsNearlyZero(Degrees)) return;
        const FCompactPoseBoneIndex Parent = Bones.GetParentBoneIndex(Bone);
        const FQuat ParentQ = Parent.GetInt() == INDEX_NONE ? FQuat::Identity : ComponentRotation(Parent);
        const FQuat Extra(Axis, FMath::DegreesToRadians(Degrees));
        Output.Pose[Bone].SetRotation((ParentQ.Inverse() * Extra * ParentQ * Output.Pose[Bone].GetRotation()).GetNormalized());
    };
    // corrida: o tronco e a cabeça vão para a frente (o quadril um pouco)
    Turn(Find(TEXT("Hips")), FVector(1.f, 0.f, 0.f), -6.f * Lean);
    Turn(Find(TEXT("Torso")), FVector(1.f, 0.f, 0.f), -10.f * Lean);
    // braços para fora do corpo: o ombro sai do tronco e o braço abre um pouco
    auto Push = [&](FCompactPoseBoneIndex Bone, const FVector& Offset)
    {
        if (Bone.GetInt() == INDEX_NONE || Offset.IsNearlyZero()) return;
        const FCompactPoseBoneIndex Parent = Bones.GetParentBoneIndex(Bone);
        const FQuat ParentQ = Parent.GetInt() == INDEX_NONE ? FQuat::Identity : ComponentRotation(Parent);
        Output.Pose[Bone].AddToTranslation(ParentQ.Inverse().RotateVector(Offset));
    };
    Push(Find(TEXT("UpperArm_L")), FVector(Body.ShoulderOut, 0.f, 0.f));
    Push(Find(TEXT("UpperArm_R")), FVector(-Body.ShoulderOut, 0.f, 0.f));
    Turn(Find(TEXT("UpperArm_L")), FVector(0.f, 1.f, 0.f), -Body.ArmSpread);
    Turn(Find(TEXT("UpperArm_R")), FVector(0.f, 1.f, 0.f), Body.ArmSpread);
}

void FVortexAnimInstanceProxy::KeepArmsOnly(FPoseContext& Output) const
{
    // pose no espaço do componente; os ossos dos braços guardam o lugar e o resto encolhe a zero
    const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
    const FReferenceSkeleton& Reference = Bones.GetReferenceSkeleton();
    const int32 Count = Output.Pose.GetNumBones();
    TArray<FTransform> Component;
    Component.SetNum(Count);
    TArray<bool> Arm;
    Arm.Init(false, Count);
    // só do antebraço para baixo (a manga larga do ombro tapava as laterais da tela)
    const int32 ArmRoots[] = { RoleBone(Reference, TEXT("Forearm_L"), TEXT("LeftForeArm")), RoleBone(Reference, TEXT("Forearm_R"), TEXT("RightForeArm")) };
    for (FCompactPoseBoneIndex Bone : Output.Pose.ForEachBoneIndex())
    {
        const FCompactPoseBoneIndex Parent = Bones.GetParentBoneIndex(Bone);
        Component[Bone.GetInt()] = Parent.GetInt() == INDEX_NONE ? Output.Pose[Bone] : Output.Pose[Bone] * Component[Parent.GetInt()];
        int32 Mesh = Bones.MakeMeshPoseIndex(Bone).GetInt();
        while (Mesh != INDEX_NONE && Mesh != ArmRoots[0] && Mesh != ArmRoots[1]) Mesh = Reference.GetParentIndex(Mesh);
        Arm[Bone.GetInt()] = Mesh != INDEX_NONE;
    }
    TArray<FTransform> Wanted = Component;
    for (int32 i = 0; i < Count; ++i) if (!Arm[i]) Wanted[i].SetScale3D(FVector(.001f));
    for (FCompactPoseBoneIndex Bone : Output.Pose.ForEachBoneIndex())
    {
        const FCompactPoseBoneIndex Parent = Bones.GetParentBoneIndex(Bone);
        Output.Pose[Bone] = Parent.GetInt() == INDEX_NONE ? Wanted[Bone.GetInt()] : Wanted[Bone.GetInt()].GetRelativeTransform(Wanted[Parent.GetInt()]);
    }
}

bool FVortexAnimInstanceProxy::Evaluate(FPoseContext& Output)
{
    if (!Current)
    {
        return false;
    }
    Sample(Current, CurrentTime, bCurrentLoops, Output);
    if (Previous && Alpha < 1.f)
    {
        FPoseContext Old(Output);
        Sample(Previous, PreviousTime, bPreviousLoops, Old);
        FPoseContext Blended(Output);
        FAnimationPoseData OldData(Old), NewData(Output), BlendedData(Blended);
        FAnimationRuntime::BlendTwoPosesTogether(NewData, OldData, Alpha, BlendedData);
        Output = Blended;
    }
    const FBoneContainer& BoneSet = Output.Pose.GetBoneContainer();
    const FReferenceSkeleton& RefSkeleton = BoneSet.GetReferenceSkeleton();
    // o tronco: Torso no herói antigo; no do Mixamo, a coluna desde a primeira vértebra (Spine)
    const int32 TorsoBone = RoleBone(RefSkeleton, TEXT("Torso"), TEXT("Spine"));
    auto UnderTorso = [&](FCompactPoseBoneIndex Bone)
    {
        int32 MeshBone = BoneSet.MakeMeshPoseIndex(Bone).GetInt();
        while (MeshBone != INDEX_NONE && MeshBone != TorsoBone) MeshBone = RefSkeleton.GetParentIndex(MeshBone);
        return MeshBone == TorsoBone;
    };
    if (HoldPose && HoldAlpha > 0.f && TorsoBone != INDEX_NONE)
    {
        // a pose de segurar manda no tronco e nos braços; quadril e pernas seguem andando
        FPoseContext Hold(Output);
        Sample(HoldPose, HoldTime, true, Hold);
        for (FCompactPoseBoneIndex Bone : Output.Pose.ForEachBoneIndex())
            if (UnderTorso(Bone)) Output.Pose[Bone].Blend(Output.Pose[Bone], Hold.Pose[Bone], HoldAlpha);
    }
    if (SteadyPose && SteadyAlpha > .01f)
    {
        // o braço da arma (do ombro para baixo) na pose do parado, só um pouco do balanço da corrida
        FPoseContext Steady(Output);
        Sample(SteadyPose, 0.f, false, Steady);
        const int32 ArmRoot = RoleBone(RefSkeleton, bSteadyLeft ? TEXT("UpperArm_L") : TEXT("UpperArm_R"), bSteadyLeft ? TEXT("LeftArm") : TEXT("RightArm"));
        for (FCompactPoseBoneIndex Bone : Output.Pose.ForEachBoneIndex())
        {
            int32 Mesh = BoneSet.MakeMeshPoseIndex(Bone).GetInt();
            while (Mesh != INDEX_NONE && Mesh != ArmRoot) Mesh = RefSkeleton.GetParentIndex(Mesh);
            if (Mesh != INDEX_NONE) Output.Pose[Bone].Blend(Output.Pose[Bone], Steady.Pose[Bone], SteadyAlpha);
        }
    }
    if (UpperBody && UpperBodyAlpha > 0.f)
    {
        FPoseContext Overlay(Output);
        Sample(UpperBody, UpperBodyTime, bUpperBodyLoops, Overlay);
        if (PreviousUpperBody && UpperBodyCrossfadeAlpha < 1.f)
        {
            FPoseContext OldOverlay(Output), BlendedOverlay(Output);
            Sample(PreviousUpperBody, PreviousUpperBodyTime, bPreviousUpperBodyLoops, OldOverlay);
            FAnimationPoseData NewData(Overlay), OldData(OldOverlay), BlendedData(BlendedOverlay);
            FAnimationRuntime::BlendTwoPosesTogether(NewData, OldData, UpperBodyCrossfadeAlpha, BlendedData);
            Overlay = BlendedOverlay;
        }
        const int32 HipsBone = TorsoBone != INDEX_NONE ? RefSkeleton.GetParentIndex(TorsoBone) : INDEX_NONE;
        const FCompactPoseBoneIndex HipsC = HipsBone != INDEX_NONE ? BoneSet.MakeCompactPoseIndex(FMeshPoseBoneIndex(HipsBone)) : FCompactPoseBoneIndex(INDEX_NONE);
        const FCompactPoseBoneIndex TorsoC = TorsoBone != INDEX_NONE ? BoneSet.MakeCompactPoseIndex(FMeshPoseBoneIndex(TorsoBone)) : FCompactPoseBoneIndex(INDEX_NONE);
        if (!bUpperBodyFull && HipsC.GetInt() != INDEX_NONE && TorsoC.GetInt() != INDEX_NONE)
        {
            // só o tronco do clipe: o quadril (e as pernas) ficam os da andada, mas o giro do quadril do clipe em volta do
            // eixo de cima passa para o tronco (01/11: no Arco_Draw o quadril vira -91° e o braço do arco ficava apontando
            // para o lado, não para a mira)
            FQuat Parent = FQuat::Identity;
            for (int32 B = RefSkeleton.GetParentIndex(HipsBone); B != INDEX_NONE; B = RefSkeleton.GetParentIndex(B))
            {
                const FCompactPoseBoneIndex C = BoneSet.MakeCompactPoseIndex(FMeshPoseBoneIndex(B));
                if (C.GetInt() != INDEX_NONE) Parent = Output.Pose[C].GetRotation() * Parent;
            }
            const FQuat BaseHips = Parent * Output.Pose[HipsC].GetRotation();
            const FQuat ClipHips = Parent * Overlay.Pose[HipsC].GetRotation();
            FQuat Swing, Twist;
            (ClipHips * BaseHips.Inverse()).GetNormalized().ToSwingTwist(FVector::UpVector, Swing, Twist);
            // 03/11 (o usuário: "correndo puxando o arco está bem feio"): o tronco girava os 91° inteiros em cima das pernas
            // correndo para a frente. Agora o quadril leva um terço do giro (as pernas correm meio de lado, como um arqueiro)
            // e o tronco o resto
            FQuat HipShare = FQuat::Identity;
            if (UpperBody->GetName().Contains(TEXT("_Arco_")))
            {
                HipShare = FQuat::Slerp(FQuat::Identity, Twist, .35f * UpperBodyAlpha);
                Output.Pose[HipsC].SetRotation((Parent.Inverse() * HipShare * BaseHips).GetNormalized());
            }
            const FQuat TurnedHips = HipShare * BaseHips;
            const FQuat Rest = HipShare.Inverse() * Twist;
            Overlay.Pose[TorsoC].SetRotation((TurnedHips.Inverse() * Rest * TurnedHips * Overlay.Pose[TorsoC].GetRotation()).GetNormalized());
        }
        if (TorsoBone != INDEX_NONE)
        {
            // G4: giros e investidas (*_Full) levam o corpo inteiro, pernas e quadril junto
            for (FCompactPoseBoneIndex Bone : Output.Pose.ForEachBoneIndex())
            {
                if (bUpperBodyFull || UnderTorso(Bone)) Output.Pose[Bone].Blend(Output.Pose[Bone], Overlay.Pose[Bone], UpperBodyAlpha);
            }
        }
    }
    if (DrinkPose && DrinkAlpha > 0.f)
    {
        // a bebida manda só no braço esquerdo e na cabeça
        FPoseContext Drink(Output);
        Sample(DrinkPose, DrinkTime, false, Drink);
        const int32 LeftArm = RoleBone(RefSkeleton, TEXT("UpperArm_L"), TEXT("LeftShoulder"));
        const int32 HeadBone = RoleBone(RefSkeleton, TEXT("Head"), TEXT("Neck"));
        for (FCompactPoseBoneIndex Bone : Output.Pose.ForEachBoneIndex())
        {
            int32 Mesh = BoneSet.MakeMeshPoseIndex(Bone).GetInt();
            while (Mesh != INDEX_NONE && Mesh != LeftArm && Mesh != HeadBone) Mesh = RefSkeleton.GetParentIndex(Mesh);
            if (Mesh != INDEX_NONE) Output.Pose[Bone].Blend(Output.Pose[Bone], Drink.Pose[Bone], DrinkAlpha);
        }
    }
    ApplyBody(Output);
    ApplyPosture(Output);
    // 10/11 (o usuário: "a cintura vira mas a parte de baixo não, e quebra"): a torção do tronco em volta da coluna caía
    // inteira na primeira vértebra (Spine), logo acima do quadril, e o bloco quebrava ali. Agora ela se reparte em terços
    // pelas 3 vértebras: o peito e os braços terminam no mesmo lugar (a mira não muda), só a cintura torce menos
    {
        const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
        const FReferenceSkeleton& Ref = Bones.GetReferenceSkeleton();
        auto Compact = [&](const TCHAR* Name)
        {
            const int32 I = RoleBone(Ref, Name, Name);
            return I != INDEX_NONE ? Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(I)) : FCompactPoseBoneIndex(INDEX_NONE);
        };
        const FCompactPoseBoneIndex S0 = Compact(TEXT("Spine")), S1 = Compact(TEXT("Spine1")), S2 = Compact(TEXT("Spine2"));
        if (S0.GetInt() != INDEX_NONE && S1.GetInt() != INDEX_NONE && S2.GetInt() != INDEX_NONE)
        {
            const FVector Axis = Output.Pose.GetRefPose(S1).GetTranslation().GetSafeNormal();
            if (!Axis.IsNearlyZero())
            {
                // a torção medida a partir da pose de descanso (S0 = Descanso * Balanço * Torção)
                const FQuat Rest = Output.Pose.GetRefPose(S0).GetRotation();
                FQuat Swing, Twist;
                (Rest.Inverse() * Output.Pose[S0].GetRotation()).GetNormalized().ToSwingTwist(Axis, Swing, Twist);
                const FQuat Third = FQuat::Slerp(FQuat::Identity, Twist, 1.f / 3.f);
                const FQuat OldS1 = Output.Pose[S1].GetRotation();
                Output.Pose[S0].SetRotation((Rest * Swing * Third).GetNormalized());
                Output.Pose[S1].SetRotation((Third * OldS1).GetNormalized());
                Output.Pose[S2].SetRotation((OldS1.Inverse() * Third * OldS1 * Output.Pose[S2].GetRotation()).GetNormalized());
            }
        }
    }
    // 09/11 (o usuário: o pé dobrava demais e quebrava o bloco): o pé fica sempre na pose de descanso, preso à canela; só o joelho dobra
    {
        const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
        for (const TCHAR* Name : { TEXT("LeftFoot"), TEXT("RightFoot"), TEXT("LeftToeBase"), TEXT("RightToeBase") })
        {
            const int32 Foot = RoleBone(Bones.GetReferenceSkeleton(), Name, Name);
            const FCompactPoseBoneIndex Bone = Foot != INDEX_NONE ? Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(Foot)) : FCompactPoseBoneIndex(INDEX_NONE);
            if (Bone.GetInt() != INDEX_NONE) Output.Pose[Bone].SetRotation(Output.Pose.GetRefPose(Bone).GetRotation());
        }
    }
    if (bArmsOnly) { KeepArmsOnly(Output); return true; }
    if (bHideArms)
    {
        // 08/11 (o usuário: "o ombro aparece e atrapalha"): do ombro para baixo some (as mangas seguem, são da mesma pose)
        const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
        for (const TCHAR* Name : { TEXT("LeftShoulder"), TEXT("RightShoulder") })
        {
            const int32 Arm = RoleBone(Bones.GetReferenceSkeleton(), Name, Name);
            const FCompactPoseBoneIndex Bone = Arm != INDEX_NONE ? Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(Arm)) : FCompactPoseBoneIndex(INDEX_NONE);
            if (Bone.GetInt() != INDEX_NONE) Output.Pose[Bone].SetScale3D(FVector(.001f));
        }
    }
    if (bHideHead)
    {
        const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
        const int32 Head = RoleBone(Bones.GetReferenceSkeleton(), TEXT("Head"), TEXT("Head"));
        if (Head != INDEX_NONE)
        {
            const FCompactPoseBoneIndex Bone = Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(Head));
            if (Bone.GetInt() != INDEX_NONE) Output.Pose[Bone].SetScale3D(FVector(.001f));
        }
    }
    return true;
}
