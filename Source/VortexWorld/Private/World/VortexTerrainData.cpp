#include "World/VortexTerrainData.h"
#include "World/VortexWorldLayout.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
    struct FReader
    {
        const TArray<uint8>& Bytes;
        int64 Offset = 0;
        bool bOk = true;

        template <typename T>
        T Read()
        {
            T Value{};
            if (Offset + (int64)sizeof(T) > Bytes.Num()) { bOk = false; return Value; }
            FMemory::Memcpy(&Value, Bytes.GetData() + Offset, sizeof(T));
            Offset += sizeof(T);
            return Value;
        }

        bool Magic(const char* Expected)
        {
            for (int32 i = 0; i < 4; ++i) { if (Read<uint8>() != (uint8)Expected[i]) { bOk = false; } }
            return bOk;
        }
    };
}

TSharedPtr<FVortexTerrainData> FVortexTerrainData::Load(const FString& Directory, FString& OutError)
{
    TArray<uint8> TerrainBytes, PropBytes, SolidBytes;
    if (!FFileHelper::LoadFileToArray(TerrainBytes, *FPaths::Combine(Directory, TEXT("terrain.bin"))) ||
        !FFileHelper::LoadFileToArray(PropBytes, *FPaths::Combine(Directory, TEXT("props.bin"))) ||
        !FFileHelper::LoadFileToArray(SolidBytes, *FPaths::Combine(Directory, TEXT("solids.bin"))))
    {
        OutError = FString::Printf(TEXT("arquivos não encontrados em %s"), *Directory);
        return nullptr;
    }
    TSharedPtr<FVortexTerrainData> Data = MakeShared<FVortexTerrainData>();
    FReader T{ TerrainBytes };
    if (!T.Magic("VTRN") || T.Read<int32>() != 1)
    {
        OutError = TEXT("terrain.bin com formato desconhecido");
        return nullptr;
    }
    Data->ChunkMinX = T.Read<int32>();
    Data->ChunkMinY = T.Read<int32>();
    Data->ChunkCountX = T.Read<int32>();
    Data->ChunkCountY = T.Read<int32>();
    Data->CellsPerChunk = T.Read<int32>();
    Data->CellSize = T.Read<float>() * 100.f;
    Data->WaterLevel = T.Read<float>() * 100.f;
    Data->WaterSurface = T.Read<float>() * 100.f;
    const int64 CellCount = (int64)Data->ChunkCountX * Data->ChunkCountY * Data->CellsPerChunk * Data->CellsPerChunk;
    if (!T.bOk || Data->ChunkCountX < 1 || Data->ChunkCountY < 1 ||
        Data->ChunkCountX > 256 || Data->ChunkCountY > 256 ||
        Data->CellsPerChunk < 1 || Data->CellsPerChunk > 128 ||
        !FMath::IsFinite(Data->CellSize) || Data->CellSize < 1.f || Data->CellSize > 500.f ||
        CellCount < 1 || CellCount > 16000000 ||
        T.Offset + CellCount * 7 != TerrainBytes.Num())
    {
        OutError = TEXT("dimensões ou tamanho inválido em terrain.bin");
        return nullptr;
    }
    Data->Cells.SetNumUninitialized(CellCount);
    for (int64 i = 0; i < CellCount && T.bOk; ++i)
    {
        FCell& C = Data->Cells[i];
        C.Height4 = T.Read<int16>();
        C.Layer = T.Read<uint8>();
        C.Rotation = T.Read<uint8>();
        C.R = T.Read<uint8>(); C.G = T.Read<uint8>(); C.B = T.Read<uint8>();
    }
    if (!T.bOk)
    {
        OutError = TEXT("terrain.bin incompleto");
        return nullptr;
    }

    FReader P{ PropBytes };
    if (!P.Magic("VPRP") || P.Read<int32>() != 1)
    {
        OutError = TEXT("props.bin com formato desconhecido");
        return nullptr;
    }
    const int32 KindCount = P.Read<int32>();
    for (int32 k = 0; k < KindCount && P.bOk; ++k)
    {
        const uint8 Length = P.Read<uint8>();
        TArray<uint8> Utf8;
        Utf8.SetNumUninitialized(Length);
        for (int32 b = 0; b < Length; ++b) Utf8[b] = P.Read<uint8>();
        Utf8.Add(0);
        Data->Kinds.Add(UTF8_TO_TCHAR(reinterpret_cast<const char*>(Utf8.GetData())));
    }
    const int32 PropCount = P.Read<int32>();
    const float Chunk = Data->ChunkSize();
    for (int32 i = 0; i < PropCount && P.bOk; ++i)
    {
        FVortexTerrainProp Prop;
        Prop.Kind = P.Read<uint16>();
        const float X = P.Read<float>(), Y = P.Read<float>(), Z = P.Read<float>();
        // Godot (x, y para cima, z) em metros -> Unreal (x, z, y) em cm; o giro troca de sentido
        // porque a troca de eixos espelha o plano
        Prop.Location = FVector(X, Z, Y) * 100.f;
        Prop.YawDegrees = -FMath::RadiansToDegrees(P.Read<float>());
        Prop.Scale = P.Read<float>();
        const FIntPoint ChunkKey(FMath::FloorToInt(Prop.Location.X / Chunk), FMath::FloorToInt(Prop.Location.Y / Chunk));
        Data->PropsByChunk.FindOrAdd(ChunkKey).Add(Prop);
    }
    if (!P.bOk)
    {
        OutError = TEXT("props.bin incompleto");
        return nullptr;
    }
    FReader S{ SolidBytes };
    if (!S.Magic("VSOL") || S.Read<int32>() != 1)
    {
        OutError = TEXT("solids.bin com formato desconhecido");
        return nullptr;
    }
    const int32 SolidCount = S.Read<int32>();
    if (SolidCount < 0 || SolidCount > 1000000)
    {
        OutError = TEXT("quantidade inválida em solids.bin");
        return nullptr;
    }
    for (int32 i = 0; i < SolidCount && S.bOk; ++i)
    {
        // em linhas separadas: a ordem de avaliação dos argumentos de um construtor não é definida
        // em C++ (o MSVC lê da direita para a esquerda e trocava X com Y)
        const int32 KeyX = S.Read<int32>();
        const int32 KeyY = S.Read<int32>();
        const FIntPoint ChunkKey(KeyX, KeyY);
        FVortexTerrainSolid Solid;
        Solid.Shape = S.Read<uint8>();
        const float X = S.Read<float>(), Y = S.Read<float>(), Z = S.Read<float>();
        Solid.Location = FVector(X, Z, Y) * 100.f;
        Solid.YawDegrees = -FMath::RadiansToDegrees(S.Read<float>());
        Solid.A = S.Read<float>() * 100.f;
        Solid.B = S.Read<float>() * 100.f;
        const float Sx = S.Read<float>(), Sy = S.Read<float>(), Sz = S.Read<float>();
        Solid.Size = FVector(Sx, Sz, Sy) * 100.f;
        const int32 PointCount = S.Read<int32>();
        if (Solid.Shape > 3 || PointCount < 0 || PointCount > 256) { S.bOk = false; break; }
        Solid.Points.Reserve(PointCount);
        for (int32 p = 0; p < PointCount && S.bOk; ++p)
        {
            const float Px = S.Read<float>(), Py = S.Read<float>(), Pz = S.Read<float>();
            Solid.Points.Add(FVector(Px, Pz, Py) * 100.f);
        }
        if (Data->IsExported(ChunkKey)) Data->SolidsByChunk.FindOrAdd(ChunkKey).Add(MoveTemp(Solid));
    }
    if (!S.bOk || S.Offset != SolidBytes.Num())
    {
        OutError = TEXT("solids.bin incompleto ou inválido");
        return nullptr;
    }
    // 03/11: a cratera do Azhûr tinha um lago bem no meio (o chão do terreno do Godot ali ficava abaixo do nível da água):
    // o fundo dela sobe até um pouco acima da água (o chão de basalto da caldeira cobre por cima)
    {
        const float Floor = Data->WaterLevel + 60.f;
        const int32 Reach = FMath::CeilToInt(VortexWorldLayout::CalderaClearRadius / Data->CellSize);
        const int32 Cx = FMath::FloorToInt(VortexWorldLayout::CalderaCenterX / Data->CellSize);
        const int32 Cy = FMath::FloorToInt(VortexWorldLayout::CalderaCenterY / Data->CellSize);
        int32 Raised = 0;
        for (int32 Dx = -Reach; Dx <= Reach; ++Dx)
            for (int32 Dy = -Reach; Dy <= Reach; ++Dy)
            {
                if (FMath::Square(Dx) + FMath::Square(Dy) > FMath::Square(Reach)) continue;
                FCell* C = const_cast<FCell*>(Data->Find(Cx + Dx, Cy + Dy));
                if (!C || C->Height4 * 25.f <= -299.f || C->Height4 * 25.f >= Floor) continue;
                C->Height4 = int16(FMath::CeilToInt(Floor / 25.f));
                ++Raised;
            }
        UE_LOG(LogTemp, Display, TEXT("VORTEX_CALDERA_FLOOR %d células subiram acima da água"), Raised);
    }
    // 06/11: a vila nova (Data/GodotTown/vila.json, Tools/Town/BuildVila.py) fica num chão plano: dentro dos polígonos do
    // "flatten" a altura vira "level"; até "blend" metros para fora ela volta aos poucos para a do terreno (sem degrau)
    {
        FString Text;
        TSharedPtr<FJsonObject> Plan;
        const TSharedPtr<FJsonObject>* Flatten = nullptr;
        if (FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir() / TEXT("Data/GodotTown/vila.json"))) &&
            FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Plan) && Plan.IsValid() && Plan->TryGetObjectField(TEXT("flatten"), Flatten))
        {
            const float Level = (*Flatten)->GetNumberField(TEXT("level")) * 100.f;
            const float Blend = FMath::Max(1.f, (float)(*Flatten)->GetNumberField(TEXT("blend")) * 100.f);
            TArray<TArray<FVector2D>> Polys;
            FBox2D Area(ForceInit);
            for (const TSharedPtr<FJsonValue>& PolyValue : (*Flatten)->GetArrayField(TEXT("polys")))
            {
                TArray<FVector2D>& Poly = Polys.AddDefaulted_GetRef();
                for (const TSharedPtr<FJsonValue>& PointValue : PolyValue->AsArray())
                {
                    const TArray<TSharedPtr<FJsonValue>>& Pt = PointValue->AsArray();
                    Poly.Add(FVector2D(Pt[0]->AsNumber() * 100.0, Pt[1]->AsNumber() * 100.0));
                    Area += Poly.Last();
                }
            }
            auto Inside = [](const TArray<FVector2D>& Poly, const FVector2D& At)
            {
                bool bIn = false;
                for (int32 i = 0, j = Poly.Num() - 1; i < Poly.Num(); j = i++)
                    if ((Poly[i].Y > At.Y) != (Poly[j].Y > At.Y) && At.X < (Poly[j].X - Poly[i].X) * (At.Y - Poly[i].Y) / (Poly[j].Y - Poly[i].Y) + Poly[i].X) bIn = !bIn;
                return bIn;
            };
            auto EdgeDistance = [](const TArray<FVector2D>& Poly, const FVector2D& At)
            {
                float Best = TNumericLimits<float>::Max();
                for (int32 i = 0, j = Poly.Num() - 1; i < Poly.Num(); j = i++)
                {
                    const FVector2D A = Poly[j], B = Poly[i];
                    const float T = FMath::Clamp(FVector2D::DotProduct(At - A, B - A) / FMath::Max(1.f, (float)(B - A).SizeSquared()), 0.f, 1.f);
                    Best = FMath::Min(Best, (float)FVector2D::Distance(At, A + (B - A) * T));
                }
                return Best;
            };
            Area = Area.ExpandBy(Blend);
            // 07/11 (o usuário: "a natureza em volta da cidade flutuando: árvores, pedras e grama"): as plantas e pedras
            // do Godot têm a altura gravada do chão de antes; elas sobem ou descem junto com o chão aplainado
            TArray<TPair<FVortexTerrainProp*, float>> Moved;
            for (TPair<FIntPoint, TArray<FVortexTerrainProp>>& PropChunk : Data->PropsByChunk)
                for (FVortexTerrainProp& Prop : PropChunk.Value)
                    if (Area.IsInside(FVector2D(Prop.Location))) Moved.Add({ &Prop, Data->GroundAt(Prop.Location.X, Prop.Location.Y) });
            TArray<TPair<FVortexTerrainSolid*, float>> MovedSolids;
            for (TPair<FIntPoint, TArray<FVortexTerrainSolid>>& SolidChunk : Data->SolidsByChunk)
                for (FVortexTerrainSolid& Solid : SolidChunk.Value)
                    if (Area.IsInside(FVector2D(Solid.Location))) MovedSolids.Add({ &Solid, Data->GroundAt(Solid.Location.X, Solid.Location.Y) });
            int32 Changed = 0;
            for (int32 Cx = FMath::FloorToInt(Area.Min.X / Data->CellSize); Cx <= FMath::CeilToInt(Area.Max.X / Data->CellSize); ++Cx)
                for (int32 Cy = FMath::FloorToInt(Area.Min.Y / Data->CellSize); Cy <= FMath::CeilToInt(Area.Max.Y / Data->CellSize); ++Cy)
                {
                    FCell* C = const_cast<FCell*>(Data->Find(Cx, Cy));
                    if (!C) continue;
                    const FVector2D At((Cx + .5f) * Data->CellSize, (Cy + .5f) * Data->CellSize);
                    float Weight = 0.f;
                    for (const TArray<FVector2D>& Poly : Polys)
                        Weight = FMath::Max(Weight, Inside(Poly, At) ? 1.f : 1.f - FMath::SmoothStep(0.f, Blend, EdgeDistance(Poly, At)));
                    if (Weight <= 0.f) continue;
                    const float Old = C->Height4 * 25.f;
                    C->Height4 = int16(FMath::RoundToInt(FMath::Lerp(Old, Level, Weight) / 25.f));
                    // 07/11 (o usuário: "às vezes um chão de cascalho aleatório"): dentro da vila, as manchas de terra,
                    // areia e pedra dos caminhos antigos do Godot (camadas 4, 5 e 6) viram grama
                    if (Weight >= 1.f && C->Layer >= 4 && C->Layer <= 6)
                    {
                        C->Layer = 0;
                        const FColor Grass = FLinearColor(.386f, .629f, .270f).ToFColor(true);
                        C->R = Grass.R; C->G = Grass.G; C->B = Grass.B;
                    }
                    ++Changed;
                }
            for (const TPair<FVortexTerrainProp*, float>& M : Moved) M.Key->Location.Z += Data->GroundAt(M.Key->Location.X, M.Key->Location.Y) - M.Value;
            for (const TPair<FVortexTerrainSolid*, float>& M : MovedSolids) M.Key->Location.Z += Data->GroundAt(M.Key->Location.X, M.Key->Location.Y) - M.Value;
            UE_LOG(LogTemp, Display, TEXT("VORTEX_TOWN_FLAT %d células no nível %.0f cm, %d plantas e pedras acompanharam o chão"), Changed, Level, Moved.Num());
        }
    }
    return Data;
}

bool FVortexTerrainData::IsExported(const FIntPoint& Chunk) const
{
    return Chunk.X >= ChunkMinX && Chunk.Y >= ChunkMinY && Chunk.X < ChunkMinX + ChunkCountX && Chunk.Y < ChunkMinY + ChunkCountY;
}

namespace
{
    // o Estádio do Despertar (AVortexAwakeningStadium::Origin, x = 4 km) tem chão próprio: em volta dele
    // o continente fica vazio, como era antes (sem terreno, plantas nem criaturas no meio da arena)
    bool NearStadium(const FIntPoint& Chunk, float ChunkSize)
    {
        return FVector2D::Distance(FVector2D((Chunk.X + .5f) * ChunkSize, (Chunk.Y + .5f) * ChunkSize), FVector2D(400000.f, 0.f)) < 16000.f;
    }
}

bool FVortexTerrainData::HasChunk(const FIntPoint& Chunk) const
{
    if (IsExported(Chunk)) return true;
    return World.IsValid() && !NearStadium(Chunk, ChunkSize());
}

bool FVortexTerrainData::HasCell(int32 CellX, int32 CellY) const
{
    FCell C;
    return Cell(CellX, CellY, C);
}

bool FVortexTerrainData::Cell(int32 CellX, int32 CellY, FCell& Out) const
{
    if (const FCell* Exported = Find(CellX, CellY)) { Out = *Exported; return true; }
    if (!World.IsValid()) return false;
    const FIntPoint Chunk(FMath::FloorToInt((float)CellX / CellsPerChunk), FMath::FloorToInt((float)CellY / CellsPerChunk));
    if (NearStadium(Chunk, ChunkSize())) return false;
    const TSharedPtr<FSynthChunk> Piece = SynthChunk(Chunk, false);
    Out = Piece->Cells[(CellX - Chunk.X * CellsPerChunk) * CellsPerChunk + (CellY - Chunk.Y * CellsPerChunk)];
    return true;
}

const FVortexTerrainData::FCell* FVortexTerrainData::Find(int32 CellX, int32 CellY) const
{
    const int32 ChunkX = FMath::FloorToInt((float)CellX / CellsPerChunk);
    const int32 ChunkY = FMath::FloorToInt((float)CellY / CellsPerChunk);
    if (!IsExported(FIntPoint(ChunkX, ChunkY)))
    {
        return nullptr;
    }
    const int32 Ix = CellX - ChunkX * CellsPerChunk;
    const int32 Iz = CellY - ChunkY * CellsPerChunk;
    const int64 ChunkIndex = (int64)(ChunkY - ChunkMinY) * ChunkCountX + (ChunkX - ChunkMinX);
    return &Cells[(ChunkIndex * CellsPerChunk + Ix) * CellsPerChunk + Iz];
}

float FVortexTerrainData::CellHeight(int32 CellX, int32 CellY) const
{
    FCell C;
    return Cell(CellX, CellY, C) ? C.Height4 * 25.f : -300.f;
}

FColor FVortexTerrainData::CellColor(int32 CellX, int32 CellY) const
{
    FCell C;
    return Cell(CellX, CellY, C) ? FColor(C.R, C.G, C.B) : FColor(90, 110, 70);
}

uint8 FVortexTerrainData::CellLayer(int32 CellX, int32 CellY) const
{
    FCell C;
    return Cell(CellX, CellY, C) ? FMath::Min<uint8>(C.Layer, 20) : 0;
}

uint8 FVortexTerrainData::CellRotation(int32 CellX, int32 CellY) const
{
    FCell C;
    return Cell(CellX, CellY, C) ? C.Rotation : 0;
}

FLinearColor FVortexTerrainData::CellTint(int32 CellX, int32 CellY) const
{
    // Exported color is the tile average multiplied by the regional tint.
    // Recover the tint before sampling the original tile in the UE material.
    static const FLinearColor Average[21] = {
        { .386f,.629f,.270f }, { .418f,.659f,.288f }, { .413f,.654f,.291f },
        { .262f,.449f,.208f }, { .634f,.521f,.365f }, { .844f,.787f,.608f },
        { .510f,.522f,.494f }, { .439f,.410f,.249f }, { .457f,.345f,.237f },
        { .480f,.491f,.462f }, { .713f,.639f,.488f }, { .927f,.947f,.967f },
        { .717f,.832f,.895f }, { .268f,.236f,.236f }, { .797f,.501f,.304f },
        { .295f,.292f,.186f }, { .359f,.287f,.393f }, { .800f,.671f,.252f },
        { .272f,.602f,.300f }, { .748f,.786f,.824f }, { .624f,.356f,.223f }
    };
    const FLinearColor Color = FLinearColor::FromSRGBColor(CellColor(CellX, CellY));
    const FLinearColor RawMean = Average[CellLayer(CellX, CellY)];
    const FLinearColor Mean = FLinearColor::FromSRGBColor(FColor(
        uint8(RawMean.R * 255.f), uint8(RawMean.G * 255.f), uint8(RawMean.B * 255.f)));
    return FLinearColor(
        FMath::Clamp(Color.R / FMath::Max(Mean.R, .01f), .25f, 1.4f),
        FMath::Clamp(Color.G / FMath::Max(Mean.G, .01f), .25f, 1.4f),
        FMath::Clamp(Color.B / FMath::Max(Mean.B, .01f), .25f, 1.4f), 1.f);
}

float FVortexTerrainData::GroundAt(float X, float Y) const
{
    return CellHeight(FMath::FloorToInt(X / CellSize), FMath::FloorToInt(Y / CellSize));
}

bool FVortexTerrainData::IsWaterAt(float X, float Y, float& OutSurface) const
{
    const float Ground = GroundAt(X, Y);
    if (Ground >= WaterLevel || Ground <= -299.f)
    {
        return false;
    }
    OutSurface = WaterSurface;
    return true;
}
