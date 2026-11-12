#include "Creatures/VortexNpc.h"

#include "Creatures/VortexBlockBody.h"

#include "Animation/VortexAnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/VortexCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "World/VortexTerrainStreamer.h"
#include "Creatures/VortexPet.h"
#include "Creatures/VortexSteering.h"
#include "Framework/VortexHairComponent.h"
#include "Framework/VortexLook.h"
#include "Animation/Skeleton.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture2D.h"
#include "Framework/VortexAudio.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "World/VortexTerrainData.h"

AVortexNpc::AVortexNpc()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;
    bReplicates = true;
    SetReplicateMovement(true);
    SetNetUpdateFrequency(10.f);
    // a população decide quando o ator existe; a distância de relevância só precisa cobrir isso
    SetNetCullDistanceSquared(FMath::Square(9000.f));
    GetCapsuleComponent()->InitCapsuleSize(AVortexCharacter::HeroCapsuleRadius, AVortexCharacter::HeroCapsuleHalfHeight);
    UCharacterMovementComponent* Movement = GetCharacterMovement();
    Movement->bRunPhysicsWithNoController = true;
    Movement->bOrientRotationToMovement = true;
    Movement->RotationRate = FRotator(0.f, 300.f, 0.f);
    Movement->MaxWalkSpeed = WalkSpeed;
    Movement->MaxStepHeight = 40.f;
    Movement->SetWalkableFloorAngle(45.f);
    bUseControllerRotationYaw = false;
    // a câmera de quem joga atravessa personagens (sem isto ela pulava para dentro do herói ao encostar)
    GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

    static ConstructorHelpers::FObjectFinder<USkeletalMesh> AdventurerMesh(
        TEXT("/Game/Vortex/Characters/Adventurer/Adventurer_Skinned/SkeletalMeshes/Adventurer_Skinned.Adventurer_Skinned"));
    if (AdventurerMesh.Succeeded())
    {
        GetMesh()->SetSkeletalMesh(AdventurerMesh.Object);
        GetMesh()->SetAnimInstanceClass(UVortexAnimInstance::StaticClass());
        GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -AVortexCharacter::HeroCapsuleHalfHeight));
        GetMesh()->SetRelativeRotation(FRotator(0.f, AVortexCharacter::MeshYawOffset, 0.f));
        GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
        GetMesh()->bEnableUpdateRateOptimizations = true;   // de longe anima menos vezes por segundo
    }
}

void AVortexNpc::InitNpc(FName Id, const FString& Name, const TArray<FVector2D>& InPatrol, int32 InPatrolIndex, float InRestYaw)
{
    NpcId = Id;
    DisplayName = Name;
    Patrol = InPatrol;
    PatrolIndex = Patrol.IsEmpty() ? 0 : FMath::Clamp(InPatrolIndex, 0, Patrol.Num() - 1);
    RestYaw = InRestYaw;
}

void AVortexNpc::InitLook(const FVortexLook& Look, FName InPet)
{
    NpcLook = AVortexCharacter::LookToNet(Look);
    PetKind = InPet;
}

void AVortexNpc::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(AVortexNpc, NpcId, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(AVortexNpc, DisplayName, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(AVortexNpc, NpcLook, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(AVortexNpc, PetKind, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(AVortexNpc, bChild, COND_InitialOnly);
    DOREPLIFETIME(AVortexNpc, Activity);
    DOREPLIFETIME(AVortexNpc, Gesture);
    DOREPLIFETIME(AVortexNpc, GestureSeq);
    DOREPLIFETIME(AVortexNpc, bSitting);
    DOREPLIFETIME(AVortexNpc, BubbleText);
    DOREPLIFETIME(AVortexNpc, bAsleep);
}

void AVortexNpc::InitRoutine(const FVector2D& InPost, float InPostYaw, bool bInChild, bool bInAtGoal)
{
    Post = InPost;
    PostYaw = InPostYaw;
    bChild = bInChild;
    bAtGoal = bInAtGoal;
}

// Uma peça do guarda-roupa (copia a pose da malha base, como no herói); vazia esconde.
void AVortexNpc::SetPart(FName PartName, FName PieceId)
{
    USkeletalMeshComponent* Part = nullptr;
    for (USkeletalMeshComponent* Candidate : Parts) if (Candidate && Candidate->GetFName() == PartName) Part = Candidate;
    if (!Part)
    {
        Part = NewObject<USkeletalMeshComponent>(this, PartName);
        Part->SetupAttachment(GetMesh());
        Part->SetLeaderPoseComponent(GetMesh());
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
        if (PieceMesh && GetMesh()->GetSkeletalMeshAsset() && PieceMesh->GetSkeleton() != GetMesh()->GetSkeletalMeshAsset()->GetSkeleton()) PieceMesh = nullptr;
    }
    Part->SetSkeletalMesh(PieceMesh);
    Part->SetVisibility(PieceMesh != nullptr);
    Part->SetMaterial(0, LookMaterial ? static_cast<UMaterialInterface*>(LookMaterial) : GetMesh()->GetMaterial(0));
}


// Veste o NPC: paleta com as cores dele, peças, peças de raça tingidas e o tamanho da raça (o mesmo
// jeito do AVortexCharacter::ApplyLookLocal).
void AVortexNpc::ApplyLook()
{
    const FVortexLookCatalog& Catalog = FVortexLookCatalog::Get();
    if (!Catalog.IsValid() || NpcLook.Race.IsNone() || !GetMesh()->GetSkeletalMeshAsset()) return;
    // o corpo de blocos do herói, quando a raça tem (o humano); senão o aventureiro com as peças, como antes
    if (VortexBlockBody::Apply(this, AVortexCharacter::LookFromNet(NpcLook)))
    {
        GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
        return;
    }
    const FVortexLook Look = Catalog.Normalized(AVortexCharacter::LookFromNet(NpcLook));
    if (!LookMaterial) LookMaterial = UMaterialInstanceDynamic::Create(GetMesh()->GetMaterial(0), this);
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
    SetPart(TEXT("Core"), TEXT("core"));
    SetPart(TEXT("Face"), Catalog.FacePiece(Look));
    for (const TPair<FName, FName>& Entry : PartOf)
    {
        const FName* Piece = Look.Pieces.Find(Entry.Key);
        SetPart(Entry.Value, Piece ? *Piece : NAME_None);
        const FVortexPieceInfo* Info = Piece ? Catalog.Piece(*Piece) : nullptr;
        const FName* Slot = SlotOf.Find(Entry.Key);
        const FColor* Base = Info && Slot ? Info->Defaults.Find(*Slot) : nullptr;
        const FColor* Chosen = Slot ? Look.Colors.Find(*Slot) : nullptr;
        if (Info && Info->Races.Num() > 0 && Base && Chosen)
        {
            TObjectPtr<UMaterialInstanceDynamic>& Tint = Tints.FindOrAdd(Entry.Value);
            if (!Tint) Tint = UMaterialInstanceDynamic::Create(GetMesh()->GetMaterial(0), this);
            Tint->SetTextureParameterValue(TEXT("BaseColorTexture"), Palette);
            const FLinearColor C = FLinearColor::FromSRGBColor(*Chosen), B = FLinearColor::FromSRGBColor(*Base);
            Tint->SetVectorParameterValue(TEXT("BaseColorFactor"), FLinearColor(FMath::Clamp(C.R / FMath::Max(B.R, .02f), 0.f, 2.5f),
                FMath::Clamp(C.G / FMath::Max(B.G, .02f), 0.f, 2.5f), FMath::Clamp(C.B / FMath::Max(B.B, .02f), 0.f, 2.5f), 1.f));
            for (USkeletalMeshComponent* Part : Parts) if (Part && Part->GetFName() == Entry.Value) Part->SetMaterial(0, Tint);
        }
    }
    const bool bTunic = Look.Pieces.FindRef(TEXT("shirt")) == TEXT("shirt_tunic");
    SetPart(TEXT("GearStrap"), bTunic ? FName(TEXT("gear_strap")) : NAME_None);
    SetPart(TEXT("GearBelt"), bTunic ? FName(TEXT("gear_belt")) : NAME_None);
    // a malha base só anima (as peças copiam a pose)
    GetMesh()->SetVisibility(false, false);
    GetMesh()->SetCastShadow(false);
    GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    if (const FVortexRaceInfo* Race = Catalog.Race(Look.Race))
    {
        GetMesh()->SetRelativeScale3D(FVector(Race->Shape.Width, Race->Shape.Width, Race->Shape.Height));
        if (UVortexAnimInstance* Anim = Cast<UVortexAnimInstance>(GetMesh()->GetAnimInstance())) Anim->Body = Race->Shape;
    }
}

void AVortexNpc::BeginPlay()
{
    Super::BeginPlay();
    // 08/11: as crianças da vila (os passantes pequenos): o ator inteiro menor, a cápsula junto
    if (bChild) SetActorScale3D(FVector(.72f));
    if (GetNetMode() != NM_DedicatedServer)
    {
        ApplyLook();
        // 08/11 (desempenho): com 38 moradores, quem está a mais de 70 m não é desenhado (a vila tem 130 m de lado)
        TArray<UPrimitiveComponent*> Shapes;
        GetComponents(Shapes);
        for (UPrimitiveComponent* Shape : Shapes) if (Shape != GetCapsuleComponent()) Shape->SetCullDistance(7000.f);
        // o bichinho é só visual: cada máquina tem o seu, seguindo o NPC
        if (PetKind == TEXT("dog"))
        {
            FActorSpawnParameters Params;
            Params.ObjectFlags |= RF_Transient;
            Pet = GetWorld()->SpawnActor<AVortexPet>(GetActorLocation() + GetActorRightVector() * 90.f - GetActorForwardVector() * 60.f, GetActorRotation(), Params);
            if (Pet.IsValid()) Pet->Follow(this);
        }
    }
    if (HasAuthority())
    {
        // o servidor só cria o ator com o chão pronto (UVortexPopulationSubsystem)
        SetActorRotation(FRotator(0.f, RestYaw, 0.f));
        // 08/11: com rotina sempre tica (anda pela vila); -VortexNoRoutines: só os postos, como antes
        static const bool bNoRoutines = FParse::Param(FCommandLine::Get(), TEXT("VortexNoRoutines"));
        const bool bRoutine = !bNoRoutines && VortexNpcRoutine::HasRoutine(NpcId);
        SetActorTickEnabled(bRoutine || !Patrol.IsEmpty());
        if (bRoutine && bAtGoal)
        {
            // a população já o criou onde a hora manda: chega fazendo o que tem que fazer
            VortexNpcRoutine::FGoal Now;
            if (VortexNpcRoutine::Plan(NpcId, Post, PostYaw, VortexNpcRoutine::HourOf(GetWorld()), WanderCount, Now))
            {
                Goal = Now;
                bHasGoal = true;
                if (!Now.bPatrol)
                {
                    if (Now.bFaceYaw) SetActorRotation(FRotator(0.f, Now.Yaw, 0.f));
                    Arrive();
                }
            }
        }
        bShown = true;
        return;
    }
    // cliente: o chão daqui pode chegar depois do ator; até lá, oculto e parado
    SetActorHiddenInGame(true);
    GetWorldTimerManager().SetTimer(ShowHandle, this, &AVortexNpc::TryShowLocally, .1f, true, 0.f);
}

void AVortexNpc::TryShowLocally()
{
    AVortexTerrainStreamer* Streamer = nullptr;
    if (TActorIterator<AVortexTerrainStreamer> It(GetWorld()); It) { Streamer = *It; }
    float Ground = 0.f;
    const FVector Here = GetActorLocation();
    if (Streamer && !Streamer->GroundReadyAt(Here.X, Here.Y, Ground)) return;
    GetWorldTimerManager().ClearTimer(ShowHandle);
    bShown = true;
    SetActorHiddenInGame(bAsleep);
    UE_LOG(LogTemp, Display, TEXT("VORTEX_NPC_VISIBLE id=%s net=%d"), *NpcId.ToString(), int32(GetNetMode()));
}

void AVortexNpc::EndPlay(const EEndPlayReason::Type Reason)
{
    if (Pet.IsValid()) Pet->Destroy();
    GetWorldTimerManager().ClearTimer(ShowHandle);
    if (!HasAuthority() && Reason == EEndPlayReason::Destroyed)
    {
        UE_LOG(LogTemp, Display, TEXT("VORTEX_NPC_GONE id=%s shown=%d"), *NpcId.ToString(), bShown ? 1 : 0);
    }
    if (bSitting || Goal.Seat >= 0) VortexNpcRoutine::ReleaseSeat(Goal.Seat, NpcId);
    Super::EndPlay(Reason);
}

// ---- 08/11: a rotina -------------------------------------------------------------------------------

void AVortexNpc::OnRep_Gesture()
{
    Say();
}

void AVortexNpc::OnRep_Asleep()
{
    SetActorHiddenInGame(bAsleep || !bShown);
    SetActorEnableCollision(!bAsleep);
    if (Pet.IsValid()) Pet->SetActorHiddenInGame(bAsleep);
}

void AVortexNpc::Say()
{
    // o "som de voz" curto (como Zelda e Animal Crossing): um murmúrio no tom do morador
    if (GetNetMode() != NM_DedicatedServer && !bAsleep)
        VortexAudio::Play(this, VortexNpcRoutine::Voice(NpcId, GestureSeq), GetActorLocation() + FVector(0.f, 0.f, 60.f), .55f);
}

void AVortexNpc::PlayGesture(FName Clip)
{
    Gesture = Clip;
    ++GestureSeq;
    Say();   // no servidor que também joga (o OnRep só roda nos outros)
}

void AVortexNpc::SetAsleep(bool bSleep)
{
    bAsleep = bSleep;
    UCharacterMovementComponent* Movement = GetCharacterMovement();
    if (bSleep) { Movement->StopMovementImmediately(); Movement->DisableMovement(); }
    else Movement->SetMovementMode(MOVE_Walking);
    OnRep_Asleep();
}

void AVortexNpc::StandUp()
{
    if (!bSitting) return;
    bSitting = false;
    VortexNpcRoutine::ReleaseSeat(Goal.Seat, NpcId);
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    if (const TSharedPtr<FVortexTerrainData> Terrain = AVortexTerrainStreamer::SharedData())
        SetActorLocation(FVector(Goal.Stand, Terrain->GroundAt(Goal.Stand.X, Goal.Stand.Y) + GetSimpleCollisionHalfHeight() + 20.f), false, nullptr, ETeleportType::TeleportPhysics);
}

bool AVortexNpc::CanChat() const
{
    return bHasGoal && bArrived && !bAsleep && !Goal.bPatrol && !Goal.bSleep && !ChatPartner.IsValid() && PauseLeft <= 0.f && PausedActivity.IsNone();
}

void AVortexNpc::JoinChat(AVortexNpc* Partner, bool bLead)
{
    ChatPartner = Partner;
    bChatLead = bLead;
    ChatSaved = Activity;
    if (!bSitting) Activity = TEXT("Npc_Conversar");
}

void AVortexNpc::EndChat()
{
    AVortexNpc* Other = ChatPartner.Get();
    ChatPartner = nullptr;
    BubbleText.Reset();
    if (!bSitting && bArrived && !ChatSaved.IsNone()) Activity = ChatSaved;
    ChatSaved = NAME_None;
    NextChatAt = GetWorld()->GetTimeSeconds() + FMath::FRandRange(35.f, 95.f);
    if (Other && Other->ChatPartner.Get() == this) Other->EndChat();
}

void AVortexNpc::TickChat(float DeltaSeconds)
{
    // 08/11: dois moradores parados perto um do outro conversam (as falas alternam em balões; só com alguém por perto para ver)
    if (AVortexNpc* Other = ChatPartner.Get())
    {
        if (!bSitting)
        {
            const FVector To = Other->GetActorLocation() - GetActorLocation();
            SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(0.f, To.Rotation().Yaw, 0.f), DeltaSeconds, 5.f));
        }
        if (!bChatLead) return;
        ChatNext -= DeltaSeconds;
        if (ChatNext > 0.f) return;
        if (!ChatLines.IsValidIndex(ChatIndex)) { EndChat(); return; }
        const FString& Line = ChatLines[ChatIndex];
        AVortexNpc* Speaker = ChatIndex % 2 ? Other : this;
        BubbleText.Reset();
        Other->BubbleText.Reset();
        Speaker->BubbleText = Line;
        Speaker->Gesture = NAME_None;
        ++Speaker->GestureSeq;
        Speaker->Say();
        ChatNext = 1.5f + Line.Len() * .06f;
        ++ChatIndex;
        return;
    }
    if (ChatPartner.IsStale()) ChatPartner = nullptr;
    const float Now = GetWorld()->GetTimeSeconds();
    if (Now < NextChatAt || !CanChat()) return;
    NextChatAt = Now + FMath::FRandRange(6.f, 14.f);   // sem par agora: tenta de novo daqui a pouco
    bool bWatched = false;
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        if (const APawn* Pawn = It->Get() ? It->Get()->GetPawn() : nullptr) bWatched |= FVector::DistSquared2D(Pawn->GetActorLocation(), GetActorLocation()) < FMath::Square(3500.f);
    if (!bWatched) return;
    AVortexNpc* Best = nullptr;
    float BestD = FMath::Square(650.f);
    for (TActorIterator<AVortexNpc> It(GetWorld()); It; ++It)
    {
        if (*It == this || !It->CanChat() || GetWorld()->GetTimeSeconds() < It->NextChatAt) continue;
        const float D = FVector::DistSquared2D(It->GetActorLocation(), GetActorLocation());
        if (D < BestD) { BestD = D; Best = *It; }
    }
    if (!Best) return;
    ChatLines = VortexNpcRoutine::Conversation(VortexNpcRoutine::HourOf(GetWorld()), FMath::Rand());
    if (ChatLines.IsEmpty()) return;
    ChatIndex = 0;
    ChatNext = 0.f;
    JoinChat(Best, true);
    Best->JoinChat(this, false);
}

void AVortexNpc::SetGoal(const VortexNpcRoutine::FGoal& Next)
{
    if (ChatPartner.IsValid()) EndChat();
    StandUp();
    if (Goal.Seat >= 0) VortexNpcRoutine::ReleaseSeat(Goal.Seat, NpcId);
    if (bAsleep && !Next.bSleep) SetAsleep(false);
    Goal = Next;
    bHasGoal = true;
    bArrived = false;
    GetCharacterMovement()->SetComponentTickEnabled(true);
    Activity = NAME_None;
    PausedActivity = NAME_None;
    Replans = 0;
    NoProgressFor = 0.f;
    ProgressFrom = FVector2D(GetActorLocation());
    WanderLeft = 40.f + float(HashCombineFast(GetTypeHash(NpcId), GetTypeHash(WanderCount)) % 50);
    // criança passeando corre um pouco; o resto anda no passo da praça
    GetCharacterMovement()->MaxWalkSpeed = bChild && Next.bWander ? 300.f : WalkSpeed;
    SetActorTickInterval(0.f);
    if (Next.bPatrol) return;
    const FVector2D Target = Next.bSit ? Next.Stand : Next.At;
    if (!VortexNpcRoutine::Path(FVector2D(GetActorLocation()), Target, Route)) Route = { Target };
    RouteIndex = 0;
    // longe (atravessar a vila): passo apressado, para chegar na hora
    float Length = 0.f;
    FVector2D From(GetActorLocation());
    for (const FVector2D& P : Route) { Length += FVector2D::Distance(From, P); From = P; }
    if (Length > 2500.f && !bChild) GetCharacterMovement()->MaxWalkSpeed = 240.f;
}

void AVortexNpc::Arrive()
{
    bArrived = true;
    Route.Reset();
    SetActorTickInterval(.1f);
    UCharacterMovementComponent* Movement = GetCharacterMovement();
    Movement->StopMovementImmediately();
    // 11/11 (auditoria de FPS): parado no posto, o movimento não precisa rodar a cada quadro (volta no SetGoal)
    Movement->SetComponentTickEnabled(false);
    if (Goal.bSleep) { SetAsleep(true); Activity = NAME_None; return; }
    if (Goal.bSit)
    {
        if (VortexNpcRoutine::ClaimSeat(Goal.Seat, NpcId))
        {
            // senta: no assento, de frente, sem empurrar o banco (a cápsula sem colisão enquanto sentado)
            bSitting = true;
            GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Movement->DisableMovement();
            if (const TSharedPtr<FVortexTerrainData> Terrain = AVortexTerrainStreamer::SharedData())
                SetActorLocationAndRotation(FVector(Goal.At, Terrain->GroundAt(Goal.At.X, Goal.At.Y) + GetSimpleCollisionHalfHeight() + 2.f), FRotator(0.f, Goal.Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
            Activity = TEXT("SentadoBanco");
            return;
        }
        // o banco já tem gente: fica em pé ali perto, conversando
        Goal.bSit = false;
        Goal.Seat = -1;
        Goal.bFaceYaw = false;
        Activity = TEXT("Npc_Conversar");
        return;
    }
    Activity = Goal.Anim;
}

void AVortexNpc::FollowRoute(float DeltaSeconds)
{
    if (RouteIndex >= Route.Num()) { Arrive(); return; }
    const FVector2D Here(GetActorLocation());
    const FVector2D Delta = Route[RouteIndex] - Here;
    const bool bLast = RouteIndex == Route.Num() - 1;
    if (Delta.Size() < (bLast ? 120.f : 70.f)) { ++RouteIndex; return; }
    // preso (outro morador, um caixote que a grade não viu): refaz o caminho; preso de novo várias vezes, aparece lá
    NoProgressFor += DeltaSeconds;
    if (FVector2D::Distance(Here, ProgressFrom) > 60.f) { NoProgressFor = 0.f; ProgressFrom = Here; }
    // perto do fim e parado (o lugar exato tem gente): já chegou
    if (bLast && Delta.Size() < 250.f && NoProgressFor > 1.5f) { Arrive(); return; }
    if (NoProgressFor > 1.6f)
    {
        NoProgressFor = 0.f;
        ProgressFrom = Here;
        const FVector2D Target = Route.Last();
        if (++Replans > 2)
        {
            if (const TSharedPtr<FVortexTerrainData> Terrain = AVortexTerrainStreamer::SharedData())
                SetActorLocation(FVector(Target, Terrain->GroundAt(Target.X, Target.Y) + GetSimpleCollisionHalfHeight() + 20.f), false, nullptr, ETeleportType::TeleportPhysics);
            Arrive();
            return;
        }
        if (!VortexNpcRoutine::Path(Here, Target, Route)) Route = { Target };
        RouteIndex = 0;
        return;
    }
    // o caminho já desvia das paredes (a grade): aqui só abre espaço entre os moradores (o desvio antigo empurrava para
    // fora do caminho nas portas e nos becos, e eles ficavam rodando)
    const FVector Want(Delta.GetSafeNormal(), 0.f);
    AddMovementInput((Want + VortexSteering::Separation(this, 110.f) * .3f).GetSafeNormal2D(), 1.f);
}

void AVortexNpc::React(float DeltaSeconds)
{
    // o herói chegou perto: olha para ele, para o serviço se ele encosta e cumprimenta (uma vez a cada 90 s)
    if (bAsleep || ChatPartner.IsValid()) return;
    const APawn* Closest = nullptr;
    float Best = 450.f;
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        if (const APawn* Pawn = It->Get() ? It->Get()->GetPawn() : nullptr)
        {
            const float D = FVector::Dist2D(Pawn->GetActorLocation(), GetActorLocation());
            if (D < Best) { Best = D; Closest = Pawn; }
        }
    const bool bStill = !bHasGoal || bArrived || (Goal.bPatrol && PauseLeft > 0.f);
    if (!Closest || !bStill || bSitting)
    {
        WatchFor = FMath::Max(0.f, WatchFor - DeltaSeconds);
        if (WatchFor <= 0.f && !PausedActivity.IsNone()) { Activity = PausedActivity; PausedActivity = NAME_None; }
        return;
    }
    WatchFor = 2.f;
    // 08/11: nem todos largam o serviço: um em cada três continua trabalhando e só olha
    if (Best < 300.f && !Activity.IsNone() && Activity != TEXT("Npc_Vigiar") && GetTypeHash(NpcId) % 3 != 0) { PausedActivity = Activity; Activity = NAME_None; }
    const FVector To = Closest->GetActorLocation() - GetActorLocation();
    SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(0.f, To.Rotation().Yaw, 0.f), DeltaSeconds, 4.f));
    const float Now = GetWorld()->GetTimeSeconds();
    if (Best < 400.f && Now - LastGreet > GreetGap)
    {
        // cada um reage do seu jeito (a lista por tipo está no npc_schedule.json, "gestos"); às vezes só olha
        LastGreet = Now;
        GreetGap = FMath::FRandRange(60.f, 140.f);
        const FName Clip = VortexNpcRoutine::GreetClip(NpcId, ++GreetCount);
        if (!Clip.IsNone()) PlayGesture(Clip);
    }
}

// Servidor, só com patrulha: anda até o próximo ponto, pausa de 2 a 5 s (estável por ID e ponto,
// como o sorteio do Godot) e segue. Sem patrulha o ator nem tica.
void AVortexNpc::TalkTo(AActor* Listener)
{
    if (!Listener || !HasAuthority()) return;
    if (ChatPartner.IsValid()) EndChat();
    TalkingWith = Listener;
    PauseLeft = FMath::Max(PauseLeft, 10.f);
    GetCharacterMovement()->StopMovementImmediately();
    const FVector To = (Listener->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
    if (!To.IsNearlyZero()) SetActorRotation(FRotator(0.f, To.Rotation().Yaw, 0.f));
}

void AVortexNpc::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!HasAuthority()) return;
    static const bool bNoRoutines = FParse::Param(FCommandLine::Get(), TEXT("VortexNoRoutines"));
    const bool bRoutine = !bNoRoutines && VortexNpcRoutine::HasRoutine(NpcId);
    if (bRoutine)
    {
        // a cada segundo: mudou o horário (ou acabou o tempo do passeio)? novo destino
        RoutineCheck -= DeltaSeconds;
        WanderLeft -= DeltaSeconds;
        if (RoutineCheck <= 0.f)
        {
            RoutineCheck = 1.f;
            const float Hour = VortexNpcRoutine::HourOf(GetWorld());
            VortexNpcRoutine::FGoal Next;
            if (VortexNpcRoutine::Plan(NpcId, Post, PostYaw, Hour, WanderCount, Next))
            {
                if (!bHasGoal || Next.Block != Goal.Block) SetGoal(Next);
                else if (Goal.bWander && bArrived && WanderLeft <= 0.f && VortexNpcRoutine::Plan(NpcId, Post, PostYaw, Hour, ++WanderCount, Next)) SetGoal(Next);
            }
        }
        React(DeltaSeconds);
        if (bAsleep) return;
        TickChat(DeltaSeconds);
        if (PauseLeft <= 0.f && bHasGoal && !Goal.bPatrol)
        {
            if (!bArrived) FollowRoute(DeltaSeconds);
            else if (!bSitting && Goal.bFaceYaw && WatchFor <= 0.f && !ChatPartner.IsValid())
                SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(0.f, Goal.Yaw, 0.f), DeltaSeconds, 3.f));
            return;
        }
    }
    if (Patrol.IsEmpty() && PauseLeft <= 0.f) return;
    if (PauseLeft > 0.f)
    {
        PauseLeft -= DeltaSeconds;
        // conversando: continua de frente para quem fala (e volta a andar quando ele se afasta)
        if (const AActor* Listener = TalkingWith.Get())
        {
            const FVector To = Listener->GetActorLocation() - GetActorLocation();
            if (To.Size2D() > 700.f) { TalkingWith = nullptr; PauseLeft = FMath::Min(PauseLeft, .5f); }
            else SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(0.f, To.Rotation().Yaw, 0.f), DeltaSeconds, 6.f));
        }
        return;
    }
    if (Patrol.IsEmpty()) return;
    const FVector2D Here(GetActorLocation());
    const FVector2D Delta = Patrol[PatrolIndex] - Here;
    // desvio de bancos, caixotes, árvores e outros moradores (VortexSteering); preso: um passo de
    // lado (a cada vez para um lado) e tenta de novo
    FVector Want(Delta.GetSafeNormal(), 0.f);
    FVector Forced;
    if (VortexSteering::Unstick(this, Want, DeltaSeconds, Steer, Forced))
    {
        AddMovementInput(Forced, 1.f);
        return;
    }
    if (Delta.Size() < 50.f)
    {
        PatrolIndex = (PatrolIndex + 1) % Patrol.Num();
        const uint32 Hash = HashCombineFast(GetTypeHash(NpcId), GetTypeHash(PatrolIndex));
        PauseLeft = 2.f + (Hash % 1000) / 999.f * 3.f;
        StuckFor = 0.f;
        return;
    }
    Want = VortexSteering::Avoid(this, Want, 160.f, Steer);
    AddMovementInput((Want + VortexSteering::Separation(this, 120.f) * .4f).GetSafeNormal2D(), 1.f);
}
