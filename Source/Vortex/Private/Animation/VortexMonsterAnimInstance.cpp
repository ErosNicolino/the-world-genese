#include "Animation/VortexMonsterAnimInstance.h"

#include "Animation/AnimSequence.h"
#include "Animation/AnimationPoseData.h"
#include "AnimationRuntime.h"
#include "Creatures/VortexMonster.h"
#include "Creatures/VortexMonsterSpecies.h"
#include "Framework/VortexCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Combat/VortexWeaponComponent.h"
#include "Combat/VortexWeaponDefinition.h"

UAnimSequence* UVortexMonsterAnimInstance::Clip(FName Name)
{
    if (Name.IsNone() || Species.IsNone()) return nullptr;
    // trocou a malha (o bandido vira o corpo de blocos no BeginPlay): os clipes guardados não servem mais
    const USkeletalMeshComponent* Owner = GetSkelMeshComponent();
    const USkeletalMesh* Asset = Owner ? Owner->GetSkeletalMeshAsset() : nullptr;
    if (Asset != ClipsMesh.Get()) { Clips.Reset(); ClipsMesh = Asset; }
    if (TObjectPtr<UAnimSequence>* Existing = Clips.Find(Name)) return *Existing;
    const FString S = Species.ToString(), N = Name.ToString();
    const FVortexSpeciesDef* Humanoid = FVortexSpeciesDef::Find(Species);
    if (Humanoid && Humanoid->bHumanoid && Asset && Asset->GetName().StartsWith(TEXT("Hero_")))
    {
        // 11/11: o bandido no corpo de blocos: os clipes do Mixamo do herói (MX_humano_*), pela arma dele
        const FString W = Humanoid->Weapon.ToString();
        const bool bBow = W.StartsWith(TEXT("arco")), bHammer = W.StartsWith(TEXT("martelo"));
        FString Mx = N;
        if (N == TEXT("Run")) Mx = bBow ? TEXT("Run") : TEXT("RunArmed");
        else if (N.StartsWith(TEXT("Swing")))
        {
            const int32 Index = FCString::Atoi(*N.RightChop(5));
            Mx = bBow ? FString(Index == 0 ? TEXT("Arco_Release") : TEXT("Atk_Espada_1"))
                : FString::Printf(TEXT("Atk_%s_%s"), bHammer ? TEXT("Martelo") : TEXT("Espada"), Index >= 2 ? TEXT("3_Full") : Index == 1 ? TEXT("2") : TEXT("1"));
        }
        auto Load = [](const FString& Clip) -> UAnimSequence*
        {
            const FString Package = TEXT("/Game/Vortex/Characters/Heroes/Anims/humano/MX_humano_") + Clip;
            return FPackageName::DoesPackageExist(Package) ? LoadObject<UAnimSequence>(nullptr, *(Package + TEXT(".MX_humano_") + Clip)) : nullptr;
        };
        UAnimSequence* Found = Load(Mx);
        if (!Found) Found = Load(TEXT("Idle"));   // nunca em pose de T
        Clips.Add(Name, Found);
        return Found;
    }
    if (const FVortexSpeciesDef* D = Humanoid; D && D->bHumanoid)
    {
        // bandidos: postura e golpes da arma (DA_<tipo>); o resto (Hit, Death...) do aventureiro
        const UVortexWeaponDefinition* Weapon = UVortexWeaponComponent::LoadType(D->Weapon);
        UAnimSequence* Found = nullptr;
        if (Weapon)
        {
            if (Name == TEXT("Idle")) Found = Weapon->StanceIdle;
            else if (Name == TEXT("Walk")) Found = Weapon->StanceWalk;
            else if (Name == TEXT("Run")) Found = Weapon->StanceRun;
            else if (N.StartsWith(TEXT("Swing")))
            {
                const int32 Index = FCString::Atoi(*N.RightChop(5));
                if (Weapon->Swings.IsValidIndex(Index)) Found = Weapon->Swings[Index].Animation;
                else if (Weapon->Swings.Num()) Found = Weapon->Swings.Last().Animation;
            }
        }
        if (!Found) Found = LoadObject<UAnimSequence>(nullptr,
            *FString::Printf(TEXT("/Game/Vortex/Characters/Adventurer/Adventurer_Skinned/SkeletalMeshes/Adventurer_Skinned%s.Adventurer_Skinned%s"), *N, *N));
        Clips.Add(Name, Found);
        return Found;
    }
    UAnimSequence* Sequence = LoadObject<UAnimSequence>(nullptr,
        *FString::Printf(TEXT("/Game/Vortex/Creatures/%s/%s_Skinned/SkeletalMeshes/%s_Skinned%s.%s_Skinned%s"), *S, *S, *S, *N, *S, *N));
    Clips.Add(Name, Sequence);
    return Sequence;
}

void UVortexMonsterAnimInstance::Play(FName Name, bool bLoop, float NewRate, float Blend)
{
    if (CurrentName == Name && !(!bLoop && !bCurrentLoops)) { Rate = NewRate; return; }
    UAnimSequence* Next = Clip(Name);
    if (!Next) return;
    Previous = Current;
    PreviousTime = CurrentTime;
    bPreviousLoops = bCurrentLoops;
    PreviousRate = Rate;
    Current = Next;
    CurrentName = Name;
    CurrentTime = 0.f;
    bCurrentLoops = bLoop;
    Rate = NewRate;
    BlendDuration = Blend;
    BlendAlpha = Previous ? 0.f : 1.f;
}

void UVortexMonsterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);
    const AVortexMonster* Monster = Cast<AVortexMonster>(TryGetPawnOwner());
    const FVortexSpeciesDef* D = Monster ? Monster->Def() : nullptr;
    if (Monster && D)
    {
        // o nome dos arquivos (as pelagens do cavalo têm arquivo próprio: Horse_Baio...)
        if (Species != Monster->GetModelName()) { Species = Monster->GetModelName(); Clips.Reset(); }        // ação nova do servidor: toca uma vez
        if (Monster->GetActSerial() != SeenSerial)
        {
            SeenSerial = Monster->GetActSerial();
            const FName Act = Monster->GetActClip();
            if (!Act.IsNone() && Clip(Act))
            {
                Play(Act, false, 1.f, Monster->IsCreatureDead() ? .1f : .12f);
                bOneShot = true;
            }
            else bOneShot = false;
        }
        const bool bFinished = bOneShot && Current && CurrentTime * Rate >= Current->GetPlayLength() - .02f;
        if (bOneShot && bFinished && !Monster->IsCreatureDead() && !(Monster->IsPassive() && Monster->GetActClip() == D->AlertClip)) bOneShot = false;
        // 16/11 (o usuário: "dele parando quando está correndo"): vinha a galope e parou em menos de meio segundo -> a
        // freada (Brake: senta nas patas de trás); se voltar a andar no meio, larga o clipe
        if (Monster->IsHorse() && !Monster->IsCreatureDead())
        {
            const float Speed = Monster->GetVelocity().Size2D();
            const double Now = GetWorld()->GetTimeSeconds();
            if (Speed > D->Speed * .8f) LastFastTime = Now;
            else if (Speed < 150.f && Now - LastFastTime < .5 && !bOneShot && Clip(TEXT("Brake")))
            {
                Play(TEXT("Brake"), false, 1.f, .1f);
                bOneShot = true;
                LastFastTime = -10.0;
            }
            else if (bOneShot && CurrentName == TEXT("Brake") && Speed > 220.f) bOneShot = false;
        }
        // 16/11 (o usuário: "ao pular com o cavalo ele continua andando no ar"): com o cavaleiro no ar, o salto (Jump, em laço:
        // as patas da frente recolhidas, as de trás esticadas)
        const AVortexCharacter* Jockey = Monster->IsHorse() ? Monster->GetRider() : nullptr;
        if (!bOneShot && !Monster->IsCreatureDead() && Jockey && Jockey->GetCharacterMovement()->IsFalling() && Clip(TEXT("Jump")))
            Play(TEXT("Jump"), true, 1.f, .12f);
        else if (!bOneShot && !Monster->IsCreatureDead())
        {
            const float Speed = Monster->GetVelocity().Size2D();
            if (D->bFlying) Play(D->IdleClip, true, FMath::Clamp(.8f + Speed / 600.f, .8f, 1.5f));
            else if (Monster->IsGrazing() && !D->GrazeClip.IsNone()) Play(D->GrazeClip, true);
            else if (Speed > 30.f)
            {
                const bool bRun = !D->RunClip.IsNone() && (Monster->IsFleeing() || Speed > D->Speed * .8f);
                const FName Clip = bRun ? D->RunClip : D->WalkClip;
                const float Pace = bRun ? FMath::Clamp(Speed / D->Speed, .7f, 1.5f) : FMath::Clamp(Speed / (D->Speed * (D->RunClip.IsNone() ? 1.f : .45f)), .5f, 1.7f);
                Play(Clip, true, Pace);
            }
            else Play(D->IdleClip, true);
        }
    }
    CurrentTime += DeltaSeconds * Rate;
    PreviousTime += DeltaSeconds * PreviousRate;
    BlendAlpha = FMath::Min(1.f, BlendAlpha + DeltaSeconds / FMath::Max(.01f, BlendDuration));
    if (BlendAlpha >= 1.f) Previous = nullptr;
}

FAnimInstanceProxy* UVortexMonsterAnimInstance::CreateAnimInstanceProxy()
{
    return new FVortexMonsterAnimInstanceProxy(this);
}

void FVortexMonsterAnimInstanceProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
    FAnimInstanceProxy::PreUpdate(InAnimInstance, DeltaSeconds);
    const UVortexMonsterAnimInstance* Instance = CastChecked<UVortexMonsterAnimInstance>(InAnimInstance);
    Current = Instance->Current;
    Previous = Instance->Previous;
    CurrentTime = Instance->CurrentTime;
    PreviousTime = Instance->PreviousTime;
    Alpha = Instance->BlendAlpha;
    bCurrentLoops = Instance->bCurrentLoops;
    bPreviousLoops = Instance->bPreviousLoops;
}

bool FVortexMonsterAnimInstanceProxy::Evaluate(FPoseContext& Output)
{
    auto Sample = [](UAnimSequence* Sequence, float Time, bool bLoop, FPoseContext& Pose)
    {
        const float Length = Sequence->GetPlayLength();
        const float Position = bLoop && Length > 0.f ? FMath::Fmod(Time, Length) : FMath::Min(Time, Length);
        FAnimationPoseData Data(Pose);
        Sequence->GetAnimationPose(Data, FAnimExtractContext(static_cast<double>(Position), false, FDeltaTimeRecord(), bLoop));
    };
    if (!Current) return false;
    Sample(Current, CurrentTime, bCurrentLoops, Output);
    if (Previous && Alpha < 1.f)
    {
        FPoseContext Old(Output), Blended(Output);
        Sample(Previous, PreviousTime, bPreviousLoops, Old);
        FAnimationPoseData OldData(Old), NewData(Output), BlendedData(Blended);
        FAnimationRuntime::BlendTwoPosesTogether(NewData, OldData, Alpha, BlendedData);
        Output = Blended;
    }
    return true;
}
