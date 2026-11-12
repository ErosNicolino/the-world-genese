#include "Creatures/VortexBlockBody.h"

#include "Animation/Skeleton.h"
#include "Animation/VortexAnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Texture2D.h"
#include "Framework/VortexHairComponent.h"
#include "Framework/VortexLook.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Rendering/SkeletalMeshRenderData.h"

// O NPC no corpo de blocos do herói (02/11: os NPCs eram o aventureiro antigo, muito diferentes do herói). Peças do
// look (Data/Population/npc_posts.json, "parts"): block_hair (o cabelo do menu, block_hair_<id>), block_face (o
// estilo dos olhos), npc_outfit (a roupa só dele: Roupa_<id> de Tools/Blender/MakeBlockOutfit.py, fora do menu) ou
// block_outfit (uma do menu). As cores: skin não muda (a textura do corpo), hair_color tinge o cabelo.
bool VortexBlockBody::Apply(ACharacter* Npc, const FVortexLook& Raw, FName ArmorSet)
{
    USkeletalMeshComponent* Mesh = Npc->GetMesh();
    if (Raw.Race != TEXT("humano")) return false;   // o corpo de blocos do NPC é só o humano (o elfo jogador usa o mesmo corpo, com outra pele)
    const FString Race = Raw.Race.ToString();
    const bool bFemale = Raw.Appearance == TEXT("female");
    const FString Folder = FString::Printf(TEXT("/Game/Vortex/Characters/Heroes/%s/"), *Race);
    // 05/11: o corpo feminino é a mesma malha do masculino (só a textura muda, abaixo), como no herói
    USkeletalMesh* Model = LoadObject<USkeletalMesh>(nullptr, *FString::Printf(TEXT("%sHero_%s.Hero_%s"), *Folder, *Race, *Race));
    if (!Model) return false;
    if (Mesh->GetSkeletalMeshAsset() != Model)
    {
        Mesh->SetSkeletalMesh(Model);
        Mesh->EmptyOverrideMaterials();
        // como no herói (AVortexCharacter::ApplyHeroModel): a raiz parada e o quadril vindo da animação
        if (USkeleton* Skeleton = Model->GetSkeleton())
        {
            const FReferenceSkeleton& Ref = Skeleton->GetReferenceSkeleton();
            for (int32 Bone = 0; Bone < Ref.GetNum(); ++Bone)
            {
                const FString Name = Ref.GetBoneName(Bone).ToString();
                const bool bHips = Name == TEXT("Hips") || Name.EndsWith(TEXT(":Hips")) || Name.EndsWith(TEXT("_Hips"));
                const EBoneTranslationRetargetingMode::Type Want = bHips && Name != TEXT("Root") ? EBoneTranslationRetargetingMode::AnimationScaled : EBoneTranslationRetargetingMode::Skeleton;
                if (Skeleton->GetBoneTranslationRetargetingMode(Bone) != Want) Skeleton->SetBoneTranslationRetargetingMode(Bone, Want);
            }
        }
    }
    // o rosto (MI_HeroFace_<raça>): a pele e os olhos do corpo escolhido e o estilo dos olhos
    if (UMaterialInterface* Face = LoadObject<UMaterialInterface>(nullptr, *FString::Printf(TEXT("%sMI_HeroFace_%s.MI_HeroFace_%s"), *Folder, *Race, *Race)))
    {
        UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Face, Npc);
        const FString F = bFemale ? TEXT("F_") : TEXT("");
        if (bFemale)
            if (UTexture* Skin = LoadObject<UTexture>(nullptr, *FString::Printf(TEXT("%sT_%s_Pixel_F.T_%s_Pixel_F"), *Folder, *Race, *Race)))
                Material->SetTextureParameterValue(TEXT("Base"), Skin);
        const FName Style = Raw.Pieces.FindRef(TEXT("block_face"));
        const FString Eyes = Style.IsNone() || Style == TEXT("padrao") ? FString::Printf(TEXT("T_%s_Eyes%s"), *Race, bFemale ? TEXT("_F") : TEXT(""))
            : FString::Printf(TEXT("T_%s_Eyes_%s%s"), *Race, *F, *Style.ToString());
        if (UTexture* Texture = LoadObject<UTexture>(nullptr, *FString::Printf(TEXT("%s%s.%s"), *Folder, *Eyes, *Eyes)))
            Material->SetTextureParameterValue(TEXT("Eyes"), Texture);
        for (int32 Slot = 0; Slot < Mesh->GetNumMaterials(); ++Slot) Mesh->SetMaterial(Slot, Material);
    }
    Mesh->SetVisibility(true, false);
    Mesh->SetCastShadow(true);
    Mesh->SetRelativeScale3D(FVector(1.f));
    if (UVortexAnimInstance* Anim = Cast<UVortexAnimInstance>(Mesh->GetAnimInstance())) Anim->Body = FVortexBodyShape();
    // as peças antigas (o guarda-roupa do aventureiro) somem
    TArray<USkeletalMeshComponent*> Old;
    Npc->GetComponents(Old);
    for (USkeletalMeshComponent* Part : Old)
        if (Part != Mesh && Part->GetFName() != TEXT("BlockOutfit") && !Part->GetName().StartsWith(TEXT("BlockArmor_"))) { Part->SetSkeletalMesh(nullptr); Part->SetVisibility(false); }

    // o cabelo (o mesmo do menu do herói, com a física e a cor)
    const FVortexBlockHair* Hair = FVortexLookCatalog::BlockHair(Raw);
    USkeletalMesh* HairMesh = Hair && !Hair->Mesh.IsEmpty() ? LoadObject<USkeletalMesh>(nullptr, *Hair->Mesh) : nullptr;
    if (HairMesh)
    {
        UVortexHairComponent* HairPart = Npc->FindComponentByClass<UVortexHairComponent>();
        if (!HairPart)
        {
            HairPart = NewObject<UVortexHairComponent>(Npc, TEXT("BlockHair"));
            HairPart->SetupAttachment(Mesh);
            HairPart->RegisterComponent();
        }
        HairPart->SetBody(Mesh);
        HairPart->SetHair(HairMesh, Hair->Looseness);
        HairPart->SetVisibility(true);
        // os cabelos _mechas (a Julia) têm o material das mechas: a cor delas em hair_streak
        const bool bStreaks = HairMesh->GetName().EndsWith(TEXT("_mechas"));
        const FString HairMaterial = FString::Printf(TEXT("/Game/Vortex/Characters/Heroes/humano/Hair/%s.%s"),
            bStreaks ? TEXT("M_BlockHairMechas") : TEXT("M_BlockHair"), bStreaks ? TEXT("M_BlockHairMechas") : TEXT("M_BlockHair"));
        if (UMaterialInstanceDynamic* Material = HairPart->CreateDynamicMaterialInstance(0, LoadObject<UMaterialInterface>(nullptr, *HairMaterial)))
        {
            Material->SetVectorParameterValue(TEXT("Tint"), FLinearColor(Raw.Colors.FindRef(TEXT("hair_color"))));
            if (const FColor* Streak = Raw.Colors.Find(TEXT("hair_streak"))) Material->SetVectorParameterValue(TEXT("StreakTint"), FLinearColor(*Streak));
        }
    }

    // a roupa: a do NPC (npc_outfit) ou uma do menu (block_outfit); segue a pose do corpo
    const FName Own = Raw.Pieces.FindRef(TEXT("npc_outfit"));
    FString OutfitPath;
    if (!Own.IsNone()) OutfitPath = FString::Printf(TEXT("%sRoupas/Roupa_%s.Roupa_%s"), *Folder, *Own.ToString(), *Own.ToString());
    else if (const FVortexBlockOutfit* Outfit = FVortexLookCatalog::BlockOutfit(Raw)) OutfitPath = Outfit->Mesh;
    if (USkeletalMesh* OutfitMesh = OutfitPath.IsEmpty() ? nullptr : LoadObject<USkeletalMesh>(nullptr, *OutfitPath))
    {
        USkeletalMeshComponent* OutfitPart = nullptr;
        for (USkeletalMeshComponent* Part : Old) if (Part->GetFName() == TEXT("BlockOutfit")) OutfitPart = Part;
        if (!OutfitPart)
        {
            OutfitPart = NewObject<USkeletalMeshComponent>(Npc, TEXT("BlockOutfit"));
            OutfitPart->SetupAttachment(Mesh);
            OutfitPart->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            OutfitPart->SetGenerateOverlapEvents(false);
            OutfitPart->bUseBoundsFromLeaderPoseComponent = true;
            OutfitPart->RegisterComponent();
        }
        OutfitPart->SetSkeletalMesh(OutfitMesh);
        OutfitPart->SetLeaderPoseComponent(Mesh);
        OutfitPart->SetVisibility(true);
    }
    // 11/11: a armadura por cima (os cavaleiros bandidos): A_<conjunto>_<peça> do herói, na pose do corpo; a roupa some onde
    // ela cobre (como no herói: as regiões M_Roupa_cabeca/_tronco/_pernas/_pes) e o cabelo some com o elmo
    static const TCHAR* Slots[] = { TEXT("head"), TEXT("chest"), TEXT("legs"), TEXT("boots") };
    static const TCHAR* Regions[] = { TEXT("_cabeca"), TEXT("_tronco"), TEXT("_pernas"), TEXT("_pes") };
    bool bCovered[4] = {};
    for (int32 i = 0; i < 4; ++i)
    {
        const FString Id = FString::Printf(TEXT("A_%s_%s"), *ArmorSet.ToString(), Slots[i]);
        USkeletalMesh* Piece = ArmorSet.IsNone() ? nullptr : LoadObject<USkeletalMesh>(nullptr, *FString::Printf(TEXT("%sArmaduras/%s.%s"), *Folder, *Id, *Id));
        const FName PartName(*FString::Printf(TEXT("BlockArmor_%s"), Slots[i]));
        USkeletalMeshComponent* Part = nullptr;
        for (USkeletalMeshComponent* Candidate : Old) if (Candidate->GetFName() == PartName) Part = Candidate;
        if (!Part && !Piece) continue;
        if (!Part)
        {
            Part = NewObject<USkeletalMeshComponent>(Npc, PartName);
            Part->SetupAttachment(Mesh);
            Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Part->SetGenerateOverlapEvents(false);
            Part->bUseBoundsFromLeaderPoseComponent = true;
            Part->RegisterComponent();
        }
        Part->SetSkeletalMesh(Piece);
        Part->SetLeaderPoseComponent(Mesh);
        Part->SetVisibility(Piece != nullptr);
        bCovered[i] = Piece != nullptr;
    }
    USkeletalMeshComponent* Outfit = nullptr;
    TArray<USkeletalMeshComponent*> Now;
    Npc->GetComponents(Now);
    for (USkeletalMeshComponent* Part : Now) if (Part->GetFName() == TEXT("BlockOutfit")) Outfit = Part;
    if (USkeletalMesh* OutfitAsset = Outfit ? Outfit->GetSkeletalMeshAsset() : nullptr)
        if (const FSkeletalMeshRenderData* Render = OutfitAsset->GetResourceForRendering(); Render && !Render->LODRenderData.IsEmpty())
        {
            const TArray<FSkeletalMaterial>& Materials = OutfitAsset->GetMaterials();
            const TArray<FSkelMeshRenderSection>& Sections = Render->LODRenderData[0].RenderSections;
            for (int32 Section = 0; Section < Sections.Num(); ++Section)
            {
                const int32 Index = Sections[Section].MaterialIndex;
                if (!Materials.IsValidIndex(Index)) continue;
                const FString Slot = Materials[Index].MaterialSlotName.ToString();
                for (int32 i = 0; i < 4; ++i)
                    if (Slot.EndsWith(Regions[i])) Outfit->ShowMaterialSection(Index, Section, !bCovered[i], 0);
            }
        }
    if (UVortexHairComponent* HairPart = Npc->FindComponentByClass<UVortexHairComponent>(); HairPart && bCovered[0]) HairPart->SetVisibility(false);
    return true;
}
