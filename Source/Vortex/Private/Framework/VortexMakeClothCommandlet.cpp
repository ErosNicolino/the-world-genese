#include "Framework/VortexMakeClothCommandlet.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "ClothingAsset.h"
#include "ClothingAssetFactory.h"
#include "Engine/SkeletalMesh.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/SkeletalBodySetup.h"
#include "ReferenceSkeleton.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace
{
    constexpr float MaxSwingCm = 45.f;   // quanto a barra pode se afastar da pose do corpo

    int32 BoneEndingWith(const FReferenceSkeleton& Ref, const TCHAR* Suffix)
    {
        for (int32 Bone = 0; Bone < Ref.GetNum(); ++Bone)
        {
            const FString Name = Ref.GetBoneName(Bone).ToString();
            if (Name.EndsWith(FString(TEXT("_")) + Suffix) || Name.EndsWith(FString(TEXT(":")) + Suffix) || Name == Suffix) return Bone;
        }
        return INDEX_NONE;
    }

    FTransform RefWorld(const FReferenceSkeleton& Ref, int32 Bone)
    {
        FTransform Out = Ref.GetRefBonePose()[Bone];
        for (int32 Parent = Ref.GetParentIndex(Bone); Parent != INDEX_NONE; Parent = Ref.GetParentIndex(Parent)) Out *= Ref.GetRefBonePose()[Parent];
        return Out;
    }

    /** Uma cápsula de From até To, presa ao osso From. */
    void AddCapsule(UPhysicsAsset* Asset, const FReferenceSkeleton& Ref, int32 From, int32 To, float Radius)
    {
        USkeletalBodySetup* Body = NewObject<USkeletalBodySetup>(Asset, NAME_None, RF_Transactional);
        Body->BoneName = Ref.GetBoneName(From);
        const FTransform A = RefWorld(Ref, From), B = RefWorld(Ref, To);
        const FVector End = A.InverseTransformPosition(B.GetLocation());
        FKSphylElem Capsule;
        Capsule.Center = End * .5f;
        Capsule.Rotation = FRotationMatrix::MakeFromZ(End.GetSafeNormal()).Rotator();
        Capsule.Radius = Radius;
        Capsule.Length = FMath::Max(1.f, float(End.Size()) - Radius * 2.f);
        Body->AggGeom.SphylElems.Add(Capsule);
        Asset->SkeletalBodySetups.Add(Body);
    }

    bool SaveAsset(UObject* Asset)
    {
        UPackage* Package = Asset->GetOutermost();
        Package->MarkPackageDirty();
        FSavePackageArgs Args;
        Args.TopLevelFlags = RF_Public | RF_Standalone;
        return UPackage::SavePackage(Package, Asset, *FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension()), Args);
    }

    bool MakeCloth(USkeletalMesh* Mesh)
    {
        const FReferenceSkeleton& Ref = Mesh->GetRefSkeleton();
        const int32 Hips = BoneEndingWith(Ref, TEXT("Hips")), Spine = BoneEndingWith(Ref, TEXT("Spine"));
        if (Hips == INDEX_NONE) return false;
        // refazer do zero: a malha importada de novo pode trazer o pano antigo, ligado à geometria velha
        for (UClothingAssetBase* Old : TArray<TObjectPtr<UClothingAssetBase>>(Mesh->GetMeshClothingAssets())) if (UClothingAssetCommon* Common = Cast<UClothingAssetCommon>(Old)) Common->UnbindFromSkeletalMesh(Mesh, INDEX_NONE, INDEX_NONE);
        Mesh->GetMeshClothingAssets().Empty();

        const FString AssetName = TEXT("PA_") + Mesh->GetName();
        const FString PackageName = FPackageName::GetLongPackagePath(Mesh->GetOutermost()->GetName()) / AssetName;
        UPhysicsAsset* Physics = NewObject<UPhysicsAsset>(CreatePackage(*PackageName), *AssetName, RF_Public | RF_Standalone);
        if (Spine != INDEX_NONE) AddCapsule(Physics, Ref, Hips, Spine, 11.f);
        const int32 Neck = BoneEndingWith(Ref, TEXT("Neck")), HeadBone = BoneEndingWith(Ref, TEXT("Head"));   // os véus encostam na cabeça
        if (Neck != INDEX_NONE && HeadBone != INDEX_NONE) AddCapsule(Physics, Ref, Neck, HeadBone, 10.f);
        const int32 Chest = BoneEndingWith(Ref, TEXT("Spine2"));   // as capas e os tabardos encostam no tronco
        if (Spine != INDEX_NONE && Chest != INDEX_NONE) AddCapsule(Physics, Ref, Spine, Chest, 13.f);
        for (const TCHAR* Side : { TEXT("Left"), TEXT("Right") })
        {
            const int32 Up = BoneEndingWith(Ref, *(FString(Side) + TEXT("UpLeg"))), Leg = BoneEndingWith(Ref, *(FString(Side) + TEXT("Leg"))), Foot = BoneEndingWith(Ref, *(FString(Side) + TEXT("Foot")));
            if (Up == INDEX_NONE || Leg == INDEX_NONE || Foot == INDEX_NONE) return false;
            AddCapsule(Physics, Ref, Up, Leg, 11.5f);   // 10/11: a perna é uma caixa: o canto dela furava a saia
            AddCapsule(Physics, Ref, Leg, Foot, 11.f);
        }
        Physics->UpdateBodySetupIndexMap();
        Physics->UpdateBoundsBodiesArray();
        Physics->SetPreviewMesh(Mesh);

        FSkeletalMeshClothBuildParams Build;
        Build.AssetName = TEXT("Tecido");
        Build.LodIndex = 0;
        Build.SourceSection = 0;
        Build.bRemoveFromMesh = false;
        Build.PhysicsAsset = Physics;
        UClothingAssetCommon* Cloth = Cast<UClothingAssetCommon>(GetMutableDefault<UClothingAssetFactory>()->CreateFromSkeletalMesh(Mesh, Build));
        if (!Cloth || Cloth->LodData.IsEmpty()) return false;
        Mesh->AddClothingAsset(Cloth);
        Cloth->PhysicsAsset = Physics;

        // a cintura presa, e cada vez mais solto até a barra
        FClothPhysicalMeshData& Data = Cloth->LodData[0].PhysicalMeshData;
        double Top = -DBL_MAX, Bottom = DBL_MAX;
        for (const FVector3f& V : Data.Vertices) { Top = FMath::Max<double>(Top, V.Z); Bottom = FMath::Min<double>(Bottom, V.Z); }
        FPointWeightMap& MaxDistance = Data.FindOrAddWeightMap(EWeightMapTargetCommon::MaxDistance);
        MaxDistance.Initialize(Data.Vertices.Num());
        for (int32 i = 0; i < Data.Vertices.Num(); ++i)
        {
            const float Down = float((Top - Data.Vertices[i].Z) / FMath::Max(1e-3, Top - Bottom));
            MaxDistance[i] = Down < .05f ? 0.f : FMath::Pow(Down, 1.2f) * MaxSwingCm;
        }
        Cloth->ApplyParameterMasks(true);
        if (!Cloth->BindToSkeletalMesh(Mesh, 0, 0, 0)) return false;
        Mesh->PostEditChange();
        return SaveAsset(Physics) && SaveAsset(Mesh);
    }
}
#endif

int32 UVortexMakeClothCommandlet::Main(const FString& Params)
{
#if WITH_EDITOR
    IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
    Registry.SearchAllAssets(true);
    int32 Made = 0, Failed = 0;
    for (const TCHAR* Folder : { TEXT("/Game/Vortex/Characters/Heroes/humano/Pecas"), TEXT("/Game/Vortex/Characters/Heroes/humano/Roupas"), TEXT("/Game/Vortex/Characters/Heroes/humano/Armaduras") })
    {
        TArray<FAssetData> Assets;
        Registry.GetAssetsByPath(FName(Folder), Assets, false);
        for (const FAssetData& Asset : Assets)
        {
            if (!Asset.AssetName.ToString().EndsWith(TEXT("_tecido"))) continue;
            USkeletalMesh* Mesh = Cast<USkeletalMesh>(Asset.GetAsset());
            if (!Mesh) continue;
            const bool bOk = MakeCloth(Mesh);
            bOk ? ++Made : ++Failed;
            UE_LOG(LogTemp, Warning, TEXT("VORTEX_CLOTH %s %s"), *Asset.AssetName.ToString(), bOk ? TEXT("ok") : TEXT("FALHOU"));
        }
    }
    UE_LOG(LogTemp, Warning, TEXT("VORTEX_CLOTH feitos %d, falhas %d"), Made, Failed);
    return Failed ? 1 : 0;
#else
    return 1;
#endif
}
