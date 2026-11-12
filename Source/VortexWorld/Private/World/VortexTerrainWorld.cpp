// O continente do Godot (WorldMap + ValleyWorld.Ground/Scatter) gerado na hora, fora da região
// exportada: relevo do mapa de 6 m com o ruído fino, as nove regiões (chão, tom, árvores, enfeites e
// plantas de cada uma), gelo e lava, o mar em volta (sem fim: não há borda para cair).
// Só leitura depois de LoadWorld; as threads do terreno pedem pedaços à vontade (guardados com trava).
#include "World/VortexTerrainData.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "World/VortexLandmarks.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
    // regiões (WorldMap.cs)
    enum : int32 { Ocean = 0, Valdora, Brasafenda, Raikor, Nivaluz, Solcanion, Aurivale, Lodonegro, Morthuria, Verdalume };
    // camadas do chão (TerrainBatch do Godot, mesma ordem do atlas)
    enum : uint8 { GrassA = 0, GrassB, GrassFlowers, GrassForest, Path, Sand, Stone, SideLip, SideDirt, SideStone, SideSand,
        Snow, Ice, Ash, RedSand, Mud, PurpleSoil, GoldGrass, ElvenGrass, SideSnow, SideRed };
    // cor média de cada camada (sRGB), a mesma de FVortexTerrainData::CellTint
    const FLinearColor LayerAverage[21] = {
        { .386f,.629f,.270f }, { .418f,.659f,.288f }, { .413f,.654f,.291f },
        { .262f,.449f,.208f }, { .634f,.521f,.365f }, { .844f,.787f,.608f },
        { .510f,.522f,.494f }, { .439f,.410f,.249f }, { .457f,.345f,.237f },
        { .480f,.491f,.462f }, { .713f,.639f,.488f }, { .927f,.947f,.967f },
        { .717f,.832f,.895f }, { .268f,.236f,.236f }, { .797f,.501f,.304f },
        { .295f,.292f,.186f }, { .359f,.287f,.393f }, { .800f,.671f,.252f },
        { .272f,.602f,.300f }, { .748f,.786f,.824f }, { .624f,.356f,.223f }
    };
    constexpr float WaterLevelM = 2.05f, HomeRadius = 300.f, CellM = .75f, VillageZ = 15.f;

    // ---- ruído (sem estado: as threads chamam à vontade) ------------------------------------------
    FORCEINLINE uint32 Mix(int32 X, int32 Y, int32 Seed)
    {
        uint32 H = uint32(X) * 0x27d4eb2du ^ uint32(Y) * 0x165667b1u ^ uint32(Seed) * 0x9e3779b9u;
        H ^= H >> 15; H *= 0x85ebca6bu; H ^= H >> 13; H *= 0xc2b2ae35u; H ^= H >> 16;
        return H;
    }
    FORCEINLINE float Unit(uint32 H) { return (H & 0xffffff) / float(0xffffff); }
    FORCEINLINE float Grad(int32 X, int32 Y, int32 Seed, float Dx, float Dy)
    {
        static const float G[8][2] = { {1,0}, {-1,0}, {0,1}, {0,-1}, {.7071f,.7071f}, {-.7071f,.7071f}, {.7071f,-.7071f}, {-.7071f,-.7071f} };
        const float* V = G[Mix(X, Y, Seed) & 7];
        return V[0] * Dx + V[1] * Dy;
    }
    float Perlin(float X, float Y, int32 Seed)
    {
        const int32 X0 = FMath::FloorToInt(X), Y0 = FMath::FloorToInt(Y);
        const float Fx = X - X0, Fy = Y - Y0;
        auto Fade = [](float T) { return T * T * T * (T * (T * 6.f - 15.f) + 10.f); };
        const float U = Fade(Fx), V = Fade(Fy);
        const float A = FMath::Lerp(Grad(X0, Y0, Seed, Fx, Fy), Grad(X0 + 1, Y0, Seed, Fx - 1.f, Fy), U);
        const float B = FMath::Lerp(Grad(X0, Y0 + 1, Seed, Fx, Fy - 1.f), Grad(X0 + 1, Y0 + 1, Seed, Fx - 1.f, Fy - 1.f), U);
        return FMath::Lerp(A, B, V) * 1.41f;
    }
    /** FBM como o FastNoiseLite do Godot (lacunaridade 2, ganho .5), em -1..1. */
    float Fbm(float X, float Y, int32 Seed, float Frequency, int32 Octaves)
    {
        float Sum = 0.f, Amp = 1.f, Norm = 0.f, F = Frequency;
        for (int32 o = 0; o < Octaves; ++o) { Sum += Perlin(X * F, Y * F, Seed + o * 131) * Amp; Norm += Amp; Amp *= .5f; F *= 2.f; }
        return Sum / Norm;
    }
    /** Celular com o valor da célula (speciesNoise do Godot): manchas grandes de valor constante. */
    float CellValue(float X, float Y, int32 Seed, float Frequency)
    {
        X *= Frequency; Y *= Frequency;
        const int32 Cx = FMath::FloorToInt(X), Cy = FMath::FloorToInt(Y);
        float Best = 1e9f;
        uint32 Value = 0;
        for (int32 dx = -1; dx <= 1; ++dx) for (int32 dy = -1; dy <= 1; ++dy)
        {
            const uint32 H = Mix(Cx + dx, Cy + dy, Seed);
            const float Px = Cx + dx + Unit(H), Py = Cy + dy + Unit(H >> 7 ^ H * 31u);
            const float D = FMath::Square(Px - X) + FMath::Square(Py - Y);
            if (D < Best) { Best = D; Value = H; }
        }
        return Unit(Value * 2654435761u) * 2.f - 1.f;
    }
    float Noise(float X, float Z) { return Fbm(X, Z, 7341, .018f, 3); }
    float ForestNoise(float X, float Z) { return Fbm(X, Z, 911, .045f, 2); }
    float Species(float X, float Z) { return CellValue(X, Z, 4242, .0075f); }
    float Meadow(float X, float Z) { return Fbm(X, Z, 515, .09f, 4); }
    float TintNoise(float X, float Z) { return Fbm(X, Z, 77, .06f, 4); }
    uint32 HashAt(float X, float Z) { return uint32(FMath::RoundToInt(X * 7.3f) * 73856093) ^ uint32(FMath::RoundToInt(Z * 7.3f) * 19349663); }

    // tronco de cada árvore (raio e altura até a copa, m), como ValleyWorld.Trunk
    void Trunk(const FString& Kind, float& Radius, float& Height)
    {
        Radius = .7f; Height = 4.6f;
        if (Kind == TEXT("tree_pine") || Kind == TEXT("tree_snow_pine") || Kind == TEXT("tree_ashpine") || Kind == TEXT("tree_storm_fir")) { Radius = .5f; Height = 4.5f; }
        else if (Kind == TEXT("tree_birch")) { Radius = .42f; Height = 5.f; }
        else if (Kind == TEXT("tree_cherry")) { Radius = .68f; Height = 2.8f; }
        else if (Kind == TEXT("tree_autumn")) { Radius = .68f; Height = 4.2f; }
        else if (Kind == TEXT("tree_cactus")) { Radius = .55f; Height = 5.5f; }
        else if (Kind == TEXT("tree_elven")) { Radius = .62f; Height = 5.2f; }
        else if (Kind == TEXT("tree_moonleaf")) { Radius = .48f; Height = 5.6f; }
        else if (Kind == TEXT("tree_acacia") || Kind == TEXT("tree_apple")) { Radius = .58f; Height = 2.2f; }
        else if (Kind == TEXT("tree_mangrove")) { Radius = .6f; Height = 5.f; }
        else if (Kind == TEXT("tree_ice")) { Radius = .6f; Height = 3.2f; }
        else if (Kind == TEXT("tree_ember") || Kind == TEXT("tree_bone")) { Radius = .62f; Height = 3.f; }
        else if (Kind == TEXT("tree_crag")) { Radius = .62f; Height = 2.5f; }
        else if (Kind == TEXT("tree_haunted")) { Radius = .68f; Height = 4.f; }
        else if (Kind == TEXT("tree_willow")) { Radius = .75f; Height = 3.4f; }
        else if (Kind == TEXT("tree_golden")) { Radius = .72f; Height = 3.6f; }
    }

    // todas as espécies que o continente usa (entram na lista do terreno ao carregar)
    const TCHAR* WorldKinds[] = {
        TEXT("tree_oak"), TEXT("tree_birch"), TEXT("tree_autumn"), TEXT("tree_pine"), TEXT("tree_cherry"), TEXT("tree_dead"),
        TEXT("tree_elven"), TEXT("tree_moonleaf"), TEXT("tree_crag"), TEXT("tree_storm_fir"), TEXT("tree_snow_pine"), TEXT("tree_ice"),
        TEXT("tree_ember"), TEXT("tree_ashpine"), TEXT("tree_cactus"), TEXT("tree_acacia"), TEXT("tree_haunted"), TEXT("tree_bone"),
        TEXT("tree_mangrove"), TEXT("tree_willow"), TEXT("tree_golden"), TEXT("tree_apple"),
        TEXT("bush_green"), TEXT("bush_flower"), TEXT("rock_pebbles"), TEXT("rock_mossy"), TEXT("rock_boulder"), TEXT("log_fallen"), TEXT("stump"),
        TEXT("basalt_spikes"), TEXT("rock_basalt"), TEXT("ember_bloom"), TEXT("slate_spire"), TEXT("rock_snowy"), TEXT("storm_crystal"),
        TEXT("alpine_bloom"), TEXT("snow_shrub"), TEXT("ice_spike"), TEXT("frost_flower"), TEXT("cactus_barrel"), TEXT("desert_brush"),
        TEXT("bleached_bones"), TEXT("rock_sandstone"), TEXT("hay_bale"), TEXT("pumpkin_patch"), TEXT("swamp_shroom"), TEXT("bog_lantern"),
        TEXT("gravestone"), TEXT("thorn_bramble"), TEXT("soul_flower"), TEXT("rock_gloom"), TEXT("elf_bloom"), TEXT("moon_fern"),
        TEXT("crystal_sprout"), TEXT("fern"), TEXT("fern_giant"), TEXT("glowcap"), TEXT("flowers_bell"), TEXT("flowers_tulip"),
        TEXT("flowers_daisy"), TEXT("flowers_lavender"), TEXT("clover"), TEXT("dandelion"), TEXT("poppy"), TEXT("wild_wheat"),
        TEXT("sunflower"), TEXT("leaf_litter"), TEXT("strawberry"), TEXT("grass_short"), TEXT("grass_tall"), TEXT("grass_seed"),
        TEXT("ash_grass"), TEXT("reeds"),
    };
}

uint16 FVortexTerrainData::KindOf(const TCHAR* Name)
{
    if (const uint16* Found = KindIndex.Find(Name)) return *Found;
    const int32 Existing = Kinds.IndexOfByKey(FString(Name));
    const uint16 Index = uint16(Existing != INDEX_NONE ? Existing : Kinds.Add(Name));
    KindIndex.Add(Name, Index);
    return Index;
}

bool FVortexTerrainData::LoadWorld(const FString& Directory, FString& OutError)
{
    FString MetaText;
    TArray<uint8> HeightBytes, RegionBytes, TintBytes, FlagBytes;
    if (!FFileHelper::LoadFileToString(MetaText, *FPaths::Combine(Directory, TEXT("world_meta.json"))) ||
        !FFileHelper::LoadFileToArray(HeightBytes, *FPaths::Combine(Directory, TEXT("world_height.bin"))) ||
        !FFileHelper::LoadFileToArray(RegionBytes, *FPaths::Combine(Directory, TEXT("world_region.bin"))) ||
        !FFileHelper::LoadFileToArray(TintBytes, *FPaths::Combine(Directory, TEXT("world_tint.bin"))) ||
        !FFileHelper::LoadFileToArray(FlagBytes, *FPaths::Combine(Directory, TEXT("world_flags.bin"))))
    {
        OutError = FString::Printf(TEXT("continente: arquivos não encontrados em %s"), *Directory);
        return false;
    }
    TSharedPtr<FJsonObject> Meta;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(MetaText), Meta) || !Meta.IsValid())
    {
        OutError = TEXT("continente: world_meta.json inválido");
        return false;
    }
    TSharedPtr<FWorldMap> Map = MakeShared<FWorldMap>();
    Map->Size = int32(Meta->GetNumberField(TEXT("size")));
    Map->Mpp = float(Meta->GetNumberField(TEXT("meters_per_pixel")));
    const TArray<TSharedPtr<FJsonValue>>& Home = Meta->GetArrayField(TEXT("home_pixel"));
    if (Home.Num() == 2) { Map->HomeU = float(Home[0]->AsNumber()); Map->HomeV = float(Home[1]->AsNumber()); }
    const int32 Pixels = Map->Size * Map->Size;
    if (Map->Size < 16 || HeightBytes.Num() != Pixels * 2 || RegionBytes.Num() != Pixels || TintBytes.Num() != Pixels * 3 || FlagBytes.Num() != Pixels)
    {
        OutError = TEXT("continente: tamanhos dos mapas não batem");
        return false;
    }
    Map->Height.SetNumUninitialized(Pixels);
    const FFloat16* Half = reinterpret_cast<const FFloat16*>(HeightBytes.GetData());
    for (int32 i = 0; i < Pixels; ++i) Map->Height[i] = Half[i].GetFloat();
    Map->Region = MoveTemp(RegionBytes);
    Map->Tint = MoveTemp(TintBytes);
    Map->Flags = MoveTemp(FlagBytes);
    Map->Names[0] = TEXT("Mar");
    for (const TSharedPtr<FJsonValue>& Value : Meta->GetArrayField(TEXT("regions")))
    {
        const TSharedPtr<FJsonObject> R = Value->AsObject();
        const int32 Id = int32(R->GetNumberField(TEXT("id")));
        if (Id <= 0 || Id >= 10) continue;
        Map->Names[Id] = R->GetStringField(TEXT("name"));
        const TArray<TSharedPtr<FJsonValue>>& Center = R->GetArrayField(TEXT("center"));
        if (Center.Num() == 2) Map->Centers[Id] = FVector2D(Center[0]->AsNumber(), Center[1]->AsNumber()) * 100.f;
    }
    // pontes e cabanas (o Godot monta em cima do terreno)
    FString FeatureText;
    TSharedPtr<FJsonObject> FeatureJson;
    if (FFileHelper::LoadFileToString(FeatureText, *FPaths::Combine(Directory, TEXT("world_features.json"))) &&
        FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(FeatureText), FeatureJson) && FeatureJson.IsValid())
    {
        for (const TCHAR* Field : { TEXT("bridges"), TEXT("cabins") })
        {
            const TArray<TSharedPtr<FJsonValue>>* List = nullptr;
            if (!FeatureJson->TryGetArrayField(Field, List)) continue;
            for (const TSharedPtr<FJsonValue>& Value : *List)
            {
                const TSharedPtr<FJsonObject> F = Value->AsObject();
                const TArray<TSharedPtr<FJsonValue>>& At = F->GetArrayField(TEXT("at"));
                if (At.Num() != 2) continue;
                FFeature Feature;
                Feature.At = FVector2D(At[0]->AsNumber(), At[1]->AsNumber()) * 100.f;
                Feature.Radians = float(F->GetNumberField(TEXT("yaw")));
                Feature.Yaw = -FMath::RadiansToDegrees(Feature.Radians);
                Feature.bBridge = FCString::Strcmp(Field, TEXT("bridges")) == 0;
                double Length = 0.0;
                if (F->TryGetNumberField(TEXT("length"), Length)) Feature.Length = float(Length) * 100.f;
                double Region = 0.0;
                if (F->TryGetNumberField(TEXT("region"), Region)) Feature.Region = int32(Region);
                // o índice da cabana (o baú "chest:N" do Godot)
                Feature.Index = WorldFeatures.FilterByPredicate([&](const FFeature& Other) { return Other.bBridge == Feature.bBridge; }).Num();
                WorldFeatures.Add(Feature);
            }
        }
    }
    World = Map;
    for (const TCHAR* Kind : WorldKinds) KindOf(Kind);
    return true;
}

FBox2D FVortexTerrainData::MapBounds() const
{
    if (!World.IsValid()) return PrototypeBounds();
    // o mapa do Godot (com o mar das bordas), alinhado aos pedaços
    const float Size = ChunkSize();
    const FIntPoint Min = MapChunkMin(), Count = MapChunkCount();
    return FBox2D(FVector2D(Min.X * Size, Min.Y * Size), FVector2D((Min.X + Count.X) * Size, (Min.Y + Count.Y) * Size));
}

FIntPoint FVortexTerrainData::MapChunkMin() const
{
    if (!World.IsValid()) return FIntPoint(ChunkMinX, ChunkMinY);
    const float Size = ChunkSize();
    return FIntPoint(FMath::FloorToInt(-World->HomeU * World->Mpp * 100.f / Size), FMath::FloorToInt(-World->HomeV * World->Mpp * 100.f / Size));
}

FIntPoint FVortexTerrainData::MapChunkCount() const
{
    if (!World.IsValid()) return FIntPoint(ChunkCountX, ChunkCountY);
    const float Span = World->Size * World->Mpp * 100.f;
    const int32 N = FMath::CeilToInt(Span / ChunkSize()) + 1;
    return FIntPoint(N, N);
}

FString FVortexTerrainData::RegionName(int32 Region) const
{
    return World.IsValid() && Region >= 0 && Region < 10 ? World->Names[Region] : FString(TEXT("Vale do Santuário"));
}

int32 FVortexTerrainData::RegionAt(float X, float Y) const
{
    if (!World.IsValid()) return Valdora;
    const float Xm = X / 100.f, Zm = Y / 100.f;
    if (FVector2D(Xm, Zm - VillageZ).Size() < HomeRadius) return Valdora;
    return MixedRegion(Xm, Zm);
}

float FVortexTerrainData::QuickGround(float X, float Y) const
{
    const int32 Cx = FMath::FloorToInt(X / CellSize), Cy = FMath::FloorToInt(Y / CellSize);
    if (const FCell* Exported = Find(Cx, Cy)) return Exported->Height4 * 25.f;
    if (!World.IsValid() || !HasChunk(FIntPoint(FMath::FloorToInt(X / ChunkSize()), FMath::FloorToInt(Y / ChunkSize())))) return -300.f;
    return FMath::Max(WorldGround(X / 100.f, Y / 100.f), -2.75f) * 100.f;
}

int32 FVortexTerrainData::AreaLevel(float X, float Y) const
{
    // só a distância da vila manda; fora do vale a faixa da região é o piso.
    // 11/11 (o usuário: "os monstros saindo dos portões estão nível 2 a 4, muito fortes para quem começa no 1"): a vila
    // cresceu (a fonte em (18, 21) m e a muralha a 71 m dela); até 220 m do centro é nível 1 (uns 150 m de campo depois dos
    // portões), depois +1 a cada 45 m (400 m ~5, 1 km ~18). Antes: nível 1 só até 70 m do centro antigo.
    const float Xm = X / 100.f, Zm = Y / 100.f;
    int32 Level = FMath::Clamp(1 + FMath::FloorToInt(FMath::Max(0.f, FVector2D(Xm - 18.f, Zm - 21.f).Size() - 220.f) / 45.f), 1, 95);
    if (World.IsValid() && FVector2D(Xm, Zm - VillageZ).Size() >= HomeRadius)
    {
        int32 Lo = 1, Hi = 15;
        switch (WorldRegion(Xm, Zm))
        {
        case Aurivale: Lo = 10; Hi = 22; break;
        case Verdalume: Lo = 15; Hi = 30; break;
        case Solcanion: Lo = 20; Hi = 35; break;
        case Lodonegro: Lo = 25; Hi = 40; break;
        case Raikor: Lo = 30; Hi = 45; break;
        case Brasafenda: Lo = 40; Hi = 55; break;
        case Nivaluz: Lo = 45; Hi = 60; break;
        case Morthuria: Lo = 55; Hi = 70; break;
        default: break;
        }
        const int32 Band = FMath::RoundToInt(FMath::Lerp(float(Lo), float(Hi), FMath::Clamp(FVector2D(Xm, Zm).Size() / 3200.f, 0.f, 1.f)));
        Level = FMath::Max(Level, FMath::Min(Band, Level + 6));
    }
    return Level;
}

// ---- o mapa do mundo (metros do Godot) --------------------------------------------------------------

float FVortexTerrainData::WorldHeight(float X, float Z) const
{
    const FWorldMap& M = *World;
    float U = X / M.Mpp + M.HomeU - .5f, V = Z / M.Mpp + M.HomeV - .5f;
    const int32 Iu = FMath::FloorToInt(U), Iv = FMath::FloorToInt(V);
    if (Iu < 0 || Iv < 0 || Iu >= M.Size - 1 || Iv >= M.Size - 1) return WaterLevelM - 6.f;
    const float Fu = U - Iu, Fv = V - Iv;
    const int32 I = Iv * M.Size + Iu;
    const float A = M.Height[I], B = M.Height[I + 1], C = M.Height[I + M.Size], D = M.Height[I + M.Size + 1];
    return (A * (1.f - Fu) + B * Fu) * (1.f - Fv) + (C * (1.f - Fu) + D * Fu) * Fv;
}

int32 FVortexTerrainData::WorldRegion(float X, float Z) const
{
    const FWorldMap& M = *World;
    const int32 Iu = FMath::FloorToInt(X / M.Mpp + M.HomeU), Iv = FMath::FloorToInt(Z / M.Mpp + M.HomeV);
    if (Iu < 0 || Iv < 0 || Iu >= M.Size || Iv >= M.Size) return Ocean;
    return M.Region[Iv * M.Size + Iu];
}

uint8 FVortexTerrainData::WorldFlag(float X, float Z) const
{
    const FWorldMap& M = *World;
    const int32 Iu = FMath::FloorToInt(X / M.Mpp + M.HomeU), Iv = FMath::FloorToInt(Z / M.Mpp + M.HomeV);
    if (Iu < 0 || Iv < 0 || Iu >= M.Size || Iv >= M.Size) return 0;
    return M.Flags[Iv * M.Size + Iu];
}

int32 FVortexTerrainData::MixedRegion(float X, float Z) const
{
    // a fronteira anda por um ruído largo (até ~55 m) e um fino (~8 m): as regiões se misturam em manchas
    const float Wx = Species(X * .35f, Z * .35f) * 55.f + Meadow(X * 2.5f, Z * 2.5f) * 8.f;
    const float Wz = Species(Z * .35f + 913.f, X * .35f - 377.f) * 55.f + Meadow(Z * 2.5f + 91.f, X * 2.5f) * 8.f;
    const int32 R = WorldRegion(X + Wx, Z + Wz);
    return R == Ocean ? WorldRegion(X, Z) : R;
}

FLinearColor FVortexTerrainData::WorldTint(float X, float Z) const
{
    const FWorldMap& M = *World;
    const float Wx = Species(X * .35f, Z * .35f) * 55.f, Wz = Species(Z * .35f + 913.f, X * .35f - 377.f) * 55.f;
    const int32 Iu = FMath::FloorToInt((X + Wx) / M.Mpp + M.HomeU), Iv = FMath::FloorToInt((Z + Wz) / M.Mpp + M.HomeV);
    FLinearColor Tint(1.f, 1.f, 1.f);
    if (Iu >= 0 && Iv >= 0 && Iu < M.Size && Iv < M.Size)
    {
        const int32 I = (Iv * M.Size + Iu) * 3;
        Tint = FLinearColor(M.Tint[I] / 200.f, M.Tint[I + 1] / 200.f, M.Tint[I + 2] / 200.f);
    }
    const float Tn = TintNoise(X, Z) * .06f;
    return FLinearColor(Tint.R + Tn, Tint.G + Tn * .5f, Tint.B - Tn * .5f);
}

float FVortexTerrainData::WorldGround(float X, float Z) const
{
    X = (FMath::FloorToFloat(X / CellM) + .5f) * CellM;
    Z = (FMath::FloorToFloat(Z / CellM) + .5f) * CellM;
    const float N = Noise(X, Z);
    // o vale de perto da vila (ondulações largas) e, a partir de 300 m, o relevo do continente
    float H = 3.6f + N * 2.6f + FMath::Sin(X * .025f) * .55f;
    const float Home = FVector2D(X, Z - VillageZ).Size();
    if (Home > HomeRadius)
    {
        const float Baked = WorldHeight(X, Z);
        const float Detail = (N * 1.8f + FMath::Sin(X * .025f) * .4f) * FMath::SmoothStep(WaterLevelM, WaterLevelM + 2.5f, Baked);
        H = FMath::Lerp(H, Baked + Detail, FMath::SmoothStep(HomeRadius, HomeRadius + 160.f, Home));
    }
    return H;
}

float FVortexTerrainData::Forest(float X, float Z, int32 Region) const
{
    float F = FMath::Clamp((ForestNoise(X, Z) + .1f) * 1.6f, 0.f, 1.f);
    switch (Region)
    {
    case Verdalume: F = FMath::Max(F * 1.2f, .4f); break;
    case Lodonegro: break;
    case Morthuria: F *= .8f; break;
    case Raikor: F *= .9f; break;
    case Nivaluz: F *= .35f; break;
    case Aurivale: case Brasafenda: F *= .25f; break;
    case Solcanion: F *= .08f; break;
    case Ocean: F = 0.f; break;
    default: break;
    }
    return FMath::Clamp(F, 0.f, 1.f);
}

uint8 FVortexTerrainData::RegionLayer(float X, float Z, float H, int32 Region, uint32 Seed) const
{
    const float N = Noise(X * 2.3f, Z * 2.3f);
    const uint8 Flag = WorldFlag(X, Z);
    if (Flag == 2) return Ice;
    // 07/11: os caminhos até os lugares marcantes (Tools/World/StampMarvels.py): terra batida
    if (Flag == 4 && H > WaterLevelM) return Path;
    const bool bShore = H < WaterLevelM + .9f;
    switch (Region)
    {
    case Brasafenda: return H > 70.f && N > -.1f ? Stone : Ash;
    case Raikor: return H > 105.f ? Snow : H > 60.f ? Stone : N > .25f ? Stone : Forest(X, Z, Region) > .5f ? GrassForest : GrassB;
    case Nivaluz: return H > 34.f && N > .2f ? Stone : Snow;
    case Solcanion: return bShore || (H < 12.f && N > .3f) ? Sand : RedSand;
    case Aurivale: return bShore ? Sand : GoldGrass;
    case Lodonegro: return N > .15f ? GrassForest : Mud;
    case Morthuria: return H > 22.f && N > .2f ? Stone : PurpleSoil;
    case Verdalume: return bShore ? Sand : N > .35f ? GrassForest : ElvenGrass;
    case Ocean: return Sand;
    default: break;
    }
    // Valdora fora do vale: campos e bosques
    if (bShore) return Sand;
    if (H > 20.f && N > .1f) return Stone;
    if (Forest(X, Z, Region) > .55f) return GrassForest;
    const float K = Unit(Seed);
    return K < .07f ? GrassFlowers : K < .55f ? GrassA : GrassB;
}

void FVortexTerrainData::SynthCell(int32 CellX, int32 CellY, FCell& Out) const
{
    const float X = (CellX + .5f) * CellM, Z = (CellY + .5f) * CellM;
    float H = WorldGround(X, Z);
    // emenda com a região exportada: a altura vai da célula da borda até a do continente em 40 m
    const int32 N = CellsPerChunk;
    const int32 MinX = ChunkMinX * N, MinY = ChunkMinY * N, MaxX = (ChunkMinX + ChunkCountX) * N - 1, MaxY = (ChunkMinY + ChunkCountY) * N - 1;
    const int32 Ex = FMath::Clamp(CellX, MinX, MaxX), Ey = FMath::Clamp(CellY, MinY, MaxY);
    const float Away = FVector2D(float(CellX - Ex), float(CellY - Ey)).Size() * CellM;
    constexpr float Blend = 40.f;
    if (Away < Blend)
        if (const FCell* Edge = Find(Ex, Ey)) H = FMath::Lerp(Edge->Height4 * .25f, H, FMath::SmoothStep(0.f, Blend, Away));
    H = FMath::Max(H, -2.75f);   // o "sem chão" do resto do jogo é -3 m
    H = FMath::FloorToFloat(H * 4.f) / 4.f;
    const bool bHome = FVector2D(X, Z - VillageZ).Size() < HomeRadius;
    const int32 Region = bHome ? Valdora : MixedRegion(X, Z);
    const uint32 Seed = Mix(CellX, CellY, 7341);
    const uint8 Layer = RegionLayer(X, Z, H, Region, Seed);
    FLinearColor Tint = bHome ? FLinearColor(1.f + TintNoise(X, Z) * .07f, 1.f, 1.f) : WorldTint(X, Z);
    if (H < WaterLevelM) Tint = FMath::Lerp(Tint, FLinearColor(.38f, .52f, .5f), FMath::SmoothStep(0.f, 1.f, (WaterLevelM - H) / 1.5f));
    const FLinearColor C = LayerAverage[Layer] * Tint;
    Out.Height4 = int16(FMath::RoundToInt(H * 4.f));
    Out.Layer = Layer;
    Out.Rotation = uint8((Seed >> 9) % 8);
    Out.R = uint8(FMath::Clamp(C.R * 255.f, 0.f, 255.f));
    Out.G = uint8(FMath::Clamp(C.G * 255.f, 0.f, 255.f));
    Out.B = uint8(FMath::Clamp(C.B * 255.f, 0.f, 255.f));
}

void FVortexTerrainData::SampleCell(int32 CellX, int32 CellY, float& OutHeight, FColor& OutColor) const
{
    FCell C;
    if (const FCell* Exported = Find(CellX, CellY)) C = *Exported;
    else if (World.IsValid())
    {
        // mar aberto: só a profundidade importa para o mapa
        const float Deep = WorldHeight((CellX + .5f) * CellM, (CellY + .5f) * CellM);
        if (Deep < WaterLevelM - 3.f) { OutHeight = FMath::Max(Deep, -2.75f) * 100.f; OutColor = FColor(60, 90, 100); return; }
        SynthCell(CellX, CellY, C);
    }
    else { OutHeight = -300.f; OutColor = FColor(10, 18, 40); return; }
    OutHeight = C.Height4 * 25.f;
    OutColor = FColor(C.R, C.G, C.B);
}

TSharedPtr<FVortexTerrainData::FSynthChunk> FVortexTerrainData::SynthChunk(const FIntPoint& Chunk, bool bScatter) const
{
    TSharedPtr<FSynthChunk> Known;
    {
        FReadScopeLock Read(SynthLock);
        if (const TSharedPtr<FSynthChunk>* Found = Synth.Find(Chunk)) Known = *Found;
        else if (const TSharedPtr<FSynthChunk>* Old = SynthOld.Find(Chunk)) Known = *Old;
    }
    if (Known && (!bScatter || Known->bScatter)) return Known;
    // monta fora da trava (as outras threads seguem); as células já prontas são aproveitadas
    TSharedPtr<FSynthChunk> Piece = MakeShared<FSynthChunk>();
    const int32 N = CellsPerChunk;
    if (Known) Piece->Cells = Known->Cells;
    else
    {
        Piece->Cells.SetNumUninitialized(N * N);
        for (int32 ix = 0; ix < N; ++ix) for (int32 iy = 0; iy < N; ++iy)
            SynthCell(Chunk.X * N + ix, Chunk.Y * N + iy, Piece->Cells[ix * N + iy]);
    }
    if (bScatter)
    {
        SynthScatter(Chunk, *Piece);
        Piece->bScatter = true;
    }
    FWriteScopeLock Write(SynthLock);
    // se outra thread montou junto, fica a dela (igual, pela mesma semente)
    if (const TSharedPtr<FSynthChunk>* Found = Synth.Find(Chunk))
        if (!bScatter || (*Found)->bScatter) return *Found;
    // memória: o continente inteiro não cabe; passando de ~8 mil pedaços, a leva atual vira a velha e a
    // velha sai (tudo se refaz igual). A velha fica viva uma leva inteira: quem leu um ponteiro de
    // plantas (PropsInChunk) termina de usar muito antes.
    if (Synth.Num() > 8000) { SynthOld = MoveTemp(Synth); Synth.Reset(); }
    Synth.Add(Chunk, Piece);
    return Piece;
}

bool FVortexTerrainData::IsScattered(const FIntPoint& Chunk) const
{
    if (IsExported(Chunk) || !World.IsValid() || !HasChunk(Chunk)) return true;
    FReadScopeLock Read(SynthLock);
    if (const TSharedPtr<FSynthChunk>* Found = Synth.Find(Chunk)) return (*Found)->bScatter;
    const TSharedPtr<FSynthChunk>* Old = SynthOld.Find(Chunk);
    return Old && (*Old)->bScatter;
}

const TArray<FVortexTerrainProp>* FVortexTerrainData::PropsInChunk(const FIntPoint& Chunk) const
{
    if (IsExported(Chunk) || !World.IsValid()) return PropsByChunk.Find(Chunk);
    if (!HasChunk(Chunk)) return nullptr;
    return &SynthChunk(Chunk, true)->Props;
}

const TArray<FVortexTerrainSolid>* FVortexTerrainData::SolidsInChunk(const FIntPoint& Chunk) const
{
    if (IsExported(Chunk) || !World.IsValid()) return SolidsByChunk.Find(Chunk);
    if (!HasChunk(Chunk)) return nullptr;
    return &SynthChunk(Chunk, true)->Solids;
}

const TCHAR* FVortexTerrainData::TreeKind(float X, float Z, float H, int32 Region) const
{
    const float S = (Species(X, Z) + 1.f) * .5f;
    const float R = (HashAt(X, Z) & 1023) / 1023.f;
    switch (Region)
    {
    case Verdalume: return S < .45f ? TEXT("tree_elven") : S < .75f ? TEXT("tree_moonleaf") : S < .88f ? TEXT("tree_cherry") : TEXT("tree_birch");
    case Raikor: return H > 70.f ? TEXT("tree_crag") : S < .7f ? TEXT("tree_storm_fir") : TEXT("tree_crag");
    case Nivaluz: return S < .78f ? TEXT("tree_snow_pine") : TEXT("tree_ice");
    case Brasafenda: return S < .55f ? TEXT("tree_ember") : TEXT("tree_ashpine");
    case Solcanion: return R < .55f ? TEXT("tree_cactus") : TEXT("tree_acacia");
    case Morthuria: return S < .6f ? TEXT("tree_haunted") : TEXT("tree_bone");
    case Lodonegro: return S < .5f ? TEXT("tree_mangrove") : S < .85f ? TEXT("tree_willow") : TEXT("tree_dead");
    case Aurivale: return S < .45f ? TEXT("tree_golden") : S < .8f ? TEXT("tree_apple") : TEXT("tree_autumn");
    default: break;
    }
    if (H > 30.f) return TEXT("tree_pine");
    return S < .12f ? TEXT("tree_cherry") : S < .35f ? TEXT("tree_birch") : S < .7f ? TEXT("tree_oak") : S < .85f ? TEXT("tree_autumn") : TEXT("tree_pine");
}

// Plantas, pedras e enfeites de um pedaço do continente (ValleyWorld.PlanScatter, fora do vale).
void FVortexTerrainData::SynthScatter(const FIntPoint& Chunk, FSynthChunk& Out) const
{
    const int32 N = CellsPerChunk;
    const float Size = N * CellM, X0 = Chunk.X * Size, Z0 = Chunk.Y * Size;
    FRandomStream Rng(int32(Mix(Chunk.X, Chunk.Y, 9157)));
    auto Rand = [&](float A, float B) { return Rng.FRandRange(A, B); };
    // altura (m) de qualquer ponto: as células deste pedaço já estão prontas; fora dele, direto
    auto Ground = [&](float X, float Z)
    {
        const int32 Cx = FMath::FloorToInt(X / CellM), Cz = FMath::FloorToInt(Z / CellM);
        const int32 Ix = Cx - Chunk.X * N, Iz = Cz - Chunk.Y * N;
        if (Ix >= 0 && Iz >= 0 && Ix < N && Iz < N) return Out.Cells[Ix * N + Iz].Height4 * .25f;
        FCell C;
        if (const FCell* Exported = Find(Cx, Cz)) return Exported->Height4 * .25f;
        SynthCell(Cx, Cz, C);
        return C.Height4 * .25f;
    };
    auto CanRoot = [&](float X, float Z)
    {
        const float C = Ground(X, Z);
        float Low = C, High = C;
        for (const FVector2D D : { FVector2D(-1, 0), FVector2D(1, 0), FVector2D(0, -1), FVector2D(0, 1) })
        {
            const float H = Ground(X + D.X, Z + D.Y);
            Low = FMath::Min(Low, H); High = FMath::Max(High, H);
        }
        return High - Low <= .75f;
    };
    auto Wet = [&](float X, float Z) { return Ground(X, Z) < WaterLevelM - .05f; };
    auto Blocked = [&](float X, float Z) { const uint8 Flag = WorldFlag(X, Z); return Flag == 1 || Flag == 2 || Flag == 4 || Wet(X, Z); };
    auto Add = [&](const TCHAR* Kind, float X, float Z, float Scale, float Sink = 0.f)
    {
        const uint16* Index = KindIndex.Find(Kind);
        if (!Index) return 0.f;
        FVortexTerrainProp Prop;
        Prop.Kind = *Index;
        Prop.Location = FVector(X, Z, Ground(X, Z) - Sink) * 100.f;
        Prop.YawDegrees = Rand(0.f, 360.f);
        Prop.Scale = Scale;
        Out.Props.Add(Prop);
        return Prop.YawDegrees;
    };
    auto Cylinder = [&](float Radius, float Height, const FVector& AtM)
    {
        FVortexTerrainSolid Solid;
        Solid.Shape = 0;
        Solid.Location = AtM * 100.f;
        Solid.A = Radius * 100.f;
        Solid.B = Height * 100.f;
        Out.Solids.Add(Solid);
    };
    auto Ball = [&](float Radius, const FVector& AtM)
    {
        FVortexTerrainSolid Solid;
        Solid.Shape = 1;
        Solid.Location = AtM * 100.f;
        Solid.A = Radius * 100.f;
        Out.Solids.Add(Solid);
    };
    // árvores: grade de 6 m com sorteio, a mata manda (copas largas, sem virar um bloco)
    for (float Gx = X0; Gx < X0 + Size - .01f; Gx += 6.f)
    for (float Gz = Z0; Gz < Z0 + Size - .01f; Gz += 6.f)
    {
        const float X = Gx + 3.f + Rand(-1.1f, 1.1f), Z = Gz + 3.f + Rand(-1.1f, 1.1f);
        if (Blocked(X, Z) || !CanRoot(X, Z)) continue;
        const int32 Region = RegionAt(X * 100.f, Z * 100.f);
        if (Region == Ocean) continue;
        const float F = Forest(X, Z, Region);
        if (Rng.FRand() > .1f + F * .64f) continue;
        const float H = Ground(X, Z);
        const FString Kind = TreeKind(X, Z, H, Region);
        const float Scale = Kind == TEXT("tree_pine") ? Rand(.8f, 1.05f) : Rand(.85f, 1.12f);
        Add(*Kind, X, Z, Scale, .06f);
        float TrunkRadius, TrunkHeight;
        Trunk(Kind, TrunkRadius, TrunkHeight);
        TrunkHeight *= Scale;
        Cylinder(TrunkRadius * Scale, TrunkHeight, FVector(X, Z, H + TrunkHeight * .5f - .06f));
        Cylinder(FMath::Min(TrunkRadius * 1.8f, TrunkRadius + .6f) * Scale, .5f, FVector(X, Z, H + .19f));
        // sob as árvores das regiões verdes: samambaias
        if ((Region == Valdora || Region == Verdalume || Region == Lodonegro || Region == Aurivale) && F > .4f && Rng.FRand() < .6f)
        {
            const float A = Rand(0.f, 2.f * UE_PI), R = Rand(1.8f, 3.2f) * Scale;
            Add(Region == Verdalume ? TEXT("moon_fern") : TEXT("fern"), X + FMath::Cos(A) * R, Z + FMath::Sin(A) * R, Rand(.8f, 1.2f));
        }
    }
    // enfeites de cada região (pedras, cristais, cactos, feno, lápides...) e as moitas e pedras de Valdora
    const int32 Decor = FMath::RoundToInt(Size * Size * .02f);
    for (int32 i = 0; i < Decor; ++i)
    {
        const float X = X0 + Rand(0.f, Size), Z = Z0 + Rand(0.f, Size);
        if (Blocked(X, Z)) continue;
        const float H = Ground(X, Z), R = Rng.FRand();
        const int32 Region = RegionAt(X * 100.f, Z * 100.f);
        auto Solid = [&](const TCHAR* Kind, float Scale, float Radius, float Height, float Sink = .04f)
        {
            if (!CanRoot(X, Z)) return;
            Add(Kind, X, Z, Scale, Sink);
            Cylinder(Radius * Scale, Height * Scale, FVector(X, Z, H + Height * Scale * .5f));
        };
        auto Rock = [&](const TCHAR* Kind, float Scale, float Sink)
        {
            Add(Kind, X, Z, Scale, Sink);
            Ball(.62f * Scale, FVector(X, Z, H + .35f * Scale));
        };
        switch (Region)
        {
        case Brasafenda:
            if (R < .3f) Solid(TEXT("basalt_spikes"), Rand(.8f, 1.4f), .55f, 2.4f);
            else if (R < .45f) Rock(TEXT("rock_basalt"), Rand(.7f, 1.2f), .2f);
            else if (R < .7f) Add(TEXT("ember_bloom"), X, Z, Rand(.9f, 1.3f));
            else Add(TEXT("rock_pebbles"), X, Z, Rand(.8f, 1.3f), .02f);
            break;
        case Raikor:
            if (R < .22f) Solid(TEXT("slate_spire"), Rand(.7f, 1.3f), .5f, 4.f);
            else if (R < .45f) Rock(H > 90.f ? TEXT("rock_snowy") : TEXT("rock_boulder"), Rand(.8f, 1.3f), .2f);
            else if (R < .6f) Add(TEXT("storm_crystal"), X, Z, Rand(1.f, 1.6f));
            else if (R < .8f) Add(TEXT("alpine_bloom"), X, Z, Rand(.9f, 1.2f));
            else Rock(TEXT("rock_mossy"), Rand(.7f, 1.2f), .08f);
            break;
        case Nivaluz:
            if (R < .3f) Solid(TEXT("snow_shrub"), Rand(1.2f, 1.7f), .38f, .5f);
            else if (R < .5f) Solid(TEXT("ice_spike"), Rand(.7f, 1.4f), .45f, 1.6f);
            else if (R < .7f) Rock(TEXT("rock_snowy"), Rand(.7f, 1.2f), .2f);
            else Add(TEXT("frost_flower"), X, Z, Rand(.9f, 1.2f));
            break;
        case Solcanion:
            if (R < .3f) Solid(TEXT("cactus_barrel"), Rand(1.1f, 1.6f), .45f, .6f);
            else if (R < .55f) Add(TEXT("desert_brush"), X, Z, Rand(1.f, 1.5f));
            else if (R < .67f) Add(TEXT("bleached_bones"), X, Z, Rand(1.f, 1.4f));
            else if (R < .85f) Rock(TEXT("rock_sandstone"), Rand(.8f, 1.4f), .2f);
            else Add(TEXT("rock_pebbles"), X, Z, Rand(.8f, 1.3f), .02f);
            break;
        case Aurivale:
            if (R < .2f) Solid(TEXT("hay_bale"), Rand(.9f, 1.1f), .7f, 1.1f);
            else if (R < .45f) Add(TEXT("pumpkin_patch"), X, Z, Rand(1.f, 1.4f));
            else if (R < .85f) Add(TEXT("bush_flower"), X, Z, Rand(1.4f, 1.9f), .04f);
            else Add(TEXT("rock_pebbles"), X, Z, Rand(.8f, 1.3f), .02f);
            break;
        case Lodonegro:
            if (R < .3f) Add(TEXT("swamp_shroom"), X, Z, Rand(.9f, 1.5f));
            else if (R < .55f) Add(TEXT("bog_lantern"), X, Z, Rand(1.f, 1.4f));
            else if (R < .85f) Add(TEXT("bush_green"), X, Z, Rand(1.4f, 1.9f), .04f);
            else Add(TEXT("stump"), X, Z, Rand(.9f, 1.1f));
            break;
        case Morthuria:
            if (R < .25f) Solid(TEXT("gravestone"), Rand(.9f, 1.2f), .55f, 1.f);
            else if (R < .5f) Add(TEXT("thorn_bramble"), X, Z, Rand(1.f, 1.5f));
            else if (R < .72f) Add(TEXT("soul_flower"), X, Z, Rand(.9f, 1.3f));
            else Rock(TEXT("rock_gloom"), Rand(.7f, 1.2f), .2f);
            break;
        case Verdalume:
            if (R < .25f) Add(TEXT("elf_bloom"), X, Z, Rand(1.f, 1.4f));
            else if (R < .45f) Add(TEXT("moon_fern"), X, Z, Rand(1.f, 1.5f));
            else if (R < .6f) Add(TEXT("crystal_sprout"), X, Z, Rand(1.f, 1.4f));
            else if (R < .8f) Rock(TEXT("rock_mossy"), Rand(.7f, 1.2f), .08f);
            break;
        case Ocean:
            break;
        default:
        {
            const float F = Forest(X, Z, Region);
            if (R < .35f)
            {
                if (!CanRoot(X, Z)) break;
                const float Scale = Rand(1.4f, 1.9f);
                Add(F > .3f ? TEXT("bush_green") : TEXT("bush_flower"), X, Z, Scale, .04f);
                Cylinder(.38f * Scale, .48f * Scale, FVector(X, Z, H + .24f * Scale));
            }
            else if (R < .6f) Add(TEXT("rock_pebbles"), X, Z, Rand(.8f, 1.3f), .02f);
            else if (R < .75f) Rock(TEXT("rock_mossy"), Rand(.7f, 1.2f), .08f);
            else if (R < .82f || (H > 6.f && R < .9f)) Rock(TEXT("rock_boulder"), Rand(.7f, 1.1f), .2f);
            else if (F > .35f && R < .95f)
            {
                const bool bLog = Rng.FRand() < .5f;
                const float Scale = Rand(.9f, 1.1f);
                Add(bLog ? TEXT("log_fallen") : TEXT("stump"), X, Z, Scale);
                Cylinder(.46f * Scale, .65f * Scale, FVector(X, Z, H + .35f * Scale));
            }
            break;
        }
        }
    }
    // tufos de cada região (a grama vira a grama com vento do terreno) e flores nos campos de Valdora
    ON_SCOPE_EXIT
    {
        // cabanas e pontes (world_features.json): nada de planta nem pedra em cima delas (PlanFeatures do Godot)
        const FVector2D Min(X0 * 100.f - 6000.f, Z0 * 100.f - 6000.f), Max((X0 + Size) * 100.f + 6000.f, (Z0 + Size) * 100.f + 6000.f);
        // os marcos (Yggdrasil e o caminho do arco, vilas, covis): clareira
        for (const AVortexLandmarks::FSite& Site : AVortexLandmarks::Sites())
        {
            if (FVector2D::Distance(Site.Center, FVector2D((X0 + Size * .5f) * 100.f, (Z0 + Size * .5f) * 100.f)) > Site.Radius + Size * 100.f) continue;
            const bool bTree = Site.Id == TEXT("yggdrasil");
            auto Clear = [&](const FVector& P)
            {
                if (FVector2D::Distance(FVector2D(P), Site.Center) < Site.Radius) return true;
                return bTree && FMath::Abs(P.X - Site.Center.X) < 700.f && P.Y < Site.Center.Y - 10000.f && P.Y > Site.Center.Y - 16800.f;
            };
            Out.Props.RemoveAll([&](const FVortexTerrainProp& Prop) { return Clear(Prop.Location); });
            Out.Solids.RemoveAll([&](const FVortexTerrainSolid& Solid) { return Clear(Solid.Location); });
        }
        for (const FFeature& Feature : WorldFeatures)
        {
            if (Feature.At.X < Min.X || Feature.At.Y < Min.Y || Feature.At.X > Max.X || Feature.At.Y > Max.Y) continue;
            const FVector2D Across(FMath::Cos(Feature.Radians), -FMath::Sin(Feature.Radians));
            const FVector2D A = Feature.At - Across * Feature.Length * .5f, B = Feature.At + Across * Feature.Length * .5f;
            auto Near = [&](const FVector& P)
            {
                const FVector2D Q(P);
                if (!Feature.bBridge) return FVector2D::Distance(Q, Feature.At) < 750.f;
                const FVector2D Span = B - A;
                const float T = FMath::Clamp(FVector2D::DotProduct(Q - A, Span) / FMath::Max(1.f, Span.SizeSquared()), 0.f, 1.f);
                return FVector2D::Distance(Q, A + Span * T) < 320.f;
            };
            Out.Props.RemoveAll([&](const FVortexTerrainProp& Prop) { return Near(Prop.Location); });
            Out.Solids.RemoveAll([&](const FVortexTerrainSolid& Solid) { return Near(Solid.Location); });
        }
    };
    const int32 Tufts = FMath::RoundToInt(Size * Size * .14f);
    for (int32 i = 0; i < Tufts; ++i)
    {
        const float X = X0 + Rand(0.f, Size), Z = Z0 + Rand(0.f, Size);
        if (Wet(X, Z)) continue;
        const float H = Ground(X, Z), K = Rng.FRand();
        const int32 Region = RegionAt(X * 100.f, Z * 100.f);
        const TCHAR* Kind = nullptr;
        switch (Region)
        {
        case Brasafenda: Kind = K < .35f ? TEXT("ash_grass") : K < .42f ? TEXT("ember_bloom") : K < .5f ? TEXT("rock_pebbles") : nullptr; break;
        case Solcanion: Kind = K < .22f ? TEXT("desert_brush") : K < .32f ? TEXT("grass_seed") : K < .4f ? TEXT("rock_pebbles") : nullptr; break;
        case Nivaluz: Kind = K < .12f ? TEXT("frost_flower") : K < .2f ? TEXT("rock_pebbles") : nullptr; break;
        case Raikor: Kind = H > 60.f ? (K < .12f ? TEXT("rock_pebbles") : K < .2f ? TEXT("alpine_bloom") : nullptr)
            : K < .45f ? TEXT("grass_short") : K < .62f ? TEXT("alpine_bloom") : K < .7f ? TEXT("fern") : nullptr; break;
        case Aurivale: Kind = K < .55f ? TEXT("wild_wheat") : K < .7f ? TEXT("sunflower") : K < .85f ? TEXT("dandelion") : TEXT("poppy"); break;
        case Lodonegro: Kind = K < .28f ? TEXT("reeds") : K < .45f ? TEXT("fern") : K < .55f ? TEXT("glowcap") : K < .62f ? TEXT("bog_lantern") : K < .8f ? TEXT("grass_tall") : TEXT("leaf_litter"); break;
        case Morthuria: Kind = K < .14f ? TEXT("soul_flower") : K < .3f ? TEXT("thorn_bramble") : K < .4f ? TEXT("ash_grass") : nullptr; break;
        case Verdalume: Kind = K < .22f ? TEXT("fern_giant") : K < .34f ? TEXT("moon_fern") : K < .46f ? TEXT("elf_bloom") : K < .54f ? TEXT("glowcap") : K < .6f ? TEXT("flowers_bell") : TEXT("grass_short"); break;
        case Ocean: break;
        default:
        {
            const float F = Forest(X, Z, Region), Field = (Meadow(X, Z) + 1.f) * .5f;
            static const TCHAR* Flowers[] = { TEXT("flowers_tulip"), TEXT("flowers_daisy"), TEXT("flowers_lavender"), TEXT("flowers_bell") };
            if (F < .3f && Field > .72f && Rng.FRand() < .45f) Kind = Flowers[int32((TintNoise(X * .5f, Z * .5f) * 2.f + 2.f)) & 3];
            else if (F > .45f && Rng.FRand() < .32f) Kind = K < .35f ? TEXT("fern_giant") : K < .65f ? TEXT("leaf_litter") : K < .8f ? TEXT("glowcap") : TEXT("strawberry");
            else if (F < .3f && Rng.FRand() < .26f) Kind = K < .3f ? TEXT("clover") : K < .55f ? TEXT("dandelion") : K < .75f ? TEXT("poppy") : K < .95f ? TEXT("wild_wheat") : TEXT("sunflower");
            else Kind = Rng.FRand() < .45f ? TEXT("grass_short") : Rng.FRand() < .7f ? TEXT("grass_tall") : TEXT("grass_seed");
            break;
        }
        }
        if (Kind) Add(Kind, X, Z, Rand(.85f, 1.2f));
    }
}
