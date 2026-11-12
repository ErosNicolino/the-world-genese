#include "World/VortexTown.h"

#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "World/VortexDayNightSubsystem.h"
#include "World/VortexFountain.h"
#include "World/VortexLandmarks.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "UObject/UnrealType.h"
#include "World/VortexTerrainData.h"
#include "World/VortexTerrainStreamer.h"
#include "World/VortexTownLife.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
    // 08/11: o que fica guardado em "static" precisa de AddToRoot: senão a limpeza de memória apaga ao trocar de mapa e a
    // próxima partida usa lixo (crash ao recomeçar, "Pure virtual function being called")
    template <typename T> T* Keep(T* Object) { if (Object) Object->AddToRoot(); return Object; }

    // muralhas do Godot (m): x -40..40, z -24..56; 03/11, a vila maior (town_extra.json): x -62..62, z -34..80
    FBox2D TownBounds(FVector2D(-4000.f, -2400.f), FVector2D(4000.f, 5600.f));

    FVector ToUnreal(double X, double Y, double Z) { return FVector(X * 100.0, Z * 100.0, Y * 100.0); }

    TSharedPtr<FJsonObject> ReadJson(const TCHAR* File)
    {
        FString Text;
        TSharedPtr<FJsonObject> Json;
        if (!FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir() / File)) ||
            !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json)) return nullptr;
        return Json;
    }

    /**
     * A planta: a do Godot (town.json) com a vila maior por cima (town_extra.json, Tools/BuildTownExpansion.py). A extra
     * troca os limites, tira os modelos da lista "remove" (a muralha antiga), soma os modelos, as peças, as árvores e as
     * pedras, e traz as listas só dela (spinners, cloth, smoke, glows, breakables).
     */
    TSharedPtr<FJsonObject> LoadPlan()
    {
        // 06/11: a vila nova (Tools/Town/BuildVila.py), sozinha, no lugar das duas de antes
        if (TSharedPtr<FJsonObject> Vila = ReadJson(TEXT("Data/GodotTown/vila.json")))
        {
            const TArray<TSharedPtr<FJsonValue>>* Bounds = nullptr;
            if (Vila->TryGetArrayField(TEXT("bounds"), Bounds) && Bounds->Num() == 4)
                TownBounds = FBox2D(FVector2D((*Bounds)[0]->AsNumber() * 100.0, (*Bounds)[1]->AsNumber() * 100.0),
                    FVector2D((*Bounds)[2]->AsNumber() * 100.0, (*Bounds)[3]->AsNumber() * 100.0));
            return Vila;
        }
        TSharedPtr<FJsonObject> Plan = ReadJson(TEXT("Data/GodotTown/town.json"));
        const TSharedPtr<FJsonObject> Extra = Plan ? ReadJson(TEXT("Data/GodotTown/town_extra.json")) : nullptr;
        if (!Plan || !Extra) return Plan;
        const TArray<TSharedPtr<FJsonValue>>* Bounds = nullptr;
        if (Extra->TryGetArrayField(TEXT("bounds"), Bounds) && Bounds->Num() == 4)
            TownBounds = FBox2D(FVector2D((*Bounds)[0]->AsNumber() * 100.0, (*Bounds)[1]->AsNumber() * 100.0),
                FVector2D((*Bounds)[2]->AsNumber() * 100.0, (*Bounds)[3]->AsNumber() * 100.0));
        TSet<FString> Removed;
        for (const TSharedPtr<FJsonValue>& V : Extra->GetArrayField(TEXT("remove"))) Removed.Add(V->AsString());
        TArray<TSharedPtr<FJsonValue>> Pieces;
        for (const TSharedPtr<FJsonValue>& V : Plan->GetArrayField(TEXT("pieces")))
            if (!Removed.Contains(V->AsObject()->GetStringField(TEXT("model")))) Pieces.Add(V);
        Pieces.Append(Extra->GetArrayField(TEXT("pieces")));
        Plan->SetArrayField(TEXT("pieces"), Pieces);
        for (const TCHAR* Field : { TEXT("trees"), TEXT("cobbles") })
        {
            TArray<TSharedPtr<FJsonValue>> All = Plan->GetArrayField(Field);
            All.Append(Extra->GetArrayField(Field));
            Plan->SetArrayField(Field, All);
        }
        const TSharedPtr<FJsonObject> Models = Plan->GetObjectField(TEXT("models"));
        for (const TPair<FString, TSharedPtr<FJsonValue>> Model : Extra->GetObjectField(TEXT("models"))->Values) Models->SetField(Model.Key, Model.Value);
        for (const TCHAR* Field : { TEXT("spinners"), TEXT("cloth"), TEXT("smoke"), TEXT("glows"), TEXT("breakables") })
        {
            const TArray<TSharedPtr<FJsonValue>>* List = nullptr;
            if (Extra->TryGetArrayField(Field, List)) Plan->SetArrayField(Field, *List);
        }
        return Plan;
    }

    // 07/11: o vidro dos lampiões (MI_Vila_Lanterna) acende à noite: um MID só, trocado em todos os lotes que o usam
    TWeakObjectPtr<UMaterialInstanceDynamic> LanternGlass;

    FVector ReadVec(const TArray<TSharedPtr<FJsonValue>>& A, int32 At = 0)
    {
        return ToUnreal(A[At]->AsNumber(), A[At + 1]->AsNumber(), A[At + 2]->AsNumber());
    }
}

AVortexTown::AVortexTown()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = .5f;
    bReplicates = false;
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;
}

AVortexTown* AVortexTown::Ensure(UWorld* World)
{
    if (!World) return nullptr;
    if (TActorIterator<AVortexTown> It(World); It) return *It;
    FActorSpawnParameters Params;
    Params.ObjectFlags |= RF_Transient;
    return World->SpawnActor<AVortexTown>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
}

AVortexTown::FDoor* AVortexTown::NearestDoor(const FVector& At, float Range)
{
    FDoor* Best = nullptr;
    float BestDistance = Range * Range;
    for (FDoor& Door : Doors)
    {
        const float D = FVector::DistSquared(Door.Handle, At);
        if (D < BestDistance && Door.Pivot.IsValid()) { Best = &Door; BestDistance = D; }
    }
    return Best;
}

bool AVortexTown::FindDoor(const UWorld* World, const FVector& At, float Range, bool& bOpen)
{
    TActorIterator<AVortexTown> It(const_cast<UWorld*>(World));
    const FDoor* Door = (World && It) ? It->NearestDoor(At, Range) : nullptr;
    bOpen = Door && Door->Target > 1.f;
    return Door != nullptr;
}

bool AVortexTown::ToggleDoor(const UWorld* World, const FVector& At, float Range)
{
    TActorIterator<AVortexTown> It(const_cast<UWorld*>(World));
    FDoor* Door = (World && It) ? It->NearestDoor(At, Range) : nullptr;
    if (!Door) return false;
    Door->Target = Door->Target > 1.f ? 0.f : 95.f;
    // parada, a vila tica a cada 0,5 s: o primeiro passo era tão grande que a porta aparecia já aberta
    It->PrimaryActorTick.UpdateTickIntervalAndCoolDown(0.f);
    It->SetActorTickInterval(0.f);
    return true;
}

bool AVortexTown::Contains(const FVector& Where)
{
    // os limites vêm da planta (a vila maior): lê na primeira vez (só na thread do jogo; nas outras ela já foi lida)
    if (IsInGameThread()) LoadGrassMask();
    return TownBounds.IsInside(FVector2D(Where));
}

namespace
{
    struct FGrassMask
    {
        bool bLoaded = false;
        TSet<FIntPoint> Cobbles;                        // células de 75 cm
        struct FRect { FVector2D Center; FVector2D Half; float Cos, Sin; };
        TArray<FRect> Floors;                           // alicerces (com 30 cm de folga)
        TArray<FRect> Clearings;                        // 11/11: a frente dos portões
        FBox2D Bounds = FBox2D(ForceInit);              // 11/11: tudo o que a máscara cobre (a estrada de fora passa da muralha)
    };
    bool InRect(const FGrassMask::FRect& R, float X, float Y)
    {
        const FVector2D D(X - R.Center.X, Y - R.Center.Y);
        const float Lx = D.X * R.Cos + D.Y * R.Sin, Ly = -D.X * R.Sin + D.Y * R.Cos;   // no espaço da peça (giro inverso)
        return FMath::Abs(Lx) <= R.Half.X && FMath::Abs(Ly) <= R.Half.Y;
    }
    FGrassMask GMask;
    FIntPoint CobbleCell(float X, float Y) { return FIntPoint(FMath::FloorToInt(X / 75.f), FMath::FloorToInt(Y / 75.f)); }
}

void AVortexTown::LoadGrassMask()
{
    if (GMask.bLoaded) return;
    GMask.bLoaded = true;
    const TSharedPtr<FJsonObject> Json = LoadPlan();
    if (!Json) return;
    for (const TSharedPtr<FJsonValue>& Value : Json->GetArrayField(TEXT("cobbles")))
    {
        const TArray<TSharedPtr<FJsonValue>>& C = Value->AsArray();
        if (C.Num() >= 2)
        {
            GMask.Cobbles.Add(CobbleCell(C[0]->AsNumber() * 100.0, C[1]->AsNumber() * 100.0));
            GMask.Bounds += FVector2D(C[0]->AsNumber() * 100.0, C[1]->AsNumber() * 100.0);
        }
    }
    const TArray<TSharedPtr<FJsonValue>>* Clearings = nullptr;
    if (Json->TryGetArrayField(TEXT("clearings"), Clearings))
        for (const TSharedPtr<FJsonValue>& Value : *Clearings)
        {
            const TArray<TSharedPtr<FJsonValue>>& C = Value->AsArray();
            if (C.Num() < 5) continue;
            FGrassMask::FRect Rect;
            Rect.Center = FVector2D(C[0]->AsNumber(), C[1]->AsNumber()) * 100.f;
            Rect.Half = FVector2D(C[2]->AsNumber(), C[3]->AsNumber()) * 100.f;
            Rect.Cos = FMath::Cos(FMath::DegreesToRadians(C[4]->AsNumber()));
            Rect.Sin = FMath::Sin(FMath::DegreesToRadians(C[4]->AsNumber()));
            GMask.Clearings.Add(Rect);
            GMask.Bounds += Rect.Center + FVector2D(Rect.Half.Size());
            GMask.Bounds += Rect.Center - FVector2D(Rect.Half.Size());
        }
    const TSharedPtr<FJsonObject> Models = Json->GetObjectField(TEXT("models"));
    for (const TSharedPtr<FJsonValue>& Value : Json->GetArrayField(TEXT("pieces")))
    {
        const TSharedPtr<FJsonObject> P = Value->AsObject();
        const TSharedPtr<FJsonObject> Info = Models->GetObjectField(P->GetStringField(TEXT("model")));
        const TArray<TSharedPtr<FJsonValue>> Foot = Info->GetArrayField(TEXT("footing"));
        if (Foot.Num() != 4) continue;
        const double F[4] = { Foot[0]->AsNumber(), Foot[1]->AsNumber(), Foot[2]->AsNumber(), Foot[3]->AsNumber() };
        const float Yaw = FMath::DegreesToRadians(-P->GetNumberField(TEXT("yaw")));
        FGrassMask::FRect Rect;
        const FVector Mid = FRotator(0.f, FMath::RadiansToDegrees(Yaw), 0.f).RotateVector(ToUnreal((F[0] + F[2]) * .5, 0.0, (F[1] + F[3]) * .5));
        Rect.Center = FVector2D(P->GetNumberField(TEXT("x")) * 100.0 + Mid.X, P->GetNumberField(TEXT("z")) * 100.0 + Mid.Y);
        Rect.Half = FVector2D((F[2] - F[0]) * 50.0 + 30.0, (F[3] - F[1]) * 50.0 + 30.0);
        Rect.Cos = FMath::Cos(Yaw);
        Rect.Sin = FMath::Sin(Yaw);
        GMask.Floors.Add(Rect);
    }
}

bool AVortexTown::BlocksGrass(float X, float Y)
{
    const FVector2D At(X, Y);
    if (!GMask.bLoaded || !(TownBounds.ExpandBy(400.f).IsInside(At) || (GMask.Bounds.bIsValid && GMask.Bounds.ExpandBy(100.f).IsInside(At)))) return false;
    if (GMask.Cobbles.Contains(CobbleCell(X, Y))) return true;
    for (const FGrassMask::FRect& R : GMask.Floors)
        if (InRect(R, X, Y)) return true;
    return false;
}

bool AVortexTown::Clears(float X, float Y)
{
    if (!GMask.bLoaded || !GMask.Bounds.bIsValid || !GMask.Bounds.IsInside(FVector2D(X, Y))) return false;
    for (const FGrassMask::FRect& R : GMask.Clearings)
        if (InRect(R, X, Y)) return true;
    return false;
}

int32 AVortexTown::MapKindAt(float X, float Y)
{
    if (!GMask.bLoaded || !TownBounds.ExpandBy(400.f).IsInside(FVector2D(X, Y))) return 0;
    for (const FGrassMask::FRect& R : GMask.Floors)
    {
        const FVector2D D(X - R.Center.X, Y - R.Center.Y);
        const float Lx = D.X * R.Cos + D.Y * R.Sin, Ly = -D.X * R.Sin + D.Y * R.Cos;
        // sem a folga de 30 cm da grama: no mapa a construção fica do tamanho dela
        if (FMath::Abs(Lx) <= R.Half.X - 30.f && FMath::Abs(Ly) <= R.Half.Y - 30.f) return 2;
    }
    return GMask.Cobbles.Contains(CobbleCell(X, Y)) ? 1 : 0;
}

void AVortexTown::BeginPlay()
{
    Super::BeginPlay();
    Build();
}

UInstancedStaticMeshComponent* AVortexTown::Batch(FName Key, const FString& MeshPath, const FString& MaterialPath, bool bShadows)
{
    if (TObjectPtr<UInstancedStaticMeshComponent>* Found = Batches.Find(Key)) return *Found;
    UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *MeshPath);
    if (!Mesh)
    {
        UE_LOG(LogTemp, Warning, TEXT("VORTEX_TOWN modelo ausente %s"), *MeshPath);
        Batches.Add(Key, nullptr);
        return nullptr;
    }
    UInstancedStaticMeshComponent* Instances = NewObject<UInstancedStaticMeshComponent>(this);
    Instances->SetStaticMesh(Mesh);
    if (!MaterialPath.IsEmpty())
    {
        if (UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, *MaterialPath)) Instances->SetMaterial(0, Material);
    }
    Instances->SetCollisionEnabled(ECollisionEnabled::NoCollision);   // colisão: as caixas do Godot
    for (int32 Slot = 0; Slot < Instances->GetNumMaterials(); ++Slot)
        if (UMaterialInterface* Used = Instances->GetMaterial(Slot); Used && Used->GetName() == TEXT("MI_Vila_Lanterna"))
        {
            if (!LanternGlass.IsValid()) LanternGlass = UMaterialInstanceDynamic::Create(Used, this);
            Instances->SetMaterial(Slot, LanternGlass.Get());
        }
        // medir o custo do vidro translúcido: -VortexTownNoGlass troca pelo material opaco do kit
        else if (Used && Used->GetName() == TEXT("M_VilaVidro") && FParse::Param(FCommandLine::Get(), TEXT("VortexTownNoGlass")))
        {
            if (UMaterialInterface* Opaque = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Town/Vila/MI_Vila_Ferro.MI_Vila_Ferro"))) Instances->SetMaterial(Slot, Opaque);
        }
        // 07/11: a água do poço é a mesma que o terreno usa agora (a da fonte; muda com a qualidade da água)
        else if (Used && Used->GetName() == TEXT("M_VortexWater"))
            for (TActorIterator<AVortexTerrainStreamer> It(GetWorld()); It; ++It)
                if (UMaterialInterface* Water = It->GetWaterMaterial()) Instances->SetMaterial(Slot, Water);
    Instances->SetCastShadow(bShadows);
    Instances->SetupAttachment(Root);
    Instances->RegisterComponent();
    Batches.Add(Key, Instances);
    return Instances;
}

void AVortexTown::Build()
{
    const TSharedPtr<FJsonObject> Json = LoadPlan();
    if (!Json)
    {
        UE_LOG(LogTemp, Warning, TEXT("VORTEX_TOWN sem Data/GodotTown/town.json"));
        return;
    }
    const TSharedPtr<FVortexTerrainData> Terrain = AVortexTerrainStreamer::SharedData();
    auto Ground = [&Terrain](const FVector2D& At) { return Terrain ? Terrain->GroundAt(At.X, At.Y) : 0.f; };
    const TSharedPtr<FJsonObject> Models = Json->GetObjectField(TEXT("models"));
    int32 Boxes = 0;

    // ---- construções e enfeites ------------------------------------------------------------
    for (const TSharedPtr<FJsonValue>& Value : Json->GetArrayField(TEXT("pieces")))
    {
        const TSharedPtr<FJsonObject> P = Value->AsObject();
        const FString Model = P->GetStringField(TEXT("model"));
        const FString Label = P->GetStringField(TEXT("label"));
        const double X = P->GetNumberField(TEXT("x")), Z = P->GetNumberField(TEXT("z")), Yaw = P->GetNumberField(TEXT("yaw"));
        const TSharedPtr<FJsonObject> Info = Models->GetObjectField(Model);
        const FRotator Rotation(0.f, -Yaw, 0.f);
        const FVector Center = ToUnreal(X, 0.0, Z);
        // assenta no ponto mais baixo do alicerce (como o PlaceModel do Godot)
        float Low = TNumericLimits<float>::Max(), High = TNumericLimits<float>::Lowest();
        const TArray<TSharedPtr<FJsonValue>> Foot = Info->GetArrayField(TEXT("footing"));
        if (Foot.Num() == 4)
        {
            const double F[4] = { Foot[0]->AsNumber(), Foot[1]->AsNumber(), Foot[2]->AsNumber(), Foot[3]->AsNumber() };
            const FVector2D Corners[] = { { F[0], F[1] }, { F[2], F[1] }, { F[0], F[3] }, { F[2], F[3] }, { (F[0] + F[2]) * .5, (F[1] + F[3]) * .5 } };
            for (const FVector2D& C : Corners)
            {
                const FVector W = Center + Rotation.RotateVector(ToUnreal(C.X, 0.0, C.Y));
                const float G = Ground(FVector2D(W));
                Low = FMath::Min(Low, G);
                High = FMath::Max(High, G);
            }
        }
        else Low = High = Ground(FVector2D(Center));
        const float Floor = FMath::Min(Low + 12.f, High);
        const FVector Base(Center.X, Center.Y, Floor);
        // a fonte da praça foi refeita na Unreal (água do Vortex, jatos, santuário)
        if (Model == TEXT("vil_fountain"))
        {
            FActorSpawnParameters Params;
            Params.ObjectFlags |= RF_Transient;
            GetWorld()->SpawnActor<AVortexFountain>(Base, FRotator::ZeroRotator, Params);
            ++PieceCount;
            continue;
        }
        const FString Path = FString::Printf(TEXT("/Game/Vortex/Props/%s/%s/StaticMeshes/%s.%s"), *Model, *Model, *Model, *Model);
        const bool bSmall = Model == TEXT("vil_lamp") || Model == TEXT("vil_bench") || Model == TEXT("vil_planter") || Model == TEXT("vil_crates")
            || Model == TEXT("vila_poste") || Model == TEXT("vila_banco") || Model == TEXT("vila_floreira") || Model == TEXT("vila_caixotes")
            || Model == TEXT("vila_cerca");
        // as placas de chão da vila nova (paralelepípedo, laje, terra): muitas e baixinhas, somem de longe
        const bool bGround = Model.StartsWith(TEXT("vila_rua")) || Model.StartsWith(TEXT("vila_laje")) || Model.StartsWith(TEXT("vila_terra"))
            || Model.StartsWith(TEXT("vila_borda"));
        if (UInstancedStaticMeshComponent* Instances = Batch(FName(*Model), Path, FString(), true))
        {
            Instances->AddInstance(FTransform(Rotation, Base), true);
            if (bSmall) Instances->SetCullDistances(0, 9000);
            if (bGround) { Instances->SetCullDistances(0, 14000); Instances->SetCastShadow(false); }
            // 11/11 (auditoria de FPS): o chão e os enfeites pequenos ficam fora da cena do Lumen (não rebatem luz que se
            // note; como a grama em 11/11); os enfeites pequenos também não fazem sombra (as sombras eram 531 desenhos)
            if (bGround || bSmall)
            {
                Instances->bAffectDistanceFieldLighting = false;
                Instances->bAffectDynamicIndirectLighting = false;
                if (bSmall && Model != TEXT("vila_poste")) Instances->SetCastShadow(false);
            }
            ++PieceCount;
        }
        // alicerce de pedra onde o chão desce (nada flutua)
        if (Foot.Num() == 4 && Floor - Low > 3.f)
        {
            const double F[4] = { Foot[0]->AsNumber(), Foot[1]->AsNumber(), Foot[2]->AsNumber(), Foot[3]->AsNumber() };
            const float Depth = Floor - Low + 40.f;
            const FVector Mid = Base + Rotation.RotateVector(ToUnreal((F[0] + F[2]) * .5, 0.0, (F[1] + F[3]) * .5)) - FVector(0.f, 0.f, Depth * .5f - 2.f);
            if (UInstancedStaticMeshComponent* Stone = Batch(TEXT("footing"), TEXT("/Engine/BasicShapes/Cube.Cube"), TEXT("/Game/Vortex/Stadium/MI_Stadium_limestone_blocks.MI_Stadium_limestone_blocks"), false))
            {
                Stone->AddInstance(FTransform(Rotation, Mid, FVector((F[2] - F[0]), (F[3] - F[1]), Depth / 100.f)), true);
            }
        }
        // caixas de colisão do Godot (espaço da peça)
        for (const TSharedPtr<FJsonValue>& BoxValue : Info->GetArrayField(TEXT("boxes")))
        {
            const TArray<TSharedPtr<FJsonValue>>& B = BoxValue->AsArray();
            if (B.Num() < 6) continue;
            UBoxComponent* Box = NewObject<UBoxComponent>(this);
            Box->SetupAttachment(Root);
            Box->SetBoxExtent(FVector(B[3]->AsNumber() * 50.0, B[5]->AsNumber() * 50.0, B[4]->AsNumber() * 50.0));
            Box->SetWorldLocationAndRotation(Base + Rotation.RotateVector(ReadVec(B)), Rotation);
            Box->SetCollisionProfileName(TEXT("BlockAll"));
            Box->SetCanEverAffectNavigation(false);
            Box->RegisterComponent();
            ++Boxes;
        }
        // placa com o nome (de frente para a rua) e lampiões
        const TArray<TSharedPtr<FJsonValue>>* Sign = nullptr;
        // 07/11 (o usuário: "melhorar as placas, uma fonte medieval e o símbolo de cada lugar"): a placa pintada
        // (Tools/Town/MakeSigns.py: madeira, o nome em Cinzel dourado e o símbolo) num cubo achatado; o texto de antes
        // fica só para um nome sem imagem
        UTexture2D* Painted = nullptr;
        if (!Label.IsEmpty())
        {
            FString Slug;
            for (const TCHAR C : Label.ToUpper())
            {
                const TCHAR* From = TEXT("ÁÀÂÃÉÊÍÓÔÕÚÇ");
                const TCHAR* To = TEXT("AAAAEEIOOOUC");
                const int32 At = FCString::Strchr(From, C) ? int32(FCString::Strchr(From, C) - From) : INDEX_NONE;
                const TCHAR Plain = At != INDEX_NONE ? To[At] : C;
                if (FChar::IsAlnum(Plain) && Plain < 128) Slug.AppendChar(Plain);
                else if (!Slug.EndsWith(TEXT("_"))) Slug.AppendChar(TEXT('_'));
            }
            Slug.RemoveFromEnd(TEXT("_"));
            Painted = LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Town/Vila/Placas/T_Placa_%s.T_Placa_%s"), *Slug, *Slug));
        }
        static UMaterialInterface* SignMaterial = Keep(LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Town/Vila/M_VilaPlaca.M_VilaPlaca")));
        if (Painted && SignMaterial && Info->TryGetArrayField(TEXT("sign"), Sign) && Sign->Num() >= 3)
        {
            const FVector Front = Rotation.RotateVector(FVector(0.f, -1.f, 0.f));
            // 07/11 (o usuário, com uma referência: "as placas tão muito coladas"): nas casas, a placa pendurada no
            // braço de ferro (vila_placa_pendurada, a origem na parede, 55 cm acima do ponto da placa antiga); no
            // quadro de missões e na carroça, a tábua achatada de antes
            // 11/11: "sign_flat" (as casas do kit da Quaternius): a tábua presa na parede em cima da porta, menor
            const bool bFlat = Info->HasField(TEXT("sign_flat")) && Info->GetBoolField(TEXT("sign_flat"));
            const bool bHanging = Model != TEXT("vila_quadro") && Model != TEXT("vila_carroca") && !bFlat;
            const FVector SignAt = Base + Rotation.RotateVector(ReadVec(*Sign)) + (bHanging ? FVector(0.f, 0.f, 55.f) - Front * 6.f : Front * (bFlat ? 2.f : 10.f));
            const float Tall = bFlat ? 64.f : 90.f, Wide = Tall * Painted->GetSizeX() / FMath::Max(1, Painted->GetSizeY());
            UStaticMeshComponent* Board = NewObject<UStaticMeshComponent>(this);
            static UStaticMesh* SignBoard = Keep(LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Vortex/Props/vila_placa/vila_placa/StaticMeshes/vila_placa.vila_placa")));
            static UStaticMesh* SignHanging = Keep(LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Vortex/Props/vila_placa_pendurada/vila_placa_pendurada/StaticMeshes/vila_placa_pendurada.vila_placa_pendurada")));
            Board->SetStaticMesh(bHanging && SignHanging ? SignHanging : SignBoard);
            Board->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            UMaterialInstanceDynamic* Paint = UMaterialInstanceDynamic::Create(SignMaterial, this);
            Paint->SetTextureParameterValue(TEXT("Tex"), Painted);
            // a tábua vila_placa (1 x 1 m, a frente para a rua como as peças do kit): a imagem no material da face da
            // frente (MV_Placa), a madeira atrás
            for (int32 Slot = 0; Slot < Board->GetNumMaterials(); ++Slot)
                if (Board->GetMaterialSlotNames().IsValidIndex(Slot) && Board->GetMaterialSlotNames()[Slot].ToString().Contains(TEXT("Placa")))
                    Board->SetMaterial(Slot, Paint);
            Board->SetupAttachment(Root);
            Board->SetWorldLocationAndRotation(SignAt, Rotation);
            if (!(bHanging && SignHanging)) Board->SetWorldScale3D(FVector(Wide / 100.f, 1.f, Tall / 100.f));
            Board->SetCastShadow(bHanging);
            Board->RegisterComponent();
        }
        else if (!Label.IsEmpty() && Info->TryGetArrayField(TEXT("sign"), Sign) && Sign->Num() >= 3)
        {
            const FVector Front = Rotation.RotateVector(FVector(0.f, -1.f, 0.f));
            UTextRenderComponent* Words = NewObject<UTextRenderComponent>(this);
            Words->SetupAttachment(Root);
            Words->SetText(FText::FromString(Label));
            Words->SetHorizontalAlignment(EHTA_Center);
            Words->SetVerticalAlignment(EVRTA_TextCenter);
            Words->SetWorldSize(Label.Len() > 18 ? 34.f : 42.f);
            Words->SetTextRenderColor(FColor(255, 226, 150));
            // 03/11 (o usuário: "o texto é maior que a placa, e ela é pequena; o dobro"): uma tábua nossa com moldura
            // por trás do nome, do tamanho do texto com folga, um pouco à frente da plaquinha do modelo (que fica coberta)
            const FVector SignAt = Base + Rotation.RotateVector(ReadVec(*Sign)) + Front * 12.f;
            Words->SetWorldLocationAndRotation(SignAt + Front * 4.f, Front.Rotation());
            Words->SetCastShadow(false);
            Words->RegisterComponent();
            const FVector TextSize = Words->GetTextLocalSize();
            const float Wide = FMath::Max(TextSize.Y > 1.f ? TextSize.Y : Label.Len() * Words->WorldSize * .55f, 120.f) + 70.f;
            const float Tall = Words->WorldSize * 2.1f;
            const FQuat Facing = Front.Rotation().Quaternion();
            if (UInstancedStaticMeshComponent* Board = Batch(TEXT("sign_board"), TEXT("/Engine/BasicShapes/Cube.Cube"), TEXT("/Game/Vortex/Stadium/MI_Stadium_wood.MI_Stadium_wood"), true))
                Board->AddInstance(FTransform(Facing, SignAt, FVector(.05f, Wide / 100.f, Tall / 100.f)), true);
            if (UInstancedStaticMeshComponent* Frame = Batch(TEXT("sign_frame"), TEXT("/Engine/BasicShapes/Cube.Cube"), TEXT("/Game/Vortex/Stadium/MI_Stadium_basalt.MI_Stadium_basalt"), true))
                Frame->AddInstance(FTransform(Facing, SignAt - Front * 2.f, FVector(.05f, (Wide + 16.f) / 100.f, (Tall + 16.f) / 100.f)), true);
        }
        for (const TSharedPtr<FJsonValue>& LampValue : Info->GetArrayField(TEXT("lamps")))
        {
            const TArray<TSharedPtr<FJsonValue>>& L = LampValue->AsArray();
            if (L.Num() < 3) continue;
            UPointLightComponent* Lamp = NewObject<UPointLightComponent>(this);
            Lamp->SetMobility(EComponentMobility::Movable);
            Lamp->SetupAttachment(Root);
            Lamp->SetWorldLocation(Base + Rotation.RotateVector(ReadVec(L)));
            Lamp->SetLightColor(FLinearColor(1.f, .72f, .38f));
            Lamp->SetIntensityUnits(ELightUnits::Candelas);
            Lamp->SetIntensity(0.f);
            Lamp->SetAttenuationRadius(900.f);      // 07/11: 650 -> 900 (o usuário: "os postes não iluminam nada")
            Lamp->MaxDrawDistance = 6000.f;
            Lamp->SetSourceRadius(15.f);
            Lamp->SetCastShadows(false);
            Lamp->SetVisibility(false);
            Lamp->RegisterComponent();
            Lamps.Add(Lamp);
        }
        // 07/11: as portas de abrir (o kit dá a dobradiça, a largura e a altura): a folha vila_porta num pivô que gira,
        // com a colisão de caixa junto; lojas, taverna e capela começam abertas, as casas fechadas
        const TArray<TSharedPtr<FJsonValue>>* DoorList = nullptr;
        if (Info->TryGetArrayField(TEXT("doors"), DoorList))
            for (const TSharedPtr<FJsonValue>& DoorValue : *DoorList)
            {
                const TArray<TSharedPtr<FJsonValue>>& L = DoorValue->AsArray();
                if (L.Num() < 5) continue;
                static UStaticMesh* Leaf = Keep(LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Vortex/Props/vila_porta/vila_porta/StaticMeshes/vila_porta.vila_porta")));
                if (!Leaf) break;
                const float Wide = L[3]->AsNumber() * 100.f, Tall = L[4]->AsNumber() * 100.f;
                USceneComponent* Pivot = NewObject<USceneComponent>(this);
                Pivot->SetMobility(EComponentMobility::Movable);
                Pivot->SetupAttachment(Root);
                Pivot->SetWorldLocationAndRotation(Base + Rotation.RotateVector(ReadVec(L)), Rotation);
                Pivot->RegisterComponent();
                UStaticMeshComponent* Panel = NewObject<UStaticMeshComponent>(this);
                Panel->SetStaticMesh(Leaf);
                Panel->SetMobility(EComponentMobility::Movable);
                Panel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                Panel->SetupAttachment(Pivot);
                Panel->SetRelativeScale3D(FVector(Wide / 100.f, 2.6f, Tall / 220.f));   // 08/11: mais grossa (o usuário: "muito fina")
                Panel->RegisterComponent();
                UBoxComponent* Block = NewObject<UBoxComponent>(this);
                Block->SetupAttachment(Pivot);
                Block->SetBoxExtent(FVector(Wide * .5f, 8.f, Tall * .5f));
                Block->SetRelativeLocation(FVector(Wide * .5f, 0.f, Tall * .5f));
                Block->SetCollisionProfileName(TEXT("BlockAll"));
                Block->SetCanEverAffectNavigation(false);
                Block->RegisterComponent();
                FDoor& Door = Doors.AddDefaulted_GetRef();
                Door.Pivot = Pivot;
                Door.Closed = Rotation;
                Door.Handle = Pivot->GetComponentLocation() + Rotation.RotateVector(FVector(Wide * .5f, 0.f, 100.f));
                Door.Angle = Door.Target = Label.IsEmpty() ? 0.f : 95.f;
                Pivot->SetWorldRotation(Rotation + FRotator(0.f, Door.Angle, 0.f));
            }
        // 07/11: a luz de dentro das casas (sempre acesa, fraca e quente): o interior aparece pelas janelas de vidro
        const TArray<TSharedPtr<FJsonValue>>* Inner = nullptr;
        static const bool bNoInner = FParse::Param(FCommandLine::Get(), TEXT("VortexTownNoInner"));   // medir o custo
        if (!bNoInner && Info->TryGetArrayField(TEXT("inner"), Inner))
            for (const TSharedPtr<FJsonValue>& InnerValue : *Inner)
            {
                const TArray<TSharedPtr<FJsonValue>>& L = InnerValue->AsArray();
                if (L.Num() < 3) continue;
                UPointLightComponent* Room = NewObject<UPointLightComponent>(this);
                Room->SetMobility(EComponentMobility::Movable);
                Room->SetupAttachment(Root);
                Room->SetWorldLocation(Base + Rotation.RotateVector(ReadVec(L)));
                Room->SetLightColor(FLinearColor(1.f, .7f, .42f));
                Room->SetIntensityUnits(ELightUnits::Candelas);
                Room->SetIntensity(8.f);
                Room->SetAttenuationRadius(520.f);
                Room->SetSourceRadius(20.f);
                Room->SetCastShadows(false);
                Room->MaxDrawDistance = 3500.f;
                Room->MaxDistanceFadeRange = 600.f;
                Room->RegisterComponent();
            }
    }

    // ---- árvores de enfeite (com tronco que bloqueia) ----------------------------------------
    for (const TSharedPtr<FJsonValue>& Value : Json->GetArrayField(TEXT("trees")))
    {
        const TSharedPtr<FJsonObject> T = Value->AsObject();
        const FString Kind = T->GetStringField(TEXT("kind"));
        const double X = T->GetNumberField(TEXT("x")), Z = T->GetNumberField(TEXT("z")), Scale = T->GetNumberField(TEXT("scale"));
        const FVector At = ToUnreal(X, 0.0, Z);
        const FVector Base(At.X, At.Y, Ground(FVector2D(At)) - 6.f);
        const FString Path = FString::Printf(TEXT("/Game/Vortex/Props/%s/%s/StaticMeshes/%s.%s"), *Kind, *Kind, *Kind, *Kind);
        if (UInstancedStaticMeshComponent* Trees = Batch(FName(*(TEXT("tree:") + Kind)), Path, FString(), true))
        {
            Trees->AddInstance(FTransform(FRotator(0.f, -FMath::RadiansToDegrees(X * 1.7 + Z), 0.f), Base, FVector(Scale)), true);
        }
        UCapsuleComponent* Trunk = NewObject<UCapsuleComponent>(this);
        Trunk->SetupAttachment(Root);
        Trunk->SetCapsuleSize(60.f * Scale, 200.f);
        Trunk->SetWorldLocation(Base + FVector(0.f, 0.f, 200.f));
        Trunk->SetCollisionProfileName(TEXT("BlockAll"));
        Trunk->RegisterComponent();
    }

    // ---- pedras da praça e das ruas (75 cm, tons variados e musgo) -----------------------------
    int32 Cobbles = 0;
    for (const TSharedPtr<FJsonValue>& Value : Json->GetArrayField(TEXT("cobbles")))
    {
        const TArray<TSharedPtr<FJsonValue>>& C = Value->AsArray();
        if (C.Num() < 4) continue;
        const FVector At = ToUnreal(C[0]->AsNumber(), 0.0, C[1]->AsNumber());
        const bool bMoss = C[3]->AsNumber() > .5;
        const double Tone = C[2]->AsNumber();
        const FName Key = bMoss ? FName(TEXT("cobble_moss")) : Tone < .33 ? FName(TEXT("cobble_dark")) : Tone < .66 ? FName(TEXT("cobble_mid")) : FName(TEXT("cobble_light"));
        const TCHAR* Material = bMoss ? TEXT("/Game/Vortex/Stadium/MI_Stadium_grass.MI_Stadium_grass")
            : Tone < .33 ? TEXT("/Game/Vortex/Stadium/MI_Stadium_limestone_blocks.MI_Stadium_limestone_blocks")
            : Tone < .66 ? TEXT("/Game/Vortex/Stadium/MI_Stadium_travertine.MI_Stadium_travertine")
            : TEXT("/Game/Vortex/Stadium/MI_Stadium_marble.MI_Stadium_marble");
        if (UInstancedStaticMeshComponent* Stones = Batch(Key, TEXT("/Engine/BasicShapes/Cube.Cube"), Material, false))
        {
            Stones->AddInstance(FTransform(FRotator::ZeroRotator, FVector(At.X, At.Y, Ground(FVector2D(At)) + 4.f), FVector(.69f, .69f, .1f)), true);
            ++Cobbles;
        }
    }
    // ---- 03/11, a vila maior: as pás do moinho, as bandeiras, a fumaça, as janelas e os barris --------------
    const TArray<TSharedPtr<FJsonValue>>* List = nullptr;
    auto Mesh = [](const FString& Model) { return FString::Printf(TEXT("/Game/Vortex/Props/%s/%s/StaticMeshes/%s.%s"), *Model, *Model, *Model, *Model); };
    if (Json->TryGetArrayField(TEXT("spinners"), List))
        for (const TSharedPtr<FJsonValue>& Value : *List)
        {
            // as pás giram em volta do eixo delas (o y local da peça, a frente do moinho)
            const TSharedPtr<FJsonObject> S = Value->AsObject();
            const FRotator Rotation(0.f, -S->GetNumberField(TEXT("yaw")), 0.f);
            const FVector Center = ToUnreal(S->GetNumberField(TEXT("x")), 0.0, S->GetNumberField(TEXT("z")));
            const TArray<TSharedPtr<FJsonValue>> L = S->GetArrayField(TEXT("local"));
            const FVector Local = L.Num() == 3 ? ReadVec(L) : FVector::ZeroVector;
            UStaticMeshComponent* Blades = NewObject<UStaticMeshComponent>(this);
            Blades->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, *Mesh(S->GetStringField(TEXT("model")))));
            Blades->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Blades->SetMobility(EComponentMobility::Movable);
            Blades->SetupAttachment(Root);
            Blades->SetWorldLocationAndRotation(FVector(Center.X, Center.Y, Ground(FVector2D(Center))) + Rotation.RotateVector(Local), Rotation);
            Blades->RegisterComponent();
            URotatingMovementComponent* Spin = NewObject<URotatingMovementComponent>(this);
            Spin->SetUpdatedComponent(Blades);
            Spin->RotationRate = FRotator(S->GetNumberField(TEXT("speed")), 0.f, 0.f);
            Spin->bRotationInLocalSpace = true;
            Spin->RegisterComponent();
            ++PieceCount;
        }
    if (Json->TryGetArrayField(TEXT("cloth"), List))
        for (const TSharedPtr<FJsonValue>& Value : *List)
        {
            // o pano pende da travessa do mastro (M_TownCloth balança com o vento)
            const TSharedPtr<FJsonObject> C = Value->AsObject();
            const FString Model = C->GetStringField(TEXT("model"));
            const FRotator Rotation(0.f, -C->GetNumberField(TEXT("yaw")), 0.f);
            const FVector Pole = ToUnreal(C->GetNumberField(TEXT("x")), 0.0, C->GetNumberField(TEXT("z")));
            const FVector At = FVector(Pole.X, Pole.Y, Ground(FVector2D(Pole)) + C->GetNumberField(TEXT("y")) * 100.0) +
                Rotation.RotateVector(FVector(C->GetNumberField(TEXT("dx")) * 100.0, 0.f, 0.f));
            if (UInstancedStaticMeshComponent* Banners = Batch(FName(*(TEXT("cloth:") + Model)), Mesh(Model), FString(), true))
            {
                Banners->AddInstance(FTransform(Rotation, At), true);
                ++PieceCount;
            }
        }
    if (Json->TryGetArrayField(TEXT("smoke"), List))
        for (const TSharedPtr<FJsonValue>& Value : *List)
        {
            const TArray<TSharedPtr<FJsonValue>>& A = Value->AsArray();
            const FVector At = ReadVec(A);
            FActorSpawnParameters Params;
            Params.ObjectFlags |= RF_Transient;
            // 06/11: [x, y, z, largura, altura] (cm) na fumaça das chaminés da vila nova: fina, clara, devagar
            const bool bChimney = A.Num() >= 5;
            if (AVortexSmokeColumn* Smoke = GetWorld()->SpawnActor<AVortexSmokeColumn>(FVector(At.X, At.Y, Ground(FVector2D(At)) + At.Z), FRotator::ZeroRotator, Params))
            {
                // 06/11 (quase não aparecia): [x, y, z, largura, altura, opacidade, bolas] — os dois últimos opcionais
                if (bChimney) Smoke->Setup(A[3]->AsNumber(), A[4]->AsNumber(), A.Num() >= 7 ? int32(A[6]->AsNumber()) : 10, FLinearColor(.8f, .8f, .82f),
                    A.Num() >= 6 ? float(A[5]->AsNumber()) : .5f, 10.f);
                else Smoke->Setup(140.f, 900.f, 5, FLinearColor(.62f, .62f, .64f), .3f, 7.f);
            }
        }
    if (Json->TryGetArrayField(TEXT("glows"), List))
        for (const TSharedPtr<FJsonValue>& Value : *List)
        {
            // a luz das janelas: quente, fraca, sem sombra, na frente da casa (acende com os lampiões)
            const FVector At = ReadVec(Value->AsArray());
            UPointLightComponent* Glow = NewObject<UPointLightComponent>(this);
            Glow->SetMobility(EComponentMobility::Movable);
            Glow->SetupAttachment(Root);
            Glow->SetWorldLocation(FVector(At.X, At.Y, Ground(FVector2D(At)) + At.Z));
            Glow->SetLightColor(FLinearColor(1.f, .62f, .3f));
            Glow->SetIntensityUnits(ELightUnits::Candelas);
            Glow->SetIntensity(0.f);
            Glow->SetAttenuationRadius(450.f);
            Glow->SetCastShadows(false);
            Glow->SetVisibility(false);
            Glow->RegisterComponent();
            Windows.Add(Glow);
        }
    // barris e caixotes que quebram: atores do jogo (AVortexBreakable, módulo Vortex), que vão pela rede; só o servidor cria
    if (GetWorld()->GetNetMode() != NM_Client && Json->TryGetArrayField(TEXT("breakables"), List))
        if (UClass* Breakable = LoadClass<AActor>(nullptr, TEXT("/Script/Vortex.VortexBreakable")))
            for (const TSharedPtr<FJsonValue>& Value : *List)
            {
                const TSharedPtr<FJsonObject> B = Value->AsObject();
                const FString Kind = B->GetStringField(TEXT("kind"));
                const FVector At = ToUnreal(B->GetNumberField(TEXT("x")), 0.0, B->GetNumberField(TEXT("z")));
                const float Half = Kind == TEXT("crate") ? 37.5f : 50.f;
                const FTransform Pose(FRotator(0.f, B->GetNumberField(TEXT("yaw")), 0.f), FVector(At.X, At.Y, Ground(FVector2D(At)) + Half + 3.f));
                AActor* Actor = GetWorld()->SpawnActorDeferred<AActor>(Breakable, Pose, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
                if (!Actor) continue;
                if (FNameProperty* KindField = FindFProperty<FNameProperty>(Breakable, TEXT("Kind"))) KindField->SetPropertyValue_InContainer(Actor, FName(*Kind));
                Actor->FinishSpawning(Pose);
            }
    // 11/11: os bichos e os sons dos lugares (enfeite em cada máquina que desenha)
    if (Json->HasField(TEXT("animals")) && GetNetMode() != NM_DedicatedServer && !FParse::Param(FCommandLine::Get(), TEXT("VortexNoTownLife")))
    {
        FActorSpawnParameters LifeParams;
        LifeParams.ObjectFlags |= RF_Transient;
        if (AVortexTownLife* Life = GetWorld()->SpawnActor<AVortexTownLife>(FVector::ZeroVector, FRotator::ZeroRotator, LifeParams)) Life->Init(Json);
    }
    UE_LOG(LogTemp, Display, TEXT("VORTEX_TOWN montada: pecas=%d caixas=%d lampioes=%d janelas=%d pedras=%d lotes=%d limites=%s"), PieceCount, Boxes, Lamps.Num(),
        Windows.Num(), Cobbles, Batches.Num(), *TownBounds.ToString());
}

void AVortexTown::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    // 07/11: as portas girando; enquanto alguma gira, o Tick é a cada quadro
    bool bMoving = false;
    for (FDoor& Door : Doors)
    {
        if (FMath::IsNearlyEqual(Door.Angle, Door.Target)) continue;
        // 07/11 (o usuário: "faça animação da porta se abrindo"): freia no fim, como a folha pesada (~0,9 s)
        Door.Angle = FMath::Abs(Door.Target - Door.Angle) < .5f ? Door.Target : FMath::FInterpTo(Door.Angle, Door.Target, FMath::Min(DeltaSeconds, 1.f / 20.f), 4.f);
        if (USceneComponent* Pivot = Door.Pivot.Get()) Pivot->SetWorldRotation(Door.Closed + FRotator(0.f, Door.Angle, 0.f));
        bMoving = true;
    }
    SetActorTickInterval(bMoving ? 0.f : .5f);
    // lampiões: acendem no entardecer e apagam de manhã
    const UVortexDayNightSubsystem* DayNight = GetWorld()->GetSubsystem<UVortexDayNightSubsystem>();
    const float Daylight = DayNight && DayNight->IsActive() ? DayNight->GetDaylight() : 1.f;
    const float Level = 1.f - FMath::SmoothStep(.2f, .55f, Daylight);
    if (FMath::Abs(Level - LampLevel) < .02f) return;
    LampLevel = Level;
    for (UPointLightComponent* Lamp : Lamps)
    {
        if (!Lamp) continue;
        Lamp->SetVisibility(Level > .01f);
        // 30/10: 90 -> 30 (pedido: "a luz das coisas muito forte, como das lâmpadas"); 07/11: 30 -> 70 ("os postes
        // não iluminam nada"), com o raio maior
        Lamp->SetIntensity(38.f * Level);   // 07/11: 70 -> 38 (o usuário: "as luzes à noite muito fortes")
    }
    for (UPointLightComponent* Glow : Windows)
    {
        if (!Glow) continue;
        Glow->SetVisibility(Level > .01f);
        Glow->SetIntensity(9.f * Level);
    }
    if (UMaterialInstanceDynamic* Glass = LanternGlass.Get()) Glass->SetScalarParameterValue(TEXT("Glow"), 2.5f * Level);
}
