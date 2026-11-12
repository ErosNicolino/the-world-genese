using Godot;
using System;
using System.Collections.Generic;
using System.IO;

// Exporta uma região do mundo para o protótipo de terreno na Unreal (Vortex-UE5-migracao):
// a mesma geração dos pedaços (mesma semente, mesma ordem de sorteio de GenerateTerrain e
// PlanScatter), sem montar nós. Assim a Unreal desenha exatamente o mesmo chão e as mesmas
// plantas, e a comparação de desempenho é justa.
//
// terrain.bin (little-endian):
//   "VTRN", int versão=1, int chunkMinX, int chunkMinZ, int countX, int countZ, int cellsPerChunk,
//   float cell (m), float waterLevel (m), float waterSurface (m)
//   para cada pedaço (Z por fora, X por dentro) e cada célula (ix por fora, iz por dentro, como
//   no Godot): short altura*4, byte camada, byte giro (0..7), byte R, G, B (cor da camada x tom)
// props.bin:
//   "VPRP", int versão=1, int espécies, (string UTF-8 com tamanho byte) por espécie,
//   int cópias, por cópia: ushort espécie, float x, y, z (m, Godot), float giro (rad), float escala
// solids.bin:
//   "VSOL", int versão=1, int formas; por forma: int chunkX, chunkZ, byte shape,
//   float x, y, z (m), float giro (rad), float A, B, float sizeX,Y,Z,
//   int pontos, seguido de float x,y,z por ponto (coordenadas locais, metros).
// Coordenadas do Godot (Y para cima, metros). A Unreal converte: X = x, Y = z, Z = y (cm).
public partial class ValleyWorld
{
	public (int Cells, int Props, int Kinds) ExportForUnreal(string dir, Vector2I chunkMin, Vector2I count)
	{
		Directory.CreateDirectory(dir);
		int cells = Mathf.RoundToInt(ChunkSize / Cell);
		var kinds = new List<string>();
		var kindIndex = new Dictionary<string, int>();
		var props = new List<(int Kind, Vector3 At, float Yaw, float Scale)>();
		var solids = new List<(Vector2I Chunk, SolidSpec Spec)>();
		int cellCount = 0;
		using (var terrain = new BinaryWriter(File.Create(Path.Combine(dir, "terrain.bin"))))
		{
			terrain.Write("VTRN".ToCharArray());
			terrain.Write(1);
			terrain.Write(chunkMin.X); terrain.Write(chunkMin.Y); terrain.Write(count.X); terrain.Write(count.Y);
			terrain.Write(cells); terrain.Write(Cell); terrain.Write(WaterLevel); terrain.Write(WaterSurface);
			for (int cz = 0; cz < count.Y; cz++)
			for (int cx = 0; cx < count.X; cx++)
			{
				var c = chunkMin + new Vector2I(cx, cz);
				float x0 = c.X * ChunkSize, z0 = c.Y * ChunkSize;
				var rng = new Random(ChunkSeed(worldSeed, c));
				// mesma ordem de GenerateTerrain/TerrainCell: altura, camada (sorteio), tom, giro (sorteio)
				for (int ix = 0; ix < cells; ix++)
				for (int iz = 0; iz < cells; iz++)
				{
					float x = x0 + (ix + .5f) * Cell, z = z0 + (iz + .5f) * Cell;
					float h = Ground(x, z);
					int top = TopLayer(x, z, h, rng);
					var tint = Tint(x, z);
					if (h < WaterLevel) tint = tint.Lerp(new Color(.38f, .52f, .5f), Mathf.SmoothStep(0, 1, (WaterLevel - h) / 1.5f));
					int rot = rng.Next(8);
					var color = TerrainBatch.LayerColor(top) * tint;
					terrain.Write((short)Mathf.RoundToInt(h * 4));
					terrain.Write((byte)top); terrain.Write((byte)rot);
					terrain.Write((byte)Mathf.Clamp(color.R * 255, 0, 255));
					terrain.Write((byte)Mathf.Clamp(color.G * 255, 0, 255));
					terrain.Write((byte)Mathf.Clamp(color.B * 255, 0, 255));
					cellCount++;
				}
				bool nearCamp = new Vector2(x0 + ChunkSize * .5f, z0 + ChunkSize * .5f - 3).Length() < 40;
				var plan = PlanScatter(x0, z0, ChunkSize, rng, nearCamp);
				foreach (var solid in plan.Solids) solids.Add((c, solid));
				foreach (var (kind, list) in plan.Sets)
				{
					if (!kindIndex.TryGetValue(kind, out int k)) { k = kindIndex[kind] = kinds.Count; kinds.Add(kind); }
					foreach (var t in list)
						props.Add((k, t.Origin, t.Basis.GetEuler().Y, t.Basis.X.Length()));
				}
			}
		}
		using (var file = new BinaryWriter(File.Create(Path.Combine(dir, "props.bin"))))
		{
			file.Write("VPRP".ToCharArray());
			file.Write(1);
			file.Write(kinds.Count);
			foreach (var k in kinds) { var bytes = System.Text.Encoding.UTF8.GetBytes(k); file.Write((byte)bytes.Length); file.Write(bytes); }
			file.Write(props.Count);
			foreach (var (k, at, yaw, scale) in props)
			{
				file.Write((ushort)k);
				file.Write(at.X); file.Write(at.Y); file.Write(at.Z);
				file.Write(yaw); file.Write(scale);
			}
		}
		using (var file = new BinaryWriter(File.Create(Path.Combine(dir, "solids.bin"))))
		{
			file.Write("VSOL".ToCharArray()); file.Write(1); file.Write(solids.Count);
			foreach (var (chunk, s) in solids)
			{
				file.Write(chunk.X); file.Write(chunk.Y); file.Write((byte)s.Shape);
				file.Write(s.At.X); file.Write(s.At.Y); file.Write(s.At.Z);
				file.Write(s.Rotation); file.Write(s.A); file.Write(s.B);
				file.Write(s.Size.X); file.Write(s.Size.Y); file.Write(s.Size.Z);
				file.Write(s.Points?.Length ?? 0);
				if (s.Points != null) foreach (var p in s.Points) { file.Write(p.X); file.Write(p.Y); file.Write(p.Z); }
			}
		}
		return (cellCount, props.Count, kinds.Count);
	}
}
