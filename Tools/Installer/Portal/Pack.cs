// Anexa o jogo empacotado ao fim do instalador (Portal.cs lê este formato).
//   Pack.exe <instalador sem pacote.exe> <pasta do jogo> <versão> <saída.exe>
// Formato: cabeçalho "TWGPAK1", versão, nº de arquivos, bytes totais; por arquivo: caminho relativo,
// tamanho, método (0 guardado, 1 deflate), tamanho gravado e os dados. No fim do .exe, 24 bytes:
// onde o pacote começa, quanto ocupa e "TWGPORT1".
using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Text;

static class Pack
{
    // já vêm comprimidos (Oodle, mídia): deflate só gastaria tempo
    static readonly HashSet<string> Stored = new HashSet<string>(StringComparer.OrdinalIgnoreCase)
        { ".pak", ".ucas", ".utoc", ".png", ".jpg", ".mp3", ".mp4", ".bk2", ".wav", ".ogg" };

    static int Main(string[] args)
    {
        if (args.Length != 4) { Console.Error.WriteLine("uso: Pack <stub.exe> <pasta do jogo> <versão> <saída.exe>"); return 2; }
        string stub = args[0], source = Path.GetFullPath(args[1]).TrimEnd('\\'), version = args[2], output = args[3];
        var files = Directory.GetFiles(source, "*", SearchOption.AllDirectories)
            .Where(f => !Excluded(f.Substring(source.Length + 1)))
            .OrderBy(f => f, StringComparer.OrdinalIgnoreCase).ToList();
        long total = files.Sum(f => new FileInfo(f).Length);

        File.Copy(stub, output, true);
        using (var fs = new FileStream(output, FileMode.Open, FileAccess.ReadWrite, FileShare.None, 1 << 20))
        {
            fs.Seek(0, SeekOrigin.End);
            long offset = fs.Position;
            var bw = new BinaryWriter(fs, Encoding.UTF8);
            bw.Write("TWGPAK1"); bw.Write(version); bw.Write(files.Count); bw.Write(total);
            int index = 0;
            foreach (string file in files)
            {
                string rel = file.Substring(source.Length + 1);
                long size = new FileInfo(file).Length;
                bool store = Stored.Contains(Path.GetExtension(file)) || size < 1024;
                bw.Write(rel); bw.Write(size); bw.Write((byte)(store ? 0 : 1));
                long methodPos = fs.Position - 1, lengthPos = fs.Position;
                bw.Write(0L); bw.Flush();
                long dataPos = fs.Position;
                if (!store)
                {
                    using (var input = File.OpenRead(file))
                    using (var deflate = new DeflateStream(fs, CompressionLevel.Optimal, true)) input.CopyTo(deflate, 1 << 20);
                    // não compensou: regrava sem compressão
                    if (fs.Position - dataPos >= size) { fs.SetLength(dataPos); fs.Seek(methodPos, SeekOrigin.Begin); fs.WriteByte(0); fs.Seek(dataPos, SeekOrigin.Begin); store = true; }
                }
                if (store) using (var input = File.OpenRead(file)) input.CopyTo(fs, 1 << 20);
                long stored = fs.Position - dataPos;
                fs.Seek(lengthPos, SeekOrigin.Begin); bw.Write(stored); bw.Flush();
                fs.Seek(0, SeekOrigin.End);
                Console.WriteLine("[{0}/{1}] {2} {3:N1} MB -> {4:N1} MB", ++index, files.Count, rel, size / 1048576.0, stored / 1048576.0);
            }
            bw.Flush();
            long length = fs.Position - offset;
            bw.Write(offset); bw.Write(length); bw.Write(Encoding.ASCII.GetBytes("TWGPORT1")); bw.Flush();
            Console.WriteLine("Pacote: {0} arquivos, {1:N2} GB do jogo, instalador com {2:N0} MB", files.Count, total / 1073741824.0, fs.Length / 1048576.0);
        }
        return 0;
    }

    // as listas do UAT e os símbolos não vão para o jogador
    static bool Excluded(string rel)
    {
        string name = Path.GetFileName(rel);
        return (!rel.Contains("\\") && name.StartsWith("Manifest_", StringComparison.OrdinalIgnoreCase))
            || name.EndsWith(".pdb", StringComparison.OrdinalIgnoreCase);
    }
}
