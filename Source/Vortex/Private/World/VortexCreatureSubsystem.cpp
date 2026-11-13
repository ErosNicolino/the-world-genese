#include "World/VortexCreatureSubsystem.h"

#include "Framework/VortexProbe.h"
#include "NiagaraSystem.h"
#include "Creatures/VortexMonsterSpecies.h"
#include "Animation/AnimSequence.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Creatures/VortexCreature.h"
#include "Creatures/VortexMonster.h"
#include "Creatures/VortexWolf.h"
#include "World/VortexPointsOfInterest.h"
#include "Dom/JsonObject.h"
#include "Engine/DataAsset.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/VortexCharacter.h"
#include "Framework/VortexGameState.h"
#include "GameFramework/PlayerController.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "TimerManager.h"
#include "UObject/Package.h"
#include "World/VortexTerrainData.h"
#include "World/VortexTerrainStreamer.h"
#include "World/VortexTown.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

// Aranhas no mundo aberto (guardas dos portais e as das teias). Desligadas a pedido: penduradas no fio no campo aberto
// ficavam no meio do céu. A espécie continua existindo (testes e dungeons).
static constexpr bool bWorldSpiders = false;

bool UVortexCreatureSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    const UWorld* World = Cast<UWorld>(Outer);
    return World && World->IsGameWorld() && !FParse::Param(FCommandLine::Get(), TEXT("VortexNoCreatures"));
}

void UVortexCreatureSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    bool bTerrain = false;
    if (TActorIterator<AVortexTerrainStreamer> It(&InWorld); It) { bTerrain = true; }
    // os testes de combate e de movimento usam mapas sem terreno: sem população (nem carga das espécies)
    if (!bTerrain) return;
    Preload();
    if (InWorld.GetNetMode() == NM_Client) return;
    // a cada partida um sorteio (antes a semente era só a quadra: os bichos nasciam sempre no mesmo lugar)
    const FString Command(FCommandLine::Get());
    const bool bTest = Command.Contains(TEXT("Smoke")) || Command.Contains(TEXT("Shot")) || Command.Contains(TEXT("Bench")) ||
        Command.Contains(TEXT("CloseUp"));
    SpawnSalt = bTest ? 0 : (FMath::Rand() | 1);
    bActive = true;
    InWorld.GetTimerManager().SetTimer(UpdateHandle, FTimerDelegate::CreateUObject(this, &UVortexCreatureSubsystem::Update), .5f, true, 1.f);
}

void UVortexCreatureSubsystem::Deinitialize()
{
    if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(UpdateHandle);
    Super::Deinitialize();
}

int32 UVortexCreatureSubsystem::AliveCount() const
{
    int32 Count = 0;
    for (const FPost& Post : Posts) if (Post.Actor.IsValid()) ++Count;
    return Count;
}

void UVortexCreatureSubsystem::Build()
{
    const TSharedPtr<FVortexTerrainData> Terrain = AVortexTerrainStreamer::SharedData();
    if (!Terrain) return;
    bBuilt = true;
    LoadFixed();
    // os chefes regionais nos covis do continente (RegionalBoss do Godot)
    if (Terrain->HasWorld())
    {
        for (const TPair<FName, FVector2D>& Boss : { TPair<FName, FVector2D>(TEXT("GlassScorpion"), FVector2D(-218000.f, -109000.f)), TPair<FName, FVector2D>(TEXT("RootTitan"), FVector2D(296000.f, 70500.f)) })
        {
            if (Posts.ContainsByPredicate([&](const FPost& P) { return P.Species == Boss.Key; })) continue;
            FPost Post;
            Post.Species = Boss.Key;
            Post.Level = Boss.Key == TEXT("GlassScorpion") ? 35 : 30;
            Post.At = Boss.Value;
            Post.bBoss = true;
            AddPost(Post);
        }
    }
    // semi-chefes (um por região, "ANCESTRAL"): por enquanto com os bichos que já existem, até chegarem os modelos
    // novos do bestiário (Docs/03_Mundo/PROMPTS_MONSTROS_3D.md). O de Valdora fica a ~220 m da cidade, no vale
    {
        struct FElite { int32 Region; const TCHAR* Species; int32 Level; };
        static const FElite Elites[] = {
            { 1, TEXT("Golem"), 12 }, { 6, TEXT("Mushroom"), 20 }, { 9, TEXT("Golem"), 28 }, { 5, TEXT("GoblinBrute"), 34 },
            { 7, TEXT("Frog"), 38 }, { 3, TEXT("Golem"), 44 }, { 2, TEXT("Golem"), 54 }, { 4, TEXT("Bat"), 58 }, { 8, TEXT("BanditBrute"), 68 } };
        for (const FElite& Elite : Elites)
        {
            if (Elite.Region != 1 && !Terrain->HasWorld()) continue;
            const FVector2D Nominal = Elite.Region == 1 || !Terrain->HasWorld() ? FVector2D(-22000.f, 9000.f) : Terrain->RegionCenter(Elite.Region);
            // chão seco em volta do ponto (anéis de 15 m), fora da cidade
            bool bFound = false;
            FVector2D At = Nominal;
            for (int32 Ring = 0; Ring < 24 && !bFound; ++Ring)
            {
                for (int32 Step = 0; Step < FMath::Max(1, Ring * 6) && !bFound; ++Step)
                {
                    const float A = Step * UE_TWO_PI / FMath::Max(1, Ring * 6);
                    const FVector2D Try = Nominal + FVector2D(FMath::Cos(A), FMath::Sin(A)) * Ring * 1500.f;
                    const float Z = Terrain->QuickGround(Try.X, Try.Y);
                    if (Terrain->InMap(Try.X, Try.Y) && Z > Terrain->WaterLevel + 50.f && FVector2D::Distance(Try, FVector2D(0.f, 1500.f)) > 12000.f)
                    {
                        At = Try;
                        bFound = true;
                    }
                }
            }
            if (!bFound) continue;
            FPost Post;
            Post.Species = Elite.Species;
            Post.Level = Elite.Level;
            Post.At = At;
            Post.bElite = true;
            AddPost(Post);
        }
    }
    // as quadras do continente são sorteadas quando alguém chega perto (Update): sortear as ~30 mil de uma vez
    // travava a entrada, e conferir todas a cada meio segundo pesava sempre
    UE_LOG(LogTemp, Display, TEXT("VORTEX_CREATURES %d postos fixos"), Posts.Num());
}

bool UVortexCreatureSubsystem::WanderSpotFree(const FVector2D& At) const
{
    const TSharedPtr<FVortexTerrainData> Terrain = AVortexTerrainStreamer::SharedData();
    if (!Terrain.IsValid() || !Terrain->InMap(At.X, At.Y)) return false;
    const float Ground = Terrain->QuickGround(At.X, At.Y);
    if (Ground < Terrain->WaterLevel || Ground <= -299.f) return false;
    // longe da vila (os mesmos 90 m do sorteio das quadras) e nunca dentro dela
    return FVector2D::Distance(At, FVector2D(0.f, 1500.f)) > 9000.f && !AVortexTown::Contains(FVector(At.X, At.Y, Ground));
}

void UVortexCreatureSubsystem::AddPost(const FPost& Post)
{
    const int32 Index = Posts.Add(Post);
    Posts[Index].Origin = Post.At;
    Grid.FindOrAdd(FIntPoint(FMath::FloorToInt(Post.At.X / CellSize), FMath::FloorToInt(Post.At.Y / CellSize))).Add(Index);
}

void UVortexCreatureSubsystem::Preload()
{
    if (GetWorld()->GetNetMode() == NM_DedicatedServer) return;
    // a malha e os clipes de cada espécie, em segundo plano: carregar na hora em que o bicho nasce travava
    // o quadro (o Colosso de Runas chegava a 600 ms)
    static const TCHAR* Names[] = { TEXT("Slime"), TEXT("Mushroom"), TEXT("Bat"), TEXT("Goblin"), TEXT("Golem"), TEXT("Frog"), TEXT("Deer"), TEXT("Rabbit"),
        TEXT("Horse_Baio"), TEXT("Horse_Tordilho"), TEXT("Horse_Malhado"), TEXT("Horse_Negro"), TEXT("Horse_Palomino"),
        TEXT("RuneColossus"), TEXT("GlassScorpion"), TEXT("RootTitan"), TEXT("Wolf"), TEXT("Spider") };
    IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
    TWeakObjectPtr<UVortexCreatureSubsystem> Self(this);
    TSet<FName> Packages;
    for (const TCHAR* Name : Names)
    {
        TArray<FAssetData> Assets;
        Registry.GetAssetsByPath(FName(*FString::Printf(TEXT("/Game/Vortex/Creatures/%s/%s_Skinned/SkeletalMeshes"), Name, Name)), Assets, true);
        for (const FAssetData& Asset : Assets) Packages.Add(Asset.PackageName);
    }
    // os bandidos são o boneco do aventureiro com roupa sorteada (UVortexDressComponent::BanditLook) e arma do
    // arsenal: sem isto cada um carregava as peças na hora e um acampamento travava o quadro em até 235 ms
    Packages.Add(TEXT("/Game/Vortex/Characters/Adventurer/Adventurer_Skinned/SkeletalMeshes/Adventurer_Skinned"));
    TArray<FString> Pieces = { TEXT("core"), TEXT("face_male"), TEXT("face_female"), TEXT("shirt_mail"), TEXT("shirt_shadow"),
        TEXT("shirt_scout"), TEXT("shirt_punk"), TEXT("shirt_tunic"), TEXT("pants_dark"), TEXT("pants_leather"), TEXT("boots_shadow"),
        TEXT("boots_scout"), TEXT("boots_wrap"), TEXT("boots_punk"), TEXT("accessory_patch"), TEXT("accessory_goggles"),
        TEXT("gear_strap"), TEXT("gear_belt") };
    for (int32 i = 1; i <= 10; ++i) Pieces.Add(FString::Printf(TEXT("hair_male_%d"), i));
    for (int32 i = 1; i <= 9; ++i) Pieces.Add(FString::Printf(TEXT("hair_female_%d"), i));
    for (const FString& Piece : Pieces)
        Packages.Add(FName(*FString::Printf(TEXT("/Game/Vortex/Characters/Adventurer/Wardrobe/%s/SkeletalMeshes/%s"), *Piece, *Piece)));
    // as armas (a definição traz junto o modelo e os clipes dos golpes)
    Packages.Add(TEXT("/Game/Vortex/Weapons/Sword/DA_DefaultSword"));
    for (const TCHAR* Type : { TEXT("espada_escudo"), TEXT("martelo_de_bram"), TEXT("arco_de_betula") })
        Packages.Add(FName(*FString::Printf(TEXT("/Game/Vortex/Weapons/Types/DA_%s"), Type)));
    // a névoa roxa dos monstros (cada um liga uma) e o rastro das flechas dos arqueiros
    Packages.Add(TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Aura_Soul"));
    Packages.Add(TEXT("/Game/ArrowTrail/FX/NS_ArrowTrail_Basic"));
    // o andar dos bandidos (o boneco do aventureiro) e a gota nova (a malha e o rosto): o passeio de 08/11 ainda via estes
    // carregando na hora
    for (const TCHAR* Clip : { TEXT("Idle"), TEXT("Walk"), TEXT("Run") })
        Packages.Add(FName(*FString::Printf(TEXT("/Game/Vortex/Characters/Adventurer/Adventurer_Skinned/SkeletalMeshes/Adventurer_Skinned%s"), Clip)));
    Packages.Add(TEXT("/Game/Vortex/Creatures/SlimeNew/SM_Slime/StaticMeshes/SM_Slime"));
    Packages.Add(TEXT("/Game/Vortex/Creatures/SlimeNew/M_SlimeFace"));
    Packages.Add(TEXT("/Game/Vortex/Characters/Adventurer/Wardrobe/rw_humano_face_01/SkeletalMeshes/rw_humano_face_01"));
    Packages.Add(TEXT("/Game/Vortex/Characters/Adventurer/Wardrobe/mouth_small/SkeletalMeshes/mouth_small"));
    // a Caldeira e o Azhûr (a pedra, a obsidiana, a lava, as paredes): o Colosso ainda travava 1,7 s carregando os materiais
    for (const TCHAR* Folder : { TEXT("/Game/Vortex/Creatures/RuneColossus/Materials"), TEXT("/Game/Vortex/World/Caldeira") })
    {
        TArray<FAssetData> Assets;
        Registry.GetAssetsByPath(FName(Folder), Assets, true);
        for (const FAssetData& Asset : Assets) Packages.Add(Asset.PackageName);
    }
    for (const TCHAR* Extra : { TEXT("/Game/Vortex/Combat/M_LavaToon"), TEXT("/Game/Vortex/Combat/M_SolidColor"), TEXT("/Game/Vortex/Weapons/Models/W_flecha/StaticMeshes/W_flecha") })
        Packages.Add(Extra);
    // 11/11: os moradores da vila (o boneco do herói com cabelo, roupa, olhos e as animações de ofício): carregados um a um
    // quando apareciam, travavam o quadro em até 150 ms no jogo empacotado ao chegar perto da praça
    for (const TCHAR* Folder : { TEXT("/Game/Vortex/Characters/Heroes/humano/Hair"), TEXT("/Game/Vortex/Characters/Heroes/humano/Roupas") })
    {
        TArray<FAssetData> Assets;
        Registry.GetAssetsByPath(FName(Folder), Assets, true);
        for (const FAssetData& Asset : Assets) Packages.Add(Asset.PackageName);
    }
    {
        TArray<FAssetData> Assets;
        Registry.GetAssetsByPath(TEXT("/Game/Vortex/Characters/Heroes/humano"), Assets, false);
        for (const FAssetData& Asset : Assets) if (Asset.AssetName.ToString().StartsWith(TEXT("T_humano_"))) Packages.Add(Asset.PackageName);
        Assets.Reset();
        Registry.GetAssetsByPath(TEXT("/Game/Vortex/Characters/Heroes/Anims/humano"), Assets, false);
        for (const FAssetData& Asset : Assets) if (Asset.AssetName.ToString().Contains(TEXT("_Npc_"))) Packages.Add(Asset.PackageName);
    }
    for (const FName& Package : Packages) RequestPackage(Package);
}

void UVortexCreatureSubsystem::RequestPackage(FName Package)
{
    if (RequestedPackages.Contains(Package)) return;
    RequestedPackages.Add(Package);
    ++PendingPreloads;
    TWeakObjectPtr<UVortexCreatureSubsystem> Self(this);
    LoadPackageAsync(Package.ToString(), FLoadPackageAsyncDelegate::CreateLambda([Self](const FName& Name, UPackage* Loaded, EAsyncLoadingResult::Type)
    {
        if (!Self.IsValid()) return;
        --Self->PendingPreloads;
        Self->ReadyPackages.Add(Name);
        if (!Loaded) return;
        ForEachObjectWithPackage(Loaded, [&Self](UObject* Object)
        {
            // 11/11: tudo o que é asset (antes só malhas animadas, animações, dados e efeitos: as texturas e malhas fixas podiam
            // ser descartadas e voltar "na hora")
            if (Object->IsAsset()) Self->Preloaded.Add(Object);
            return true;
        }, false);
        if (Self->PendingPreloads == 0) UE_LOG(LogTemp, Display, TEXT("VORTEX_PRELOAD pronta (%d pacotes)"), Self->ReadyPackages.Num());
    }));
}

bool UVortexCreatureSubsystem::AssetsReady(FName Species)
{
    // servidor dedicado não desenha: não precisa esperar malha
    if (GetWorld()->GetNetMode() == NM_DedicatedServer) return true;
    if (PendingPreloads > 0) return false;
    const FVortexSpeciesDef* D = FVortexSpeciesDef::Find(Species);
    if (Species == TEXT("Wolf") || (D && !D->bHumanoid && !Species.ToString().StartsWith(TEXT("Slime"))))
    {
        const FString Model = Species == TEXT("Wolf") ? FString(TEXT("Wolf")) : (D && !D->Model.IsNone() ? D->Model.ToString() : Species.ToString());
        const FName Package(*FString::Printf(TEXT("/Game/Vortex/Creatures/%s/%s_Skinned/SkeletalMeshes/%s_Skinned"), *Model, *Model, *Model));
        if (ReadyPackages.Contains(Package)) return true;
        if (FindPackage(nullptr, *Package.ToString()) && !RequestedPackages.Contains(Package)) return true;   // já estava carregado
        RequestPackage(Package);
        return false;
    }
    return true;
}

/** Matilha de lobos: 1, 2 ou 3, sorteado (pedido: "de 1 a 3 aleatório, no 3 sempre um alfa maior"). */
static int32 WolfPackSize(FRandomStream& Rng) { return 1 + Rng.RandRange(0, 2); }

// 12/11 ("os monstros muito colados"): a chance de bando, o espaço entre eles e as espécies de cada região em
// Data/Population/spawn_tuning.json (lido uma vez; sem o arquivo, os valores abaixo)
struct FSpawnTuning
{
    float GroupChance = .36f, SpreadMin = 500.f, SpreadMax = 750.f, BanditCamp = 450.f, NightChance = .22f;
    TArray<TArray<FName>> Regions;
};
static const FSpawnTuning& SpawnTuning()
{
    static const FSpawnTuning Tuning = []
    {
        FSpawnTuning T;
        FString Text;
        TSharedPtr<FJsonObject> Root;
        if (!FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir() / TEXT("Data/Population/spawn_tuning.json"))) ||
            !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root) return T;
        Root->TryGetNumberField(TEXT("group_chance"), T.GroupChance);
        Root->TryGetNumberField(TEXT("bandit_camp_cm"), T.BanditCamp);
        Root->TryGetNumberField(TEXT("night_chance"), T.NightChance);
        const TArray<TSharedPtr<FJsonValue>>* Spread = nullptr;
        if (Root->TryGetArrayField(TEXT("pack_spread_cm"), Spread) && Spread->Num() == 2)
        {
            T.SpreadMin = (*Spread)[0]->AsNumber();
            T.SpreadMax = FMath::Max(T.SpreadMin, float((*Spread)[1]->AsNumber()));
        }
        const TArray<TSharedPtr<FJsonValue>>* Regions = nullptr;
        if (Root->TryGetArrayField(TEXT("regions"), Regions))
            for (const TSharedPtr<FJsonValue>& Region : *Regions)
            {
                TArray<FName>& Kinds = T.Regions.AddDefaulted_GetRef();
                for (const TSharedPtr<FJsonValue>& Kind : Region->AsArray()) Kinds.Add(FName(*Kind->AsString()));
            }
        return T;
    }();
    return Tuning;
}

void UVortexCreatureSubsystem::LoadFixed()
{
    FString Text;
    TSharedPtr<FJsonObject> Root;
    if (!FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir() / TEXT("Data/Population/creature_posts.json"))) ||
        !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root) return;
    const TArray<TSharedPtr<FJsonValue>>& AreaValues = Root->GetArrayField(TEXT("area_cm"));
    if (AreaValues.Num() == 4)
    {
        Area = FBox2D(FVector2D(AreaValues[0]->AsNumber(), AreaValues[1]->AsNumber()), FVector2D(AreaValues[2]->AsNumber(), AreaValues[3]->AsNumber()));
    }
    for (const TSharedPtr<FJsonValue>& Value : Root->GetArrayField(TEXT("posts")))
    {
        const TSharedPtr<FJsonObject> O = Value->AsObject();
        FPost Post;
        Post.Species = FName(*O->GetStringField(TEXT("species")));
        Post.Level = int32(O->GetNumberField(TEXT("level")));
        const TArray<TSharedPtr<FJsonValue>>& At = O->GetArrayField(TEXT("at_cm"));
        Post.At = FVector2D(At[0]->AsNumber(), At[1]->AsNumber());
        O->TryGetBoolField(TEXT("boss"), Post.bBoss);
        // os postos do Godot são de antes das muralhas: dentro da cidade (e 15 m em volta) não nasce bicho. As gotas
        // e cogumelos de nível 1-3 que ficavam ali saem para o anel de fora (80-100 m do centro, na mesma direção,
        // girando até achar chão seco): são os bichos do começo, perto dos portões
        // 06/11: a vila nova é maior (muralha de ~160 m e as hortas fora do portão norte): "dentro" vem da planta
        // (AVortexTown::Contains), com 15 m de folga em volta; antes era a caixa da muralha antiga
        auto InTown = [](const FVector2D& P)
        {
            for (int32 i = 0; i <= 8; ++i)
            {
                // 07/11 (o usuário: "sempre num portão nascem 2 lobos"): 45 m de folga em volta da muralha (eram 15)
                const FVector2D Off = i == 8 ? FVector2D::ZeroVector : FVector2D(4500.f, 0.f).GetRotated(i * 45.f);
                if (AVortexTown::Contains(FVector(P + Off, 0.f))) return true;
            }
            return false;
        };
        const TSharedPtr<FVortexTerrainData> Terrain = AVortexTerrainStreamer::SharedData();
        const FVector2D Center(0.f, 1500.f);
        // o nível do Godot menos a curva do Godot = o quanto este posto é mais forte que os vizinhos (elite, chefe)
        const int32 GodotCurve = FMath::RoundToInt(1.f + FVector2D(Post.At.X / 100.f, Post.At.Y / 100.f - 4.f).Size() / 42.f);
        if (!Post.bBoss && InTown(Post.At))
        {
            const FVector2D Dir = (Post.At - Center).GetSafeNormal();
            bool bDry = false;
            // do lado de fora da muralha nova: anéis de 130 a 190 m do centro, girando até achar chão seco
            for (int32 Ring = 0; Ring < 4 && !bDry; ++Ring)
                for (int32 Turn = 0; Turn < 18 && !bDry; ++Turn)
                {
                    const float Radius = 13000.f + Ring * 2000.f + FVector2D::Distance(Post.At, Center) * .1f;
                    const FVector2D Try = Center + (Dir.IsNearlyZero() ? FVector2D(1.f, 0.f) : Dir).GetRotated(Turn * 20.f) * Radius;
                    bDry = (!Terrain || Terrain->QuickGround(Try.X, Try.Y) > Terrain->WaterLevel + 20.f) && !InTown(Try);
                    if (bDry) Post.At = Try;
                }
            if (!bDry) continue;
        }
        if (!Post.bBoss && Terrain) Post.Level = FMath::Max(1, Terrain->AreaLevel(Post.At.X, Post.At.Y) + FMath::Max(0, Post.Level - GodotCurve));
        // 07/11 ("os monstros nascem sempre nos mesmos lugares"): depois de vencido, volta a 6-20 m de onde nasceu, como os do mato
        Post.bWander = !Post.bBoss;
        // lobo no vale do começo: os postos do Godot têm um lobo só ("sempre que via um lobo era sozinho");
        // vira matilha de 1 a 3 como no resto do mundo (o do posto lidera; com 3, ele é o ALFA, maior)
        if (Post.Species == TEXT("Wolf") && !Post.bBoss)
        {
            FRandomStream Size(HashCombine(GetTypeHash(FMath::RoundToInt(Post.At.X)), GetTypeHash(FMath::RoundToInt(Post.At.Y))));
            const int32 Count = WolfPackSize(Size);
            Post.PackId = Count > 1 ? PackCounter++ : -1; Post.PackSlot = 0; Post.PackSize = Count;
            AddPost(Post);
            for (int32 Slot = 1; Slot < Count; ++Slot)
            {
                FPost Mate = Post;
                const float A = Slot * PI + FMath::Frac(Post.At.X * .001f) * 2.f;
                Mate.At = Post.At + FVector2D(FMath::Cos(A), FMath::Sin(A)) * 300.f;
                Mate.PackSlot = Slot;
                if (!Terrain || Terrain->QuickGround(Mate.At.X, Mate.At.Y) > Terrain->WaterLevel + 20.f) AddPost(Mate);
            }
            continue;
        }
        AddPost(Post);
    }
}

// BanditCampAt: VortexWorldLayout::BanditCampAt (módulo VortexWorld)

bool UVortexCreatureSubsystem::NearestBanditCamp(const FVector2D& From, int32 Rings, FVector2D& OutAt)
{
    const FIntPoint Home(FMath::FloorToInt(From.X / CellSize), FMath::FloorToInt(From.Y / CellSize));
    float Best = TNumericLimits<float>::Max();
    for (int32 DX = -Rings; DX <= Rings; ++DX) for (int32 DY = -Rings; DY <= Rings; ++DY)
    {
        FVector2D At;
        if (BanditCampAt(Home + FIntPoint(DX, DY), At) && FVector2D::DistSquared(At, From) < Best) { Best = FVector2D::DistSquared(At, From); OutAt = At; }
    }
    return Best < TNumericLimits<float>::Max();
}

void UVortexCreatureSubsystem::GenerateCell(const FIntPoint& CellKey)
{
    // 12/11: -VortexQuietWorld (filmagem): nenhum bicho do mundo nasce; os do /spawn e os chefes fixos continuam
    static const bool bQuiet = FParse::Param(FCommandLine::Get(), TEXT("VortexQuietWorld"));
    if (bQuiet) return;
    const TSharedPtr<FVortexTerrainData> Terrain = AVortexTerrainStreamer::SharedData();
    const FBox2D Bounds = Terrain->MapBounds();
    constexpr float Region = CellSize;   // quadras de 72 m, como o Godot
    const FVector2D Town(0.f, 1500.f);
    auto Free = [&Terrain, &Town](float X, float Y, float TownGap)
    {
        float Surface = 0.f;
        // o continente: só o relevo (montar os pedaços do mundo todo travaria a entrada)
        const float Ground = Terrain->QuickGround(X, Y);
        if (!Terrain->InMap(X, Y) || Ground < Terrain->WaterLevel || Ground <= -299.f) return false;
        if (FVector2D::Distance(FVector2D(X, Y), Town) < TownGap) return false;
        return !AVortexTown::Contains(FVector(X, Y, Ground));
    };
    auto NearWater = [&Terrain](float X, float Y)
    {
        float Surface = 0.f;
        for (int32 i = 0; i < 8; ++i)
        {
            const float A = i * UE_TWO_PI / 8.f;
            if (Terrain->QuickGround(X + FMath::Cos(A) * 1500.f, Y + FMath::Sin(A) * 1500.f) < Terrain->WaterLevel) return true;
        }
        return false;
    };
    {
        {
            const int32 CellX = CellKey.X, CellY = CellKey.Y;
            const float X0 = CellX * Region, Y0 = CellY * Region;
            if (X0 + Region < Bounds.Min.X || X0 > Bounds.Max.X || Y0 + Region < Bounds.Min.Y || Y0 > Bounds.Max.Y) return;
            const float Half = Region * .5f;
            // animais: coelhos em dupla/trio, cervos em pequenos rebanhos, cavalos em manadas. Cada região tem a sua
            // fauna: quanto aparece, a mistura (cavalo/cervo/coelho), o tamanho da manada e as pelagens dos cavalos.
            // (Antes era igual no mapa todo: 42% das quadras, até no vulcão, na neve e na terra dos mortos.)
            struct FFauna { float Chance, Horse, Deer; int32 HerdExtra; TArray<const TCHAR*> Coats; };
            static const TArray<const TCHAR*> AllCoats = { TEXT("Horse_Baio"), TEXT("Horse_Tordilho"), TEXT("Horse_Malhado"), TEXT("Horse_Negro"), TEXT("Horse_Palomino") };
            // 0 mar, 1 Valdora, 2 Brasafenda, 3 Raikor, 4 Nivaluz, 5 Solcânion, 6 Aurivale, 7 Lodonegro, 8 Morthúria, 9 Verdalume
            static const FFauna Faunas[10] = {
                { 0.f, 0.f, 0.f, 0, {} },
                { .42f, .15f, .40f, 0, AllCoats },                                              // campos e mata do vale
                { .04f, 0.f, 0.f, 0, {} },                                                      // vulcão: só um coelho perdido
                { .28f, .20f, .55f, 0, { TEXT("Horse_Tordilho"), TEXT("Horse_Negro") } },        // montanhas: cervos e cavalos escuros
                { .18f, 0.f, .50f, 0, {} },                                                     // neve: cervos e lebres
                { .10f, .30f, 0.f, 0, { TEXT("Horse_Palomino"), TEXT("Horse_Baio") } },          // deserto: lebres e cavalos claros, raros
                { .55f, .40f, .30f, 2, { TEXT("Horse_Palomino"), TEXT("Horse_Baio"), TEXT("Horse_Malhado") } },   // planícies douradas: manadas grandes
                { .20f, 0.f, .35f, 0, {} },                                                     // pântano: poucos
                { 0.f, 0.f, 0.f, 0, {} },                                                       // terra dos mortos: nenhum
                { .50f, 0.f, .70f, 1, {} },                                                     // mata de Verdalume: muitos cervos
            };
            auto Salted = [this](uint32 Hash) { return SpawnSalt ? HashCombine(Hash, uint32(SpawnSalt)) : Hash; };
            FRandomStream Animals(Salted(HashCombine(GetTypeHash(CellX * 7717), GetTypeHash(CellY + 31))));
            int32 AnimalCell = -1;
            const float CellChance = Animals.FRand();
            const int32 FaunaRegion = Terrain->HasWorld() ? FMath::Clamp(Terrain->RegionAt(X0 + Half, Y0 + Half), 0, 9) : 1;
            const FFauna& Fauna = Faunas[FaunaRegion];
            if (CellChance < Fauna.Chance)
            {
                AnimalCell = Animals.RandRange(0, 3);
                const float X = X0 + (AnimalCell % 2) * Half + 800.f + Animals.FRand() * (Half - 1600.f);
                const float Y = Y0 + (AnimalCell / 2) * Half + 800.f + Animals.FRand() * (Half - 1600.f);
                if (Free(X, Y, 6000.f))
                {
                    const float Roll = Animals.FRand();
                    const bool bHorse = Roll < Fauna.Horse && Fauna.Coats.Num() > 0, bDeer = !bHorse && Roll < Fauna.Horse + Fauna.Deer;
                    const int32 Count = (bHorse ? 2 + Animals.RandRange(0, 1) : bDeer ? 1 + Animals.RandRange(0, 2) : 2 + Animals.RandRange(0, 1)) + Fauna.HerdExtra;
                    const TArray<const TCHAR*>& Coats = Fauna.Coats.Num() ? Fauna.Coats : AllCoats;
                    const int32 Pack = PackCounter++;
                    for (int32 K = 0; K < Count; ++K)
                    {
                        const float A = K * 2.4f + Animals.FRand(), D = K == 0 ? 0.f : 200.f + Animals.FRand() * 300.f;
                        FPost Post;
                        Post.Species = bHorse ? FName(Coats[Animals.RandRange(0, Coats.Num() - 1)]) : bDeer ? FName(TEXT("Deer")) : FName(TEXT("Rabbit"));
                        Post.At = FVector2D(X + FMath::Cos(A) * D, Y + FMath::Sin(A) * D);
                        Post.PackId = Pack; Post.PackSlot = K; Post.PackSize = Count;
                        Post.bWander = true;
                        if (Free(Post.At.X, Post.At.Y, 6000.f)) AddPost(Post);
                    }
                }
            }
            // bando de bandidos (ValleyPopulation.GenerateBandits do Godot): 3 a 4 em volta da fogueira,
            // o primeiro é o capitão (espada); os outros sorteados: escudo, ladrão, brutamontes, batedor
            FVector2D Camp;
            if (BanditCampAt(CellKey, Camp))
            {
                FRandomStream Band(HashCombine(GetTypeHash(CellX * 9104), GetTypeHash(CellY + 9104)));
                const int32 Count = 3 + Band.RandRange(0, 1);
                const int32 Pack = PackCounter++;
                const int32 Level = FMath::Max(2, Terrain->AreaLevel(Camp.X, Camp.Y));
                for (int32 K = 0; K < Count; ++K)
                {
                    const float A = K * UE_TWO_PI / Count + Band.FRand() * .5f, D = K == 0 ? 0.f : SpawnTuning().BanditCamp;
                    FPost Post;
                    const int32 Roll = Band.RandRange(0, 9);
                    Post.Species = K == 0 ? FName(TEXT("Bandit")) : Roll < 4 ? (K % 2 == 0 ? FName(TEXT("BanditShield")) : FName(TEXT("Bandit")))
                        : Roll < 7 ? FName(TEXT("BanditBrute")) : FName(TEXT("BanditArcher"));
                    Post.Level = Level;
                    Post.LookSeed = HashCombine(GetTypeHash(CellX), GetTypeHash(CellY * 31 + K));
                    Post.At = FVector2D(Camp.X + FMath::Cos(A) * D, Camp.Y + FMath::Sin(A) * D);
                    if (K > 0 && !Free(Post.At.X, Post.At.Y, 4000.f)) Post.At = FVector2D(Camp.X + (K - 2) * 150.f, Camp.Y);
                    Post.PackId = Pack; Post.PackSlot = K; Post.PackSize = Count;
                    AddPost(Post);
                }
            }
            // guardas dos pontos de interesse: goblins na boca da mina; mortos no cemitério (um de dia, mais à noite)
            FVortexPoi Poi;
            if (AVortexPointsOfInterest::PoiAt(CellKey, Poi) && (Poi.Kind == EVortexPoi::Mine || Poi.Kind == EVortexPoi::Graveyard))
            {
                FRandomStream Guard(HashCombine(GetTypeHash(CellX * 3301), GetTypeHash(CellY + 3301)));
                const bool bMine = Poi.Kind == EVortexPoi::Mine;
                const int32 Count = bMine ? 3 + Guard.RandRange(0, 1) : 4;
                const int32 Pack = PackCounter++;
                const int32 Level = FMath::Min(95, Terrain->AreaLevel(Poi.At.X, Poi.At.Y) + (bMine ? 1 : 2));
                for (int32 K = 0; K < Count; ++K)
                {
                    FPost Post;
                    if (bMine) Post.Species = K == 0 ? FName(TEXT("Goblin")) : Guard.FRand() < .45f ? FName(TEXT("GoblinBrute")) : FName(TEXT("GoblinScout"));
                    else Post.Species = K == 3 ? FName(TEXT("UndeadArcher")) : FName(TEXT("Undead"));
                    Post.Level = Level;
                    Post.LookSeed = HashCombine(GetTypeHash(CellX * 7), GetTypeHash(CellY * 13 + K));
                    const float A = K * UE_TWO_PI / Count;
                    Post.At = bMine ? AVortexPointsOfInterest::LocalToWorld(Poi, FMath::Cos(A) * 2.5f, -6.f + FMath::Sin(A) * 2.f)
                        : AVortexPointsOfInterest::LocalToWorld(Poi, FMath::Cos(A) * 3.f, FMath::Sin(A) * 2.f);
                    Post.PackId = Pack; Post.PackSlot = K; Post.PackSize = Count;
                    Post.bNight = !bMine && K > 0;
                    AddPost(Post);
                }
            }
            // monstros dos portais: aranhas e esqueletos em volta de cada portal espalhado; quanto mais difícil o
            // ranque, mais deles e mais fortes (o vermelho, S, tem 7 e +15 níveis)
            {
                FVector2D GateSpot;
                FString GateRank;
                float GateYaw = 0.f;
                if (AVortexPointsOfInterest::GateAt(CellKey, GateSpot, GateRank, GateYaw))
                {
                    const int32 Tier = FMath::Max(0, FString(TEXT("EDCBAS")).Find(GateRank));
                    const int32 Count = 2 + Tier;
                    const int32 Pack = PackCounter++;
                    const int32 Level = FMath::Min(95, Terrain->AreaLevel(GateSpot.X, GateSpot.Y) + Tier * 3);
                    for (int32 K = 0; K < Count; ++K)
                    {
                        FPost Post;
                        // aranhas fora do mundo por enquanto ("aranhas penduradas no céu"): só esqueletos guardam o portal
                        Post.Species = bWorldSpiders && K % 3 == 0 ? FName(TEXT("Spider")) : K % 2 == 0 ? FName(TEXT("Skeleton")) : FName(TEXT("SkeletonArcher"));
                        Post.Level = Level;
                        Post.LookSeed = HashCombine(GetTypeHash(CellX * 17), GetTypeHash(CellY * 23 + K));
                        const float A = K * UE_TWO_PI / Count;
                        Post.At = GateSpot + FVector2D(FMath::Cos(A), FMath::Sin(A)) * (700.f + 150.f * (K % 2));
                        Post.PackId = Pack; Post.PackSlot = K; Post.PackSize = Count;
                        AddPost(Post);
                    }
                }
            }
            // aranhas das teias: penduradas no fio em cima das teias e uma andando por perto
            // (desligadas por enquanto: no campo aberto o fio não tem onde prender e elas ficavam penduradas no céu)
            if (bWorldSpiders)
            {
                TArray<AVortexPointsOfInterest::FWebSpot> WebSpots;
                if (AVortexPointsOfInterest::WebsAt(CellKey, WebSpots))
                {
                    const int32 Pack = PackCounter++;
                    const int32 Level = FMath::Min(95, Terrain->AreaLevel(WebSpots[0].At.X, WebSpots[0].At.Y) + 1);
                    const int32 Count = FMath::Min(3, WebSpots.Num());
                    for (int32 K = 0; K < Count; ++K)
                    {
                        FPost Post;
                        Post.Species = TEXT("Spider");
                        Post.Level = Level;
                        Post.At = WebSpots[K].At + FVector2D(120.f, 0.f).GetRotated(WebSpots[K].Yaw);
                        Post.bHang = K > 0;
                        Post.PackId = Pack; Post.PackSlot = K; Post.PackSize = Count;
                        AddPost(Post);
                    }
                }
            }
            // dentro da área feita à mão quem manda são os postos fixos
            // bichos da noite (19h às 5h, somem de manhã): bandos 2 níveis acima da região. Por enquanto os bichos
            // que já existem (morcegos, lobos, sapos, golens), até chegarem os noturnos novos do bestiário
            {
                FRandomStream Dark(Salted(HashCombine(GetTypeHash(CellX * 6151), GetTypeHash(CellY - 911))));
                if (Dark.FRand() < SpawnTuning().NightChance)
                {
                    static const TArray<TArray<const TCHAR*>> NightKinds = {
                        {}, { TEXT("Bat"), TEXT("Wolf") }, { TEXT("Bat") }, { TEXT("Wolf"), TEXT("Bat") }, { TEXT("Wolf") }, { TEXT("Bat") },
                        { TEXT("Bat"), TEXT("Wolf") }, { TEXT("Frog"), TEXT("Bat") }, { TEXT("Bat"), TEXT("Golem") }, { TEXT("Wolf") } };
                    const float X = X0 + Half + (Dark.FRand() - .5f) * (Half * 1.4f);
                    const float Y = Y0 + Half + (Dark.FRand() - .5f) * (Half * 1.4f);
                    const int32 Home = Terrain->HasWorld() ? FMath::Clamp(Terrain->RegionAt(X, Y), 0, 9) : 1;
                    if (NightKinds[Home].Num() && Free(X, Y, 9000.f))
                    {
                        const FName Kind(NightKinds[Home][Dark.RandRange(0, NightKinds[Home].Num() - 1)]);
                        const int32 Count = Kind == TEXT("Golem") ? 1 : Kind == TEXT("Wolf") ? WolfPackSize(Dark) : 2;
                        const int32 Pack = Count > 1 ? PackCounter++ : -1;
                        for (int32 Slot = 0; Slot < Count; ++Slot)
                        {
                            const float A = Slot * UE_TWO_PI / Count + Dark.FRand();
                            FPost Post;
                            Post.Species = Kind;
                            Post.Level = FMath::Min(95, Terrain->AreaLevel(X, Y) + 2);
                            const float D = FMath::Lerp(SpawnTuning().SpreadMin, SpawnTuning().SpreadMax, Dark.FRand());
                            Post.At = Slot == 0 ? FVector2D(X, Y) : FVector2D(X + FMath::Cos(A) * D, Y + FMath::Sin(A) * D);
                            Post.PackId = Pack; Post.PackSlot = Slot; Post.PackSize = Count;
                            Post.bNight = true;
                            Post.bWander = true;
                            if (Free(Post.At.X, Post.At.Y, 9000.f)) AddPost(Post);
                        }
                    }
                }
            }
            const FVector2D Center(X0 + Half, Y0 + Half);
            if (Area.bIsValid && Area.IsInside(Center)) return;
            FRandomStream Rng(Salted(HashCombine(GetTypeHash(CellX * 4099), GetTypeHash(CellY - 77))));
            for (int32 Cell = 0; Cell < 4; ++Cell)
            {
                if (Cell == AnimalCell || Rng.FRand() > SpawnTuning().GroupChance) continue;
                const float X = X0 + (Cell % 2) * Half + 700.f + Rng.FRand() * (Half - 1400.f);
                const float Y = Y0 + (Cell / 2) * Half + 700.f + Rng.FRand() * (Half - 1400.f);
                if (!Free(X, Y, 9000.f)) continue;
                const float Distance = FVector2D::Distance(FVector2D(X, Y), Town);
                // no vale exportado, como sempre; no continente, as criaturas e o nível de cada região (WorldMap.Species/LevelAt)
                const bool bValley = Terrain->IsExported(FIntPoint(FMath::FloorToInt(X / Terrain->ChunkSize()), FMath::FloorToInt(Y / Terrain->ChunkSize())));
                // o nível sobe com a distância da vila inicial (1 + 1 a cada 42 m, como o Godot); no continente a faixa da região é o piso
                const int32 Level = Terrain->AreaLevel(X, Y);
                const float Roll = Rng.FRand();
                FName Species = Roll < .26f ? TEXT("Slime") : Roll < .42f ? TEXT("Wolf") : Roll < .57f ? TEXT("Mushroom") : Roll < .7f ? TEXT("Bat")
                    : Roll < .88f ? TEXT("Goblin") : TEXT("Golem");
                if (!bValley)
                {
                    const TArray<TArray<FName>>& ByRegion = SpawnTuning().Regions;
                    const int32 Home = Terrain->RegionAt(X, Y);
                    if (ByRegion.IsValidIndex(Home) && ByRegion[Home].Num()) Species = ByRegion[Home][FMath::Min(int32(Roll * ByRegion[Home].Num()), ByRegion[Home].Num() - 1)];
                    // a gota da região: de brasa em Brasafenda, de ferro em Raikor e Solcânion, peçonhenta em Lodonegro e
                    // Morthúria (a de jade no resto)
                    if (Species == TEXT("Slime"))
                        Species = Home == 2 ? FName(TEXT("SlimeFire")) : Home == 3 || Home == 5 ? FName(TEXT("SlimeIron"))
                            : Home == 7 || Home == 8 ? FName(TEXT("SlimePoison")) : Species;
                }
                // brejo: perto da água o sapo-gigante aparece no lugar da gota ou do cogumelo
                if ((Species == TEXT("Slime") || Species == TEXT("Mushroom")) && NearWater(X, Y) && Rng.FRand() < .6f) Species = TEXT("Frog");
                const int32 Companions = Species == TEXT("Wolf") ? WolfPackSize(Rng) - 1 : Species == TEXT("Goblin") ? 2 + Rng.RandRange(0, 1) : Species.ToString().StartsWith(TEXT("Slime")) && Rng.FRand() < .5f ? 1 : 0;
                const int32 Pack = Companions > 0 ? PackCounter++ : -1;
                for (int32 Slot = 0; Slot <= Companions; ++Slot)
                {
                    const float A = Slot * UE_TWO_PI / FMath::Max(1, Companions) + Rng.FRand();
                    FPost Post;
                    Post.Species = Species;
                    // bando de goblins: o chefe é o Goblin; os outros sorteiam entre o comum, o brutamontes e o ladino
                    if (Species == TEXT("Goblin") && Slot > 0)
                    {
                        const float Kind = Rng.FRand();
                        Post.Species = Kind < .3f ? FName(TEXT("GoblinBrute")) : Kind < .65f ? FName(TEXT("GoblinScout")) : FName(TEXT("Goblin"));
                    }
                    Post.Level = Level;
                    Post.LookSeed = HashCombine(GetTypeHash(CellX * 29 + Cell), GetTypeHash(CellY * 37 + Slot));   // os mortos de Morthúria, cada um de um jeito
                    const float D = FMath::Lerp(SpawnTuning().SpreadMin, SpawnTuning().SpreadMax, Rng.FRand());
                    Post.At = Slot == 0 ? FVector2D(X, Y) : FVector2D(X + FMath::Cos(A) * D, Y + FMath::Sin(A) * D);
                    Post.PackId = Pack; Post.PackSlot = Slot; Post.PackSize = Companions + 1;
                    Post.bWander = true;
                    if (Free(Post.At.X, Post.At.Y, 9000.f)) AddPost(Post);
                }
            }
        }
    }
}

AActor* UVortexCreatureSubsystem::Spawn(FPost& Post, float GroundZ)
{
    UWorld* World = GetWorld();
    const FVector At(Post.At.X, Post.At.Y, GroundZ + 150.f);
    const FRotator Facing(0.f, FMath::FRandRange(0.f, 360.f), 0.f);
    const FTransform Pose(Facing, At);
    if (Post.Species == TEXT("Wolf"))
    {
        AVortexWolf* Wolf = World->SpawnActorDeferred<AVortexWolf>(AVortexWolf::StaticClass(), Pose, nullptr, nullptr,
            ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
        if (!Wolf) return nullptr;
        Wolf->Level = Post.Level;
        Wolf->SetHome(At);
        if (Post.PackId >= 0) Wolf->SetPack(Post.PackId, Post.PackSlot);
        // matilha de 3: o líder é o ALFA (maior, mais vida e mordida mais forte)
        Wolf->bAlpha = Post.PackSize >= 3 && Post.PackSlot == 0;
        Wolf->FinishSpawning(Pose);
        return Wolf;
    }
    AVortexMonster* Monster = nullptr;
    {
        VORTEX_PROBE("Nascer.Ator");
        Monster = World->SpawnActorDeferred<AVortexMonster>(AVortexMonster::StaticClass(), Pose, nullptr, nullptr,
            ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
    }
    if (!Monster) return nullptr;
    Monster->SetLookSeed(Post.LookSeed);
    if (Post.bHang) Monster->SetHanging(true);
    if (Post.bElite) Monster->MakeElite();
    {
        VORTEX_PROBE("Nascer.Especie");
        Monster->Setup(Post.Species, Post.Level, Post.PackId, Post.PackSlot, Post.PackSize);
    }
    {
        VORTEX_PROBE("Nascer.Terminar");
        Monster->FinishSpawning(Pose);
    }
    return Monster;
}

void UVortexCreatureSubsystem::Update()
{
    VORTEX_PROBE("Criaturas.Postos");
    if (!bActive) return;
    if (!bBuilt) { Build(); if (!bBuilt) return; }
    UWorld* World = GetWorld();
    AVortexTerrainStreamer* Streamer = nullptr;
    if (TActorIterator<AVortexTerrainStreamer> It(World); It) { Streamer = *It; }
    if (!Streamer) return;
    TArray<FVector2D> Players;
    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
    {
        if (const APawn* Pawn = It->Get() ? It->Get()->GetPawn() : nullptr) Players.Add(FVector2D(Pawn->GetActorLocation()));
    }
    const double Now = World->GetTimeSeconds();
    const AVortexGameState* Clock = World->GetGameState<AVortexGameState>();
    const bool bNight = Clock && (Clock->GetHour() >= 19.f || Clock->GetHour() < 5.f);
    // só as quadras em volta de cada jogador (sorteadas na primeira vez, poucas por vez) e os postos com bicho
    const int32 Reach = FMath::CeilToInt(DematerializeRadius / CellSize);
    TSet<int32> Check = Live;
    int32 NewCells = 0;
    for (const FVector2D& P : Players)
    {
        const FIntPoint Home(FMath::FloorToInt(P.X / CellSize), FMath::FloorToInt(P.Y / CellSize));
        for (int32 DX = -Reach; DX <= Reach; ++DX)
        {
            for (int32 DY = -Reach; DY <= Reach; ++DY)
            {
                const FIntPoint Key(Home.X + DX, Home.Y + DY);
                if (!Generated.Contains(Key))
                {
                    if (NewCells >= 6) continue;
                    Generated.Add(Key);
                    GenerateCell(Key);
                    ++NewCells;
                }
                if (const TArray<int32>* InCell = Grid.Find(Key)) Check.Append(*InCell);
            }
        }
    }
    // 08/11: o gerente de importância (como o Significance Manager dos jogos grandes): quem está longe de todo mundo
    // pensa e anda menos vezes por segundo; perto ou brigando, a cada quadro. O ator e o movimento no mesmo passo (o
    // movimento soma o tempo que passou: a velocidade não muda)
    static const bool bNoSignificance = FParse::Param(FCommandLine::Get(), TEXT("VortexNoSignificance"));
    if (!bNoSignificance)
        for (const int32 Index : Live)
            if (AActor* Actor = Posts.IsValidIndex(Index) ? Posts[Index].Actor.Get() : nullptr)
            {
                float Nearest = 1e12f;
                for (const FVector2D& P : Players) Nearest = FMath::Min(Nearest, FVector2D::Distance(P, FVector2D(Actor->GetActorLocation())));
                const IVortexCreature* Creature = Cast<IVortexCreature>(Actor);
                const float Interval = (Creature && Creature->IsEngaged()) || Nearest < 3000.f ? 0.f : Nearest < 6000.f ? .033f : Nearest < 12000.f ? .1f : .25f;
                if (!FMath::IsNearlyEqual(Actor->GetActorTickInterval(), Interval))
                {
                    Actor->SetActorTickInterval(Interval);
                    if (const ACharacter* Character = Cast<ACharacter>(Actor))
                        if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement()) Movement->SetComponentTickInterval(Interval);
                }
            }
    int32 Spawns = 0;
    for (const int32 Index : Check)
    {
        FPost& Post = Posts[Index];
        float Nearest = 1e12f;
        for (const FVector2D& P : Players) Nearest = FMath::Min(Nearest, FVector2D::Distance(P, Post.At));
        if (AActor* Actor = Post.Actor.Get())
        {
            const IVortexCreature* Creature = Cast<IVortexCreature>(Actor);
            if (Creature && Creature->IsCreatureDead())
            {
                // vencido: volta depois (o ator some sozinho após a animação)
                if (Post.DeadUntil <= 0.0)
                {
                    Post.DeadUntil = Now + (Post.bBoss ? 300.0 : Post.bElite ? 900.0 : FMath::FRandRange(60.f, 120.f));
                    ++Post.Deaths;
                }
                continue;
            }
            // amanheceu: os bichos da noite vão embora (menos os que estão brigando)
            if (Post.bNight && !bNight && !(Creature && Creature->IsEngaged()))
            {
                Actor->Destroy();
                Post.Actor = nullptr;
                if (Post.DeadUntil <= 0.0) Live.Remove(Index);
                continue;
            }
            // todos longe (e ninguém brigando com ele): some, e o posto volta inteiro depois
            if (Nearest > DematerializeRadius && !(Creature && Creature->IsEngaged()))
            {
                Actor->Destroy();
                Post.Actor = nullptr;
                if (Post.DeadUntil <= 0.0) Live.Remove(Index);
            }
            continue;
        }
        if (Now < Post.DeadUntil) continue;
        Post.DeadUntil = 0.0;
        Live.Remove(Index);
        // do mato e já vencido: volta noutro ponto, a 6-20 m de onde nasceu (o bando inteiro anda o mesmo tanto)
        if (Post.bWander && Post.Deaths > Post.MovedFor)
        {
            Post.MovedFor = Post.Deaths;
            FRandomStream Move(HashCombine(GetTypeHash(Post.PackId >= 0 ? Post.PackId : Index), GetTypeHash(Post.Deaths * 7919 + SpawnSalt)));
            for (int32 Try = 0; Try < 6; ++Try)
            {
                const float Angle = Move.FRand() * UE_TWO_PI, Distance = 600.f + Move.FRand() * 1400.f;
                const FVector2D To = Post.Origin + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Distance;
                if (WanderSpotFree(To)) { Post.At = To; break; }
            }
        }
        if (Post.bNight && !bNight) continue;
        if (Nearest > MaterializeRadius || Spawns >= SpawnsPerUpdate) continue;
        if (!AssetsReady(Post.Species)) continue;   // 08/11: a malha ainda chegando: nasce no próximo giro, sem travar
        float Ground = 0.f;
        if (!Streamer->GroundReadyAt(Post.At.X, Post.At.Y, Ground)) continue;
        VORTEX_PROBE("Criaturas.Nascer");
        const double SpawnStart = FPlatformTime::Seconds();
        AActor* Spawned = Spawn(Post, Ground);
        const double SpawnMs = (FPlatformTime::Seconds() - SpawnStart) * 1000.0;
        if (SpawnMs > 30.0) UE_LOG(LogTemp, Display, TEXT("VORTEX_SLOW_SPAWN %s %.0f ms"), *Post.Species.ToString(), SpawnMs);
        if (Spawned)
        {
            Post.Actor = Spawned;
            Live.Add(Index);
            ++Spawns;
        }
    }
}

int32 UVortexCreatureSubsystem::DebugSpawnNear(FName Species, const FVector& Where, int32 Level)
{
    FPost Post;
    Post.Species = Species;
    Post.Level = Level;
    Post.At = FVector2D(Where);
    float Ground = Where.Z - 150.f;
    if (TActorIterator<AVortexTerrainStreamer> It(GetWorld()); It) { It->GroundReadyAt(Where.X, Where.Y, Ground); }
    AActor* Actor = Spawn(Post, Ground);
    if (!Actor) return 0;
    Post.Actor = Actor;
    Post.DeadUntil = 1e12;   // de teste: não volta
    Live.Add(Posts.Add(Post));
    return 1;
}
