#include "Framework/VortexHairComponent.h"

#include "Engine/SkeletalMesh.h"

namespace
{
    // os ossos do Mixamo chegam como "mixamorig:Head" ou "mixamorig_Head", conforme a importação
    FName MixamoBone(const USkinnedMeshComponent* Mesh, const TCHAR* Name)
    {
        for (const TCHAR* Prefix : { TEXT("mixamorig:"), TEXT("mixamorig_") })
        {
            const FName Bone(FString(Prefix) + Name);
            if (Mesh->GetBoneIndex(Bone) != INDEX_NONE) return Bone;
        }
        return NAME_None;
    }

    constexpr float Step = 1.f / 60.f;
}

UVortexHairComponent::UVortexHairComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    // depois da animação do corpo (e o pré-requisito em SetBody)
    PrimaryComponentTick.TickGroup = TG_PostPhysics;
    SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SetGenerateOverlapEvents(false);
    bReceivesDecals = false;
    bCastHiddenShadow = true;
}

void UVortexHairComponent::SetBody(USkeletalMeshComponent* InBody)
{
    if (Body.Get() == InBody) return;
    if (Body.IsValid()) RemoveTickPrerequisiteComponent(Body.Get());
    Body = InBody;
    if (LeaderPoseComponent.IsValid()) SetLeaderPoseComponent(InBody);
    if (InBody) AddTickPrerequisiteComponent(InBody);
}

void UVortexHairComponent::SetHair(USkeletalMesh* Mesh, float InLooseness)
{
    if (GetSkinnedAsset() != Mesh)
    {
        SetLeaderPoseComponent(nullptr);
        SetSkinnedAssetAndUpdate(Mesh);
    }
    Looseness = InLooseness;
    Chains.Reset();
    MappedBody = nullptr;
    if (!Mesh) return;
    // as correntes: cada osso Hair_* filho da cabeça começa uma; os filhos dele continuam a mesma
    const FReferenceSkeleton& Ref = Mesh->GetRefSkeleton();
    for (int32 Bone = 0; Bone < Ref.GetNum(); ++Bone)
    {
        const FName Name = Ref.GetBoneName(Bone);
        if (!Name.ToString().StartsWith(TEXT("Hair_"))) continue;
        const int32 Parent = Ref.GetParentIndex(Bone);
        const FName ParentName = Parent != INDEX_NONE ? Ref.GetBoneName(Parent) : NAME_None;
        FChain* Into = Chains.FindByPredicate([ParentName](const FChain& C) { return C.Bones.Last() == ParentName; });
        if (!Into) Into = &Chains.AddDefaulted_GetRef();
        Into->Bones.Add(Name);
    }
    Chains.RemoveAll([](const FChain& C) { return C.Bones.Num() < 2; });
    // sem física: segue o corpo como as roupas, na pose que vai para a tela. A cópia osso a osso se soltava da cabeça no
    // combate (01/11: o moicano ficou torto ao lado do ombro depois de bater e girar rápido)
    const bool bFollow = Looseness <= 0.f || Chains.IsEmpty();
    bUseBoundsFromLeaderPoseComponent = bFollow;
    SetLeaderPoseComponent(bFollow ? Body.Get() : nullptr);
}

void UVortexHairComponent::CopyBodyPose(USkeletalMeshComponent* Driver)
{
    // a pose final do corpo (o espaço do componente, a mesma que vai para a tela), e não os ossos locais da animação
    const FReferenceSkeleton& Ref = GetSkinnedAsset()->GetRefSkeleton();
    if (MappedBody.Get() != Driver->GetSkinnedAsset() || BodyBone.Num() != Ref.GetNum())
    {
        BodyBone.SetNum(Ref.GetNum());
        for (int32 Bone = 0; Bone < Ref.GetNum(); ++Bone) BodyBone[Bone] = Driver->GetBoneIndex(Ref.GetBoneName(Bone));
        MappedBody = Driver->GetSkinnedAsset();
    }
    const TArray<FTransform>& Space = Driver->GetComponentSpaceTransforms();
    if (BoneSpaceTransforms.Num() != Ref.GetNum()) return;
    for (int32 Bone = 0; Bone < Ref.GetNum(); ++Bone)
    {
        // os ossos Hair_* (e quem não tem o pai no corpo) ficam como no modelo, presos ao pai
        const int32 From = BodyBone[Bone], Parent = Ref.GetParentIndex(Bone);
        const int32 ParentFrom = Parent != INDEX_NONE ? BodyBone[Parent] : INDEX_NONE;
        if (!Space.IsValidIndex(From) || (Parent != INDEX_NONE && !Space.IsValidIndex(ParentFrom)))
            BoneSpaceTransforms[Bone] = Ref.GetRefBonePose()[Bone];
        else
            BoneSpaceTransforms[Bone] = Parent == INDEX_NONE ? Space[From] : Space[From].GetRelativeTransform(Space[ParentFrom]);
    }
    MarkRefreshTransformDirty();
}

void UVortexHairComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    USkeletalMeshComponent* Driver = Body.Get();
    // seguindo o corpo (sem física) não há o que fazer aqui
    if (!Driver || !GetSkinnedAsset() || !IsVisible() || LeaderPoseComponent.IsValid()) return;
    CopyBodyPose(Driver);
    // a pose copiada vale já neste quadro: só marcada, ela entrava no quadro seguinte e o cabelo saía da cabeça nas
    // trocas de animação e nos movimentos rápidos (01/11: até 60 cm num quadro; o moicano "saía ou ficava torto")
    if (Looseness > 0.f && Chains.Num()) Simulate(DeltaTime);
    else RefreshBoneTransforms();
}

void UVortexHairComponent::Simulate(float DeltaTime)
{
    // as esferas do corpo (no tamanho do herói)
    USkeletalMeshComponent* Driver = Body.Get();
    const float Scale = Driver->GetComponentScale().X;
    TArray<FSphere, TInlineAllocator<12>> Spheres;
    const FName Head = MixamoBone(Driver, TEXT("Head")), Top = MixamoBone(Driver, TEXT("HeadTop_End"));
    if (!Head.IsNone())
    {
        const FVector A = Driver->GetBoneLocation(Head), B = Top.IsNone() ? A + FVector(0.f, 0.f, 33.f * Scale) : Driver->GetBoneLocation(Top);
        Spheres.Emplace((A + B) * .5f, FVector::Dist(A, B) * .5f + CollisionMargin);
    }
    const TPair<const TCHAR*, float> Parts[] = { { TEXT("Neck"), 7.f }, { TEXT("Spine2"), 10.f }, { TEXT("Spine1"), 10.f }, { TEXT("Spine"), 10.f },
        { TEXT("Hips"), 11.f }, { TEXT("LeftShoulder"), 8.f }, { TEXT("RightShoulder"), 8.f }, { TEXT("LeftArm"), 7.f }, { TEXT("RightArm"), 7.f } };
    for (const TPair<const TCHAR*, float>& Part : Parts)
    {
        const FName Bone = MixamoBone(Driver, Part.Key);
        if (!Bone.IsNone()) Spheres.Emplace(Driver->GetBoneLocation(Bone), Part.Value * Scale + CollisionMargin);
    }

    // passos fixos de 1/60 s (no máximo 4 por quadro): o mesmo balanço em qualquer FPS
    const float Damping = FMath::Lerp(.14f, .05f, Looseness);
    const float Stiffness = FMath::Lerp(.30f, .035f, Looseness);
    const FVector Gravity(0.f, 0.f, -980.f * Step * Step);
    StepLeft = FMath::Min(StepLeft + DeltaTime, Step * 4.f);
    const int32 Steps = FMath::FloorToInt(StepLeft / Step);
    StepLeft -= Steps * Step;

    for (FChain& Chain : Chains)
    {
        const int32 Count = Chain.Bones.Num();
        // a forma do modelo agora (a corrente parada, presa à cabeça): as juntas e a ponta, que continua o último osso
        TArray<FTransform, TInlineAllocator<8>> Rest;
        TArray<FVector, TInlineAllocator<9>> Target;
        for (const FName& Bone : Chain.Bones)
        {
            Rest.Add(GetBoneTransformByName(Bone, EBoneSpaces::WorldSpace));
            Target.Add(Rest.Last().GetLocation());
        }
        Target.Add(Target[Count - 1] + (Target[Count - 1] - Target[Count - 2]));
        TArray<float, TInlineAllocator<9>> Length;
        Length.Add(0.f);
        for (int32 i = 1; i <= Count; ++i) Length.Add(FVector::Dist(Target[i], Target[i - 1]));

        TArray<FVector>& Points = Chain.Points;
        TArray<FVector>& Previous = Chain.Previous;
        // começou agora ou teleportou (renasceu, montou, trocou de cena): parte da forma do modelo
        if (Points.Num() != Count + 1 || FVector::DistSquared(Points[0], Target[0]) > FMath::Square(150.f))
        {
            Points = TArray<FVector>(Target);
            Previous = Points;
        }
        for (int32 Tick = 0; Tick < Steps; ++Tick)
        {
            Points[0] = Previous[0] = Target[0];
            for (int32 i = 1; i <= Count; ++i)
            {
                const FVector Velocity = (Points[i] - Previous[i]) * (1.f - Damping);
                Previous[i] = Points[i];
                Points[i] += Velocity + Gravity;
                Points[i] += (Target[i] - Points[i]) * Stiffness;
            }
            for (int32 Pass = 0; Pass < 3; ++Pass)
            {
                for (int32 i = 1; i <= Count; ++i)
                {
                    Points[i] = Points[i - 1] + (Points[i] - Points[i - 1]).GetSafeNormal() * Length[i];
                    for (const FSphere& Sphere : Spheres)
                    {
                        const FVector Away = Points[i] - Sphere.Center;
                        if (Away.SizeSquared() < FMath::Square(Sphere.W)) Points[i] = Sphere.Center + Away.GetSafeNormal() * Sphere.W;
                    }
                }
            }
        }

        // os ossos apontam para o ponto seguinte (da raiz para a ponta: cada um já vê o pai no lugar novo)
        for (int32 i = 0; i < Count; ++i)
        {
            const FVector From = (Target[i + 1] - Target[i]).GetSafeNormal(), To = (Points[i + 1] - Points[i]).GetSafeNormal();
            FTransform Bone = Rest[i];
            Bone.SetRotation(FQuat::FindBetweenNormals(From, To) * Rest[i].GetRotation());
            Bone.SetLocation(Points[i]);
            SetBoneTransformByName(Chain.Bones[i], Bone, EBoneSpaces::WorldSpace);
        }
    }
    RefreshBoneTransforms();
}
