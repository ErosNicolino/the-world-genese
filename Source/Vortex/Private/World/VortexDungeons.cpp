#include "World/VortexDungeons.h"

#include "Components/PointLightComponent.h"
#include "Creatures/VortexMonster.h"
#include "Creatures/VortexMonsterSpecies.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/VortexCharacter.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "Rpg/VortexHeroSheet.h"
#include "World/VortexPointsOfInterest.h"
#include "World/VortexPortal.h"
#include "World/VortexTerrainData.h"
#include "World/VortexTerrainStreamer.h"
#include "World/VortexWorldFeatures.h"

FString VortexDungeonRules::Name(int32 Rule)
{
    switch (Rule)
    {
    case NoPotions: return TEXT("Sem poções");
    case Darkness: return TEXT("Escuridão");
    case NoRegen: return TEXT("Sem regeneração");
    case Fragile: return TEXT("Vida frágil (-25%)");
    case Fury: return TEXT("Inimigos em fúria (+30% dano)");
    case Swift: return TEXT("Inimigos rápidos");
    case Blessing: return TEXT("Bênção do portal (+50% XP)");
    case Luck: return TEXT("Sorte (mais saque)");
    case TimeTrial: return TEXT("Desafio do tempo (10 min)");
    case Echo: return TEXT("Eco dos heróis (cura ao vencer)");
    default: return FString();
    }
}

bool VortexDungeonRules::IsBonus(int32 Rule) { return Rule >= Blessing; }

namespace
{
    FLinearColor DHex(const TCHAR* Code, float Shade = 1.f)
    {
        const FLinearColor C = FLinearColor(FColor::FromHex(Code)) * Shade;
        return FLinearColor(C.R, C.G, C.B, 1.f);
    }

    /** Blocos alinhados aos eixos, em cm do mundo. */
    struct FDungeonBlocks
    {
        TArray<FVector> Vertices, Normals;
        TArray<int32> Triangles;
        TArray<FLinearColor> Colors;
        void Box(const FVector& Center, const FVector& Size, const FLinearColor& Color)
        {
            const FVector H = Size * .5f;
            static const FVector Dirs[6] = { FVector(1, 0, 0), FVector(-1, 0, 0), FVector(0, 1, 0), FVector(0, -1, 0), FVector(0, 0, 1), FVector(0, 0, -1) };
            for (const FVector& N : Dirs)
            {
                // os dois eixos da face
                const FVector A = FMath::Abs(N.Z) > .5f ? FVector(1, 0, 0) : FVector(0, 0, 1);
                const FVector B = FVector::CrossProduct(N, A);
                const FVector Mid = Center + N * H;
                const FVector EA = A * H, EB = B * H;
                const int32 Base = Vertices.Num();
                const FVector P[4] = { Mid - EA - EB, Mid + EA - EB, Mid + EA + EB, Mid - EA + EB };
                for (const FVector& V : P) { Vertices.Add(V); Normals.Add(N); Colors.Add(Color); }
                const bool bFlip = FVector::DotProduct(FVector::CrossProduct(P[1] - P[0], P[2] - P[0]), N) > 0.f;
                if (bFlip) Triangles.Append({ Base, Base + 2, Base + 1, Base, Base + 3, Base + 2 });
                else Triangles.Append({ Base, Base + 1, Base + 2, Base, Base + 2, Base + 3 });
            }
        }
    };

    struct FBiomeLook { FLinearColor Floor, Floor2, Wall, Wall2, Ceiling, Accent, Light; };
    FBiomeLook LookOf(EVortexDungeonBiome Biome)
    {
        switch (Biome)
        {
        case EVortexDungeonBiome::Crystal:     return { DHex(TEXT("3a3448")), DHex(TEXT("2e2a3c")), DHex(TEXT("4a4260")), DHex(TEXT("3c3550")), DHex(TEXT("2a2638")), DHex(TEXT("b07aff")), FLinearColor(.7f, .45f, 1.f) };
        case EVortexDungeonBiome::Mine:        return { DHex(TEXT("5a4a3c")), DHex(TEXT("4a3c30")), DHex(TEXT("6a5e52")), DHex(TEXT("5a5048")), DHex(TEXT("3e342c")), DHex(TEXT("e8c060")), FLinearColor(1.f, .7f, .35f) };
        case EVortexDungeonBiome::Flooded:     return { DHex(TEXT("4a5a58")), DHex(TEXT("3a4a48")), DHex(TEXT("566a66")), DHex(TEXT("4a5e4a")), DHex(TEXT("344440")), DHex(TEXT("3aa0c8")), FLinearColor(.45f, .8f, 1.f) };
        case EVortexDungeonBiome::SpiderNest:  return { DHex(TEXT("3a3238")), DHex(TEXT("2e282e")), DHex(TEXT("4a4048")), DHex(TEXT("e6e2d8")), DHex(TEXT("2a242a")), DHex(TEXT("9aff6a")), FLinearColor(.6f, 1.f, .5f) };
        case EVortexDungeonBiome::Catacombs:   return { DHex(TEXT("6a6458")), DHex(TEXT("5a554a")), DHex(TEXT("7a7262")), DHex(TEXT("665e50")), DHex(TEXT("4a453c")), DHex(TEXT("ffcf6a")), FLinearColor(1.f, .75f, .4f) };
        case EVortexDungeonBiome::Jungle:      return { DHex(TEXT("3a5a2e")), DHex(TEXT("2e4a24")), DHex(TEXT("4a5e3c")), DHex(TEXT("3e6a2e")), DHex(TEXT("24361c")), DHex(TEXT("7affb0")), FLinearColor(.5f, 1.f, .7f) };
        case EVortexDungeonBiome::Swamp:       return { DHex(TEXT("3e4a30")), DHex(TEXT("34402a")), DHex(TEXT("4a5438")), DHex(TEXT("3e4a2e")), DHex(TEXT("262e1c")), DHex(TEXT("8adf3a")), FLinearColor(.65f, 1.f, .3f) };
        case EVortexDungeonBiome::IceFortress: return { DHex(TEXT("b8d4e8")), DHex(TEXT("9cc0dc")), DHex(TEXT("d8e8f4")), DHex(TEXT("a8d8f0")), DHex(TEXT("8eb2cc")), DHex(TEXT("7ae0ff")), FLinearColor(.6f, .85f, 1.f) };
        case EVortexDungeonBiome::Volcano:     return { DHex(TEXT("2e2624")), DHex(TEXT("3a2e2a")), DHex(TEXT("3e3432")), DHex(TEXT("4a3a34")), DHex(TEXT("221c1a")), DHex(TEXT("ff7a2a")), FLinearColor(1.f, .45f, .15f) };
        case EVortexDungeonBiome::SunkenCity:  return { DHex(TEXT("c8b48a")), DHex(TEXT("b29d70")), DHex(TEXT("a8a088")), DHex(TEXT("6a9a8a")), DHex(TEXT("7a7462")), DHex(TEXT("ff8aa8")), FLinearColor(.5f, .9f, 1.f) };
        case EVortexDungeonBiome::RedAbyss:    return { DHex(TEXT("2a1416")), DHex(TEXT("3a1a1c")), DHex(TEXT("341a1e")), DHex(TEXT("4a1e22")), DHex(TEXT("1a0c0e")), DHex(TEXT("ff2a2a")), FLinearColor(1.f, .15f, .12f) };
        case EVortexDungeonBiome::PortalThrone: return { DHex(TEXT("1e1a2a")), DHex(TEXT("2a2238")), DHex(TEXT("2a2438")), DHex(TEXT("3a2a52")), DHex(TEXT("141020")), DHex(TEXT("c03aff")), FLinearColor(.75f, .25f, 1.f) };
        default:                               return { DHex(TEXT("5a554e")), DHex(TEXT("4a4640")), DHex(TEXT("6a6660")), DHex(TEXT("58544e")), DHex(TEXT("3e3a36")), DHex(TEXT("6ad0ff")), FLinearColor(1.f, .72f, .4f) };
        }
    }

    /** Os bichos de cada bioma (os que já existem; os novos estão nas ideias do documento) e o chefe. */
    void SpeciesOf(EVortexDungeonBiome Biome, TArray<FName>& Out, FName& Boss)
    {
        auto Set = [&](std::initializer_list<const TCHAR*> List, const TCHAR* Chief) { Out.Reset(); for (const TCHAR* S : List) Out.Add(S); Boss = Chief; };
        switch (Biome)
        {
        case EVortexDungeonBiome::Crystal:      Set({ TEXT("Slime"), TEXT("Bat"), TEXT("Spider"), TEXT("Spider"), TEXT("Golem") }, TEXT("Golem")); break;
        case EVortexDungeonBiome::Mine:         Set({ TEXT("Goblin"), TEXT("GoblinBrute"), TEXT("GoblinScout"), TEXT("Spider"), TEXT("SlimeIron") }, TEXT("GoblinBrute")); break;
        case EVortexDungeonBiome::Flooded:      Set({ TEXT("Frog"), TEXT("Slime"), TEXT("Bat"), TEXT("Spider") }, TEXT("Frog")); break;
        case EVortexDungeonBiome::SpiderNest:   Set({ TEXT("Spider"), TEXT("Spider"), TEXT("Spider"), TEXT("Bat") }, TEXT("Spider")); break;
        case EVortexDungeonBiome::Catacombs:    Set({ TEXT("Skeleton"), TEXT("SkeletonArcher"), TEXT("Undead"), TEXT("UndeadArcher"), TEXT("Bat") }, TEXT("Skeleton")); break;
        case EVortexDungeonBiome::Jungle:       Set({ TEXT("Frog"), TEXT("Mushroom"), TEXT("Goblin"), TEXT("Spider") }, TEXT("Mushroom")); break;
        case EVortexDungeonBiome::Swamp:        Set({ TEXT("Frog"), TEXT("SlimePoison"), TEXT("Undead"), TEXT("Spider") }, TEXT("Frog")); break;
        case EVortexDungeonBiome::IceFortress:  Set({ TEXT("Golem"), TEXT("Skeleton"), TEXT("SkeletonArcher"), TEXT("Bat") }, TEXT("Golem")); break;
        case EVortexDungeonBiome::Volcano:      Set({ TEXT("Golem"), TEXT("Bat"), TEXT("SlimeFire"), TEXT("Mushroom"), TEXT("GoblinBrute") }, TEXT("Golem")); break;
        case EVortexDungeonBiome::SunkenCity:   Set({ TEXT("Frog"), TEXT("Undead"), TEXT("Skeleton"), TEXT("SkeletonArcher") }, TEXT("Undead")); break;
        case EVortexDungeonBiome::RedAbyss:     Set({ TEXT("Skeleton"), TEXT("UndeadArcher"), TEXT("Spider"), TEXT("BanditBrute") }, TEXT("BanditBrute")); break;
        case EVortexDungeonBiome::PortalThrone: Set({ TEXT("Skeleton"), TEXT("SkeletonArcher"), TEXT("Spider"), TEXT("Undead") }, TEXT("Skeleton")); break;
        default:                                Set({ TEXT("Bat"), TEXT("Slime"), TEXT("Goblin"), TEXT("Spider"), TEXT("Spider") }, TEXT("Golem")); break;
        }
    }

    bool IsCaveLike(EVortexDungeonBiome Biome)
    {
        return Biome == EVortexDungeonBiome::Cave || Biome == EVortexDungeonBiome::Crystal || Biome == EVortexDungeonBiome::Mine
            || Biome == EVortexDungeonBiome::Flooded || Biome == EVortexDungeonBiome::SpiderNest;
    }
}

FVector FVortexDungeonSpec::RoomCenter(int32 Index) const
{
    const FIntPoint G = Rooms.IsValidIndex(Index) ? Rooms[Index].Grid : FIntPoint::ZeroValue;
    const float Step = AVortexDungeons::Pitch * AVortexDungeons::Block;
    return Origin + FVector(G.X * Step, G.Y * Step, 0.f);
}

FString AVortexDungeons::BiomeName(EVortexDungeonBiome Biome)
{
    switch (Biome)
    {
    case EVortexDungeonBiome::Crystal: return TEXT("Gruta de Cristais");
    case EVortexDungeonBiome::Mine: return TEXT("Mina Profunda");
    case EVortexDungeonBiome::Flooded: return TEXT("Caverna Alagada");
    case EVortexDungeonBiome::SpiderNest: return TEXT("Ninho de Aranhas");
    case EVortexDungeonBiome::Catacombs: return TEXT("Catacumbas");
    case EVortexDungeonBiome::Jungle: return TEXT("Selva Sombria");
    case EVortexDungeonBiome::Swamp: return TEXT("Pântano Tóxico");
    case EVortexDungeonBiome::IceFortress: return TEXT("Fortaleza de Gelo");
    case EVortexDungeonBiome::Volcano: return TEXT("Coração do Vulcão");
    case EVortexDungeonBiome::SunkenCity: return TEXT("Cidade Afundada");
    case EVortexDungeonBiome::RedAbyss: return TEXT("Abismo Vermelho");
    case EVortexDungeonBiome::PortalThrone: return TEXT("Trono do Portal");
    default: return TEXT("Caverna de Pedra");
    }
}

AVortexDungeons::AVortexDungeons()
{
    PrimaryActorTick.bCanEverTick = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

AVortexDungeons* AVortexDungeons::Find(const UWorld* World)
{
    if (!World) return nullptr;
    if (TActorIterator<AVortexDungeons> It(const_cast<UWorld*>(World)); It) return *It;
    return nullptr;
}

FName AVortexDungeons::MakeId(const FVector& PortalAt, const FString& Rank, int32 Level)
{
    const int32 Seed = int32(HashCombine(GetTypeHash(FMath::RoundToInt(PortalAt.X / 50.f)), GetTypeHash(FMath::RoundToInt(PortalAt.Y / 50.f))) & 0x7fffffff);
    return FName(*FString::Printf(TEXT("dg_%d_%s_%d"), Seed, *Rank, FMath::Clamp(Level, 1, 99)));
}

FVortexDungeonSpec AVortexDungeons::SpecFor(FName Id)
{
    FVortexDungeonSpec Spec;
    Spec.Id = Id;
    TArray<FString> Parts;
    Id.ToString().ParseIntoArray(Parts, TEXT("_"));
    if (Parts.Num() == 4 && Parts[0] == TEXT("dg"))
    {
        Spec.Seed = FCString::Atoi(*Parts[1]);
        Spec.Rank = Parts[2];
        Spec.Level = FMath::Clamp(FCString::Atoi(*Parts[3]), 1, 99);
    }
    else Spec.Seed = int32(GetTypeHash(Id) & 0x7fffffff);
    Spec.Tier = FMath::Max(0, FString(TEXT("EDCBAS")).Find(Spec.Rank));
    FRandomStream R(Spec.Seed);
    // o bioma: os básicos são cavernas; os altos, outros mundos
    static const TArray<TArray<EVortexDungeonBiome>> ByTier = {
        { EVortexDungeonBiome::Cave, EVortexDungeonBiome::Crystal },
        { EVortexDungeonBiome::Mine, EVortexDungeonBiome::Flooded, EVortexDungeonBiome::Cave },
        { EVortexDungeonBiome::SpiderNest, EVortexDungeonBiome::Catacombs },
        { EVortexDungeonBiome::Jungle, EVortexDungeonBiome::Swamp },
        { EVortexDungeonBiome::IceFortress, EVortexDungeonBiome::Volcano, EVortexDungeonBiome::SunkenCity },
        { EVortexDungeonBiome::RedAbyss, EVortexDungeonBiome::PortalThrone } };
    const TArray<EVortexDungeonBiome>& Options = ByTier[FMath::Clamp(Spec.Tier, 0, 5)];
    Spec.Biome = Options[R.RandRange(0, Options.Num() - 1)];
    // as regras: 0-1 nos básicos, 1-2 no meio, 2 nos altos (desafio vem com chance de bônus)
    const int32 RuleCount = Spec.Tier == 0 ? (R.FRand() < .45f ? 1 : 0) : Spec.Tier <= 2 ? 1 + (R.FRand() < .35f ? 1 : 0) : 2;
    for (int32 K = 0, Guard = 0; K < RuleCount && Guard < 50; ++Guard)
    {
        const int32 Rule = R.RandRange(0, VortexDungeonRules::Count - 1);
        if ((Spec.Rules >> Rule) & 1) continue;
        Spec.Rules |= uint16(1 << Rule);
        ++K;
    }
    // as salas: crescem a partir da entrada em passos para os lados (sem cruzar)
    static const int32 RoomsByTier[6] = { 5, 6, 8, 9, 11, 13 };
    const int32 Count = RoomsByTier[FMath::Clamp(Spec.Tier, 0, 5)];
    Spec.Rooms.Add(FVortexDungeonRoom());
    Spec.Rooms[0].Kind = FVortexDungeonRoom::Start;
    static const FIntPoint Steps[4] = { FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1) };
    for (int32 Guard = 0; Spec.Rooms.Num() < Count && Guard < 500; ++Guard)
    {
        // cresce mais a partir das salas do fim (o caminho fica comprido, com alguns galhos)
        const int32 From = R.FRand() < .6f ? Spec.Rooms.Num() - 1 : R.RandRange(0, Spec.Rooms.Num() - 1);
        const FIntPoint Next = Spec.Rooms[From].Grid + Steps[R.RandRange(0, 3)];
        if (Spec.Rooms.ContainsByPredicate([Next](const FVortexDungeonRoom& Room) { return Room.Grid == Next; })) continue;
        FVortexDungeonRoom Room;
        Room.Grid = Next;
        Room.Parent = From;
        Room.Depth = Spec.Rooms[From].Depth + 1;
        Spec.Rooms.Add(Room);
    }
    int32 Deepest = 1;
    for (int32 i = 1; i < Spec.Rooms.Num(); ++i) if (Spec.Rooms[i].Depth > Spec.Rooms[Deepest].Depth) Deepest = i;
    Spec.Rooms[Deepest].Kind = FVortexDungeonRoom::Boss;
    // o tesouro: uma ponta de galho que não seja o chefe
    int32 Treasure = INDEX_NONE;
    for (int32 i = Spec.Rooms.Num() - 1; i >= 1; --i)
    {
        const bool bLeaf = !Spec.Rooms.ContainsByPredicate([i](const FVortexDungeonRoom& Room) { return Room.Parent == i; });
        if (bLeaf && i != Deepest) { Treasure = i; break; }
    }
    if (Treasure == INDEX_NONE) for (int32 i = 1; i < Spec.Rooms.Num(); ++i) if (i != Deepest) { Treasure = i; break; }
    if (Treasure != INDEX_NONE) Spec.Rooms[Treasure].Kind = FVortexDungeonRoom::Treasure;
    // o lugar: uma vaga numa grade fora do mapa (600 m entre uma dungeon e outra), 50 m acima do mar
    FBox2D Map(FVector2D(-100000.f), FVector2D(100000.f));
    if (const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData()) Map = Data->MapBounds();
    const int32 Slot = Spec.Seed % 1024;
    Spec.Origin = FVector(Map.Max.X + 100000.f + (Slot % 32) * 60000.f, Map.Min.Y + (Slot / 32) * 60000.f, 5000.f);
    return Spec;
}

void AVortexDungeons::Build(const FVortexDungeonSpec& Spec)
{
    if (Built.Contains(Spec.Id)) return;
    if (!Material) Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/World/M_PoiBlocks.M_PoiBlocks"));
    FBuiltDungeon& Out = Built.Add(Spec.Id);
    Out.Spec = Spec;
    const FBiomeLook L = LookOf(Spec.Biome);
    FRandomStream R(Spec.Seed ^ 0x5bd1e995);
    FDungeonBlocks M;
    const float B = Block, Half = RoomSize * B * .5f, Tall = RoomHeight * B, Step = Pitch * B;
    auto Shade = [&R](const FLinearColor& C) { const float K = R.FRandRange(.85f, 1.1f); return FLinearColor(C.R * K, C.G * K, C.B * K, 1.f); };
    auto HasLink = [&Spec](int32 Room, const FIntPoint& Dir)
    {
        const FIntPoint Other = Spec.Rooms[Room].Grid + Dir;
        for (int32 i = 0; i < Spec.Rooms.Num(); ++i)
            if (Spec.Rooms[i].Grid == Other && (Spec.Rooms[i].Parent == Room || Spec.Rooms[Room].Parent == i)) return true;
        return false;
    };
    TArray<TPair<FVector, FLinearColor>> Lights;
    for (int32 i = 0; i < Spec.Rooms.Num(); ++i)
    {
        const FVector C = Spec.RoomCenter(i);
        // chão, teto e o piso com blocos soltos
        M.Box(C + FVector(0.f, 0.f, -B * .5f), FVector(RoomSize * B + 2 * B, RoomSize * B + 2 * B, B), L.Floor);
        M.Box(C + FVector(0.f, 0.f, Tall + B * .5f), FVector(RoomSize * B + 2 * B, RoomSize * B + 2 * B, B), L.Ceiling);
        for (int32 K = 0; K < 22; ++K)
            M.Box(C + FVector((R.RandRange(-6, 6)) * B, (R.RandRange(-6, 6)) * B, 4.f), FVector(B, B, 8.f), Shade(L.Floor2));
        // paredes: abertas onde há corredor
        static const FIntPoint Dirs[4] = { FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1) };
        for (const FIntPoint& D : Dirs)
        {
            const FVector Normal(D.X, D.Y, 0.f);
            const FVector Side(-D.Y, D.X, 0.f);
            const FVector WallMid = C + Normal * (Half + B * .5f) + FVector(0.f, 0.f, Tall * .5f);
            const FVector Thick = FVector(FMath::Abs(Normal.X) * B, FMath::Abs(Normal.Y) * B, 0.f);
            const FVector Along = FVector(FMath::Abs(Side.X), FMath::Abs(Side.Y), 0.f);
            if (!HasLink(i, D))
            {
                M.Box(WallMid, Thick + Along * (RoomSize * B + 2 * B) + FVector(0.f, 0.f, Tall), Shade(L.Wall));
            }
            else
            {
                const float Gap = CorridorWidth * B, Piece = (RoomSize * B + 2 * B - Gap) * .5f;
                for (const float S : { -1.f, 1.f })
                    M.Box(WallMid + Side * S * (Gap * .5f + Piece * .5f), Thick + Along * Piece + FVector(0.f, 0.f, Tall), Shade(L.Wall));
                const float Lintel = Tall - CorridorHeight * B;
                M.Box(C + Normal * (Half + B * .5f) + FVector(0.f, 0.f, CorridorHeight * B + Lintel * .5f), Thick + Along * Gap + FVector(0.f, 0.f, Lintel), Shade(L.Wall2));
            }
            // pedras saltadas na parede
            for (int32 K = 0; K < 4; ++K)
            {
                const float S = R.FRandRange(-5.f, 5.f) * B, Z = R.RandRange(0, RoomHeight - 1) * B + B * .5f;
                if (HasLink(i, D) && FMath::Abs(S) < (CorridorWidth + 1) * B * .5f && Z < CorridorHeight * B) continue;
                M.Box(C + Normal * (Half - B * .25f) + Side * S + FVector(0.f, 0.f, Z), FVector(B * .6f, B * .6f, B * .8f), Shade(L.Wall2));
            }
            // tocha na parede (a luz entra depois); na escuridão só uma por sala
            if ((i + D.X * 3 + D.Y * 7) % 2 == 0 || !Spec.Has(VortexDungeonRules::Darkness))
            {
                if (Spec.Has(VortexDungeonRules::Darkness) && Lights.Num() > i) continue;
                const FVector Torch = C + Normal * (Half - B * .2f) + Side * (HasLink(i, D) ? 3.5f * B : 0.f) + FVector(0.f, 0.f, 3.4f * B);
                M.Box(Torch - FVector(0.f, 0.f, B * .4f), FVector(B * .2f, B * .2f, B * .6f), DHex(TEXT("3a2a1e")));
                M.Box(Torch, FVector(B * .3f, B * .3f, B * .3f), L.Accent);
                Lights.Add({ Torch - Normal * B * .8f, L.Light });
            }
        }
        // corredor até a sala mãe
        if (Spec.Rooms[i].Parent != INDEX_NONE)
        {
            const FVector P = Spec.RoomCenter(Spec.Rooms[i].Parent);
            const FVector Dir = (P - C).GetSafeNormal();
            const FVector Side(-Dir.Y, Dir.X, 0.f);
            const float Length = Step - RoomSize * B - 2 * B;
            const FVector Mid = (C + P) * .5f;
            const FVector Run = FVector(FMath::Abs(Dir.X), FMath::Abs(Dir.Y), 0.f) * (Length + 2 * B);
            const FVector Across = FVector(FMath::Abs(Side.X), FMath::Abs(Side.Y), 0.f);
            M.Box(Mid + FVector(0.f, 0.f, -B * .5f), Run + Across * (CorridorWidth * B + 2 * B) + FVector(0.f, 0.f, B), L.Floor2);
            M.Box(Mid + FVector(0.f, 0.f, CorridorHeight * B + B * .5f), Run + Across * (CorridorWidth * B + 2 * B) + FVector(0.f, 0.f, B), L.Ceiling);
            for (const float S : { -1.f, 1.f })
                M.Box(Mid + Side * S * (CorridorWidth * B * .5f + B * .5f) + FVector(0.f, 0.f, CorridorHeight * B * .5f), Run + Across * B + FVector(0.f, 0.f, CorridorHeight * B), Shade(L.Wall));
        }
        // a decoração do bioma (fora da entrada, que fica limpa)
        if (Spec.Rooms[i].Kind == FVortexDungeonRoom::Start) continue;
        auto Spot = [&](float Margin = 1.5f) { return C + FVector(R.FRandRange(-Half + Margin * B, Half - Margin * B), R.FRandRange(-Half + Margin * B, Half - Margin * B), 0.f); };
        const int32 Props = 5 + R.RandRange(0, 3);
        for (int32 K = 0; K < Props; ++K)
        {
            const FVector At = Spot();
            switch (Spec.Biome)
            {
            case EVortexDungeonBiome::Crystal: case EVortexDungeonBiome::RedAbyss:
            {
                // cristais: blocos altos e finos acesos, em tufo
                for (int32 c = 0; c < 3; ++c)
                {
                    const float Hc = R.FRandRange(1.f, 3.2f) * B;
                    M.Box(At + FVector(R.FRandRange(-.6f, .6f) * B, R.FRandRange(-.6f, .6f) * B, Hc * .5f), FVector(B * .35f, B * .35f, Hc), L.Accent);
                }
                if (K % 3 == 0) Lights.Add({ At + FVector(0.f, 0.f, 2.f * B), L.Light });
                break;
            }
            case EVortexDungeonBiome::Mine:
            {
                // escora de madeira e carrinho de minério
                M.Box(At + FVector(0.f, 0.f, 2.f * B), FVector(B * .3f, B * .3f, 4.f * B), DHex(TEXT("6a4a2a")));
                M.Box(At + FVector(0.f, 0.f, 4.f * B), FVector(B * 2.4f, B * .35f, B * .35f), DHex(TEXT("6a4a2a")));
                if (K % 2 == 0)
                {
                    M.Box(At + FVector(B * 1.5f, 0.f, B * .45f), FVector(B * 1.2f, B * .8f, B * .7f), DHex(TEXT("2e2a26")));
                    M.Box(At + FVector(B * 1.5f, 0.f, B * .9f), FVector(B * .9f, B * .6f, B * .3f), L.Accent);
                }
                break;
            }
            case EVortexDungeonBiome::Flooded: case EVortexDungeonBiome::Swamp: case EVortexDungeonBiome::SunkenCity: case EVortexDungeonBiome::Volcano:
            {
                // poça (água, lodo tóxico ou lava) e um bloco de pedra na beira
                const float W = R.FRandRange(2.f, 3.5f) * B;
                M.Box(At + FVector(0.f, 0.f, 3.f), FVector(W, W * R.FRandRange(.7f, 1.3f), 6.f), Spec.Biome == EVortexDungeonBiome::Volcano ? L.Accent : Spec.Biome == EVortexDungeonBiome::Swamp ? DHex(TEXT("5a8a2a")) : DHex(TEXT("2a6a8a")));
                if (Spec.Biome == EVortexDungeonBiome::Volcano && K % 2 == 0) Lights.Add({ At + FVector(0.f, 0.f, B), L.Light });
                if (Spec.Biome == EVortexDungeonBiome::SunkenCity) M.Box(At + FVector(W * .6f, 0.f, 1.5f * B), FVector(B * .8f, B * .8f, 3.f * B), Shade(L.Wall2));
                break;
            }
            case EVortexDungeonBiome::SpiderNest:
            {
                // sacos de ovos e ossos
                for (int32 e = 0; e < 4; ++e) M.Box(At + FVector(R.FRandRange(-.7f, .7f) * B, R.FRandRange(-.7f, .7f) * B, R.FRandRange(.2f, .8f) * B), FVector(B * .5f, B * .5f, B * .6f), L.Wall2);
                M.Box(At + FVector(B * 1.4f, 0.f, 8.f), FVector(B * .9f, B * .15f, B * .15f), DHex(TEXT("e8e0c8")));
                break;
            }
            case EVortexDungeonBiome::Catacombs: case EVortexDungeonBiome::PortalThrone:
            {
                // caixão de pedra com tampa e vela acesa
                M.Box(At + FVector(0.f, 0.f, B * .45f), FVector(B * 2.2f, B * 1.f, B * .9f), Shade(L.Wall2));
                M.Box(At + FVector(0.f, 0.f, B * .95f), FVector(B * 2.4f, B * 1.15f, B * .15f), Shade(L.Floor2));
                M.Box(At + FVector(B * 1.3f, 0.f, B * 1.2f), FVector(B * .15f, B * .15f, B * .3f), L.Accent);
                if (K % 3 == 0) Lights.Add({ At + FVector(B * 1.3f, 0.f, B * 1.8f), L.Light });
                break;
            }
            case EVortexDungeonBiome::Jungle:
            {
                // raiz atravessada e cogumelo aceso
                M.Box(At + FVector(0.f, 0.f, B * .3f), FVector(B * 3.f, B * .5f, B * .5f), DHex(TEXT("4a3a24")));
                M.Box(At + FVector(B, B, B * .4f), FVector(B * .2f, B * .2f, B * .8f), DHex(TEXT("d8d0b0")));
                M.Box(At + FVector(B, B, B * .85f), FVector(B * .6f, B * .6f, B * .2f), L.Accent);
                break;
            }
            case EVortexDungeonBiome::IceFortress:
            {
                const float Hc = R.FRandRange(1.5f, 4.f) * B;
                M.Box(At + FVector(0.f, 0.f, Hc * .5f), FVector(B * .6f, B * .6f, Hc), L.Accent);
                M.Box(At + FVector(0.f, 0.f, Hc + B * .3f), FVector(B * .3f, B * .3f, B * .6f), L.Wall2);
                break;
            }
            default:
            {
                // estalagmite do chão e estalactite do teto
                for (int32 s = 0; s < 3; ++s)
                    M.Box(At + FVector(0.f, 0.f, (s + .5f) * B), FVector(B * (1.f - s * .28f), B * (1.f - s * .28f), B), Shade(L.Wall2));
                const FVector Top = Spot();
                for (int32 s = 0; s < 2; ++s)
                    M.Box(Top + FVector(0.f, 0.f, Tall - (s + .5f) * B), FVector(B * (.8f - s * .3f), B * (.8f - s * .3f), B), Shade(L.Wall2));
                break;
            }
            }
        }
        // cipós/correntes/teias pendurados do teto
        if (Spec.Biome == EVortexDungeonBiome::Jungle || Spec.Biome == EVortexDungeonBiome::RedAbyss || Spec.Biome == EVortexDungeonBiome::PortalThrone)
            for (int32 K = 0; K < 6; ++K)
            {
                const FVector At = Spot(1.f);
                const float Len = R.FRandRange(1.5f, 3.5f) * B;
                M.Box(At + FVector(0.f, 0.f, Tall - Len * .5f), FVector(B * .12f, B * .12f, Len), Spec.Biome == EVortexDungeonBiome::Jungle ? DHex(TEXT("3e7a2e")) : DHex(TEXT("1a1418")));
            }
        // o trono do chefe
        if (Spec.Rooms[i].Kind == FVortexDungeonRoom::Boss && Spec.Biome == EVortexDungeonBiome::PortalThrone)
        {
            M.Box(C + FVector(0.f, Half - 2.f * B, B), FVector(3.f * B, 2.f * B, 2.f * B), L.Wall2);
            M.Box(C + FVector(0.f, Half - 1.2f * B, 3.f * B), FVector(3.f * B, B * .5f, 4.f * B), L.Wall2);
        }
    }
    UProceduralMeshComponent* Mesh = NewObject<UProceduralMeshComponent>(this);
    Mesh->SetupAttachment(RootComponent);
    Mesh->SetUsingAbsoluteLocation(true);
    Mesh->SetUsingAbsoluteRotation(true);
    Mesh->bUseComplexAsSimpleCollision = true;
    Mesh->RegisterComponent();
    Mesh->CreateMeshSection_LinearColor(0, M.Vertices, M.Triangles, M.Normals, {}, M.Colors, {}, true);
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Mesh->SetCollisionResponseToAllChannels(ECR_Block);
    if (Material) Mesh->SetMaterial(0, Material);
    Out.Parts.Add(Mesh);
    // as luzes (só quem desenha)
    if (GetNetMode() != NM_DedicatedServer)
    {
        const bool bDark = Spec.Has(VortexDungeonRules::Darkness);
        for (const TPair<FVector, FLinearColor>& Light : Lights)
        {
            UPointLightComponent* Lamp = NewObject<UPointLightComponent>(this);
            Lamp->SetupAttachment(RootComponent);
            Lamp->SetUsingAbsoluteLocation(true);
            Lamp->SetWorldLocation(Light.Key);
            Lamp->SetLightColor(Light.Value);
            Lamp->SetIntensity(bDark ? 900.f : 2500.f);   // 30/10: mais fracas (eram 1400 / 4500)
            Lamp->SetAttenuationRadius(bDark ? 700.f : 1300.f);
            Lamp->SetCastShadows(false);
            Lamp->RegisterComponent();
            Out.Parts.Add(Lamp);
        }
    }
    // baús: o do tesouro e o do chefe (guardados por quem está na sala); teias no ninho e nas cavernas
    if (AVortexWorldFeatures* Features = AVortexWorldFeatures::Find(GetWorld()))
        for (int32 i = 0; i < Spec.Rooms.Num(); ++i)
        {
            const FVortexDungeonRoom& Room = Spec.Rooms[i];
            if (Room.Kind != FVortexDungeonRoom::Treasure && Room.Kind != FVortexDungeonRoom::Boss) continue;
            const FVector At = Spec.RoomCenter(i) + FVector(0.f, Room.Kind == FVortexDungeonRoom::Boss ? -3.f * B : 0.f, 0.f);
            Features->AddCampChest(FName(*FString::Printf(TEXT("%s:bau%d"), *Spec.Id.ToString(), i)), At, 0.f, 1, Room.Kind == FVortexDungeonRoom::Boss || Spec.Tier >= 3, true);
        }
    if (AVortexPointsOfInterest* Pois = AVortexPointsOfInterest::Find(GetWorld()))
        if (Spec.Biome == EVortexDungeonBiome::SpiderNest || Spec.Biome == EVortexDungeonBiome::Cave)
            for (int32 i = 1; i < Spec.Rooms.Num(); ++i)
            {
                TArray<AVortexPointsOfInterest::FWebSpot> Spots;
                const int32 Count = Spec.Biome == EVortexDungeonBiome::SpiderNest ? 3 : (i % 2);
                for (int32 K = 0; K < Count; ++K)
                {
                    AVortexPointsOfInterest::FWebSpot Spot;
                    const FVector At = Spec.RoomCenter(i) + FVector(R.FRandRange(-4.f, 4.f) * B, R.FRandRange(-4.f, 4.f) * B, 0.f);
                    Spot.At = FVector2D(At);
                    Spot.Yaw = 90.f * R.RandRange(0, 1);
                    Spot.Width = R.FRandRange(2.4f, 3.2f);
                    Spot.bGround = R.FRand() < .4f;
                    Spots.Add(Spot);
                }
                if (Spots.Num()) Pois->AddPinnedWebs(FIntPoint(-1000000 - (Spec.Seed % 100000), i), Spots, Spec.Origin.Z);
            }
    UE_LOG(LogTemp, Display, TEXT("VORTEX_DUNGEON montada %s: %s, ranque %s, nível %d, %d salas, regras %d"), *Spec.Id.ToString(), *BiomeName(Spec.Biome), *Spec.Rank, Spec.Level, Spec.Rooms.Num(), int32(Spec.Rules));
}

void AVortexDungeons::Drop(FName Id)
{
    FBuiltDungeon* B = Built.Find(Id);
    if (!B) return;
    for (USceneComponent* Part : B->Parts) if (Part) Part->DestroyComponent();
    if (AVortexWorldFeatures* Features = AVortexWorldFeatures::Find(GetWorld()))
        for (int32 i = 0; i < B->Spec.Rooms.Num(); ++i) Features->RemoveChest(FName(*FString::Printf(TEXT("%s:bau%d"), *Id.ToString(), i)));
    if (AVortexPointsOfInterest* Pois = AVortexPointsOfInterest::Find(GetWorld()))
        for (int32 i = 1; i < B->Spec.Rooms.Num(); ++i) Pois->RemovePinnedWebs(FIntPoint(-1000000 - (B->Spec.Seed % 100000), i));
    Built.Remove(Id);
}

void AVortexDungeons::Populate(const FVortexDungeonSpec& Spec, FLive& State)
{
    UWorld* World = GetWorld();
    TArray<FName> Kinds;
    FName BossKind;
    SpeciesOf(Spec.Biome, Kinds, BossKind);
    FRandomStream R(Spec.Seed ^ 0x2545F491);
    const float B = Block, Half = RoomSize * B * .5f, Ceiling = Spec.Origin.Z + RoomHeight * B;
    const float Damage = Spec.Has(VortexDungeonRules::Fury) ? 1.3f : 1.f, Speed = Spec.Has(VortexDungeonRules::Swift) ? 1.25f : 1.f;
    auto Spawn = [&](FName Kind, const FVector& Floor, bool bBoss, bool bHang)
    {
        const FVortexSpeciesDef* Def = FVortexSpeciesDef::Find(Kind);
        const float HalfHeight = Def ? FMath::Max(Def->Radius, Def->Height * .5f) : 70.f;
        const FVector At = bHang ? FVector(Floor.X, Floor.Y, Ceiling - 180.f) : Floor + FVector(0.f, 0.f, HalfHeight * (bBoss ? AVortexMonster::EliteScale : 1.f) + 5.f);
        const FTransform Pose(FRotator(0.f, R.FRandRange(0.f, 360.f), 0.f), At);
        AVortexMonster* Monster = World->SpawnActorDeferred<AVortexMonster>(AVortexMonster::StaticClass(), Pose, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        if (!Monster) return (AVortexMonster*)nullptr;
        Monster->SetLookSeed(R.RandRange(0, 1 << 30));
        Monster->SetDungeon(Ceiling, Damage, Speed);
        if (bHang) Monster->SetHanging(true);
        if (bBoss) Monster->MakeElite();
        Monster->Setup(Kind, FMath::Min(99, Spec.Level + (bBoss ? 3 : 0)));
        Monster->FinishSpawning(Pose);
        State.Monsters.Add(Monster);
        return Monster;
    };
    for (int32 i = 0; i < Spec.Rooms.Num(); ++i)
    {
        const FVortexDungeonRoom& Room = Spec.Rooms[i];
        if (Room.Kind == FVortexDungeonRoom::Start) continue;
        const FVector C = Spec.RoomCenter(i);
        auto Floor = [&]() { return C + FVector(R.FRandRange(-Half + 2.f * B, Half - 2.f * B), R.FRandRange(-Half + 2.f * B, Half - 2.f * B), 0.f); };
        if (Room.Kind == FVortexDungeonRoom::Boss)
        {
            State.Boss = Spawn(BossKind, C + FVector(0.f, 2.f * B, 0.f), true, false);
            for (int32 K = 0; K < 2 + Spec.Tier / 2; ++K) Spawn(Kinds[R.RandRange(0, Kinds.Num() - 1)], Floor(), false, false);
            continue;
        }
        const int32 Count = (Room.Kind == FVortexDungeonRoom::Treasure ? 2 : 3) + Spec.Tier / 2 + R.RandRange(0, 1);
        for (int32 K = 0; K < Count; ++K)
        {
            const FName Kind = Kinds[R.RandRange(0, Kinds.Num() - 1)];
            // nas cavernas, parte das aranhas espera pendurada no teto
            const bool bHang = Kind == TEXT("Spider") && IsCaveLike(Spec.Biome) && R.FRand() < .4f;
            Spawn(Kind, Floor(), false, bHang);
        }
    }
}

void AVortexDungeons::SpawnExit(const FVortexDungeonSpec& Spec, FLive& State, const FVector& At)
{
    if (State.Exit.IsValid()) return;
    AVortexPortal* Exit = AVortexPortal::Open(GetWorld(), At, 0.f, TEXT("E"), TEXT("exit"));
    if (!Exit) return;
    Exit->Destination = State.Return;
    State.Exit = Exit;
}

void AVortexDungeons::Enter(AVortexCharacter* Hero, AVortexPortal* Portal)
{
    if (!Hero || !Portal || !Hero->GetSheet()) return;
    if (!Portal->DungeonId.ToString().StartsWith(TEXT("dg_"))) Portal->DungeonId = MakeId(Portal->GetActorLocation(), Portal->Rank, 3 + 3 * FMath::Max(0, FString(TEXT("EDCBAS")).Find(Portal->Rank)));
    const FVortexDungeonSpec Spec = SpecFor(Portal->DungeonId);
    if (!Spec.IsValid()) return;
    Build(Spec);
    FLive& State = Live.FindOrAdd(Spec.Id);
    if (State.StartedAt <= 0.0)
    {
        // a volta: na frente do portal de fora
        State.Return = Portal->GetActorLocation() - Portal->GetActorForwardVector() * 450.f * AVortexPortal::RankSize(Portal->Rank) + FVector(0.f, 0.f, 120.f);
        State.StartedAt = GetWorld()->GetTimeSeconds();
        Populate(Spec, State);
        // o portal de volta na sala de entrada (no vermelho, só depois do chefe)
        if (Spec.Rank != TEXT("S")) SpawnExit(Spec, State, Spec.Origin + FVector(-(RoomSize * .5f - 2.f) * Block, 0.f, 0.f));
    }
    State.EmptySince = -1.0;
    UVortexHeroSheet* Sheet = Hero->GetSheet();
    Sheet->SetDungeon(Spec.Id, Spec.Rules);
    const int32 Spread = Hero->GetUniqueID() % 4;
    Hero->TeleportTo(Spec.Origin + FVector(1.5f * Block, (Spread - 1.5f) * 120.f, 120.f), FRotator(0.f, 0.f, 0.f));
    TArray<FString> Lines;
    for (int32 Rule = 0; Rule < VortexDungeonRules::Count; ++Rule) if (Spec.Has(Rule)) Lines.Add(VortexDungeonRules::Name(Rule));
    Sheet->PushNotice(FString::Printf(TEXT("%s  •  ranque %s  •  nível %d%s"), *BiomeName(Spec.Biome), *Spec.Rank, Spec.Level,
        Lines.Num() ? *(FString(TEXT("  •  ")) + FString::Join(Lines, TEXT(", "))) : TEXT("")), 2);
}

void AVortexDungeons::Leave(AVortexCharacter* Hero)
{
    if (UVortexHeroSheet* Sheet = Hero ? Hero->GetSheet() : nullptr) Sheet->SetDungeon(NAME_None, 0);
}

void AVortexDungeons::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Since += DeltaSeconds;
    if (Since < .25f) return;
    Since = 0.f;
    UWorld* World = GetWorld();
    const bool bServer = GetNetMode() != NM_Client;
    const double Now = World->GetTimeSeconds();
    // as dungeons de quem está dentro (montadas em cada máquina)
    TMap<FName, TArray<AVortexCharacter*>> Inside;
    for (TActorIterator<AVortexCharacter> It(World); It; ++It)
    {
        const UVortexHeroSheet* Sheet = It->GetSheet();
        if (!Sheet || Sheet->GetDungeonId().IsNone()) continue;
        const FVortexDungeonSpec Spec = SpecFor(Sheet->GetDungeonId());
        // morreu e renasceu lá fora (ou foi levado embora): a dungeon não vale mais para ele
        if (bServer && FVector::Dist2D(It->GetActorLocation(), FVector(Spec.Origin)) > 40000.f) { Leave(*It); continue; }
        Inside.FindOrAdd(Sheet->GetDungeonId()).Add(*It);
        if (!Built.Contains(Sheet->GetDungeonId())) Build(Spec);
    }
    if (bServer)
    {
        TArray<FName> Ended;
        for (TPair<FName, FLive>& Pair : Live)
        {
            FLive& State = Pair.Value;
            const FVortexDungeonSpec Spec = SpecFor(Pair.Key);
            // o chefe caiu: vitória; no vermelho, o portal de volta aparece na sala dele
            const IVortexCreature* Boss = Cast<IVortexCreature>(State.Boss.Get());
            if (!State.bCleared && (!Boss || Boss->IsCreatureDead()) && State.StartedAt > 0.0)
            {
                State.bCleared = true;
                int32 BossRoom = 0;
                for (int32 i = 0; i < Spec.Rooms.Num(); ++i) if (Spec.Rooms[i].Kind == FVortexDungeonRoom::Boss) BossRoom = i;
                SpawnExit(Spec, State, Spec.RoomCenter(BossRoom) + FVector(0.f, 3.f * Block, 0.f));
                const bool bFast = Spec.Has(VortexDungeonRules::TimeTrial) && Now - State.StartedAt < 600.0;
                if (TArray<AVortexCharacter*>* Heroes = Inside.Find(Pair.Key))
                    for (AVortexCharacter* Hero : *Heroes)
                        if (UVortexHeroSheet* Sheet = Hero->GetSheet())
                        {
                            Sheet->PushNotice(TEXT("Dungeon vencida! O baú do chefe e o portal de volta te esperam."), 2);
                            if (bFast) { Sheet->AddCoins(60 + 60 * Spec.Tier); Sheet->PushNotice(TEXT("Desafio do tempo cumprido: moedas extras!"), 2); }
                        }
            }
            // vazia há 30 s: fecha (monstros e portal de volta somem)
            if (Inside.Contains(Pair.Key)) { State.EmptySince = -1.0; continue; }
            if (State.EmptySince < 0.0) State.EmptySince = Now;
            if (Now - State.EmptySince < 30.0) continue;
            for (const TWeakObjectPtr<AActor>& Monster : State.Monsters) if (AActor* M = Monster.Get()) M->Destroy();
            if (AVortexPortal* Exit = State.Exit.Get()) Exit->Destroy();
            Ended.Add(Pair.Key);
        }
        for (const FName& Id : Ended) { Live.Remove(Id); Drop(Id); }
    }
    else
    {
        TArray<FName> Gone;
        for (const TPair<FName, FBuiltDungeon>& B : Built) if (!Inside.Contains(B.Key)) Gone.Add(B.Key);
        for (const FName& Id : Gone) Drop(Id);
    }
}
