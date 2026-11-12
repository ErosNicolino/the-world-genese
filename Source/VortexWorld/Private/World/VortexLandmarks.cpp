#include "World/VortexLandmarks.h"
#include "World/VortexWorldHooks.h"
#include "World/VortexWorldLayout.h"

#include "Framework/VortexProbe.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "World/VortexTerrainData.h"
#include "World/VortexTerrainStreamer.h"

namespace
{
    enum : int32 { Yggdrasil = 0, Dunes, Hunters, DuneDen, GroveDen, FirstMarvel };
    constexpr int32 RootKeyBase = 200000;

    /** 07/11: os lugares marcantes e as raízes da Árvore do Mundo (Tools/World/StampMarvels.py escreve em
     *  world_features.json; o relevo já vem carimbado no mapa). Posições em cm, Godot (x, z) = Unreal (X, Y). */
    struct FMarvelProp { FString Prop; FVector2D At; float Yaw = 0.f; };
    struct FMarvel { FString Id, Name, Kind; FVector2D At; TArray<FMarvelProp> Props; };
    struct FWorldRoot { bool bArch = false; FVector2D At; float Yaw = 0.f, Scale = 1.f; int32 Variant = 0; };
    struct FMarvelData { TArray<FMarvel> Marvels; TArray<FWorldRoot> Roots; };

    const FMarvelData& MarvelData()
    {
        static const FMarvelData Loaded = []
        {
            FMarvelData Out;
            FString Text;
            TSharedPtr<FJsonObject> Json;
            if (!FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir() / TEXT("Data/GodotWorld/world_features.json"))) ||
                !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json) || !Json.IsValid()) return Out;
            auto At = [](const TSharedPtr<FJsonObject>& O)
            {
                const TArray<TSharedPtr<FJsonValue>>* A = nullptr;
                return O->TryGetArrayField(TEXT("at"), A) && A->Num() >= 2 ? FVector2D((*A)[0]->AsNumber(), (*A)[1]->AsNumber()) * 100.f : FVector2D::ZeroVector;
            };
            const TArray<TSharedPtr<FJsonValue>>* List = nullptr;
            if (Json->TryGetArrayField(TEXT("marvels"), List))
                for (const TSharedPtr<FJsonValue>& Value : *List)
                {
                    const TSharedPtr<FJsonObject> O = Value->AsObject();
                    FMarvel& M = Out.Marvels.AddDefaulted_GetRef();
                    M.Id = O->GetStringField(TEXT("id")); M.Name = O->GetStringField(TEXT("name")); M.Kind = O->GetStringField(TEXT("kind"));
                    M.At = At(O);
                    const TArray<TSharedPtr<FJsonValue>>* Props = nullptr;
                    if (O->TryGetArrayField(TEXT("props"), Props))
                        for (const TSharedPtr<FJsonValue>& P : *Props)
                            M.Props.Add({ P->AsObject()->GetStringField(TEXT("prop")), At(P->AsObject()), float(P->AsObject()->GetNumberField(TEXT("yaw"))) });
                }
            if (Json->TryGetArrayField(TEXT("roots"), List))
                for (const TSharedPtr<FJsonValue>& Value : *List)
                {
                    const TSharedPtr<FJsonObject> O = Value->AsObject();
                    FWorldRoot& R = Out.Roots.AddDefaulted_GetRef();
                    R.bArch = O->GetStringField(TEXT("kind")) == TEXT("root_arch");
                    R.At = At(O);
                    R.Yaw = O->GetNumberField(TEXT("yaw"));
                    O->TryGetNumberField(TEXT("scale"), R.Scale);
                    O->TryGetNumberField(TEXT("variant"), R.Variant);
                }
            return Out;
        }();
        return Loaded;
    }

    /** Uma peça de Tools/Blender/MakeMarvels.py (/Game/Vortex/World/Marvels), a própria malha como colisão se bSolid. */
    UStaticMeshComponent* MarvelPiece(AActor* Owner, const FString& Name, const FVector& At, float Yaw, const FVector& Scale, bool bSolid)
    {
        UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Vortex/World/Marvels/SM_%s.SM_%s"), *Name, *Name));
        if (!Mesh) { UE_LOG(LogTemp, Warning, TEXT("VORTEX_MARVEL sem a peça %s"), *Name); return nullptr; }
        UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(Owner);
        Part->SetStaticMesh(Mesh);
        Part->SetCollisionEnabled(bSolid ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
        if (bSolid) Part->SetCollisionProfileName(TEXT("BlockAll"));
        Part->SetCanEverAffectNavigation(false);
        Part->SetupAttachment(Owner->GetRootComponent());
        Part->SetWorldLocationAndRotation(At, FRotator(0.f, Yaw, 0.f));
        Part->SetWorldScale3D(Scale);
        Part->RegisterComponent();
        return Part;
    }

    /** O lombo de uma raiz (a curva do MakeMarvels.py: bézier P0-P1-P2 em m, raio R0 nas pontas e R1 no meio) em caixas
     *  inclinadas: os degraus de blocos da malha são altos demais para subir andando. */
    void RootWalk(AActor* Owner, const FVortexTerrainData& Data, const FVector& Origin, float Yaw, float Scale,
        const FVector& P0, const FVector& P1, const FVector& P2, float R0, float R1, TArray<TObjectPtr<USceneComponent>>& Out)
    {
        const FQuat Turn = FRotator(0.f, Yaw, 0.f).Quaternion();
        auto Top = [&](float T)
        {
            const FVector P = (1.f - T) * (1.f - T) * P0 + 2.f * (1.f - T) * T * P1 + T * T * P2;
            const float R = R0 + (R1 - R0) * (1.f - FMath::Abs(2.f * T - 1.f));
            return Origin + Turn.RotateVector((P + FVector(0.f, 0.f, R * 1.05f)) * 100.f * Scale);
        };
        constexpr int32 Steps = 14;
        for (int32 i = 0; i < Steps; ++i)
        {
            const FVector A = Top(float(i) / Steps), B = Top(float(i + 1) / Steps);
            if (A.Z < Data.GroundAt(A.X, A.Y) - 30.f && B.Z < Data.GroundAt(B.X, B.Y) - 30.f) continue;   // ainda debaixo da terra
            const FVector Dir = B - A;
            const FQuat Rot = FRotationMatrix::MakeFromXZ(Dir, FVector::UpVector).ToQuat();
            UBoxComponent* Box = NewObject<UBoxComponent>(Owner);
            Box->SetMobility(EComponentMobility::Static);
            Box->SetupAttachment(Owner->GetRootComponent());
            Box->SetBoxExtent(FVector(Dir.Size() * .5f + 25.f, FMath::Lerp(R0, R1, .5f) * 60.f * Scale, 15.f));
            Box->SetWorldLocationAndRotation((A + B) * .5f - Rot.GetUpVector() * 15.f, Rot);
            Box->SetCollisionProfileName(TEXT("BlockAll"));
            Box->SetCanEverAffectNavigation(false);
            Box->RegisterComponent();
            Out.Add(Box);
        }
    }

    FLinearColor LandmarkHex(const TCHAR* Code, float Shift = 0.f)
    {
        FLinearColor C(FColor::FromHex(Code));
        C = FLinearColor(FMath::Clamp(C.R + Shift, 0.f, 1.f), FMath::Clamp(C.G + Shift, 0.f, 1.f), FMath::Clamp(C.B + Shift, 0.f, 1.f));
        return FLinearColor::FromSRGBColor(C.ToFColor(false));
    }

    /** Blocos soltos no mundo (Godot, metros) numa malha só, com a cor no vértice. */
    struct FLandmarkBoxes
    {
        TArray<FVector> Vertices, Normals;
        TArray<int32> Triangles;
        TArray<FLinearColor> Colors;
        void Box(const FVector& CenterM, const FVector& SizeM, const FLinearColor& Color)
        {
            // Godot (x, y para cima, z) -> Unreal (x, z, y) em cm
            const FVector C(CenterM.X * 100.f, CenterM.Z * 100.f, CenterM.Y * 100.f);
            const FVector H(SizeM.X * 50.f, SizeM.Z * 50.f, SizeM.Y * 50.f);
            static const FVector N[6] = { FVector(-1, 0, 0), FVector(1, 0, 0), FVector(0, -1, 0), FVector(0, 1, 0), FVector(0, 0, -1), FVector(0, 0, 1) };
            for (const FVector& Normal : N)
            {
                // dois eixos da face e os quatro cantos, na ordem da frente da Unreal
                const FVector U = FMath::Abs(Normal.Z) > .5f ? FVector(1, 0, 0) : FVector(0, 0, 1);
                const FVector V = FVector::CrossProduct(Normal, U);
                const FVector Mid = C + Normal * H;
                const FVector Du = U * H, Dv = V * H;
                const int32 Base = Vertices.Num();
                for (const FVector& P : { Mid - Du - Dv, Mid + Du - Dv, Mid + Du + Dv, Mid - Du + Dv }) { Vertices.Add(P); Normals.Add(Normal); Colors.Add(Color); }
                const bool bFlip = FVector::DotProduct(FVector::CrossProduct(Vertices[Base + 1] - Vertices[Base], Vertices[Base + 2] - Vertices[Base]), Normal) > 0.f;
                if (bFlip) Triangles.Append({ Base, Base + 2, Base + 1, Base, Base + 3, Base + 2 });
                else Triangles.Append({ Base, Base + 1, Base + 2, Base, Base + 2, Base + 3 });
            }
        }
    };
}

AVortexSmokeColumn::AVortexSmokeColumn()
{
    PrimaryActorTick.bCanEverTick = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
    // pontes e cabanas não se movem: com as caixas de colisão estáticas, o herói em cima delas não vira
    // "base móvel" na rede (o cliente não acha as caixas, que cada máquina cria)
    GetRootComponent()->SetMobility(EComponentMobility::Static);
}

void AVortexSmokeColumn::Setup(float InWidth, float InHeight, int32 Count, const FLinearColor& Tint, float InOpacity, float Life)
{
    Width = InWidth; Height = InHeight; Opacity = InOpacity; PuffLife = Life;
    UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Sky/M_SmokePuff.M_SmokePuff"));
    for (int32 i = 0; i < Count; ++i)
    {
        FPuff Puff;
        Puff.Ball = NewObject<UStaticMeshComponent>(this);
        Puff.Ball->SetupAttachment(RootComponent);
        Puff.Ball->SetStaticMesh(Sphere);
        Puff.Ball->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Puff.Ball->SetCastShadow(false);
        Puff.Ball->SetUsingAbsoluteScale(true);
        Puff.Ball->RegisterComponent();
        if (Base)
        {
            Puff.Look = UMaterialInstanceDynamic::Create(Base, this);
            Puff.Look->SetVectorParameterValue(TEXT("Tint"), Tint * FMath::FRandRange(.8f, 1.15f));
            Puff.Ball->SetMaterial(0, Puff.Look);
        }
        Puff.Age = PuffLife * i / Count;   // já começa como uma coluna inteira
        Puff.Drift = FVector(FMath::FRandRange(-.3f, .3f), FMath::FRandRange(-.3f, .3f), 0.f);
        Puffs.Add(Puff);
    }
}

void AVortexSmokeColumn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    // 07/11: as ~60 chaminés da vila moviam ~800 baforadas por quadro (o FPS caiu pela metade na rua). Longe da
    // câmera (mais de 120 m, ou 8 vezes a altura da coluna) a coluna some e confere só duas vezes por segundo
    if (const APlayerController* PC = GetWorld()->GetFirstPlayerController(); PC && PC->PlayerCameraManager && !Puffs.IsEmpty())
    {
        const bool bFar = FVector::Dist(PC->PlayerCameraManager->GetCameraLocation(), GetActorLocation()) > FMath::Max(12000.f, Height * 8.f);
        if (bFar == Puffs[0].Ball->IsVisible())
        {
            for (FPuff& Puff : Puffs) if (Puff.Ball) Puff.Ball->SetVisibility(!bFar);
            SetActorTickInterval(bFar ? .5f : 0.f);
        }
        if (bFar) return;
    }
    const FVector Wind(.55f, .2f, 0.f);   // a coluna se inclina com o vento
    for (FPuff& Puff : Puffs)
    {
        Puff.Age += DeltaSeconds;
        if (Puff.Age > PuffLife)
        {
            Puff.Age -= PuffLife;
            Puff.Drift = FVector(FMath::FRandRange(-.3f, .3f), FMath::FRandRange(-.3f, .3f), 0.f);
        }
        const float U = Puff.Age / PuffLife;
        const float Rise = Height * U;
        const FVector At = GetActorLocation() + FVector(0.f, 0.f, Rise) + (Wind * U * U + Puff.Drift * U) * Height * .5f;
        const float Size = Width * (.35f + U * 1.1f) / 100.f;
        if (Puff.Ball) { Puff.Ball->SetWorldLocation(At); Puff.Ball->SetWorldScale3D(FVector(Size, Size, Size * .8f)); }
        if (Puff.Look) Puff.Look->SetScalarParameterValue(TEXT("Fade"), Opacity * FMath::Clamp(U * 6.f, 0.f, 1.f) * (1.f - U));
    }
}

const TArray<AVortexLandmarks::FSite>& AVortexLandmarks::Sites()
{
    static const TArray<FSite> List = []
    {
        TArray<FSite> Out = {
            { TEXT("yggdrasil"), TEXT("Yggdrasil"), FVector2D(2910.f, 390.f) * 100.f, 12800.f },
            { TEXT("dunas"), TEXT("Acampamento das Dunas"), FVector2D(-2188.f, -1006.f) * 100.f, 3200.f },
            { TEXT("cacadores"), TEXT("Vila dos Caçadores"), FVector2D(2870.f, 614.f) * 100.f, 3200.f },
            { TEXT("seryth"), TEXT("Seryth · dunas de vidro"), FVector2D(-2180.f, -1090.f) * 100.f, 2400.f },
            { TEXT("thorun"), TEXT("Thorun · bosque antigo"), FVector2D(2960.f, 705.f) * 100.f, 2400.f },
        };
        // 07/11: os lugares marcantes (a partir de FirstMarvel): o raio é a clareira e onde o nome aparece na tela
        for (const FMarvel& M : MarvelData().Marvels)
        {
            const float Radius = M.Kind == TEXT("lake") ? 13000.f : M.Kind == TEXT("waterfall") || M.Kind == TEXT("crystals") ? 9000.f : 6000.f;
            Out.Add({ M.Id, M.Name, M.At, Radius });
        }
        return Out;
    }();
    return List;
}

AVortexLandmarks::AVortexLandmarks()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = .5f;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
    // as caixas de colisão são fixas: a raiz também (antes ~570 avisos "AttachTo ... is not static" por teste)
    GetRootComponent()->SetMobility(EComponentMobility::Static);
}

void AVortexLandmarks::BeginPlay()
{
    Super::BeginPlay();
    // as peças: as da cidade (town.json) e as élficas (elven_props.json, copiado do Godot)
    Models = MakeShared<FJsonObject>();
    FString Text;
    TSharedPtr<FJsonObject> Json;
    if (FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir() / TEXT("Data/GodotTown/town.json"))) &&
        FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json) && Json.IsValid())
        for (const TPair<FString, TSharedPtr<FJsonValue>> Pair : Json->GetObjectField(TEXT("models"))->Values) Models->SetField(Pair.Key, Pair.Value);
    if (FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir() / TEXT("Data/GodotWorld/elven_props.json"))) &&
        FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json) && Json.IsValid())
        for (const TPair<FString, TSharedPtr<FJsonValue>> Pair : Json->Values) Models->SetField(Pair.Key, Pair.Value);
    // a árvore do mundo fica sempre (é o marco do horizonte)
    BuildSite(Yggdrasil);
    // o vulcão de Brasafenda fumega (coluna grossa saindo da cratera, vista de longe)
    const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
    if (Data && Data->HasWorld() && GetNetMode() != NM_DedicatedServer)
    {
        const FVector2D Crater(-120000.f, -333000.f);
        FActorSpawnParameters Params;
        Params.ObjectFlags |= RF_Transient;
        if (AVortexSmokeColumn* Column = GetWorld()->SpawnActor<AVortexSmokeColumn>(FVector(Crater, Data->QuickGround(Crater.X, Crater.Y) + 300.f), FRotator::ZeroRotator, Params))
            Column->Setup(9000.f, 60000.f, 22, FLinearColor(.16f, .14f, .14f), .55f, 40.f);
    }
}

void AVortexLandmarks::Tick(float DeltaSeconds)
{
    VORTEX_PROBE("Marcos");
    Super::Tick(DeltaSeconds);
    // a miragem só acende longe (perto ela seria uma casca transparente cobrindo a tela)
    if (const APlayerController* Local = GetWorld()->GetFirstPlayerController(); Local && Local->PlayerCameraManager && Mirage.Num())
    {
        const bool bFar = FVector::Dist2D(Local->PlayerCameraManager->GetCameraLocation(), FVector(Sites()[Yggdrasil].Center, 0.f)) > 70000.f;
        for (UStaticMeshComponent* Ghost : Mirage) if (Ghost && Ghost->IsVisible() != bFar) Ghost->SetVisibility(bFar);
    }
    TArray<FVector2D> Players;
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        if (const APawn* Pawn = It->Get() ? It->Get()->GetPawn() : nullptr) Players.Add(FVector2D(Pawn->GetActorLocation()));
    for (int32 i = 1; i < Sites().Num(); ++i)
    {
        float Nearest = 1e12f;
        for (const FVector2D& P : Players) Nearest = FMath::Min(Nearest, FVector2D::Distance(P, Sites()[i].Center));
        // os lugares marcantes são vistos de longe: montam a 1,5 km
        const bool bMarvel = i >= FirstMarvel;
        if (!Parts.Contains(i) && Nearest < (bMarvel ? 150000.f : 40000.f)) BuildSite(i);
        else if (Parts.Contains(i) && Nearest > (bMarvel ? 170000.f : 50000.f)) DropSite(i);
    }
    // 07/11: as raízes da Árvore do Mundo, montadas a 600 m
    const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
    const TArray<FWorldRoot>& Roots = MarvelData().Roots;
    for (int32 r = 0; r < Roots.Num() && Data && Data->HasWorld(); ++r)
    {
        float Nearest = 1e12f;
        for (const FVector2D& P : Players) Nearest = FMath::Min(Nearest, FVector2D::Distance(P, Roots[r].At));
        const int32 Key = RootKeyBase + r;
        if (Parts.Contains(Key)) { if (Nearest > 70000.f) DropSite(Key); continue; }
        if (Nearest > 60000.f) continue;
        const FWorldRoot& R = Roots[r];
        TArray<TObjectPtr<USceneComponent>>& List = Parts.Add(Key);
        if (R.bArch)
        {
            // atravessa o rio: o meio sobre a água, as pontas entrando nas margens; anda-se por cima
            const FVector Origin(R.At, Data->WaterSurface + 40.f);
            if (UStaticMeshComponent* Part = MarvelPiece(this, TEXT("root_arch"), Origin, R.Yaw, FVector(R.Scale), false)) List.Add(Part);
            RootWalk(this, *Data, Origin, R.Yaw, R.Scale, FVector(-18, 2, -4), FVector(0, -3, 6), FVector(18, -2, -4), 2.8f, 2.f, List);
        }
        else if (UStaticMeshComponent* Part = MarvelPiece(this, FString::Printf(TEXT("root_emerge_%d"), FMath::Clamp(R.Variant, 0, 2)),
            FVector(R.At, Data->GroundAt(R.At.X, R.At.Y) - 40.f * R.Scale), R.Yaw, FVector(R.Scale), true))
            List.Add(Part);
    }
    UpdateCamps(Players);
}

void AVortexLandmarks::UpdateCamps(const TArray<FVector2D>& Players)
{
    const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
    if (!Data || !Data->HasWorld()) return;
    // monta a 200 m, desmonta a 300 m (os bandidos nascem perto do jogador; a fogueira já está lá antes)
    constexpr float Cell = VortexWorldLayout::BanditCellSize;
    TSet<FIntPoint> Wanted;
    for (const FVector2D& P : Players)
    {
        const FIntPoint Home(FMath::FloorToInt(P.X / Cell), FMath::FloorToInt(P.Y / Cell));
        for (int32 DX = -3; DX <= 3; ++DX) for (int32 DY = -3; DY <= 3; ++DY)
        {
            const FIntPoint Key = Home + FIntPoint(DX, DY);
            FVector2D At;
            if (!VortexWorldLayout::BanditCampAt(Key, At)) continue;
            const float Distance = FVector2D::Distance(At, P);
            if (Distance < 30000.f) Wanted.Add(Key);
            if (Distance < 20000.f && !Camps.Contains(Key)) BuildCamp(Key, At);
        }
    }
    TArray<FIntPoint> Drop;
    for (const TPair<FIntPoint, int32>& Camp : Camps) if (!Wanted.Contains(Camp.Key)) Drop.Add(Camp.Key);
    for (const FIntPoint& Key : Drop)
    {
        DropSite(Camps[Key]);
        Camps.Remove(Key);
        VortexWorldHooks::RemoveChest(GetWorld(), CampChestId(Key));
    }
}

void AVortexLandmarks::BuildCamp(const FIntPoint& CellKey, const FVector2D& At)
{
    const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
    if (!Data) return;
    const int32 Key = NextCampKey++;
    Camps.Add(CellKey, Key);
    Parts.FindOrAdd(Key);
    auto Ground = [&](float X, float Z) { return Data->GroundAt(X * 100.f, Z * 100.f) / 100.f; };
    FRandomStream Rng(HashCombine(GetTypeHash(CellKey.X * 31), GetTypeHash(CellKey.Y + 7)));
    const float X = At.X / 100.f, Z = At.Y / 100.f, Turn = Rng.FRandRange(0.f, 360.f);
    auto Around = [&](float Distance, float Angle) { const float A = FMath::DegreesToRadians(Turn + Angle); return FVector2D(X + FMath::Cos(A) * Distance, Z + FMath::Sin(A) * Distance); };
    // a fogueira (o mesmo desenho das vilas): pedras em roda, lenha cruzada e brasa
    FLandmarkBoxes Stones;
    const float Y = Ground(X, Z);
    for (int32 i = 0; i < 9; ++i)
    {
        const float A = i * UE_TWO_PI / 9.f;
        Stones.Box(FVector(X + FMath::Cos(A) * .7f, Y + .1f, Z + FMath::Sin(A) * .7f), FVector(.3f, .22f, .3f), LandmarkHex(i % 2 == 0 ? TEXT("6d6a64") : TEXT("84807a")));
    }
    Stones.Box(FVector(X, Y + .12f, Z), FVector(.9f, .14f, .16f), LandmarkHex(TEXT("5a3a22")));
    Stones.Box(FVector(X, Y + .2f, Z), FVector(.16f, .14f, .9f), LandmarkHex(TEXT("4a2e1a")));
    Stones.Box(FVector(X, Y + .06f, Z), FVector(.8f, .06f, .8f), LandmarkHex(TEXT("2a1c14")));
    // troncos de assento em volta do fogo
    for (int32 i = 0; i < 3; ++i)
    {
        const FVector2D L = Around(2.4f, 40.f + i * 120.f);
        const bool bAlong = i % 2 == 0;
        Stones.Box(FVector(L.X, Ground(L.X, L.Y) + .2f, L.Y), bAlong ? FVector(1.6f, .4f, .45f) : FVector(.45f, .4f, 1.6f), LandmarkHex(i == 1 ? TEXT("6a4a2e") : TEXT("5c3f26")));
    }
    if (Stones.Vertices.Num())
    {
        UProceduralMeshComponent* Mesh = NewObject<UProceduralMeshComponent>(this);
        Mesh->SetupAttachment(RootComponent);
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->RegisterComponent();
        Mesh->CreateMeshSection_LinearColor(0, Stones.Vertices, Stones.Triangles, Stones.Normals, {}, Stones.Colors, {}, false);
        if (UMaterialInterface* Painted = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Materials/M_FarTerrain.M_FarTerrain"))) Mesh->SetMaterial(0, Painted);
        Parts[Key].Add(Mesh);
    }
    // caixotes do saque e a carroça roubada
    const FVector2D Crates = Around(3.6f, 200.f), Crates2 = Around(4.2f, 240.f), Wagon = Around(6.f, 300.f);
    Place(Key, TEXT("vil_crates"), Crates.X, Crates.Y, Turn + 25.f);
    Place(Key, TEXT("vil_crates"), Crates2.X, Crates2.Y, Turn + 140.f);
    if (Rng.FRand() < .6f) Place(Key, TEXT("vil_wagon"), Wagon.X, Wagon.Y, Turn + 90.f);
    // o baú do saque, entre os caixotes, de frente para a fogueira: só abre com o bando vencido
    // (AVortexWorldFeatures::GuardsAlive); aberto fica aberto no save de cada herói, como os das cabanas
    {
        const FVector2D Spot = Around(3.f, 222.f);
        const FVector Floor(Spot.X * 100.f, Spot.Y * 100.f, Ground(Spot.X, Spot.Y) * 100.f);
        // virado para o fogo: o baú olha para -Z do nó (Godot); o giro em Y leva -Z até a direção do centro
        const FVector2D ToFire = FVector2D(X - Spot.X, Z - Spot.Y).GetSafeNormal();
        const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(-ToFire.X, -ToFire.Y));
        VortexWorldHooks::AddCampChest(GetWorld(), CampChestId(CellKey), Floor, Yaw, Data->RegionAt(At.X, At.Y), Rng.FRand() < .25f);
    }
    if (GetNetMode() != NM_DedicatedServer)
        if (UNiagaraSystem* Fire = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Vefects/Free_Fire/Shared/Particles/NS_Fire_Small.NS_Fire_Small")))
            if (UNiagaraComponent* Flame = UNiagaraFunctionLibrary::SpawnSystemAttached(Fire, RootComponent, NAME_None, FVector(X * 100.f, Z * 100.f, Y * 100.f + 15.f),
                FRotator::ZeroRotator, FVector(.6f), EAttachLocation::KeepWorldPosition, true, ENCPoolMethod::None, true))
                Parts[Key].Add(Flame);
    UE_LOG(LogTemp, Display, TEXT("VORTEX_BANDIT_CAMP montado em (%.0f, %.0f)"), At.X, At.Y);
}

void AVortexLandmarks::DropSite(int32 Site)
{
    if (TArray<TObjectPtr<USceneComponent>>* List = Parts.Find(Site))
        for (USceneComponent* Part : *List) if (Part) Part->DestroyComponent();
    Parts.Remove(Site);
}

void AVortexLandmarks::Place(int32 Site, const FString& Model, float X, float Z, float Yaw)
{
    const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
    const TSharedPtr<FJsonObject>* Info = nullptr;
    UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Props/%s/%s/StaticMeshes/%s.%s"), *Model, *Model, *Model, *Model));
    if (!Data || !Mesh || !Models->TryGetObjectField(Model, Info)) { UE_LOG(LogTemp, Warning, TEXT("VORTEX_LANDMARK peça ausente %s"), *Model); return; }
    TArray<TObjectPtr<USceneComponent>>& List = Parts.FindOrAdd(Site);
    const FRotator Rotation(0.f, -Yaw, 0.f);
    const FVector Center(X * 100.f, Z * 100.f, 0.f);
    auto Local = [](double Lx, double Ly, double Lz) { return FVector(Lx * 100.0, Lz * 100.0, Ly * 100.0); };
    // assenta no ponto mais baixo do alicerce (PlaceModel do Godot)
    float Low = 1e9f, High = -1e9f;
    const TArray<TSharedPtr<FJsonValue>>* Foot = nullptr;
    if ((*Info)->TryGetArrayField(TEXT("footing"), Foot) && Foot->Num() == 4)
    {
        const double F[4] = { (*Foot)[0]->AsNumber(), (*Foot)[1]->AsNumber(), (*Foot)[2]->AsNumber(), (*Foot)[3]->AsNumber() };
        for (const FVector2D C : { FVector2D(F[0], F[1]), FVector2D(F[2], F[1]), FVector2D(F[0], F[3]), FVector2D(F[2], F[3]), FVector2D((F[0] + F[2]) * .5, (F[1] + F[3]) * .5) })
        {
            const FVector W = Center + Rotation.RotateVector(Local(C.X, 0.0, C.Y));
            const float G = Data->GroundAt(W.X, W.Y);
            Low = FMath::Min(Low, G); High = FMath::Max(High, G);
        }
    }
    else Low = High = Data->GroundAt(Center.X, Center.Y);
    const FVector Base(Center.X, Center.Y, FMath::Min(Low + 12.f, High));
    UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this);
    Part->SetStaticMesh(Mesh);
    Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Part->SetupAttachment(RootComponent);
    Part->SetWorldLocationAndRotation(Base, Rotation);
    Part->RegisterComponent();
    List.Add(Part);
    const TArray<TSharedPtr<FJsonValue>>* Boxes = nullptr;
    if ((*Info)->TryGetArrayField(TEXT("boxes"), Boxes))
        for (const TSharedPtr<FJsonValue>& Value : *Boxes)
        {
            const TArray<TSharedPtr<FJsonValue>>& B = Value->AsArray();
            if (B.Num() < 6) continue;
            UBoxComponent* Box = NewObject<UBoxComponent>(this);
            Box->SetMobility(EComponentMobility::Static);
            Box->SetupAttachment(RootComponent);
            Box->SetBoxExtent(FVector(B[3]->AsNumber() * 50.0, B[5]->AsNumber() * 50.0, B[4]->AsNumber() * 50.0));
            Box->SetWorldLocationAndRotation(Base + Rotation.RotateVector(Local(B[0]->AsNumber(), B[1]->AsNumber(), B[2]->AsNumber())), Rotation);
            Box->SetCollisionProfileName(TEXT("BlockAll"));
            Box->SetCanEverAffectNavigation(false);
            Box->RegisterComponent();
            List.Add(Box);
        }
}

void AVortexLandmarks::BuildSite(int32 Site)
{
    const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData();
    if (!Data || !Data->HasWorld()) return;
    Parts.FindOrAdd(Site);
    if (Site >= FirstMarvel)
    {
        // 07/11: um lugar marcante (o relevo já está no mapa; aqui as peças por cima)
        if (!MarvelData().Marvels.IsValidIndex(Site - FirstMarvel)) return;
        TArray<TObjectPtr<USceneComponent>>& List = Parts[Site];
        auto Add = [&](UStaticMeshComponent* Part) { if (Part) List.Add(Part); };
        for (const FMarvelProp& P : MarvelData().Marvels[Site - FirstMarvel].Props)
        {
            const float A = FMath::DegreesToRadians(P.Yaw);
            const FVector2D U(FMath::Cos(A), FMath::Sin(A));      // o eixo do lugar: para a cidade
            const float Floor = Data->GroundAt(P.At.X, P.At.Y);
            if (P.Prop == TEXT("waterfall"))
            {
                // do alto do paredão até a lâmina do lago, esticada; a espuma onde a água bate. O alto é o ponto mais alto
                // até 30 m para trás e a borda o primeiro ponto (de quem vem do lago) quase nessa altura: o lago carimbado
                // comeu a frente do paredão, e cada ponto do mapa tem 6 m (a queda é uma rampa)
                float Top = -1e9f;
                for (int32 Step = 0; Step <= 60; ++Step) { const FVector2D Q = P.At - U * (Step * 50.f); Top = FMath::Max(Top, Data->GroundAt(Q.X, Q.Y)); }
                FVector2D Edge = P.At;
                for (int32 Step = 0; Step <= 60; ++Step) { Edge = P.At - U * (Step * 50.f); if (Data->GroundAt(Edge.X, Edge.Y) >= Top - 300.f) break; }
                Edge -= U * 150.f;   // a água é grossa (Y x4, ~11 m para o lago): cobre a rampa da queda
                Top += 30.f;
                Add(MarvelPiece(this, TEXT("waterfall"), FVector(Edge, Top), P.Yaw - 90.f, FVector(1.6f, 4.f, FMath::Max(1.f, (Top - Data->WaterSurface) / 1050.f)), false));
                Add(MarvelPiece(this, TEXT("waterfall_foam"), FVector(Edge + U * 900.f, Data->WaterSurface - 20.f), P.Yaw + 90.f, FVector(1.3f), false));
            }
            else if (P.Prop == TEXT("rootbridge"))
            {
                // o lombo da raiz no nível das margens (o ponto mais alto da curva fica ~2 m acima)
                const FVector2D L = P.At - U * 2600.f, R = P.At + U * 2600.f;
                const FVector Origin(P.At, (Data->GroundAt(L.X, L.Y) + Data->GroundAt(R.X, R.Y)) * .5f - 240.f);
                Add(MarvelPiece(this, TEXT("rootbridge"), Origin, P.Yaw, FVector(1.f), false));
                RootWalk(this, *Data, Origin, P.Yaw, 1.f, FVector(-34, 0, -8), FVector(0, 0, 9), FVector(34, 0, -8), 5.f, 3.6f, List);
            }
            else
            {
                const float Sink = P.Prop == TEXT("stonering") ? 40.f : P.Prop == TEXT("crystals") ? 50.f : 20.f;
                Add(MarvelPiece(this, P.Prop, FVector(P.At, Floor - Sink), P.Yaw, FVector(1.f), true));
            }
            if (P.Prop == TEXT("crystals") && GetNetMode() != NM_DedicatedServer)
            {
                // a luz verde do ninho (de noite o lugar inteiro brilha)
                UPointLightComponent* Light = NewObject<UPointLightComponent>(this);
                Light->SetupAttachment(RootComponent);
                Light->SetWorldLocation(FVector(P.At, Floor + 700.f));
                Light->SetLightColor(FLinearColor(.25f, 1.f, .5f));
                Light->SetIntensity(9000.f);
                Light->SetAttenuationRadius(4500.f);
                Light->SetCastShadows(false);
                Light->RegisterComponent();
                List.Add(Light);
            }
        }
        UE_LOG(LogTemp, Display, TEXT("VORTEX_MARVEL montado %s (%d peças)"), *Sites()[Site].Name, List.Num());
        return;
    }
    auto Ground = [&](float X, float Z) { return Data->GroundAt(X * 100.f, Z * 100.f) / 100.f; };
    UMaterialInterface* Painted = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Materials/M_FarTerrain.M_FarTerrain"));
    FLandmarkBoxes Stones;
    TArray<FVector> Fires;
    TArray<TPair<FVector, FVector>> Solids;   // centro e tamanho (cm) das caixas de colisão
    const FVector2D C = Sites()[Site].Center / 100.f;   // Godot (x, z)
    auto Campfire = [&](float X, float Z)
    {
        const float Y = Ground(X, Z);
        for (int32 i = 0; i < 9; ++i)
        {
            const float A = i * UE_TWO_PI / 9.f;
            Stones.Box(FVector(X + FMath::Cos(A) * .7f, Y + .1f, Z + FMath::Sin(A) * .7f), FVector(.3f, .22f, .3f), LandmarkHex(i % 2 == 0 ? TEXT("6d6a64") : TEXT("84807a")));
        }
        Stones.Box(FVector(X, Y + .12f, Z), FVector(.9f, .14f, .16f), LandmarkHex(TEXT("5a3a22")));
        Stones.Box(FVector(X, Y + .2f, Z), FVector(.16f, .14f, .9f), LandmarkHex(TEXT("4a2e1a")));
        Stones.Box(FVector(X, Y + .06f, Z), FVector(.8f, .06f, .8f), LandmarkHex(TEXT("2a1c14")));
        Fires.Add(FVector(X * 100.f, Z * 100.f, Y * 100.f + 15.f));
    };
    // praça de pedras pequenas no chão de verdade (sem laje flutuando)
    auto Square = [&](const TCHAR* Tone)
    {
        for (float Dx = -10.f; Dx <= 10.f; Dx += .75f)
        for (float Dz = -11.f; Dz <= 14.f; Dz += .75f)
        {
            if (FMath::Abs(Dx) > 6.f && FMath::Abs(Dz) > 6.f) continue;
            const float X = C.X + Dx, Z = C.Y + Dz;
            const float Noise = ((FMath::RoundToInt(Dx * 17.f + Dz * 23.f) & 7) - 3) * .014f;
            Stones.Box(FVector(X, Ground(X, Z) + .025f, Z), FVector(.65f, .07f, .65f), LandmarkHex(Tone, Noise));
        }
    };
    if (Site == Yggdrasil)
    {
        // a árvore: tronco (com colisão, raízes para subir) e copa
        const FVector Foot(C.X * 100.f, C.Y * 100.f, (Ground(C.X, C.Y) - .5f) * 100.f);
        for (const TCHAR* Piece : { TEXT("yggdrasil_trunk"), TEXT("yggdrasil_canopy") })
        {
            UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Props/%s/%s/StaticMeshes/%s.%s"), Piece, Piece, Piece, Piece));
            if (!Mesh) { UE_LOG(LogTemp, Warning, TEXT("VORTEX_LANDMARK sem %s"), Piece); continue; }
            UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this);
            Part->SetStaticMesh(Mesh);
            const bool bTrunk = FCString::Strcmp(Piece, TEXT("yggdrasil_trunk")) == 0;
            Part->SetCollisionEnabled(bTrunk ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
            if (bTrunk) Part->SetCollisionProfileName(TEXT("BlockAll"));
            Part->SetupAttachment(RootComponent);
            Part->SetWorldLocation(Foot);
            // depois de 2,3 km só a miragem fica (a névoa para antes disso e a árvore apareceria nítida)
            Part->SetCullDistance(230000.f);
            Part->RegisterComponent();
            Parts[Site].Add(Part);
            // a miragem: a mesma silhueta, maior, translúcida e tremendo como ar quente
            UMaterialInterface* MirageLook = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Sky/M_YggMirage.M_YggMirage"));
            if (MirageLook && GetNetMode() != NM_DedicatedServer)
            {
                UStaticMeshComponent* Ghost = NewObject<UStaticMeshComponent>(this);
                Ghost->SetStaticMesh(Mesh);
                Ghost->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                Ghost->SetCastShadow(false);
                Ghost->bVisibleInRayTracing = false;
                Ghost->bAffectDistanceFieldLighting = false;
                Ghost->SetBoundsScale(1.2f);
                for (int32 Slot = 0; Slot < Mesh->GetStaticMaterials().Num(); ++Slot) Ghost->SetMaterial(Slot, MirageLook);
                Ghost->SetupAttachment(RootComponent);
                Ghost->SetWorldLocation(Foot + FVector(0.f, 0.f, 2500.f));
                Ghost->SetWorldScale3D(FVector(1.6f));   // miragem: maior e pairando, como o ar quente aumenta
                Ghost->SetVisibility(false);
                Ghost->RegisterComponent();
                Mirage.Add(Ghost);
            }
        }
        // a aldeia élfica: casas em anel de frente para a árvore, arco, Poço da Lua e lanternas
        auto Facing = [&](float X, float Z) { return FMath::RadiansToDegrees(FMath::Atan2(-(C.X - X), -(C.Y - Z))); };
        struct FHome { const TCHAR* Model; float Angle, Distance; };
        static const FHome Homes[] = { { TEXT("elf_house_a"), 205, 96 }, { TEXT("elf_house_b"), 238, 100 }, { TEXT("elf_house_b"), 302, 100 }, { TEXT("elf_house_a"), 335, 96 },
            { TEXT("elf_house_a"), 20, 98 }, { TEXT("elf_house_b"), 62, 104 }, { TEXT("elf_house_a"), 118, 98 }, { TEXT("elf_house_b"), 160, 104 } };
        for (const FHome& Home : Homes)
        {
            const float A = FMath::DegreesToRadians(Home.Angle);
            const float X = C.X + FMath::Cos(A) * Home.Distance, Z = C.Y + FMath::Sin(A) * Home.Distance;
            Place(Site, Home.Model, X, Z, Facing(X, Z));
        }
        Place(Site, TEXT("elf_arch"), C.X, C.Y - 146.f, 0.f);
        Place(Site, TEXT("elf_moonwell"), C.X, C.Y - 78.f, 0.f);
        for (float D = 30.f; D <= 140.f; D += 13.f)
            for (const int32 Side : { -1, 1 })
                if (FMath::Abs(D - 78.f) > 6.f) Place(Site, TEXT("elf_lamp"), C.X + Side * 4.6f, C.Y - D, Side < 0 ? 90.f : -90.f);
        for (int32 k = 0; k < 16; ++k)
        {
            const float A = k * UE_TWO_PI / 16.f + .1f;
            const float X = C.X + FMath::Cos(A) * 72.f, Z = C.Y + FMath::Sin(A) * 72.f;
            if (FMath::Abs(X - C.X) < 8.f && Z < C.Y) continue;
            Place(Site, TEXT("elf_lamp"), X, Z, Facing(X, Z) + 90.f);
        }
    }
    else if (Site == Dunes)
    {
        // Acampamento das Dunas: carroças, tendas, torre, poço, cercas, lampiões e a fogueira
        auto P = [&](const TCHAR* Model, float Dx, float Dz, float Yaw) { Place(Site, Model, C.X + Dx, C.Y + Dz, Yaw); };
        P(TEXT("vil_wagon"), -17, 8, 90); P(TEXT("vil_wagon"), 17, 8, -90);
        P(TEXT("vil_stall_a"), -17, -8, 90); P(TEXT("vil_stall_b"), 17, -8, -90);
        P(TEXT("vil_tower"), 0, 20, 180); P(TEXT("vil_well"), -9, 17, 0); P(TEXT("vil_crates"), 10, 16, 25);
        for (int32 i = -1; i <= 1; ++i) { P(TEXT("vil_fence"), -22 + i * 2.1f, -17, 0); P(TEXT("vil_fence"), 22 + i * 2.1f, -17, 0); }
        for (const float X : { -11.f, 11.f }) for (const float Z : { -13.f, 13.f }) P(TEXT("vil_lamp"), X, Z, X < 0 ? 90.f : -90.f);
        Square(TEXT("b89969"));
        Campfire(C.X, C.Y - 3.f);
    }
    else if (Site == Hunters)
    {
        // Vila dos Caçadores: chalés, loja, arco élfico, poço, banco, lanternas e a fogueira
        auto P = [&](const TCHAR* Model, float Dx, float Dz, float Yaw) { Place(Site, Model, C.X + Dx, C.Y + Dz, Yaw); };
        P(TEXT("town_cottage"), -18, -10, 90); P(TEXT("town_cottage"), -18, 11, 90); P(TEXT("town_cottage"), 18, 11, -90);
        P(TEXT("town_shop"), 18, -10, -90); P(TEXT("elf_arch"), 0, -20, 0); P(TEXT("vil_well"), 0, 16, 0);
        P(TEXT("vil_bench"), -8, 14, 0); P(TEXT("vil_crates"), 11, 15, 60);
        for (const float Z : { -13.f, 0.f, 13.f }) for (const float X : { -5.f, 5.f }) P(TEXT("elf_lamp"), X, Z, X < 0 ? 90.f : -90.f);
        Square(TEXT("778b71"));
        Campfire(C.X, C.Y - 4.f);
    }
    else
    {
        // covil do chefe: mosaico baixo em anel e oito obeliscos com runas acesas
        const bool bDune = Site == DuneDen;
        const FLinearColor Dark = LandmarkHex(bDune ? TEXT("715b48") : TEXT("3d4b39")), Light = LandmarkHex(bDune ? TEXT("c3a270") : TEXT("658459"));
        const FLinearColor Glow = LandmarkHex(bDune ? TEXT("75c5c9") : TEXT("78c99a")) * 3.f;
        for (int32 X = -22; X <= 22; X += 2) for (int32 Z = -22; Z <= 22; Z += 2)
        {
            const float R = FMath::Sqrt(float(X * X + Z * Z));
            if (R < 10.f || R > 22.f || ((X * 7 + Z * 11) & 3) == 0) continue;
            const float Wx = C.X + X, Wz = C.Y + Z;
            Stones.Box(FVector(Wx, Ground(Wx, Wz) + .035f, Wz), FVector(1.65f, .08f, 1.65f), ((X + Z) & 4) == 0 ? Light : Dark);
        }
        for (int32 i = 0; i < 8; ++i)
        {
            const float A = i * UE_TWO_PI / 8.f;
            const float X = C.X + FMath::Cos(A) * 18.f, Z = C.Y + FMath::Sin(A) * 18.f, Y = Ground(X, Z);
            const float Height = bDune ? 2.8f + i % 3 * .35f : 2.3f + i % 2 * .5f;
            Stones.Box(FVector(X, Y + Height * .5f, Z), FVector(1.3f, Height, 1.3f), Dark);
            Stones.Box(FVector(X, Y + Height + .18f, Z), FVector(1.6f, .38f, 1.6f), Light);
            Stones.Box(FVector(X, Y + Height * .68f, Z + .69f), FVector(.45f, .55f, .08f), Glow);
            Solids.Add({ FVector(X * 100.f, Z * 100.f, (Y + Height * .5f) * 100.f), FVector(130.f, 130.f, Height * 100.f) });
        }
    }
    if (Stones.Vertices.Num())
    {
        UProceduralMeshComponent* Mesh = NewObject<UProceduralMeshComponent>(this);
        Mesh->SetupAttachment(RootComponent);
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->RegisterComponent();
        Mesh->CreateMeshSection_LinearColor(0, Stones.Vertices, Stones.Triangles, Stones.Normals, {}, Stones.Colors, {}, false);
        if (Painted) Mesh->SetMaterial(0, Painted);
        Parts[Site].Add(Mesh);
    }
    for (const TPair<FVector, FVector>& Solid : Solids)
    {
        UBoxComponent* Box = NewObject<UBoxComponent>(this);
        Box->SetMobility(EComponentMobility::Static);
        Box->SetupAttachment(RootComponent);
        Box->SetBoxExtent(Solid.Value * .5f);
        Box->SetWorldLocation(Solid.Key);
        Box->SetCollisionProfileName(TEXT("BlockAll"));
        Box->RegisterComponent();
        Parts[Site].Add(Box);
    }
    if (GetNetMode() != NM_DedicatedServer)
        if (UNiagaraSystem* Fire = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Vefects/Free_Fire/Shared/Particles/NS_Fire_Small.NS_Fire_Small")))
            for (const FVector& At : Fires)
                if (UNiagaraComponent* Flame = UNiagaraFunctionLibrary::SpawnSystemAttached(Fire, RootComponent, NAME_None, At, FRotator::ZeroRotator, FVector(.6f), EAttachLocation::KeepWorldPosition, true, ENCPoolMethod::None, true))
                    Parts[Site].Add(Flame);
}
