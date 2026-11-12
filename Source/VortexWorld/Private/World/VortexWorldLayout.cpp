#include "World/VortexWorldLayout.h"
#include "World/VortexWorldHooks.h"

#include "World/VortexTerrainData.h"
#include "World/VortexTerrainStreamer.h"
#include "World/VortexTown.h"

VortexWorldHooks::FHandlers& VortexWorldHooks::Handlers()
{
    static FHandlers Instance;
    return Instance;
}

bool VortexWorldLayout::BanditCampAt(const FIntPoint& Cell, FVector2D& OutAt)
{
    // como o Godot (ValleyPopulation.BanditCampAt): ~1 a cada 4 quadras, longe da cidade (120 m) e
    // fora do vale feito à mão; só em chão seco, fora da cidade
    const TSharedPtr<FVortexTerrainData> Terrain = AVortexTerrainStreamer::SharedData();
    if (!Terrain) return false;
    FRandomStream Rng(HashCombine(GetTypeHash(Cell.X * 9103), GetTypeHash(Cell.Y - 9103)));
    if (Rng.FRand() > .24f) return false;
    const float X0 = Cell.X * BanditCellSize, Y0 = Cell.Y * BanditCellSize;
    const float X = X0 + 1200.f + Rng.FRand() * (BanditCellSize - 2400.f), Y = Y0 + 1200.f + Rng.FRand() * (BanditCellSize - 2400.f);
    if (FVector2D(X, Y - 1600.f).Size() < 12000.f) return false;
    if (Terrain->IsExported(FIntPoint(FMath::FloorToInt(X / Terrain->ChunkSize()), FMath::FloorToInt(Y / Terrain->ChunkSize())))
        && FVector2D(X, Y).Size() < 30000.f) return false;
    const float Ground = Terrain->QuickGround(X, Y);
    if (!Terrain->InMap(X, Y) || Ground < Terrain->WaterLevel || Ground <= -299.f) return false;
    if (AVortexTown::Contains(FVector(X, Y, Ground))) return false;
    OutAt = FVector2D(X, Y);
    return true;
}
