#include "World/VortexArsenal.h"

#include "AbilitySystemComponent.h"
#include "Animation/AnimSequence.h"
#include "Camera/CameraActor.h"
#include "Camera/PlayerCameraManager.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "Combat/VortexAttributes.h"
#include "Combat/VortexClassSkills.h"
#include "Combat/VortexSwords.h"
#include "Combat/VortexWeaponComponent.h"
#include "Combat/VortexWeaponDefinition.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/VortexCharacter.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Rpg/VortexArmor.h"
#include "Rpg/VortexHeroSheet.h"
#include "Rpg/VortexRpg.h"
#include "World/VortexAwakeningStadium.h"
#include "World/VortexItemDrop.h"

namespace
{
    const TCHAR* const Ranks[] = { TEXT("common"), TEXT("uncommon"), TEXT("rare"), TEXT("epic"), TEXT("legendary"), TEXT("unique"), TEXT("divine") };
    const TCHAR* const RankNames[] = { TEXT("Normal"), TEXT("Incomum"), TEXT("Raro"), TEXT("Épico"), TEXT("Lendário"), TEXT("Único"), TEXT("Divino") };

    int32 RankOf(const FString& Rarity)
    {
        for (int32 i = 0; i < UE_ARRAY_COUNT(Ranks); ++i) if (Rarity == Ranks[i]) return i;
        return 0;
    }

    FLinearColor RankColor(int32 Rank)
    {
        static const FLinearColor Colors[] = { FLinearColor(.72f, .72f, .72f), FLinearColor(.3f, .82f, .3f), FLinearColor(.25f, .5f, 1.f),
            FLinearColor(.68f, .32f, 1.f), FLinearColor(1.f, .58f, .12f), FLinearColor(1.f, .28f, .28f), FLinearColor(1.f, .88f, .45f) };
        return Colors[FMath::Clamp(Rank, 0, 6)];
    }

    UStaticMesh* Basic(const TCHAR* Name)
    {
        return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"), Name, Name));
    }

    /** As placas ficam de frente para quem vem do centro (do lugar onde o herói nasce). */
    FRotator FacingCenter(const FVector& Local)
    {
        return FRotator(0.f, Local.X >= 0.f ? 180.f : 0.f, 0.f);
    }
}

AVortexArsenal::AVortexArsenal()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = false;   // cada máquina monta a sua cópia (o modo é de um jogador só)
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

bool AVortexArsenal::IsArsenal(const UWorld* World)
{
    return World && World->GetMapName().Contains(TEXT("L_Arsenal"));
}

AVortexArsenal* AVortexArsenal::Find(const UWorld* World)
{
    if (!World) return nullptr;
    if (TActorIterator<AVortexArsenal> It(const_cast<UWorld*>(World)); It) return *It;
    return nullptr;
}

void AVortexArsenal::BeginPlay()
{
    Super::BeginPlay();
    Build();
}

void AVortexArsenal::AddBlock(const FVector& Center, const FVector& SizeCm, const FLinearColor& Color, bool bGround)
{
    // um lote por cor (as cores repetem: o chão, os pedestais de cada raridade)
    const FString Key = Color.ToString();
    UInstancedStaticMeshComponent* Ism = nullptr;
    for (UInstancedStaticMeshComponent* B : Blocks) if (B && B->ComponentTags.Contains(FName(*Key))) Ism = B;
    if (!Ism)
    {
        Ism = NewObject<UInstancedStaticMeshComponent>(this);
        Ism->SetupAttachment(RootComponent);
        Ism->SetStaticMesh(Basic(TEXT("Cube")));
        Ism->ComponentTags.Add(FName(*Key));
        Ism->SetCollisionProfileName(TEXT("BlockAll"));
        if (UMaterialInterface* Solid = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Combat/M_SolidColor.M_SolidColor")))
        {
            UMaterialInstanceDynamic* Look = UMaterialInstanceDynamic::Create(Solid, this);
            Look->SetVectorParameterValue(TEXT("Color"), Color);
            Ism->SetMaterial(0, Look);
        }
        Ism->RegisterComponent();
        Blocks.Add(Ism);
    }
    Ism->AddInstance(FTransform(FRotator::ZeroRotator, Center, SizeCm / 100.f));
    if (bGround) Ism->SetCastShadow(false);
}

UTextRenderComponent* AVortexArsenal::AddText(const FVector& Where, const FString& Text, float Size, const FColor& Color)
{
    UTextRenderComponent* T = NewObject<UTextRenderComponent>(this);
    T->SetupAttachment(RootComponent);
    T->SetRelativeLocationAndRotation(Where, FacingCenter(Where));
    T->SetHorizontalAlignment(EHTA_Center);
    T->SetVerticalAlignment(EVRTA_TextCenter);
    T->SetWorldSize(Size);
    T->SetTextRenderColor(Color);
    T->SetText(FText::FromString(Text));
    T->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    T->RegisterComponent();
    return T;
}

void AVortexArsenal::AddStand(const FVector& Where, const FString& Label, const FString& Rarity, int32 Slot, const TArray<FName>& Items, UStaticMesh* Mesh, float MeshScale)
{
    const int32 Rank = RankOf(Rarity);
    const FLinearColor Color = RankColor(Rank);
    AddBlock(Where + FVector(0, 0, 45), FVector(70, 70, 90), Color * .45f);
    AddBlock(Where + FVector(0, 0, 94), FVector(80, 80, 8), Color);
    FStand S;
    S.Where = GetActorLocation() + Where;
    S.Label = Label;
    S.Slot = Slot;
    S.Items = Items;
    if (Mesh)
    {
        S.Show = NewObject<UStaticMeshComponent>(this);
        S.Show->SetupAttachment(RootComponent);
        S.Show->SetStaticMesh(Mesh);
        S.Show->SetRelativeLocation(Where + FVector(0, 0, 165));
        S.Show->SetRelativeScale3D(FVector(MeshScale));
        S.Show->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        S.Show->RegisterComponent();
    }
    S.Text = AddText(Where + FVector(0, 0, 255), Rarity.IsEmpty() ? Label : FString::Printf(TEXT("%s\n%s"), *Label, RankNames[Rank]), 15.f, Color.ToFColor(true));
    Stands.Add(S);
}

void AVortexArsenal::AddMannequin(const FVector& Where, const FString& Label, const FString& Rarity, const TArray<FName>& Pieces)
{
    const int32 Rank = RankOf(Rarity);
    const FLinearColor Color = RankColor(Rank);
    AddBlock(Where + FVector(0, 0, 10), FVector(90, 90, 20), Color);
    // o manequim: o corpo do herói de blocos parado, com as peças do conjunto presas no mesmo esqueleto
    USkeletalMeshComponent* Body = NewObject<USkeletalMeshComponent>(this);
    Body->SetupAttachment(RootComponent);
    Body->SetSkeletalMesh(LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Vortex/Characters/Heroes/humano/Hero_humano.Hero_humano")));
    Body->SetRelativeLocationAndRotation(Where + FVector(0, 0, 20), FRotator(0.f, Where.X >= 0.f ? 90.f : -90.f, 0.f));
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Body->RegisterComponent();
    if (UAnimSequence* Idle = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Vortex/Characters/Heroes/Anims/humano/MX_humano_Idle.MX_humano_Idle")))
        Body->PlayAnimation(Idle, true);
    for (const FName& Item : Pieces)
    {
        const VortexArmor::FPiece* Piece = VortexArmor::VisualPiece(Item);
        USkeletalMesh* Mesh = Piece ? LoadObject<USkeletalMesh>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Characters/Heroes/humano/Armaduras/%s.%s"), *Piece->Mesh, *Piece->Mesh)) : nullptr;
        if (!Mesh) continue;
        USkeletalMeshComponent* Part = NewObject<USkeletalMeshComponent>(this);
        Part->SetupAttachment(Body);
        Part->SetSkeletalMesh(Mesh);
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->bUseBoundsFromLeaderPoseComponent = true;
        Part->RegisterComponent();
        Part->SetLeaderPoseComponent(Body);
    }
    FStand S;
    S.Where = GetActorLocation() + Where;
    S.Label = Label;
    S.Slot = -1;
    S.Items = Pieces;
    S.Text = AddText(Where + FVector(0, 0, 250), FString::Printf(TEXT("%s\n%s"), *Label, RankNames[Rank]), 15.f, Color.ToFColor(true));
    Stands.Add(S);
}

void AVortexArsenal::Build()
{
    const FVortexRpgCatalog& Rpg = FVortexRpgCatalog::Get();
    // o chão: placas de 2 m em xadrez (dá a noção de distância), um pouco abaixo do zero
    for (int32 x = -7; x < 30; ++x)
        for (int32 y = -12; y < 12; ++y)
            AddBlock(FVector(x * 200.f + 100.f, y * 200.f + 100.f, -10.f), FVector(200, 200, 20),
                (x + y) % 2 ? FLinearColor(.3f, .52f, .24f) : FLinearColor(.26f, .47f, .21f), true);

    // ---- espadas e arcos da tabela: uma grade por raridade (fileiras) com 5 em cada
    for (int32 Bows = 0; Bows < 2; ++Bows)
    {
        TArray<TPair<int32, const FVortexItemDef*>> List;
        for (const TPair<FName, FVortexItemDef>& It : Rpg.Items)
        {
            const VortexSwords::FSword* Sword = VortexSwords::Find(It.Key);
            if (Sword && Sword->Weapon == (Bows ? TEXT("arco_de_betula") : TEXT("espada_de_cinzas"))) List.Add({ RankOf(It.Value.Rarity), &It.Value });
        }
        List.StableSort([](const auto& A, const auto& B) { return A.Key < B.Key; });
        TMap<int32, int32> InRow;
        for (const TPair<int32, const FVortexItemDef*>& Entry : List)
        {
            const int32 Col = InRow.FindOrAdd(Entry.Key)++;
            const FVector At(500.f + Entry.Key * 270.f, (Bows ? 760.f : -1440.f) + Col * 170.f, 0.f);
            const VortexSwords::FSword* Sword = VortexSwords::Find(Entry.Value->Id);
            UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Weapons/Models/%s.%s"), *Sword->Model, *Sword->Model));
            AddStand(At, Entry.Value->Name, Entry.Value->Rarity, VortexRpg::Weapon, { Entry.Value->Id }, Mesh, 1.f);
        }
        AddText(FVector(300.f, Bows ? 1100.f : -1100.f, 400.f), Bows ? TEXT("ARCOS") : TEXT("ESPADAS"), 60.f, FColor(255, 240, 200));
    }

    // ---- as outras armas: uma de cada tipo (a de menor raridade do catálogo)
    TMap<FName, const FVortexItemDef*> ByType;
    for (const TPair<FName, FVortexItemDef>& It : Rpg.Items)
    {
        if (VortexRpg::EquipSlotOf(It.Value.Slot) != VortexRpg::Weapon) continue;
        const FName Type = Rpg.WeaponType(It.Key);
        if (Type.IsNone() || Type == TEXT("espada_de_cinzas") || Type == TEXT("arco_de_betula")) continue;
        const FVortexItemDef*& Best = ByType.FindOrAdd(Type);
        if (!Best || RankOf(It.Value.Rarity) < RankOf(Best->Rarity) || It.Key == Type) Best = &It.Value;
    }
    int32 Index = 0;
    for (const TPair<FName, const FVortexItemDef*>& It : ByType)
    {
        const UVortexWeaponDefinition* Def = UVortexWeaponComponent::LoadType(It.Key);
        const FVector At(2750.f + (Index / 12) * 260.f, -1100.f + (Index % 12) * 200.f, 0.f);
        AddStand(At, Def && !Def->DisplayName.IsEmpty() ? Def->DisplayName : It.Value->Name, FString(), VortexRpg::Weapon, { It.Value->Id },
            Def ? Def->VisualMesh.Get() : nullptr, 1.f);
        ++Index;
    }
    AddText(FVector(2500.f, 0.f, 400.f), TEXT("OUTRAS ARMAS"), 60.f, FColor(255, 240, 200));

    // ---- as armaduras: os conjuntos com malha nova, vestidos em manequins (atrás de onde o herói nasce)
    TArray<TPair<int32, const FVortexItemSet*>> Sets;
    for (const TPair<FName, FVortexItemSet>& It : Rpg.Sets)
    {
        const FVortexItemDef* Head = Rpg.Item(FName(*FString::Printf(TEXT("arm_%s_head"), *It.Key.ToString())));
        if (Head && VortexArmor::Piece(Head->Id)) Sets.Add({ RankOf(Head->Rarity), &It.Value });
    }
    Sets.StableSort([](const auto& A, const auto& B) { return A.Key < B.Key; });
    Index = 0;
    for (const TPair<int32, const FVortexItemSet*>& Set : Sets)
    {
        TArray<FName> Pieces;
        for (const TCHAR* Slot : { TEXT("head"), TEXT("chest"), TEXT("legs"), TEXT("boots") })
        {
            const FName Piece(*FString::Printf(TEXT("arm_%s_%s"), *Set.Value->Id.ToString(), Slot));
            if (Rpg.Item(Piece)) Pieces.Add(Piece);
        }
        const FVector At(-650.f - (Index / 11) * 420.f, -1000.f + (Index % 11) * 200.f, 0.f);
        AddMannequin(At, Set.Value->Name.IsEmpty() ? Set.Value->Id.ToString() : Set.Value->Name, Ranks[Set.Key], Pieces);
        ++Index;
    }
    AddStand(FVector(-650.f - (Index / 11) * 420.f, -1000.f + (Index % 11) * 200.f, 0.f), TEXT("Tirar a armadura"), FString(), -2, {}, nullptr, 1.f);
    AddText(FVector(-420.f, 0.f, 430.f), TEXT("ARMADURAS"), 60.f, FColor(255, 240, 200));

    // ---- o campo de treino: a linha de tiro e os bonecos a 5, 10 e 20 m dela
    AddBlock(FVector(3500.f, 0.f, 1.f), FVector(30, 1400, 4), FLinearColor(.95f, .95f, .9f), true);
    AddText(FVector(3450.f, 0.f, 380.f), TEXT("CAMPO DE TREINO"), 55.f, FColor(255, 240, 200));
    for (const float Meters : { 5.f, 10.f, 20.f })
    {
        AddText(FVector(3500.f + Meters * 100.f, -560.f, 60.f), FString::Printf(TEXT("%.0f m"), Meters), 40.f, FColor::White);
        AddBlock(FVector(3500.f + Meters * 100.f, 0.f, 1.f), FVector(12, 1000, 3), FLinearColor(.8f, .8f, .75f), true);
        if (HasAuthority())
            for (const float Side : { -300.f, 0.f, 300.f })
            {
                FActorSpawnParameters Params;
                Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                GetWorld()->SpawnActor<AVortexTrainingDummy>(GetActorLocation() + FVector(3500.f + Meters * 100.f, Side, 95.f), FRotator(0.f, 180.f, 0.f), Params);
            }
    }

    // ---- a placa de ajuda onde o herói nasce e o aviso do pedestal perto
    AddText(FVector(260.f, 0.f, 230.f), TEXT("ARSENAL\nE perto de um pedestal: equipar\nMana infinita e poder sem recarga\nEsc: voltar ao menu"), 18.f, FColor::White);
    // 07/11: o nível do herói e o das armas e armaduras (para testar a trava e o dano de cada nível)
    AddStand(FVector(200.f, -520.f, 0.f), TEXT("Herói -10"), FString(), -3, {}, nullptr, 1.f);
    AddStand(FVector(200.f, -360.f, 0.f), TEXT("Herói +10"), FString(), -4, {}, nullptr, 1.f);
    AddStand(FVector(200.f, 360.f, 0.f), TEXT("Armas -10"), FString(), -5, {}, nullptr, 1.f);
    AddStand(FVector(200.f, 520.f, 0.f), TEXT("Armas +10"), FString(), -6, {}, nullptr, 1.f);
    Status = AddText(FVector(260.f, 0.f, 330.f), FString(), 22.f, FColor(255, 230, 120));
    Prompt = AddText(FVector::ZeroVector, FString(), 20.f, FColor(255, 230, 120));
    Prompt->SetVisibility(false);
    UE_LOG(LogTemp, Display, TEXT("VORTEX_ARSENAL pedestais=%d"), Stands.Num());
}

void AVortexArsenal::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const float Spin = GetWorld()->GetTimeSeconds() * 60.f;
    for (FStand& S : Stands)
        if (S.Show) S.Show->SetRelativeRotation(FRotator(0.f, Spin, 0.f));

    // o pedestal mais perto do herói daqui: o aviso "E" em cima dele, virado para a câmera
    APlayerController* Pc = GetWorld()->GetFirstPlayerController();
    const APawn* Pawn = Pc ? Pc->GetPawn() : nullptr;
    Near = INDEX_NONE;
    float Best = 230.f;
    if (Pawn)
        for (int32 i = 0; i < Stands.Num(); ++i)
        {
            const float D = FVector::Dist2D(Stands[i].Where, Pawn->GetActorLocation());
            if (D < Best) { Best = D; Near = i; }
        }
    if (Prompt)
    {
        Prompt->SetVisibility(Near != INDEX_NONE);
        if (Near != INDEX_NONE)
        {
            const FStand& S = Stands[Near];
            Prompt->SetWorldLocation(S.Where + FVector(0, 0, 320));
            if (Pc->PlayerCameraManager)
                Prompt->SetWorldRotation(FRotator(0.f, (Pc->PlayerCameraManager->GetCameraLocation() - S.Where).Rotation().Yaw, 0.f));
            Prompt->SetText(FText::FromString(S.Slot <= -3 ? TEXT("[E] mudar") : S.Slot == -2 ? TEXT("[E] tirar tudo") : S.Slot == -1 ? TEXT("[E] vestir o conjunto") : TEXT("[E] equipar")));
        }
    }
    if (Status)
    {
        const AVortexCharacter* Mine = Cast<AVortexCharacter>(Pawn);
        const int32 HeroLevel = Mine && Mine->GetSheet() ? Mine->GetSheet()->GetLevel() : 1;
        Status->SetText(FText::FromString(FString::Printf(TEXT("Herói: nível %d     Armas e armaduras: %s"), HeroLevel,
            GearLevel > 0 ? *FString::Printf(TEXT("nível %d"), GearLevel) : TEXT("nível natural da raridade"))));
    }

#if !UE_BUILD_SHIPPING
    // -VortexArsenalShots: a conferência (Saved/Screenshots/arsenal_*.png): a vista de cima, uma espada, um arco e uma
    // armadura equipados pelo E, e sai
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexArsenalShots")) && Pc)
    {
        static int32 Step = 0;
        static float Clock = 0.f;
        static TWeakObjectPtr<AActor> Cam;
        Clock += DeltaSeconds;
        AVortexCharacter* Hero = Cast<AVortexCharacter>(Pc->GetPawn());
        auto Shot = [](const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / FString::Printf(TEXT("Screenshots/arsenal_%s.png"), Name), false, false); };
        auto Look = [&](const FVector& From, const FVector& At)
        {
            if (!Cam.IsValid()) { Cam = GetWorld()->SpawnActor<ACameraActor>(); Pc->SetViewTargetWithBlend(Cam.Get(), 0.f); }
            Cam->SetActorLocationAndRotation(From, (At - From).Rotation());
        };
        auto UseAt = [&](int32 Index)
        {
            if (!Hero || !Stands.IsValidIndex(Index)) return;
            Hero->SetActorLocation(Stands[Index].Where + FVector(-120.f, 0.f, 100.f), false, nullptr, ETeleportType::TeleportPhysics);
            TryUse(Hero);
        };
        auto FindStand = [&](const FString& Part) { return Stands.IndexOfByPredicate([&Part](const FStand& S) { return S.Label.Contains(Part); }); };
        const FVector Base = GetActorLocation();
        if (Step == 0 && Clock > 5.f && Hero) { Look(Base + FVector(-1600.f, -2600.f, 1500.f), Base + FVector(1800.f, 0.f, 0.f)); Step = 1; Clock = 0.f; }
        else if (Step == 1 && Clock > 1.f) { Shot(TEXT("vista")); Step = 2; Clock = 0.f; }
        else if (Step == 2 && Clock > .5f) { Look(Base + FVector(-200.f, -1100.f, 450.f), Base + FVector(1300.f, -1100.f, 100.f)); Step = 3; Clock = 0.f; }
        else if (Step == 3 && Clock > 1.f) { Shot(TEXT("espadas")); Look(Base + FVector(300.f, 1300.f, 400.f), Base + FVector(-900.f, 0.f, 100.f)); Step = 4; Clock = 0.f; }
        else if (Step == 4 && Clock > 1.f)
        {
            // a trava: armas no nível 40 com o herói no 1 (recusa) e depois no 40 (veste)
            Shot(TEXT("armaduras"));
            for (int32 k = 0; k < 4; ++k) UseAt(FindStand(TEXT("Armas +10")));
            UseAt(FindStand(TEXT("Arco das Estrelas")));
            for (int32 k = 0; k < 4; ++k) UseAt(FindStand(TEXT("Herói +10")));
            UseAt(FindStand(TEXT("Arco das Estrelas")));
            Step = 5; Clock = 0.f;
        }
        else if (Step == 5 && Clock > 1.f && Hero) { Look(Hero->GetActorLocation() + Hero->GetActorRightVector() * 220.f + FVector(80.f, 0.f, 40.f), Hero->GetActorLocation()); Step = 6; Clock = 0.f; }
        else if (Step == 6 && Clock > .5f) { Shot(TEXT("espada_equipada")); UseAt(Stands.IndexOfByPredicate([](const FStand& S) { return S.Slot == -1 && S.Label.Contains(TEXT("Drag")); })); Step = 7; Clock = 0.f; }
        else if (Step == 7 && Clock > 1.f && Hero) { Look(Hero->GetActorLocation() + Hero->GetActorRightVector() * 220.f + FVector(80.f, 0.f, 40.f), Hero->GetActorLocation()); Step = 8; Clock = 0.f; }
        else if (Step == 8 && Clock > .5f)
        {
            // o saque no chão: uma espada de nível 40, um capacete e uma poção, perto de onde o herói nasce
            Shot(TEXT("armadura_vestida"));
            AVortexItemDrop::Spawn(GetWorld(), FVortexRpgCatalog::LevelledId(TEXT("asa_da_fenix"), 40), 1, Base + FVector(150.f, -150.f, 80.f), .3f);
            AVortexItemDrop::Spawn(GetWorld(), TEXT("arm_dragao_rubro_head"), 1, Base + FVector(150.f, 0.f, 80.f), .3f);
            AVortexItemDrop::Spawn(GetWorld(), TEXT("potion_red"), 3, Base + FVector(150.f, 150.f, 80.f), .3f);
            if (Hero) Hero->SetActorLocation(Base + FVector(-150.f, 0.f, 100.f), false, nullptr, ETeleportType::TeleportPhysics);
            Look(Base + FVector(-200.f, -380.f, 260.f), Base + FVector(160.f, 0.f, 60.f));
            Step = 81; Clock = 0.f;
        }
        else if (Step == 81 && Clock > 2.f)
        {
            Shot(TEXT("chao"));
            if (Hero && Hero->GetSheet())
                if (AVortexItemDrop* Drop = AVortexItemDrop::FindNear(GetWorld(), Base + FVector(150.f, 0.f, 0.f), 400.f))
                {
                    Hero->SetActorLocation(Drop->GetActorLocation() + FVector(0.f, 0.f, 60.f), false, nullptr, ETeleportType::TeleportPhysics);
                    const FName Got = Drop->GetItem();
                    Hero->GetSheet()->ServerPickup_Implementation(Drop);
                    UE_LOG(LogTemp, Display, TEXT("VORTEX_ARSENAL pegou %s mochila=%d"), *Got.ToString(), Hero->GetSheet()->CountOf(Got));
                }
            // a forja do Bram: herói no 50, o arco (nível 40) sobe 1 nível com moedas e fragmentos de colosso
            if (Hero && Hero->GetSheet())
            {
                UVortexHeroSheet* Sh = Hero->GetSheet();
                UseAt(FindStand(TEXT("Herói +10")));
                Sh->bShopAnywhere = true;
                Sh->AddCoins(20000);
                Sh->GiveItem(TEXT("colossus_shard"), 5, false);
                const FName Before = Sh->GetEquipped(VortexRpg::Weapon);
                const int32 Coins = Sh->GetCoins();
                Sh->ServerForge_Implementation(uint8(VortexRpg::Weapon));
                UE_LOG(LogTemp, Display, TEXT("VORTEX_ARSENAL forja %s -> %s custou %d moedas e %d fragmentos"), *Before.ToString(),
                    *Sh->GetEquipped(VortexRpg::Weapon).ToString(), Coins - Sh->GetCoins(), 5 - Sh->CountOf(TEXT("colossus_shard")));
            }
            Step = 20; Clock = 0.f;
        }
        else if (Step >= 20 && Step < 28)
        {
            // os poderes de classe: o Serafim da Aurora e o Necromante, os 4 de cada, de lado no campo de treino
            static const TCHAR* const Classes[] = { TEXT("serafim_da_aurora"), TEXT("necromancer") };
            const int32 K = Step - 20, Slot = K % 4;
            if (Clock > 0.f && Clock - DeltaSeconds <= 0.f && Hero && Hero->GetSheet() && Hero->GetClassSkills())
            {
                Hero->GetSheet()->DebugSetClass(Classes[K / 4]);
                Hero->SetActorLocation(Base + FVector(3200.f, 0.f, 100.f), false, nullptr, ETeleportType::TeleportPhysics);
                Hero->SetActorRotation(FRotator::ZeroRotator);
                Pc->SetControlRotation(FRotator(-10.f, 0.f, 0.f));
                Look(Base + FVector(3150.f, -700.f, 260.f), Base + FVector(3550.f, 0.f, 80.f));
            }
            if (Clock > .2f && Clock - DeltaSeconds <= .2f && Hero && Hero->GetClassSkills()) Hero->GetClassSkills()->Press(Slot);
            const FString Name = FString::Printf(TEXT("classe_%s_%d"), Classes[K / 4], Slot);
            if (Clock > .75f && Clock - DeltaSeconds <= .75f) Shot(*(Name + TEXT("a")));
            if (Clock > 1.4f && Clock - DeltaSeconds <= 1.4f) Shot(*(Name + TEXT("b")));
            if (Clock > 2.6f) { Step = K == 7 ? 9 : Step + 1; Clock = 0.f; if (Step == 9) UE_LOG(LogTemp, Display, TEXT("VORTEX_ARSENAL_SHOTS fim")); }
        }
        else if (Step == 9 && Clock > 1.f) { FPlatformMisc::RequestExit(false); Step = 10; }
    }
#endif

    // mana infinita e poder sem recarga (fora do meio do poder e com as armas arremessadas de volta na mão)
    if (HasAuthority())
        for (TActorIterator<AVortexCharacter> It(GetWorld()); It; ++It)
        {
            if (UAbilitySystemComponent* ASC = It->GetAbilitySystemComponent())
                ASC->SetNumericAttributeBase(UVortexAttributes::GetManaAttribute(), ASC->GetNumericAttribute(UVortexAttributes::GetMaxManaAttribute()));
            if (UVortexWeaponComponent* Weapon = It->GetWeapon(); Weapon && !Weapon->IsSpecialActive() && Weapon->GetThrowMask() == 0) Weapon->DebugResetSpecial();
        }
}

bool AVortexArsenal::TryUse(AVortexCharacter* Hero)
{
    UVortexHeroSheet* Sheet = Hero ? Hero->GetSheet() : nullptr;
    if (!Sheet || !Hero->HasAuthority()) return false;
    int32 Pick = INDEX_NONE;
    float Best = 230.f;
    for (int32 i = 0; i < Stands.Num(); ++i)
    {
        const float D = FVector::Dist2D(Stands[i].Where, Hero->GetActorLocation());
        if (D < Best) { Best = D; Pick = i; }
    }
    if (Pick == INDEX_NONE) return false;
    const FStand& S = Stands[Pick];
    // veste direto no lugar (como o pedestal do Estádio), sem passar pela mochila: a ficha daqui não grava
    // (IsArsenal); o Equipped é privado da ficha, então pela reflexão (UPROPERTY) em vez de mexer no cabeçalho dela
    FArrayProperty* Prop = FindFProperty<FArrayProperty>(UVortexHeroSheet::StaticClass(), TEXT("Equipped"));
    TArray<FName>* Equipped = Prop ? Prop->ContainerPtrToValuePtr<TArray<FName>>(Sheet) : nullptr;
    if (!Equipped) return false;
    auto Put = [Equipped](int32 Slot, FName Item) { if (Equipped->IsValidIndex(Slot)) (*Equipped)[Slot] = Item; };
    const FVortexRpgCatalog& Rpg = FVortexRpgCatalog::Get();
    if (S.Slot <= -3)
    {
        if (S.Slot >= -4)
        {
            // o nível do herói direto na ficha (a ficha daqui não grava), pela reflexão como o Equipped
            const int32 Want = FMath::Clamp((Sheet->GetLevel() / 10) * 10 + (S.Slot == -4 ? 10 : -10), 1, 100);
            if (FInt64Property* XpProp = FindFProperty<FInt64Property>(UVortexHeroSheet::StaticClass(), TEXT("Xp")))
                XpProp->SetPropertyValue_InContainer(Sheet, FVortexRpgCatalog::XpForLevel(Want));
            if (FIntProperty* LevelProp = FindFProperty<FIntProperty>(UVortexHeroSheet::StaticClass(), TEXT("Level")))
                LevelProp->SetPropertyValue_InContainer(Sheet, Want);
            Sheet->RefreshVitals();
            Sheet->PushNotice(FString::Printf(TEXT("Herói no nível %d"), Want));
        }
        else
        {
            GearLevel = FMath::Clamp((GearLevel / 10) * 10 + (S.Slot == -6 ? 10 : -10), 0, 100);
            Sheet->PushNotice(GearLevel > 0 ? FString::Printf(TEXT("Armas e armaduras no nível %d"), GearLevel) : FString(TEXT("Armas e armaduras no nível natural")));
        }
        return true;
    }
    if (S.Slot == -2)
    {
        for (int32 Slot : { int32(VortexRpg::Head), int32(VortexRpg::Chest), int32(VortexRpg::Legs), int32(VortexRpg::Boots) }) Put(Slot, NAME_None);
        Sheet->RefreshVitals();
        Sheet->PushNotice(TEXT("Armadura tirada"));
        return true;
    }
    // 07/11: no nível escolhido; o item trava se o herói não tem o nível dele (como no jogo)
    for (const FName& Base : S.Items)
    {
        const FName Item = GearLevel > 0 ? FVortexRpgCatalog::LevelledId(Base, GearLevel) : Base;
        const FVortexItemDef* Def = Rpg.Item(Item);
        if (!Def) continue;
        if (Def->ReqLevel > Sheet->GetLevel())
        {
            Sheet->PushNotice(FString::Printf(TEXT("%s: precisa do nível %d (você está no %d)"), *Def->Name, Def->ReqLevel, Sheet->GetLevel()), 1);
            UE_LOG(LogTemp, Display, TEXT("VORTEX_ARSENAL travado %s nivel=%d heroi=%d"), *Item.ToString(), Def->ReqLevel, Sheet->GetLevel());
            return true;
        }
        Put(VortexRpg::EquipSlotOf(Def->Slot), Item);
    }
    Sheet->RefreshVitals();   // Changed(): a arma na mão (o tipo novo), as peças no corpo e os números
    if (S.Slot == VortexRpg::Weapon && Hero->GetWeapon()) Hero->GetWeapon()->RefreshVisual();
    Sheet->PushNotice(FString::Printf(TEXT("Equipado: %s"), *S.Label));
    UE_LOG(LogTemp, Display, TEXT("VORTEX_ARSENAL equipado %s (%s) ataque=%.0f"), *S.Label, *Sheet->GetEquipped(S.Slot == -1 ? VortexRpg::Chest : VortexRpg::Weapon).ToString(),
        Rpg.Item(Sheet->GetEquipped(VortexRpg::Weapon)) ? Rpg.Item(Sheet->GetEquipped(VortexRpg::Weapon))->Stats.FindRef(TEXT("attack")) : 0.f);
    return true;
}
