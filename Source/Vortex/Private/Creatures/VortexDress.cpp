#include "Creatures/VortexDress.h"

#include "Animation/VortexAnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"

// Uma peça (copia a pose da malha base); vazia esconde. O mesmo do AVortexNpc::SetPart.
void UVortexDressComponent::SetPart(USkeletalMeshComponent* Base, FName PartName, FName PieceId)
{
    USkeletalMeshComponent* Part = nullptr;
    for (USkeletalMeshComponent* Candidate : Parts) if (Candidate && Candidate->GetFName() == PartName) Part = Candidate;
    // peça vazia e ainda sem componente: nada a esconder (criar ~6 componentes vazios por bandido custava tempo)
    if (!Part && (PieceId.IsNone() || PieceId.ToString().EndsWith(TEXT("_none")))) return;
    if (!Part)
    {
        Part = NewObject<USkeletalMeshComponent>(GetOwner(), PartName);
        Part->SetupAttachment(Base);
        Part->SetLeaderPoseComponent(Base);
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetReceivesDecals(false);
        Part->bUseBoundsFromLeaderPoseComponent = true;
        Part->RegisterComponent();
        Parts.Add(Part);
    }
    USkeletalMesh* PieceMesh = nullptr;
    if (!PieceId.IsNone() && !PieceId.ToString().EndsWith(TEXT("_none")))
    {
        const FString Id = PieceId.ToString();
        PieceMesh = LoadObject<USkeletalMesh>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Characters/Adventurer/Wardrobe/%s/SkeletalMeshes/%s.%s"), *Id, *Id, *Id));
        if (PieceMesh && Base->GetSkeletalMeshAsset() && PieceMesh->GetSkeleton() != Base->GetSkeletalMeshAsset()->GetSkeleton()) PieceMesh = nullptr;
    }
    Part->SetSkeletalMesh(PieceMesh);
    Part->SetVisibility(PieceMesh != nullptr);
    Part->SetMaterial(0, LookMaterial ? static_cast<UMaterialInterface*>(LookMaterial) : Base->GetMaterial(0));
}

void UVortexDressComponent::Apply(USkeletalMeshComponent* Base, const FVortexLook& InLook)
{
    const FVortexLookCatalog& Catalog = FVortexLookCatalog::Get();
    if (!Base || !Catalog.IsValid() || !Base->GetSkeletalMeshAsset()) return;
    const FVortexLook Look = Catalog.Normalized(InLook);
    if (!LookMaterial) LookMaterial = UMaterialInstanceDynamic::Create(Base->GetMaterial(0), this);
    if (UTexture2D* Built = Catalog.BuildPalette(Look, this))
    {
        Palette = Built;
        LookMaterial->SetTextureParameterValue(TEXT("BaseColorTexture"), Palette);
        LookMaterial->SetTextureParameterValue(TEXT("Palette"), Palette);
    }
    static const TPair<FName, FName> PartOf[] = {
        {TEXT("race"), TEXT("Race")}, {TEXT("hair"), TEXT("Hair")}, {TEXT("eyes"), TEXT("Eyes")}, {TEXT("mouth"), TEXT("Mouth")},
        {TEXT("head"), TEXT("Accessory")}, {TEXT("face_gear"), TEXT("AccessoryFace")}, {TEXT("back"), TEXT("AccessoryBack")},
        {TEXT("shirt"), TEXT("Shirt")}, {TEXT("pants"), TEXT("Pants")}, {TEXT("shoes"), TEXT("Shoes")}, {TEXT("gloves"), TEXT("Gloves")},
    };
    static const TMap<FName, FName> SlotOf = { {TEXT("hair"), TEXT("hair_color")}, {TEXT("shirt"), TEXT("shirt_color")},
        {TEXT("pants"), TEXT("pants_color")}, {TEXT("shoes"), TEXT("boots_color")}, {TEXT("gloves"), TEXT("glove_color")} };
    SetPart(Base, TEXT("Core"), TEXT("core"));
    SetPart(Base, TEXT("Face"), Catalog.FacePiece(Look));
    for (const TPair<FName, FName>& Entry : PartOf)
    {
        const FName* Piece = Look.Pieces.Find(Entry.Key);
        SetPart(Base, Entry.Value, Piece ? *Piece : NAME_None);
        const FVortexPieceInfo* Info = Piece ? Catalog.Piece(*Piece) : nullptr;
        const FName* Slot = SlotOf.Find(Entry.Key);
        const FColor* PieceBase = Info && Slot ? Info->Defaults.Find(*Slot) : nullptr;
        const FColor* Chosen = Slot ? Look.Colors.Find(*Slot) : nullptr;
        if (Info && Info->Races.Num() > 0 && PieceBase && Chosen)
        {
            TObjectPtr<UMaterialInstanceDynamic>& Tint = Tints.FindOrAdd(Entry.Value);
            if (!Tint) Tint = UMaterialInstanceDynamic::Create(Base->GetMaterial(0), this);
            Tint->SetTextureParameterValue(TEXT("BaseColorTexture"), Palette);
            const FLinearColor C = FLinearColor::FromSRGBColor(*Chosen), B = FLinearColor::FromSRGBColor(*PieceBase);
            Tint->SetVectorParameterValue(TEXT("BaseColorFactor"), FLinearColor(FMath::Clamp(C.R / FMath::Max(B.R, .02f), 0.f, 2.5f),
                FMath::Clamp(C.G / FMath::Max(B.G, .02f), 0.f, 2.5f), FMath::Clamp(C.B / FMath::Max(B.B, .02f), 0.f, 2.5f), 1.f));
            for (USkeletalMeshComponent* Part : Parts) if (Part && Part->GetFName() == Entry.Value) Part->SetMaterial(0, Tint);
        }
    }
    const bool bTunic = Look.Pieces.FindRef(TEXT("shirt")) == TEXT("shirt_tunic");
    SetPart(Base, TEXT("GearStrap"), bTunic ? FName(TEXT("gear_strap")) : NAME_None);
    SetPart(Base, TEXT("GearBelt"), bTunic ? FName(TEXT("gear_belt")) : NAME_None);
    // a malha base só anima (as peças copiam a pose)
    Base->SetVisibility(false, false);
    Base->SetCastShadow(false);
    Base->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
}

FVortexLook UVortexDressComponent::BanditLook(int32 Seed, bool bLeader)
{
    FRandomStream R(Seed * 7919 + 13);
    auto Pick = [&R](std::initializer_list<const TCHAR*> List) { const int32 I = R.RandRange(0, int32(List.size()) - 1); return *(List.begin() + I); };
    FVortexLook Look;
    const bool bFemale = R.FRand() < .3f;
    Look.Appearance = bFemale ? TEXT("female") : TEXT("male");
    Look.Pieces.Add(TEXT("hair"), FName(*FString::Printf(TEXT("hair_%s_%d"), bFemale ? TEXT("female") : TEXT("male"), R.RandRange(1, bFemale ? 9 : 10))));
    Look.Pieces.Add(TEXT("shirt"), bLeader ? FName(TEXT("shirt_mail")) : FName(Pick({ TEXT("shirt_shadow"), TEXT("shirt_scout"), TEXT("shirt_punk"), TEXT("shirt_tunic") })));
    Look.Pieces.Add(TEXT("pants"), FName(Pick({ TEXT("pants_dark"), TEXT("pants_leather") })));
    Look.Pieces.Add(TEXT("shoes"), FName(Pick({ TEXT("boots_shadow"), TEXT("boots_scout"), TEXT("boots_wrap"), TEXT("boots_punk") })));
    const TCHAR* Gear = Pick({ TEXT("accessory_patch"), TEXT(""), TEXT("accessory_patch"), TEXT("accessory_goggles") });
    if (*Gear) Look.Pieces.Add(TEXT("face_gear"), FName(Gear));
    auto Hex = [](const TCHAR* H) { return FColor::FromHex(H); };
    Look.Colors.Add(TEXT("skin_color"), Hex(Pick({ TEXT("#F1C6A1"), TEXT("#E2AE8D"), TEXT("#D8A07E"), TEXT("#C98E6A"), TEXT("#AA7558"), TEXT("#8A5A40"), TEXT("#EDC4A0"), TEXT("#6E4630") })));
    Look.Colors.Add(TEXT("hair_color"), Hex(Pick({ TEXT("#30201D"), TEXT("#784026"), TEXT("#B8612E"), TEXT("#2E211E"), TEXT("#5A3A26"), TEXT("#9A9A9A"), TEXT("#A0301E"), TEXT("#1E1A1C") })));
    Look.Colors.Add(TEXT("shirt_color"), Hex(bLeader ? Pick({ TEXT("#5A1E22"), TEXT("#3B1518") })
        : Pick({ TEXT("#3A302A"), TEXT("#2B2724"), TEXT("#4A3B2C"), TEXT("#35302E"), TEXT("#4E2E26"), TEXT("#2E3A2C") })));
    // as mangas na cor da camisa (sem isto ficavam no verde padrão do herói: parecia a túnica do aventureiro)
    Look.Colors.Add(TEXT("sleeve_color"), Look.Colors[TEXT("shirt_color")]);
    Look.Colors.Add(TEXT("pants_color"), Hex(Pick({ TEXT("#241F1C"), TEXT("#2F2822"), TEXT("#1E1C1E"), TEXT("#3A2E24") })));
    Look.Colors.Add(TEXT("strap_color"), Hex(TEXT("#2A2320")));
    Look.Colors.Add(TEXT("glove_color"), Hex(TEXT("#231D1A")));
    Look.Colors.Add(TEXT("boots_color"), Hex(TEXT("#2B221D")));
    return Look;
}

FVortexLook UVortexDressComponent::SkeletonLook(int32 Seed)
{
    FRandomStream R(Seed * 2909 + 13);
    FVortexLook Look = FVortexLookCatalog::Get().Random(TEXT("morto_vivo"), FName(TEXT("male")), R);
    static const TCHAR* Bones[] = { TEXT("#E8E2CE"), TEXT("#DCD4BC"), TEXT("#F0EAD8") };
    const FColor Bone = FColor::FromHex(Bones[R.RandRange(0, UE_ARRAY_COUNT(Bones) - 1)]);
    for (const TCHAR* Key : { TEXT("skin_color"), TEXT("shirt_color"), TEXT("sleeve_color"), TEXT("pants_color"), TEXT("boots_color"), TEXT("glove_color") })
        Look.Colors.Add(Key, Bone);
    Look.Colors.Add(TEXT("strap_color"), FColor::FromHex(TEXT("#3A3226")));
    return Look;
}

FVortexLook UVortexDressComponent::UndeadLook(int32 Seed)
{
    FRandomStream R(Seed * 4513 + 71);
    FVortexLook Look = FVortexLookCatalog::Get().Random(TEXT("morto_vivo"), R.FRand() < .3f ? FName(TEXT("female")) : FName(TEXT("male")), R);
    // trapos: camisa, mangas e calça em tons de terra e cinza apagados
    static const TCHAR* Rags[] = { TEXT("#3A3A32"), TEXT("#2E2A26"), TEXT("#3B3530"), TEXT("#2A2E2A"), TEXT("#40362E") };
    const FColor Shirt = FColor::FromHex(Rags[R.RandRange(0, UE_ARRAY_COUNT(Rags) - 1)]);
    Look.Colors.Add(TEXT("shirt_color"), Shirt);
    Look.Colors.Add(TEXT("sleeve_color"), Shirt);
    Look.Colors.Add(TEXT("pants_color"), FColor::FromHex(TEXT("#24221E")));
    return Look;
}