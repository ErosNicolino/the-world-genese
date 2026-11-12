// "Portal": o instalador do The World Gêneses. Janela própria em WPF (sem XAML, compila só com o csc):
// o Vórtice é desenhado a cada quadro, os nove Selos das regiões acendem conforme a instalação avança e
// os quadros da abertura contam a lenda enquanto os arquivos chegam.
// O jogo vai anexado ao fim deste .exe pelo Pack.cs. Sem pacote anexado, o mesmo programa é o desinstalador.
//   /uninstall [pasta]   desinstala (o Painel de Controle chama assim)
//   /dir <pasta>         sugere a pasta de instalação
//   /demo                mostra a instalação inteira sem gravar nada (para ver a arte)
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Security.Principal;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Animation;
using System.Windows.Media.Effects;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;
using System.Windows.Shell;
using Microsoft.Win32;
using Path = System.IO.Path;
using Glyph = System.Windows.Shapes.Path;

[assembly: AssemblyTitle("The World Gêneses - Instalador")]
[assembly: AssemblyProduct("The World Gêneses")]
[assembly: AssemblyCompany("Eros Nicolino")]
[assembly: AssemblyCopyright("Eros Nicolino")]
[assembly: AssemblyVersion("1.0.0.0")]
[assembly: AssemblyFileVersion("1.0.0.0")]

namespace Geneses.Portal
{
    static class Product
    {
        public const string Name = "The World Gêneses";
        public const string Publisher = "Eros Nicolino";
        public const string Folder = "The World Geneses";
        public const string Exe = "Vortex.exe";
        public const string Icon = "Vortex.ico";
        public const string GameProcess = "Vortex-Win64-Shipping";
        public const string Uninstaller = "Desinstalar.exe";
        public const string Manifest = "portal.files";
        public const string Key = @"SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\TheWorldGeneses";
        // instalações antigas feitas com o Inno Setup (Tools\Installer\TheWorldGeneses.iss)
        public const string LegacyKey = @"SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{6E2B9C1A-4F7D-4B8E-9A31-5C30205D4869}_is1";
        public static readonly string[] Regions = { "Valdora", "Brasafenda", "Raikor", "Nivaluz", "Solcânion", "Aurivale", "Lodonegro", "Morthúria", "Verdalume" };
        public static readonly bool Admin = new WindowsPrincipal(WindowsIdentity.GetCurrent()).IsInRole(WindowsBuiltInRole.Administrator);
    }

    sealed class Options
    {
        public bool Uninstall, FromTemp, Demo;
        public string Dir;
        public static Options Parse(string[] args)
        {
            var o = new Options();
            for (int i = 0; i < args.Length; i++)
            {
                string a = args[i].ToLowerInvariant();
                if (a == "/uninstall") o.Uninstall = true;
                else if (a == "/fromtemp") o.FromTemp = true;
                else if (a == "/demo") o.Demo = true;
                else if (a == "/dir" && i + 1 < args.Length) o.Dir = args[++i];
                else if (!a.StartsWith("/")) o.Dir = args[i];
            }
            return o;
        }
    }

    static class Program
    {
        [STAThread]
        static int Main(string[] args)
        {
            var opt = Options.Parse(args);
            string exe = Assembly.GetExecutingAssembly().Location;
            Payload pack = opt.Demo ? null : Payload.Open(exe);
            if (pack == null && !opt.Demo) opt.Uninstall = true;
            string home = Path.GetDirectoryName(exe);
            if (opt.Uninstall && opt.Dir == null) opt.Dir = File.Exists(Path.Combine(home, Product.Exe)) ? home : Setup.FindInstalled() ?? home;

            // o desinstalador mora na pasta que vai apagar: roda uma cópia no TEMP
            if (opt.Uninstall && !opt.FromTemp && !opt.Demo)
            {
                string temp = Path.Combine(Path.GetTempPath(), "TWG_Selar_" + Guid.NewGuid().ToString("N").Substring(0, 8) + ".exe");
                File.Copy(exe, temp, true);
                Process.Start(new ProcessStartInfo(temp, "/uninstall /fromtemp \"" + opt.Dir.TrimEnd('\\') + "\"") { UseShellExecute = false });
                return 0;
            }

            var app = new Application { ShutdownMode = ShutdownMode.OnMainWindowClose };
            app.DispatcherUnhandledException += (s, e) =>
            {
                MessageBox.Show(e.Exception.ToString(), Product.Name, MessageBoxButton.OK, MessageBoxImage.Error);
                e.Handled = true; app.Shutdown(1);
            };
            Assets.Init();
            int code = app.Run(new PortalWindow(opt, pack));
            Assets.Cleanup();
            if (opt.FromTemp)
                Process.Start(new ProcessStartInfo("cmd.exe", "/c ping 127.0.0.1 -n 3 > nul & del /f /q \"" + exe + "\"")
                    { CreateNoWindow = true, UseShellExecute = false, WindowStyle = ProcessWindowStyle.Hidden });
            return code;
        }
    }

    // ---------------------------------------------------------------- pacote anexado

    sealed class Payload
    {
        const string HeaderMagic = "TWGPAK1", TrailerMagic = "TWGPORT1";
        public string ExePath, Version;
        public int FileCount;
        public long TotalBytes, Offset;
        long dataStart;

        public static Payload Open(string exe)
        {
            try
            {
                using (var fs = File.OpenRead(exe))
                {
                    if (fs.Length < 64) return null;
                    var br = new BinaryReader(fs, Encoding.UTF8);
                    fs.Seek(-24, SeekOrigin.End);
                    long offset = br.ReadInt64(), length = br.ReadInt64();
                    if (Encoding.ASCII.GetString(br.ReadBytes(8)) != TrailerMagic || offset <= 0 || offset + length + 24 != fs.Length) return null;
                    fs.Seek(offset, SeekOrigin.Begin);
                    if (br.ReadString() != HeaderMagic) return null;
                    var p = new Payload { ExePath = exe, Offset = offset, Version = br.ReadString(), FileCount = br.ReadInt32(), TotalBytes = br.ReadInt64() };
                    p.dataStart = fs.Position;
                    return p;
                }
            }
            catch { return null; }
        }

        public void Extract(string dir, Action<string, long> progress, CancellationToken ct, List<string> written)
        {
            string root = Path.GetFullPath(dir).TrimEnd('\\') + "\\";
            var buffer = new byte[1 << 20];
            long done = 0;
            using (var fs = new FileStream(ExePath, FileMode.Open, FileAccess.Read, FileShare.Read, 1 << 16))
            {
                var br = new BinaryReader(fs, Encoding.UTF8);
                fs.Seek(dataStart, SeekOrigin.Begin);
                for (int i = 0; i < FileCount; i++)
                {
                    string rel = br.ReadString();
                    long size = br.ReadInt64();
                    byte method = br.ReadByte();
                    long stored = br.ReadInt64(), next = fs.Position + stored;
                    string target = Path.GetFullPath(Path.Combine(root, rel));
                    if (!target.StartsWith(root, StringComparison.OrdinalIgnoreCase)) throw new InvalidDataException("Caminho inválido no pacote: " + rel);
                    Directory.CreateDirectory(Path.GetDirectoryName(target));
                    if (File.Exists(target)) File.SetAttributes(target, FileAttributes.Normal);
                    written.Add(target);
                    progress(rel, done);
                    using (var output = new FileStream(target, FileMode.Create, FileAccess.Write, FileShare.None, 1 << 16))
                    using (Stream input = method == 1 ? (Stream)new DeflateStream(new Slice(fs, stored), CompressionMode.Decompress) : new Slice(fs, stored))
                    {
                        output.SetLength(size);
                        int n;
                        while ((n = input.Read(buffer, 0, buffer.Length)) > 0)
                        {
                            ct.ThrowIfCancellationRequested();
                            output.Write(buffer, 0, n);
                            done += n;
                            progress(rel, done);
                        }
                    }
                    fs.Seek(next, SeekOrigin.Begin);
                }
            }
        }

        // o desinstalador é este mesmo programa sem o jogo anexado
        public void WriteStub(string path)
        {
            using (var input = File.OpenRead(ExePath))
            using (var output = File.Create(path)) new Slice(input, Offset).CopyTo(output, 1 << 20);
        }
    }

    // janela de leitura dentro do .exe; não fecha o arquivo de baixo
    sealed class Slice : Stream
    {
        readonly Stream inner; long left;
        public Slice(Stream inner, long length) { this.inner = inner; left = length; }
        public override int Read(byte[] buffer, int offset, int count)
        {
            if (left <= 0) return 0;
            int n = inner.Read(buffer, offset, (int)Math.Min(count, left));
            left -= n;
            return n;
        }
        public override bool CanRead { get { return true; } }
        public override bool CanSeek { get { return false; } }
        public override bool CanWrite { get { return false; } }
        public override long Length { get { throw new NotSupportedException(); } }
        public override long Position { get { throw new NotSupportedException(); } set { throw new NotSupportedException(); } }
        public override void Flush() { }
        public override long Seek(long offset, SeekOrigin origin) { throw new NotSupportedException(); }
        public override void SetLength(long value) { throw new NotSupportedException(); }
        public override void Write(byte[] buffer, int offset, int count) { throw new NotSupportedException(); }
    }

    // ---------------------------------------------------------------- instalar e desinstalar

    // o que o trabalho em segundo plano conta para a janela
    sealed class Work
    {
        public double Progress;
        public long Done, Total;
        public volatile string Status = "", File = "", Error;
        public volatile bool Finished, Cancelled;
        public readonly CancellationTokenSource Cts = new CancellationTokenSource();
    }

    static class Setup
    {
        public static string DefaultDir()
        {
            return Path.Combine(Product.Admin ? Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles)
                : Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "Programs"), Product.Folder);
        }

        public static string FindInstalled()
        {
            foreach (var hive in new[] { RegistryHive.LocalMachine, RegistryHive.CurrentUser })
                foreach (string key in new[] { Product.Key, Product.LegacyKey })
                    try
                    {
                        using (var root = RegistryKey.OpenBaseKey(hive, RegistryView.Registry64))
                        using (var k = root.OpenSubKey(key))
                        {
                            string dir = k == null ? null : k.GetValue("InstallLocation") as string;
                            if (!string.IsNullOrEmpty(dir) && Directory.Exists(dir)) return dir.TrimEnd('\\');
                        }
                    }
                    catch { }
            return null;
        }

        public static bool GameRunning() { return Process.GetProcessesByName(Product.GameProcess).Length > 0; }

        public static void Install(Payload pack, string dir, bool desktop, bool startMenu, Work w)
        {
            var written = new List<string>();
            bool fresh = !Directory.Exists(dir) || !Directory.EnumerateFileSystemEntries(dir).Any();
            var started = Stopwatch.StartNew();
            try
            {
                Directory.CreateDirectory(dir);
                w.Total = pack.TotalBytes;
                w.Status = "Trazendo Valdora para o seu computador";
                pack.Extract(dir, (rel, done) => { w.File = rel; Interlocked.Exchange(ref w.Done, done); w.Progress = .9 * done / Math.Max(1, pack.TotalBytes); }, w.Cts.Token, written);

                string uninstaller = Path.Combine(dir, Product.Uninstaller);
                pack.WriteStub(uninstaller);
                written.Add(uninstaller);
                w.Progress = .92;

                // bibliotecas do Visual C++ que a Unreal precisa (já instaladas = nada acontece)
                string vc = Path.Combine(dir, @"Engine\Extras\Redist\en-us\vc_redist.x64.exe");
                if (Product.Admin && File.Exists(vc))
                {
                    w.Status = "Preparando os componentes do Windows";
                    w.File = "vc_redist.x64.exe";
                    using (var p = Process.Start(new ProcessStartInfo(vc, "/install /quiet /norestart") { UseShellExecute = false, CreateNoWindow = true }))
                        p.WaitForExit(5 * 60 * 1000);
                }
                w.Progress = .97;

                w.Status = "Gravando os atalhos";
                string exe = Path.Combine(dir, Product.Exe), icon = Path.Combine(dir, Product.Icon);
                if (startMenu) written.Add(Shortcut(Environment.SpecialFolder.CommonPrograms, Environment.SpecialFolder.Programs, exe, icon, dir));
                if (desktop) written.Add(Shortcut(Environment.SpecialFolder.CommonDesktopDirectory, Environment.SpecialFolder.DesktopDirectory, exe, icon, dir));
                RemoveLegacy(dir);
                Register(dir, pack.Version, pack.TotalBytes);

                // a lista de arquivos guia o desinstalador; numa atualização, soma à lista anterior
                string manifest = Path.Combine(dir, Product.Manifest);
                var all = new SortedSet<string>(written, StringComparer.OrdinalIgnoreCase);
                if (File.Exists(manifest)) all.UnionWith(File.ReadAllLines(manifest).Where(l => l.Length > 0));
                all.Add(manifest);
                File.WriteAllLines(manifest, all, new UTF8Encoding(false));
                w.Progress = 1;
                w.Status = "Pronto";
                w.Finished = true;
            }
            catch (OperationCanceledException)
            {
                Rollback(written, dir);
                w.Cancelled = true;
            }
            catch (Exception e)
            {
                if (fresh) Rollback(written, dir);
                w.Error = e is UnauthorizedAccessException ? "Sem permissão para gravar em " + dir + ". Escolha outra pasta ou rode o instalador como administrador." : e.Message;
            }
        }

        public static void Uninstall(string dir, bool wipeSaves, Work w)
        {
            try
            {
                w.Status = "Selando o portal";
                List<string> files = Listed(dir);
                for (int i = 0; i < files.Count; i++)
                {
                    w.File = Path.GetFileName(files[i]);
                    w.Progress = .9 * i / Math.Max(1, files.Count);
                    TryDelete(files[i]);
                    Thread.Sleep(Math.Max(1, 2400 / Math.Max(1, files.Count)));
                }
                foreach (var hive in new[] { RegistryHive.LocalMachine, RegistryHive.CurrentUser })
                    foreach (string key in new[] { Product.Key, Product.LegacyKey })
                        try { using (var root = RegistryKey.OpenBaseKey(hive, RegistryView.Registry64)) root.DeleteSubKeyTree(key, false); } catch { }
                RemoveEmptyDirs(dir);
                if (wipeSaves)
                {
                    w.Status = "Apagando saves e configurações";
                    foreach (string saves in SaveFolders()) WipeSaved(saves);
                }
                w.Progress = 1;
                w.Finished = true;
            }
            catch (Exception e) { w.Error = e.Message; }
        }

        [DllImport("kernel32.dll")] static extern uint WTSGetActiveConsoleSessionId();
        [DllImport("wtsapi32.dll", CharSet = CharSet.Unicode)] static extern bool WTSQuerySessionInformation(IntPtr server, uint session, int infoClass, out IntPtr buffer, out uint bytes);
        [DllImport("wtsapi32.dll")] static extern void WTSFreeMemory(IntPtr memory);

        // elevado com a senha de outra conta, o AppData "atual" é o do administrador: os saves ficam na pasta de quem está usando o PC
        static IEnumerable<string> SaveFolders()
        {
            var folders = new HashSet<string>(StringComparer.OrdinalIgnoreCase) { Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "Vortex") };
            IntPtr buffer;
            uint bytes;
            if (WTSQuerySessionInformation(IntPtr.Zero, WTSGetActiveConsoleSessionId(), 5 /* WTSUserName */, out buffer, out bytes))
            {
                string user = Marshal.PtrToStringUni(buffer);
                WTSFreeMemory(buffer);
                string profiles = Path.GetDirectoryName(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile));
                if (!string.IsNullOrEmpty(user) && profiles != null) folders.Add(Path.Combine(profiles, user, "AppData", "Local", "Vortex"));
            }
            return folders;
        }

        // arquivo por arquivo: um único preso (o jogo ou o relatório de erro abertos) fazia o Directory.Delete desistir de tudo sem avisar
        static void WipeSaved(string saves)
        {
            string saved = Path.Combine(saves, "Saved");
            if (!Directory.Exists(saved)) return;
            try { foreach (string file in Directory.GetFiles(saved, "*", SearchOption.AllDirectories)) TryDelete(file); } catch { }
            RemoveEmptyDirs(saves);
        }

        // só apaga o que está na lista (ou, sem lista, o que o jogo empacotado traz): nunca a pasta inteira às cegas
        static List<string> Listed(string dir)
        {
            string root = Path.GetFullPath(dir).TrimEnd('\\') + "\\";
            string manifest = Path.Combine(dir, Product.Manifest);
            IEnumerable<string> list;
            if (File.Exists(manifest)) list = File.ReadAllLines(manifest);
            else
            {
                var found = new List<string>();
                if (File.Exists(Path.Combine(dir, Product.Exe)))
                {
                    foreach (string sub in new[] { "Engine", "Vortex" })
                        if (Directory.Exists(Path.Combine(dir, sub))) found.AddRange(Directory.GetFiles(Path.Combine(dir, sub), "*", SearchOption.AllDirectories));
                    foreach (string f in new[] { Product.Exe, Product.Icon, "NOTICES.txt", Product.Uninstaller, "unins000.exe", "unins000.dat" }) found.Add(Path.Combine(dir, f));
                }
                list = found;
            }
            return list.Where(f => f.Length > 0 && (f.StartsWith(root, StringComparison.OrdinalIgnoreCase) || f.EndsWith(".lnk", StringComparison.OrdinalIgnoreCase)))
                .Distinct(StringComparer.OrdinalIgnoreCase).ToList();
        }

        static void Rollback(List<string> written, string dir)
        {
            for (int i = written.Count - 1; i >= 0; i--) TryDelete(written[i]);
            RemoveEmptyDirs(dir);
        }

        static void TryDelete(string file)
        {
            for (int attempt = 0; attempt < 3; attempt++)
                try
                {
                    if (!File.Exists(file)) return;
                    File.SetAttributes(file, FileAttributes.Normal);
                    File.Delete(file);
                    return;
                }
                catch { Thread.Sleep(150); }
        }

        static void RemoveEmptyDirs(string dir)
        {
            if (!Directory.Exists(dir)) return;
            foreach (string sub in Directory.GetDirectories(dir)) RemoveEmptyDirs(sub);
            try { if (!Directory.EnumerateFileSystemEntries(dir).Any()) Directory.Delete(dir); } catch { }
        }

        static string Shortcut(Environment.SpecialFolder common, Environment.SpecialFolder personal, string exe, string icon, string dir)
        {
            string folder = Environment.GetFolderPath(Product.Admin ? common : personal);
            string link = Path.Combine(folder, Product.Name + ".lnk");
            Type shellType = Type.GetTypeFromProgID("WScript.Shell");
            object shell = Activator.CreateInstance(shellType);
            try
            {
                object sc = shellType.InvokeMember("CreateShortcut", BindingFlags.InvokeMethod, null, shell, new object[] { link });
                Type t = sc.GetType();
                t.InvokeMember("TargetPath", BindingFlags.SetProperty, null, sc, new object[] { exe });
                t.InvokeMember("WorkingDirectory", BindingFlags.SetProperty, null, sc, new object[] { dir });
                t.InvokeMember("IconLocation", BindingFlags.SetProperty, null, sc, new object[] { icon + ",0" });
                t.InvokeMember("Description", BindingFlags.SetProperty, null, sc, new object[] { "Sua jornada começa em Valdora" });
                t.InvokeMember("Save", BindingFlags.InvokeMethod, null, sc, null);
                Marshal.FinalReleaseComObject(sc);
            }
            finally { Marshal.FinalReleaseComObject(shell); }
            return link;
        }

        static void Register(string dir, string version, long bytes)
        {
            using (var root = RegistryKey.OpenBaseKey(Product.Admin ? RegistryHive.LocalMachine : RegistryHive.CurrentUser, RegistryView.Registry64))
            using (var k = root.CreateSubKey(Product.Key))
            {
                k.SetValue("DisplayName", Product.Name);
                k.SetValue("DisplayVersion", version);
                k.SetValue("Publisher", Product.Publisher);
                k.SetValue("DisplayIcon", Path.Combine(dir, Product.Icon));
                k.SetValue("InstallLocation", dir);
                k.SetValue("UninstallString", "\"" + Path.Combine(dir, Product.Uninstaller) + "\" /uninstall");
                k.SetValue("InstallDate", DateTime.Now.ToString("yyyyMMdd"));
                k.SetValue("EstimatedSize", (int)(bytes / 1024), RegistryValueKind.DWord);
                k.SetValue("NoModify", 1, RegistryValueKind.DWord);
                k.SetValue("NoRepair", 1, RegistryValueKind.DWord);
            }
        }

        // instalado antes pelo Inno Setup: este instalador assume a pasta e os atalhos
        static void RemoveLegacy(string dir)
        {
            foreach (var hive in new[] { RegistryHive.LocalMachine, RegistryHive.CurrentUser })
                try
                {
                    using (var root = RegistryKey.OpenBaseKey(hive, RegistryView.Registry64))
                    {
                        if (root.OpenSubKey(Product.LegacyKey) == null) continue;
                        root.DeleteSubKeyTree(Product.LegacyKey, false);
                    }
                    TryDelete(Path.Combine(dir, "unins000.exe"));
                    TryDelete(Path.Combine(dir, "unins000.dat"));
                    foreach (var f in new[] { Environment.SpecialFolder.CommonPrograms, Environment.SpecialFolder.Programs })
                    {
                        string group = Path.Combine(Environment.GetFolderPath(f), Product.Name);
                        if (Directory.Exists(group)) Directory.Delete(group, true);
                    }
                }
                catch { }
        }
    }

    // ---------------------------------------------------------------- arte, fontes e música

    static class Assets
    {
        public static string Temp;
        public static FontFamily Display = new FontFamily("Georgia");
        public static readonly FontFamily Serif = new FontFamily("Georgia"), Ui = new FontFamily("Segoe UI");
        public static string[] Lore = new string[0];

        public static void Init()
        {
            Temp = Path.Combine(Path.GetTempPath(), "TWG_Portal_" + Process.GetCurrentProcess().Id);
            Directory.CreateDirectory(Temp);
            try
            {
                Save("Cinzel-700.ttf"); Save("Cinzel-900.ttf");
                Display = new FontFamily(new Uri(Temp + "\\"), "./#Cinzel");
            }
            catch { }
            using (var s = Res("lore.txt"))
                if (s != null) Lore = new StreamReader(s, Encoding.UTF8).ReadToEnd().Replace("\r", "").Split('\n').Where(l => l.Length > 0).Select(l => l.Replace("|", "\n")).ToArray();
        }

        // a música é opcional: só entra se o BuildPortal.ps1 recebeu -Music
        public static bool HasMusic { get { using (var s = Res("music.mp3")) return s != null; } }

        public static void Cleanup() { try { Directory.Delete(Temp, true); } catch { } }

        public static Stream Res(string name) { return Assembly.GetExecutingAssembly().GetManifestResourceStream(name); }

        public static string Save(string name)
        {
            string path = Path.Combine(Temp, name);
            using (var s = Res(name))
            {
                if (s == null) return null;
                using (var f = File.Create(path)) s.CopyTo(f);
            }
            return path;
        }

        public static BitmapImage Image(string name)
        {
            using (var s = Res(name))
            {
                if (s == null) return null;
                var img = new BitmapImage();
                img.BeginInit();
                img.StreamSource = s;
                img.CacheOption = BitmapCacheOption.OnLoad;
                img.EndInit();
                img.Freeze();
                return img;
            }
        }
    }

    sealed class Music
    {
        readonly MediaPlayer player = new MediaPlayer();
        double volume;
        public bool On = true;

        public Music(string file)
        {
            if (file == null) { On = false; return; }
            player.MediaEnded += (s, e) => { player.Position = TimeSpan.Zero; player.Play(); };
            player.Volume = 0;
            player.Open(new Uri(file));
            player.Play();
        }

        public void Step(double dt, double duck)
        {
            double target = On ? .34 * duck : 0;
            volume += (target - volume) * Math.Min(1, dt * 1.6);
            player.Volume = volume;
        }

        public void Stop() { player.Stop(); player.Close(); }
    }

    static class Theme
    {
        public static readonly Color Bg = Color.FromRgb(0x07, 0x0B, 0x10), Gold = Color.FromRgb(0xE9, 0xC2, 0x7A),
            Cream = Color.FromRgb(0xFA, 0xEF, 0xD3), Muted = Color.FromRgb(0xB4, 0xAB, 0x95), Emerald = Color.FromRgb(0x3B, 0xE8, 0xA0),
            Purple = Color.FromRgb(0x9A, 0x5C, 0xFF), Ember = Color.FromRgb(0xFF, 0x86, 0x4A), Danger = Color.FromRgb(0xFF, 0x6B, 0x5B);
        public static readonly CultureInfo Pt = new CultureInfo("pt-BR");

        public static Color A(Color c, double a) { return Color.FromArgb((byte)(Math.Max(0, Math.Min(1, a)) * 255), c.R, c.G, c.B); }

        public static SolidColorBrush B(Color c, double a = 1)
        {
            var b = new SolidColorBrush(A(c, a));
            b.Freeze();
            return b;
        }

        public static TextBlock Text(string text, FontFamily font, double size, FontWeight weight, Brush brush)
        {
            return new TextBlock { Text = text, FontFamily = font, FontSize = size, FontWeight = weight, Foreground = brush, TextWrapping = TextWrapping.Wrap };
        }

        // letras espaçadas para os sobretítulos
        public static string Spaced(string s) { return string.Join(" ", s.ToCharArray().Select(c => c.ToString())); }

        public static string Bytes(long b)
        {
            return b >= 1L << 30 ? (b / (double)(1L << 30)).ToString("0.0", Pt) + " GB" : (b / (double)(1L << 20)).ToString("0", Pt) + " MB";
        }

        public static void Animate(Animatable target, DependencyProperty prop, double to, double ms, double delayMs = 0, IEasingFunction ease = null)
        {
            target.BeginAnimation(prop, new DoubleAnimation(to, TimeSpan.FromMilliseconds(ms))
                { BeginTime = TimeSpan.FromMilliseconds(delayMs), EasingFunction = ease ?? new CubicEase { EasingMode = EasingMode.EaseOut } });
        }

        public static void Animate(UIElement target, DependencyProperty prop, double to, double ms, double delayMs = 0, IEasingFunction ease = null)
        {
            target.BeginAnimation(prop, new DoubleAnimation(to, TimeSpan.FromMilliseconds(ms))
                { BeginTime = TimeSpan.FromMilliseconds(delayMs), EasingFunction = ease ?? new CubicEase { EasingMode = EasingMode.EaseOut } });
        }
    }

    // ---------------------------------------------------------------- o Vórtice

    // Partículas em espiral numa imagem de meia resolução que se apaga um pouco a cada quadro (os rastros).
    // A imagem é esticada para a janela inteira; a ampliação suave vira o brilho.
    sealed class VortexView
    {
        public const int W = 520, H = 310;
        public const double Scale = 2, CenterX = 730, CenterY = 322;
        const float Cx = (float)(CenterX / Scale), Cy = (float)(CenterY / Scale), Tilt = .84f, Eye = 24, Outer = 168;
        const int N = 3200;

        public readonly Image Element;
        public double Target = .32, PresenceTarget = 1;
        double intensity = .1, presence;
        // com transparência: o que não brilha deixa ver a arte de capa por baixo (a luz soma, como num "screen")
        readonly WriteableBitmap bitmap = new WriteableBitmap(W, H, 96, 96, PixelFormats.Pbgra32, null);
        readonly uint[] px = new uint[W * H];
        readonly float[] radius = new float[N], angle = new float[N], speed = new float[N], glow = new float[N];
        readonly byte[] kind = new byte[N];
        readonly Random rng = new Random(9);
        readonly int[] starAt = new int[260];
        readonly float[] starGlow = new float[260], starPhase = new float[260];
        readonly int eyeX0, eyeY0, eyeSize;
        readonly float[] eyeShade, eyeRing;
        // roxo, esmeralda, ouro (centelhas) e o branco-violeta perto do olho
        static readonly float[,] Palette = { { 154, 92, 255 }, { 59, 232, 160 }, { 233, 194, 122 }, { 225, 215, 255 } };

        public VortexView()
        {
            Element = new Image { Source = bitmap, Stretch = Stretch.Fill, IsHitTestVisible = false };
            RenderOptions.SetBitmapScalingMode(Element, BitmapScalingMode.Linear);
            for (int i = 0; i < N; i++) Spawn(i, true);
            for (int i = 0; i < starAt.Length; i++)
            {
                starAt[i] = rng.Next(W) + rng.Next(H) * W;
                starGlow[i] = (float)(Math.Pow(rng.NextDouble(), 3) * 26 + 3);
                starPhase[i] = (float)(rng.NextDouble() * 6.28);
            }
            // o olho: um disco escuro com um anel de luz, calculado uma vez
            eyeSize = (int)(Eye * 2.6f);
            eyeX0 = (int)Cx - eyeSize / 2; eyeY0 = (int)Cy - eyeSize / 2;
            eyeShade = new float[eyeSize * eyeSize]; eyeRing = new float[eyeSize * eyeSize];
            for (int y = 0; y < eyeSize; y++)
                for (int x = 0; x < eyeSize; x++)
                {
                    float dx = eyeX0 + x - Cx, dy = (eyeY0 + y - Cy) / Tilt, d = (float)Math.Sqrt(dx * dx + dy * dy);
                    float t = Math.Max(0, Math.Min(1, (d - (Eye - 9)) / 10f));
                    eyeShade[y * eyeSize + x] = t * t * (3 - 2 * t);
                    float ring = (d - Eye) / 2.6f;
                    eyeRing[y * eyeSize + x] = (float)Math.Exp(-ring * ring);
                }
        }

        void Spawn(int i, bool anywhere)
        {
            float r = anywhere ? Eye + (Outer - Eye) * (float)Math.Sqrt(rng.NextDouble()) : Outer - 40 + 40 * (float)rng.NextDouble();
            double roll = rng.NextDouble();
            kind[i] = (byte)(roll < .04 ? 2 : roll < .40 ? 1 : 0);
            int arm = rng.Next(3);
            radius[i] = r;
            angle[i] = (float)(arm * 2.0944 + Math.Log(r) * 2.4 + Gauss() * .32);
            speed[i] = .8f + .4f * (float)rng.NextDouble();
            glow[i] = .45f + .55f * (float)rng.NextDouble();
        }

        double Gauss() { return Math.Sqrt(-2 * Math.Log(1 - rng.NextDouble())) * Math.Cos(6.2832 * rng.NextDouble()); }

        public double Intensity { get { return intensity; } }

        // um empurrão (o portal abrindo)
        public void Burst(double amount) { intensity += amount; }

        public void Step(double dt, double time)
        {
            intensity += (Target - intensity) * Math.Min(1, dt * 1.2);
            presence += (PresenceTarget - presence) * Math.Min(1, dt * .9);
            float I = (float)intensity, P = (float)presence;

            // rastros: tudo perde um pouco de luz por quadro
            uint f = (uint)(256 * Math.Pow(.9 - .05 * Math.Min(1, I), dt * 60));
            for (int i = 0; i < px.Length; i++)
            {
                uint p = px[i];
                px[i] = ((((p & 0xFF00FFu) * f) >> 8) & 0xFF00FFu) | ((((p & 0x00FF00u) * f) >> 8) & 0x00FF00u);
            }
            float keep = 1 - f / 256f;
            for (int i = 0; i < starAt.Length; i++)
            {
                float s = starGlow[i] * keep * (.65f + .35f * (float)Math.Sin(time * 1.3 + starPhase[i]));
                Add(starAt[i], s * .85f, s * .9f, s);
            }

            float spin = (.22f + 1.9f * I) * 22f, pull = 3f + 30f * I;
            for (int i = 0; i < N; i++)
            {
                float r = radius[i];
                angle[i] += spin / (r + 10f) * speed[i] * (float)dt;
                r -= pull * (float)dt * (.35f + 30f / (r + 6f)) * speed[i];
                if (r < Eye - 3) { Spawn(i, false); r = radius[i]; }
                radius[i] = r;
                float x = Cx + (float)Math.Cos(angle[i]) * r, y = Cy + (float)Math.Sin(angle[i]) * r * Tilt;
                float near = 1 - (r - Eye) / (Outer - Eye), edge = Math.Min(1, (Outer + 4 - r) / 34f);
                float b = glow[i] * (.18f + .82f * near * near) * (.55f + .7f * I) * edge * P * 230f;
                int k = kind[i];
                float w = k == 2 ? 0 : Math.Max(0, (near - .72f) / .28f);
                Splat(x, y, b * (Palette[k, 0] * (1 - w) + Palette[3, 0] * w) / 255f, b * (Palette[k, 1] * (1 - w) + Palette[3, 1] * w) / 255f,
                    b * (Palette[k, 2] * (1 - w) + Palette[3, 2] * w) / 255f);
            }

            float ring = (.3f + .7f * I) * P * 130f * keep;
            for (int y = 0; y < eyeSize; y++)
            {
                int row = (eyeY0 + y) * W + eyeX0;
                for (int x = 0; x < eyeSize; x++)
                {
                    int at = row + x, e = y * eyeSize + x;
                    uint p = px[at];
                    float s = eyeShade[e];
                    px[at] = ((uint)(((p >> 16) & 255) * s) << 16) | ((uint)(((p >> 8) & 255) * s) << 8) | (uint)((p & 255) * s);
                    float g = eyeRing[e] * ring;
                    if (g > .5f) Add(at, g * .45f, g, g * .75f);
                }
            }
            // a opacidade de cada ponto é o canal mais claro dele (as cores já estão pré-multiplicadas); o
            // apagamento do próximo quadro limpa esses bits de novo
            for (int i = 0; i < px.Length; i++)
            {
                uint p = px[i] & 0xFFFFFFu, r = p >> 16, g = (p >> 8) & 255, b = p & 255;
                px[i] = p | (Math.Max(r, Math.Max(g, b)) << 24);
            }
            bitmap.WritePixels(new Int32Rect(0, 0, W, H), px, W * 4, 0);
        }

        void Splat(float x, float y, float r, float g, float b)
        {
            int ix = (int)x, iy = (int)y;
            if (ix < 0 || iy < 0 || ix >= W - 1 || iy >= H - 1) return;
            float fx = x - ix, fy = y - iy;
            int at = iy * W + ix;
            float w00 = (1 - fx) * (1 - fy), w10 = fx * (1 - fy), w01 = (1 - fx) * fy, w11 = fx * fy;
            Add(at, r * w00, g * w00, b * w00);
            Add(at + 1, r * w10, g * w10, b * w10);
            Add(at + W, r * w01, g * w01, b * w01);
            Add(at + W + 1, r * w11, g * w11, b * w11);
        }

        void Add(int at, float r, float g, float b)
        {
            uint p = px[at];
            uint R = Math.Min(255u, ((p >> 16) & 255) + (uint)r), G = Math.Min(255u, ((p >> 8) & 255) + (uint)g), B = Math.Min(255u, (p & 255) + (uint)b);
            px[at] = (R << 16) | (G << 8) | B;
        }
    }

    // ---------------------------------------------------------------- os nove Selos

    sealed class SealRing : Canvas
    {
        public const double RadiusX = 222, RadiusY = 186;
        // uma runa por região, desenhada numa caixa de -10 a 10
        static readonly string[] Runes =
        {
            "M0,-10 L0,10 M0,-10 L7,-3 M0,-2 L7,5",
            "M-6,10 L0,-10 L6,10 M-3.5,3 L3.5,3",
            "M-6,-10 L4,-1 L-4,1 L6,10",
            "M0,-10 L0,10 M-8,-5 L8,5 M-8,5 L8,-5",
            "M-8,0 A8,8 0 1 1 8,0 A8,8 0 1 1 -8,0 M0,-3 L0,3 M-3,0 L3,0",
            "M-7,-9 L0,0 L7,-9 M0,0 L0,10",
            "M-6,-10 L-6,10 L7,10 M-6,1 L6,-8",
            "M0,-10 L0,10 M-7,-10 L0,-3 L7,-10 M-5,10 L5,10",
            "M0,10 L0,-10 M0,-3 L-7,-10 M0,-3 L7,-10 M0,4 L-7,-3 M0,4 L7,-3",
        };
        readonly Grid[] seals = new Grid[9];
        readonly Ellipse[] discs = new Ellipse[9];
        readonly Glyph[] runes = new Glyph[9];
        readonly Image[] emblems = new Image[9];
        readonly BitmapSource[] color = new BitmapSource[9], gray = new BitmapSource[9];
        readonly TextBlock[] names = new TextBlock[9];
        readonly bool[] lit = new bool[9];
        readonly RotateTransform spin = new RotateTransform();
        public Color Tint = Theme.Emerald;

        public SealRing()
        {
            IsHitTestVisible = false;
            double cx = VortexView.CenterX, cy = VortexView.CenterY;
            // órbita tracejada que gira devagar
            var orbit = new Ellipse
            {
                Width = RadiusX * 2, Height = RadiusX * 2, Stroke = Theme.B(Theme.Gold, .22), StrokeThickness = 1,
                StrokeDashArray = new DoubleCollection { 2, 6 }, RenderTransformOrigin = new Point(.5, .5),
                RenderTransform = new TransformGroup { Children = new TransformCollection { spin, new ScaleTransform(1, RadiusY / RadiusX) } }
            };
            SetLeft(orbit, cx - RadiusX); SetTop(orbit, cy - RadiusX);
            Children.Add(orbit);
            var inner = new Ellipse { Width = 300, Height = 300 * RadiusY / RadiusX, Stroke = Theme.B(Theme.Gold, .08), StrokeThickness = 1 };
            SetLeft(inner, cx - 150); SetTop(inner, cy - 150 * RadiusY / RadiusX);
            Children.Add(inner);
            spin.BeginAnimation(RotateTransform.AngleProperty, new DoubleAnimation(0, 360, TimeSpan.FromSeconds(120)) { RepeatBehavior = RepeatBehavior.Forever });

            for (int i = 0; i < 9; i++)
            {
                double a = -Math.PI / 2 + i * 2 * Math.PI / 9;
                double x = cx + Math.Cos(a) * RadiusX, y = cy + Math.Sin(a) * RadiusY;
                var g = new Grid { Width = 90, Height = 70, RenderTransformOrigin = new Point(.5, .3) };
                discs[i] = new Ellipse { Width = 40, Height = 40, VerticalAlignment = VerticalAlignment.Top, StrokeThickness = 1.5 };
                runes[i] = new Glyph
                {
                    Data = Geometry.Parse(Runes[i]), Width = 40, Height = 40, VerticalAlignment = VerticalAlignment.Top, StrokeThickness = 1.8,
                    StrokeStartLineCap = PenLineCap.Round, StrokeEndLineCap = PenLineCap.Round, StrokeLineJoin = PenLineJoin.Round,
                    RenderTransform = new TranslateTransform(20, 20)
                };
                // o emblema pintado do Selo (Data/UI/Seals), se veio no instalador: apagado e cinza até acender
                color[i] = Assets.Image("seal_" + i + ".png");
                if (color[i] != null)
                {
                    gray[i] = new FormatConvertedBitmap(color[i], PixelFormats.Gray8, null, 0);
                    gray[i].Freeze();
                    emblems[i] = new Image { Width = 40, Height = 40, VerticalAlignment = VerticalAlignment.Top, Clip = new EllipseGeometry(new Point(20, 20), 19.5, 19.5) };
                    RenderOptions.SetBitmapScalingMode(emblems[i], BitmapScalingMode.HighQuality);
                    runes[i].Visibility = Visibility.Collapsed;
                }
                names[i] = new TextBlock
                {
                    Text = Theme.Spaced(Product.Regions[i].ToUpper(Theme.Pt)), FontFamily = Assets.Display, FontSize = 8.5, FontWeight = FontWeights.Bold,
                    HorizontalAlignment = HorizontalAlignment.Center, VerticalAlignment = VerticalAlignment.Top, Margin = new Thickness(0, 45, 0, 0)
                };
                g.Children.Add(discs[i]); g.Children.Add(runes[i]); g.Children.Add(names[i]);
                if (emblems[i] != null) g.Children.Insert(1, emblems[i]);
                g.RenderTransform = new ScaleTransform(1, 1);
                SetLeft(g, x - 45); SetTop(g, y - 20);
                seals[i] = g;
                Children.Add(g);
                Paint(i, false);
            }
        }

        void Paint(int i, bool on)
        {
            lit[i] = on;
            discs[i].Fill = Theme.B(on ? Color.FromRgb(0x0A, 0x1A, 0x16) : Theme.Bg, .88);
            discs[i].Stroke = Theme.B(on ? Tint : Theme.Gold, on ? .95 : .32);
            runes[i].Stroke = Theme.B(on ? Theme.Cream : Theme.Muted, on ? 1 : .45);
            names[i].Foreground = Theme.B(on ? Theme.Cream : Theme.Muted, on ? .9 : .42);
            discs[i].Effect = on ? new DropShadowEffect { Color = Tint, BlurRadius = 22, ShadowDepth = 0, Opacity = .95 } : null;
            runes[i].Effect = on ? new DropShadowEffect { Color = Tint, BlurRadius = 10, ShadowDepth = 0, Opacity = 1 } : null;
            if (emblems[i] != null)
            {
                emblems[i].Source = on ? color[i] : gray[i];
                emblems[i].Opacity = on ? 1 : .5;
            }
        }

        public int Lit { get { return lit.Count(l => l); } }

        // acende os n primeiros (ou apaga do último para o primeiro); devolve o selo que mudou por último
        public int Set(int count, bool animate)
        {
            int changed = -1;
            for (int i = 0; i < 9; i++)
            {
                bool on = i < count;
                if (on == lit[i]) continue;
                Paint(i, on);
                changed = i;
                if (!animate) continue;
                var s = (ScaleTransform)seals[i].RenderTransform;
                var pop = new DoubleAnimation(on ? 1.45 : .8, 1, TimeSpan.FromMilliseconds(on ? 650 : 450)) { EasingFunction = new ElasticEase { Oscillations = 1, Springiness = 5 } };
                s.BeginAnimation(ScaleTransform.ScaleXProperty, pop);
                s.BeginAnimation(ScaleTransform.ScaleYProperty, pop);
                if (on) Shockwave(i);
            }
            return changed;
        }

        // um anel que se espalha a partir do selo que acendeu
        void Shockwave(int i)
        {
            var ring = new Ellipse { Width = 40, Height = 40, Stroke = Theme.B(Tint), StrokeThickness = 2, RenderTransformOrigin = new Point(.5, .5) };
            var scale = new ScaleTransform(1, 1);
            ring.RenderTransform = scale;
            SetLeft(ring, GetLeft(seals[i]) + 25); SetTop(ring, GetTop(seals[i]));
            Children.Add(ring);
            var grow = new DoubleAnimation(1, 3.2, TimeSpan.FromMilliseconds(900)) { EasingFunction = new CubicEase { EasingMode = EasingMode.EaseOut } };
            scale.BeginAnimation(ScaleTransform.ScaleXProperty, grow);
            scale.BeginAnimation(ScaleTransform.ScaleYProperty, grow);
            var fade = new DoubleAnimation(.9, 0, TimeSpan.FromMilliseconds(900));
            fade.Completed += (s, e) => Children.Remove(ring);
            ring.BeginAnimation(OpacityProperty, fade);
        }
    }

    // ---------------------------------------------------------------- controles

    // segurar para confirmar: a barra enche enquanto o botão está pressionado
    sealed class HoldButton : Border
    {
        const double Seconds = 1.1;
        readonly ScaleTransform fill = new ScaleTransform(0, 1);
        readonly SolidColorBrush edge;
        readonly Color tint;
        bool pressed, fired, enabled = true;
        double hold;
        public event Action Completed;

        public HoldButton(string text, Color tint)
        {
            this.tint = tint;
            Width = 340; Height = 56; CornerRadius = new CornerRadius(28); HorizontalAlignment = HorizontalAlignment.Left;
            BorderThickness = new Thickness(1.5); BorderBrush = edge = new SolidColorBrush(Theme.A(tint, .8));
            Background = Theme.B(Theme.Bg, .75); Cursor = Cursors.Hand;
            var grid = new Grid();
            var bar = new Rectangle
            {
                Fill = new LinearGradientBrush(Theme.A(Theme.Purple, .55), Theme.A(tint, .75), 0), RenderTransform = fill
            };
            var label = Theme.Text(text, Assets.Display, 14, FontWeights.Bold, Theme.B(Theme.Cream));
            label.HorizontalAlignment = HorizontalAlignment.Center; label.VerticalAlignment = VerticalAlignment.Center;
            label.Effect = new DropShadowEffect { Color = Colors.Black, BlurRadius = 6, ShadowDepth = 0, Opacity = .8 };
            grid.Children.Add(bar); grid.Children.Add(label);
            grid.SizeChanged += (s, e) => grid.Clip = new RectangleGeometry(new Rect(e.NewSize), 27, 27);
            Child = grid;
            MouseLeftButtonDown += (s, e) => { if (!enabled || fired) return; pressed = true; CaptureMouse(); e.Handled = true; };
            MouseLeftButtonUp += (s, e) => { pressed = false; ReleaseMouseCapture(); };
            LostMouseCapture += (s, e) => pressed = false;
        }

        public bool Enabled
        {
            get { return enabled; }
            set { enabled = value; Opacity = value ? 1 : .4; Cursor = value ? Cursors.Hand : Cursors.Arrow; }
        }

        public void Step(double dt, double time)
        {
            edge.Color = Theme.A(tint, (IsMouseOver ? .95 : .55) + .2 * Math.Sin(time * 2.4));
            if (fired) return;
            hold = pressed ? Math.Min(1, hold + dt / Seconds) : Math.Max(0, hold - dt * 2.5);
            fill.ScaleX = 1 - Math.Pow(1 - hold, 2.2);
            if (hold >= 1) Fire();
        }

        public void Fire()
        {
            if (!enabled || fired) return;
            fired = true; pressed = false; hold = 1; fill.ScaleX = 1;
            ReleaseMouseCapture();
            if (Completed != null) Completed();
        }
    }

    sealed class Btn : Border
    {
        public event Action Click;
        public Btn(string text, bool primary, Color tint)
        {
            Height = 46; Padding = new Thickness(26, 0, 26, 0); CornerRadius = new CornerRadius(23); Cursor = Cursors.Hand;
            Margin = new Thickness(0, 0, 12, 0); BorderThickness = new Thickness(1.5);
            Background = primary ? (Brush)new LinearGradientBrush(Theme.A(tint, .95), Theme.A(Theme.Gold, .95), 0) : Theme.B(Colors.White, .03);
            BorderBrush = primary ? Theme.B(tint) : Theme.B(Theme.Gold, .45);
            var label = Theme.Text(text, Assets.Display, 13, FontWeights.Bold, primary ? Theme.B(Theme.Bg) : Theme.B(Theme.Cream));
            label.VerticalAlignment = VerticalAlignment.Center;
            Child = label;
            if (primary) Effect = new DropShadowEffect { Color = tint, BlurRadius = 24, ShadowDepth = 0, Opacity = .55 };
            MouseEnter += (s, e) => Opacity = .86;
            MouseLeave += (s, e) => Opacity = 1;
            MouseLeftButtonDown += (s, e) => { CaptureMouse(); e.Handled = true; };
            MouseLeftButtonUp += (s, e) =>
            {
                bool inside = IsMouseOver;
                ReleaseMouseCapture();
                if (inside && Click != null) Click();
            };
        }
    }

    sealed class Toggle : StackPanel
    {
        public bool On;
        readonly Border box;
        readonly Glyph check;

        public Toggle(string text, bool on)
        {
            On = on; Orientation = Orientation.Horizontal; Cursor = Cursors.Hand; Background = Brushes.Transparent; Margin = new Thickness(0, 0, 22, 0);
            check = new Glyph { Data = Geometry.Parse("M3.5,8 L6.5,11 L12,4.5"), Stroke = Theme.B(Theme.Bg), StrokeThickness = 2.2, StrokeLineJoin = PenLineJoin.Round, StrokeStartLineCap = PenLineCap.Round, StrokeEndLineCap = PenLineCap.Round };
            box = new Border { Width = 18, Height = 18, CornerRadius = new CornerRadius(5), BorderThickness = new Thickness(1.5), Margin = new Thickness(0, 0, 9, 0), Child = check, VerticalAlignment = VerticalAlignment.Center };
            var label = Theme.Text(text, Assets.Ui, 12.5, FontWeights.Normal, Theme.B(Theme.Cream, .85));
            label.VerticalAlignment = VerticalAlignment.Center;
            Children.Add(box); Children.Add(label);
            MouseLeftButtonUp += (s, e) => { On = !On; Refresh(); };
            Refresh();
        }

        void Refresh()
        {
            box.Background = On ? Theme.B(Theme.Emerald) : Theme.B(Colors.White, .04);
            box.BorderBrush = On ? Theme.B(Theme.Emerald) : Theme.B(Theme.Gold, .45);
            check.Visibility = On ? Visibility.Visible : Visibility.Hidden;
        }
    }

    // ---------------------------------------------------------------- a janela

    sealed class PortalWindow : Window
    {
        enum Phase { Welcome, Working, Done, Failed }

        readonly Options opt;
        readonly Payload pack;
        readonly bool uninstall;
        readonly Grid layers = new Grid();
        readonly Grid pageHost = new Grid { Width = 450, HorizontalAlignment = HorizontalAlignment.Left, Margin = new Thickness(56, 76, 0, 30) };
        readonly VortexView vortex = new VortexView();
        readonly SealRing seals = new SealRing();
        readonly Image emblem = new Image { Width = 78, Height = 78 };
        readonly TextBlock percent;
        readonly Rectangle flash = new Rectangle { Opacity = 0, IsHitTestVisible = false };
        readonly Grid overlay = new Grid { Visibility = Visibility.Collapsed };
        readonly Glyph musicIcon = new Glyph();
        readonly Stopwatch clock = Stopwatch.StartNew();
        Music music;
        Phase phase;
        Work work;
        string dir;
        double lastFrame, shown, lastText, workStarted;
        HoldButton hold;
        Toggle desktop, startMenu, wipeSaves;
        TextBlock pathText, spaceText, statText, statusText, caption;
        Border bar;
        Image[] cards;
        int card = -1, front;
        System.Windows.Threading.DispatcherTimer loreTimer;

        public PortalWindow(Options opt, Payload pack)
        {
            this.opt = opt; this.pack = pack; uninstall = opt.Uninstall;
            dir = opt.Dir ?? Setup.FindInstalled() ?? Setup.DefaultDir();
            Title = uninstall ? "Desinstalar " + Product.Name : "Instalar " + Product.Name;
            // 02/11: sem AllowsTransparency. Com ela o WPF desenha a janela inteira no processador a cada quadro (janela em camadas)
            // e o instalador travava com o Vórtice animado; os cantos ficam retos, com o fundo da própria moldura
            Width = 1040; Height = 620; WindowStyle = WindowStyle.None; AllowsTransparency = false; ResizeMode = ResizeMode.CanMinimize;
            Background = Theme.B(Theme.Bg); WindowStartupLocation = WindowStartupLocation.CenterScreen;
            UseLayoutRounding = true;
            TextOptions.SetTextFormattingMode(this, TextFormattingMode.Ideal);
            try { Icon = BitmapFrame.Create(Assets.Res("icon.ico")); } catch { }
            TaskbarItemInfo = new TaskbarItemInfo();

            percent = Theme.Text("", Assets.Display, 26, FontWeights.Black, Theme.B(Theme.Cream));
            percent.TextWrapping = TextWrapping.NoWrap;
            percent.Effect = new DropShadowEffect { Color = Theme.Emerald, BlurRadius = 18, ShadowDepth = 0, Opacity = .8 };

            var frame = new Border
            {
                CornerRadius = new CornerRadius(16), Background = Theme.B(Theme.Bg), BorderBrush = Theme.B(Theme.Gold, .28), BorderThickness = new Thickness(1),
                Child = layers
            };
            layers.SizeChanged += (s, e) => layers.Clip = new RectangleGeometry(new Rect(e.NewSize), 15, 15);
            // sem transparência a janela não esmaece: quem aparece aos poucos é a moldura
            frame.Opacity = 0;
            Content = frame;

            // a arte de capa no fundo, apagada para o Vórtice e o texto ficarem por cima
            var cover = Assets.Image("capa.jpg");
            if (cover != null)
            {
                layers.Children.Add(new Image { Source = cover, Stretch = Stretch.UniformToFill, Opacity = .55, IsHitTestVisible = false });
                layers.Children.Add(new Rectangle { Fill = Theme.B(Theme.Bg, .35), IsHitTestVisible = false });
            }
            layers.Children.Add(vortex.Element);
            // escurece o lado do texto e as bordas
            layers.Children.Add(new Rectangle
            {
                IsHitTestVisible = false,
                Fill = new LinearGradientBrush(new GradientStopCollection
                {
                    new GradientStop(Theme.A(Theme.Bg, .96), 0), new GradientStop(Theme.A(Theme.Bg, .82), .36), new GradientStop(Theme.A(Theme.Bg, 0), .6)
                }, 0)
            });
            layers.Children.Add(new Rectangle
            {
                IsHitTestVisible = false,
                Fill = new RadialGradientBrush(new GradientStopCollection { new GradientStop(Theme.A(Theme.Bg, 0), .65), new GradientStop(Theme.A(Theme.Bg, .7), 1) })
            });
            layers.Children.Add(seals);
            var center = new Canvas { IsHitTestVisible = false };
            emblem.Source = Assets.Image("emblem.png");
            emblem.Effect = new DropShadowEffect { Color = Theme.Emerald, BlurRadius = 30, ShadowDepth = 0, Opacity = .7 };
            emblem.RenderTransformOrigin = new Point(.5, .5);
            emblem.RenderTransform = new ScaleTransform(1, 1);
            Canvas.SetLeft(emblem, VortexView.CenterX - 39); Canvas.SetTop(emblem, VortexView.CenterY - 41);
            center.Children.Add(emblem);
            var percentBox = new Grid { Width = 140, Height = 40, Children = { percent } };
            percent.HorizontalAlignment = HorizontalAlignment.Center; percent.VerticalAlignment = VerticalAlignment.Center;
            Canvas.SetLeft(percentBox, VortexView.CenterX - 70); Canvas.SetTop(percentBox, VortexView.CenterY - 21);
            center.Children.Add(percentBox);
            layers.Children.Add(center);
            layers.Children.Add(pageHost);
            layers.Children.Add(TitleBar());
            flash.Fill = new RadialGradientBrush(Theme.A(Theme.Cream, 1), Theme.A(Theme.Emerald, .6)) { Center = new Point(.7, .52), GradientOrigin = new Point(.7, .52), RadiusX = .8, RadiusY = 1 };
            layers.Children.Add(flash);
            layers.Children.Add(overlay);

            if (uninstall) { seals.Set(9, false); vortex.Target = .9; }
            ShowWelcome();

            Loaded += (s, e) =>
            {
                Theme.Animate((UIElement)Content, OpacityProperty, 1, 450);
                if (!uninstall && Assets.HasMusic) music = new Music(Assets.Save("music.mp3"));
                CompositionTarget.Rendering += OnFrame;
            };
            KeyDown += OnKey;
            Closing += OnClosing;
            Closed += (s, e) => { CompositionTarget.Rendering -= OnFrame; if (music != null) music.Stop(); };
        }

        // ---- barra de título

        UIElement TitleBar()
        {
            var bar = new Grid { Height = 46, VerticalAlignment = VerticalAlignment.Top, Background = Brushes.Transparent };
            bar.MouseLeftButtonDown += (s, e) => { if (e.ClickCount == 1) DragMove(); };
            var left = new StackPanel { Orientation = Orientation.Horizontal, Margin = new Thickness(18, 0, 0, 0), VerticalAlignment = VerticalAlignment.Center };
            left.Children.Add(new Image { Source = Assets.Image("emblem.png"), Width = 20, Height = 20, Margin = new Thickness(0, 0, 10, 0) });
            var name = Theme.Text(Theme.Spaced("THE WORLD GÊNESES"), Assets.Display, 11, FontWeights.Bold, Theme.B(Theme.Cream, .85));
            name.VerticalAlignment = VerticalAlignment.Center;
            left.Children.Add(name);
            var kind = Theme.Text(uninstall ? "  ·  desinstalador" : "  ·  instalador" + (pack != null ? " " + pack.Version : ""), Assets.Ui, 11, FontWeights.Normal, Theme.B(Theme.Muted, .7));
            kind.VerticalAlignment = VerticalAlignment.Center;
            left.Children.Add(kind);
            bar.Children.Add(left);

            var right = new StackPanel { Orientation = Orientation.Horizontal, HorizontalAlignment = HorizontalAlignment.Right, Margin = new Thickness(0, 0, 8, 0) };
            if (!uninstall && Assets.HasMusic)
            {
                SetMusicIcon(true);
                right.Children.Add(IconButton(musicIcon, "Música", () => { if (music == null) return; music.On = !music.On; SetMusicIcon(music.On); }));
            }
            right.Children.Add(IconButton(Line("M0,6 L10,6"), "Minimizar", () => WindowState = WindowState.Minimized));
            right.Children.Add(IconButton(Line("M0,0 L10,10 M10,0 L0,10"), "Fechar", Close));
            bar.Children.Add(right);
            return bar;
        }

        static Glyph Line(string data)
        {
            return new Glyph { Data = Geometry.Parse(data), Stroke = Theme.B(Theme.Cream, .8), StrokeThickness = 1.4, StrokeStartLineCap = PenLineCap.Round, StrokeEndLineCap = PenLineCap.Round };
        }

        void SetMusicIcon(bool on)
        {
            musicIcon.Data = Geometry.Parse(on ? "M0,4 L3,4 L7,0.5 L7,11.5 L3,8 L0,8 Z M10,3 Q12.5,6 10,9 M12,1 Q16,6 12,11" : "M0,4 L3,4 L7,0.5 L7,11.5 L3,8 L0,8 Z M10,4 L14,8 M14,4 L10,8");
            musicIcon.Stroke = Theme.B(Theme.Cream, on ? .85 : .5);
            musicIcon.StrokeThickness = 1.3;
            musicIcon.StrokeLineJoin = PenLineJoin.Round;
        }

        static Border IconButton(UIElement icon, string tip, Action click)
        {
            var b = new Border { Width = 40, Height = 32, CornerRadius = new CornerRadius(8), Background = Brushes.Transparent, Cursor = Cursors.Hand, ToolTip = tip, Margin = new Thickness(2, 7, 0, 0) };
            var host = new Grid { HorizontalAlignment = HorizontalAlignment.Center, VerticalAlignment = VerticalAlignment.Center, Children = { icon } };
            b.Child = host;
            b.MouseEnter += (s, e) => b.Background = Theme.B(Colors.White, .08);
            b.MouseLeave += (s, e) => b.Background = Brushes.Transparent;
            b.MouseLeftButtonDown += (s, e) => e.Handled = true;
            b.MouseLeftButtonUp += (s, e) => click();
            return b;
        }

        // ---- páginas

        void ShowPage(UIElement page)
        {
            foreach (UIElement old in pageHost.Children.Cast<UIElement>().ToList())
            {
                var gone = old;
                var fade = new DoubleAnimation(0, TimeSpan.FromMilliseconds(220));
                fade.Completed += (s, e) => pageHost.Children.Remove(gone);
                gone.IsHitTestVisible = false;
                gone.BeginAnimation(OpacityProperty, fade);
            }
            var slide = new TranslateTransform(24, 0);
            page.RenderTransform = slide;
            page.Opacity = 0;
            pageHost.Children.Add(page);
            Theme.Animate(page, OpacityProperty, 1, 520, 160);
            Theme.Animate(slide, TranslateTransform.XProperty, 0, 700, 160);
        }

        static TextBlock Kicker(string text, Color color)
        {
            var t = Theme.Text(Theme.Spaced(text), Assets.Ui, 11, FontWeights.SemiBold, Theme.B(color, .95));
            t.Margin = new Thickness(0, 0, 0, 10);
            return t;
        }

        static TextBlock Heading(string text, double size, FontWeight weight)
        {
            var t = Theme.Text(text, Assets.Display, size, weight, new LinearGradientBrush(Theme.Cream, Theme.Gold, 90));
            t.Effect = new DropShadowEffect { Color = Theme.Gold, BlurRadius = 26, ShadowDepth = 0, Opacity = .28 };
            t.LineHeight = size * 1.02;
            t.LineStackingStrategy = LineStackingStrategy.BlockLineHeight;
            return t;
        }

        static UIElement Rule()
        {
            return new Rectangle { Width = 70, Height = 2, HorizontalAlignment = HorizontalAlignment.Left, Margin = new Thickness(0, 14, 0, 14), Fill = new LinearGradientBrush(Theme.Gold, Theme.A(Theme.Gold, 0), 0) };
        }

        static TextBlock Body(string text, double size = 13.5, double alpha = .82)
        {
            var t = Theme.Text(text, Assets.Serif, size, FontWeights.Normal, Theme.B(Theme.Cream, alpha));
            t.LineHeight = size * 1.5;
            return t;
        }

        void ShowWelcome()
        {
            phase = Phase.Welcome;
            var p = new StackPanel();
            if (uninstall)
            {
                p.Children.Add(Kicker("DESINSTALAR  ·  THE WORLD GÊNESES", Theme.Ember));
                p.Children.Add(Heading("SELAR O", 34, FontWeights.Bold));
                p.Children.Add(Heading("PORTAL", 60, FontWeights.Black));
                p.Children.Add(Rule());
                var quote = Body("Nove Selos… nove destinos… Nove Guardiões em sono profundo.", 15, .95);
                quote.FontStyle = FontStyles.Italic; quote.Foreground = Theme.B(Theme.Gold, .95);
                p.Children.Add(quote);
                var text = Body("Os Selos voltarão a dormir e o jogo será removido desta pasta:");
                text.Margin = new Thickness(0, 10, 0, 12);
                p.Children.Add(text);
                p.Children.Add(PathCard(false));
                wipeSaves = new Toggle("Apagar também meus saves e configurações", false) { Margin = new Thickness(0, 16, 0, 0) };
                p.Children.Add(wipeSaves);
                hold = new HoldButton("SEGURE PARA SELAR O PORTAL", Theme.Ember) { Margin = new Thickness(0, 26, 0, 0) };
            }
            else
            {
                bool update = Setup.FindInstalled() != null;
                p.Children.Add(Kicker((update ? "ATUALIZAR  ·  " : "UMA LENDA EM CONSTRUÇÃO  ·  ") + "ALPHA " + VersionShort(), Theme.Gold));
                p.Children.Add(Heading("THE WORLD", 34, FontWeights.Bold));
                p.Children.Add(Heading("GÊNESES", 62, FontWeights.Black));
                p.Children.Add(Rule());
                var quote = Body("Antes do nome, antes da dor, havia o mar… havia Gênese.", 15, .95);
                quote.FontStyle = FontStyles.Italic; quote.Foreground = Theme.B(Theme.Gold, .95);
                p.Children.Add(quote);
                var text = Body("Abra o portal e traga Valdora para o seu computador. Cada Selo desperta enquanto o mundo atravessa.");
                text.Margin = new Thickness(0, 8, 0, 14);
                p.Children.Add(text);
                p.Children.Add(PathCard(true));
                var toggles = new WrapPanel { Margin = new Thickness(2, 14, 0, 0) };
                desktop = new Toggle("Atalho na área de trabalho", true);
                startMenu = new Toggle("Atalho no menu Iniciar", true);
                toggles.Children.Add(desktop); toggles.Children.Add(startMenu);
                p.Children.Add(toggles);
                hold = new HoldButton(update ? "SEGURE PARA REABRIR O PORTAL" : "SEGURE PARA ABRIR O PORTAL", Theme.Emerald) { Margin = new Thickness(0, 22, 0, 0) };
            }
            hold.Completed += Begin;
            p.Children.Add(hold);
            var hint = Theme.Text("segure o botão  ·  ou aperte Enter", Assets.Ui, 11, FontWeights.Normal, Theme.B(Theme.Muted, .6));
            hint.Margin = new Thickness(26, 8, 0, 0);
            p.Children.Add(hint);
            ShowPage(p);
            RefreshPath();
        }

        string VersionShort()
        {
            string v = pack != null ? pack.Version : "1.0.0";
            int dash = v.IndexOf('-');
            if (dash > 0) v = v.Substring(0, dash);
            return v.EndsWith(".0") && v.Count(c => c == '.') == 2 ? v.Substring(0, v.Length - 2) : v;
        }

        Border PathCard(bool editable)
        {
            var card = new Border
            {
                CornerRadius = new CornerRadius(12), Background = Theme.B(Colors.White, .035), BorderBrush = Theme.B(Theme.Gold, .2),
                BorderThickness = new Thickness(1), Padding = new Thickness(16, 11, 12, 11)
            };
            var g = new Grid();
            g.ColumnDefinitions.Add(new ColumnDefinition());
            g.ColumnDefinitions.Add(new ColumnDefinition { Width = GridLength.Auto });
            var info = new StackPanel();
            info.Children.Add(Theme.Text(Theme.Spaced("DESTINO"), Assets.Ui, 9.5, FontWeights.SemiBold, Theme.B(Theme.Gold, .8)));
            pathText = Theme.Text("", Assets.Ui, 12.5, FontWeights.Normal, Theme.B(Theme.Cream, .95));
            pathText.TextWrapping = TextWrapping.NoWrap; pathText.TextTrimming = TextTrimming.CharacterEllipsis; pathText.Margin = new Thickness(0, 3, 0, 2);
            info.Children.Add(pathText);
            spaceText = Theme.Text("", Assets.Ui, 11, FontWeights.Normal, Theme.B(Theme.Muted, .85));
            info.Children.Add(spaceText);
            g.Children.Add(info);
            if (editable)
            {
                var change = new Border
                {
                    CornerRadius = new CornerRadius(8), Padding = new Thickness(12, 6, 12, 6), BorderThickness = new Thickness(1), BorderBrush = Theme.B(Theme.Gold, .4),
                    Background = Theme.B(Colors.White, .03), Cursor = Cursors.Hand, VerticalAlignment = VerticalAlignment.Center, Margin = new Thickness(10, 0, 0, 0),
                    Child = Theme.Text("Alterar", Assets.Ui, 12, FontWeights.SemiBold, Theme.B(Theme.Gold))
                };
                change.MouseEnter += (s, e) => change.Background = Theme.B(Theme.Gold, .12);
                change.MouseLeave += (s, e) => change.Background = Theme.B(Colors.White, .03);
                change.MouseLeftButtonUp += (s, e) => PickFolder();
                Grid.SetColumn(change, 1);
                g.Children.Add(change);
            }
            card.Child = g;
            return card;
        }

        void RefreshPath()
        {
            if (pathText == null) return;
            pathText.Text = dir;
            pathText.ToolTip = dir;
            if (uninstall)
            {
                spaceText.Text = Directory.Exists(dir) ? "Os arquivos do jogo e os atalhos serão removidos." : "Pasta não encontrada: só os atalhos e o registro serão limpos.";
                return;
            }
            long need = pack != null ? pack.TotalBytes : 1L << 30, free = -1;
            try { free = new DriveInfo(Path.GetPathRoot(dir)).AvailableFreeSpace; } catch { }
            bool ok = free < 0 || free > need + (200L << 20);
            spaceText.Text = "Ocupa " + Theme.Bytes(need) + (free >= 0 ? "  ·  livre no disco: " + Theme.Bytes(free) : "") + (ok ? "" : "  ·  espaço insuficiente");
            spaceText.Foreground = Theme.B(ok ? Theme.Muted : Theme.Danger, .9);
            if (hold != null) hold.Enabled = ok;
        }

        void PickFolder()
        {
            string start = Directory.Exists(dir) ? dir : Path.GetDirectoryName(dir);
            string chosen = FolderPicker.Pick(this, "Onde o portal será aberto?", start);
            if (chosen == null) return;
            dir = string.Equals(Path.GetFileName(chosen.TrimEnd('\\')), Product.Folder, StringComparison.OrdinalIgnoreCase) ? chosen.TrimEnd('\\') : Path.Combine(chosen, Product.Folder);
            RefreshPath();
        }

        // ---- trabalho

        void Begin()
        {
            if (!opt.Demo && Setup.GameRunning())
            {
                ShowFailed("O jogo está aberto", "Feche The World Gêneses antes de continuar.");
                return;
            }
            phase = Phase.Working;
            work = new Work();
            shown = 0;
            workStarted = clock.Elapsed.TotalSeconds;
            vortex.Burst(uninstall ? .2 : .7);
            if (!uninstall) Theme.Animate(emblem, OpacityProperty, 0, 600);
            TaskbarItemInfo.ProgressState = TaskbarItemProgressState.Normal;
            ShowWorking();

            bool wantDesktop = desktop != null && desktop.On, wantStart = startMenu != null && startMenu.On, wipe = wipeSaves != null && wipeSaves.On;
            string target = dir;
            Task.Run(() =>
            {
                if (opt.Demo) Demo(work, uninstall ? 5 : 26);
                else if (uninstall) Setup.Uninstall(target, wipe, work);
                else Setup.Install(pack, target, wantDesktop, wantStart, work);
            });
        }

        static void Demo(Work w, double seconds)
        {
            w.Total = 1060L << 20;
            var t = Stopwatch.StartNew();
            string[] files = { @"Vortex\Content\Paks\Vortex-Windows.ucas", @"Vortex\Binaries\Win64\Vortex-Win64-Shipping.exe", @"Engine\Binaries\ThirdParty\DbgHelp\dbghelp.dll", @"Vortex\Data\Story\regioes.json" };
            while (t.Elapsed.TotalSeconds < seconds)
            {
                double p = t.Elapsed.TotalSeconds / seconds;
                w.Progress = p;
                Interlocked.Exchange(ref w.Done, (long)(w.Total * p));
                w.File = files[(int)(p * 37) % files.Length];
                w.Status = "Trazendo Valdora para o seu computador";
                if (w.Cts.IsCancellationRequested) { w.Cancelled = true; return; }
                Thread.Sleep(50);
            }
            w.Progress = 1;
            w.Finished = true;
        }

        void ShowWorking()
        {
            var p = new StackPanel();
            p.Children.Add(Kicker(uninstall ? "SELANDO O PORTAL" : "O PORTAL ESTÁ SE ABRINDO", uninstall ? Theme.Ember : Theme.Emerald));
            p.Children.Add(Heading(uninstall ? "Os Selos adormecem" : "A Lenda de Gênese", 26, FontWeights.Bold));

            var art = new Grid { Width = 380, Height = 260, ClipToBounds = true };
            cards = new[] { new Image { Stretch = Stretch.UniformToFill }, new Image { Stretch = Stretch.UniformToFill, Opacity = 0 } };
            foreach (var c in cards)
            {
                c.RenderTransformOrigin = new Point(.5, .5);
                c.RenderTransform = new ScaleTransform(1, 1);
                RenderOptions.SetBitmapScalingMode(c, BitmapScalingMode.HighQuality);
                art.Children.Add(c);
            }
            art.Children.Add(new Rectangle { Fill = new LinearGradientBrush(new GradientStopCollection { new GradientStop(Theme.A(Theme.Bg, 0), .55), new GradientStop(Theme.A(Theme.Bg, .55), 1) }, 90) });
            var frame = new Border
            {
                Width = 384, Height = 264, HorizontalAlignment = HorizontalAlignment.Left, Margin = new Thickness(0, 14, 0, 0), CornerRadius = new CornerRadius(12),
                BorderThickness = new Thickness(2), BorderBrush = new LinearGradientBrush(Theme.A(Theme.Gold, .85), Theme.A(Theme.Gold, .25), 45), Child = art,
                Effect = new DropShadowEffect { Color = Colors.Black, BlurRadius = 30, ShadowDepth = 6, Opacity = .6 }
            };
            art.SizeChanged += (s, e) => art.Clip = new RectangleGeometry(new Rect(e.NewSize), 10, 10);
            p.Children.Add(frame);

            caption = Body("", 14, .95);
            caption.FontStyle = FontStyles.Italic;
            caption.Height = 72; caption.Margin = new Thickness(2, 12, 0, 0);
            caption.Foreground = Theme.B(Theme.Gold, .95);
            p.Children.Add(caption);

            var track = new Border { Height = 4, Width = 384, HorizontalAlignment = HorizontalAlignment.Left, CornerRadius = new CornerRadius(2), Background = Theme.B(Colors.White, .08), Margin = new Thickness(0, 6, 0, 0) };
            bar = new Border
            {
                Height = 4, Width = 0, HorizontalAlignment = HorizontalAlignment.Left, CornerRadius = new CornerRadius(2),
                Background = new LinearGradientBrush(Theme.Purple, uninstall ? Theme.Ember : Theme.Emerald, 0),
                Effect = new DropShadowEffect { Color = uninstall ? Theme.Ember : Theme.Emerald, BlurRadius = 12, ShadowDepth = 0, Opacity = .9 }
            };
            track.Child = bar;
            p.Children.Add(track);
            statText = Theme.Text("", Assets.Ui, 12, FontWeights.Normal, Theme.B(Theme.Cream, .8));
            statText.Margin = new Thickness(0, 9, 0, 0);
            p.Children.Add(statText);
            statusText = Theme.Text("", Assets.Ui, 11.5, FontWeights.Normal, Theme.B(Theme.Muted, .75));
            statusText.TextWrapping = TextWrapping.NoWrap; statusText.TextTrimming = TextTrimming.CharacterEllipsis; statusText.Width = 384; statusText.HorizontalAlignment = HorizontalAlignment.Left;
            statusText.Margin = new Thickness(0, 3, 0, 0);
            p.Children.Add(statusText);
            ShowPage(p);

            // a lenda passa quadro a quadro; na desinstalação fica só o quadro dos Selos
            NextCard(uninstall ? 7 : 0);
            if (!uninstall)
            {
                loreTimer = new System.Windows.Threading.DispatcherTimer { Interval = TimeSpan.FromSeconds(7.5) };
                loreTimer.Tick += (s, e) => NextCard((card + 1) % Math.Max(1, Assets.Lore.Length));
                loreTimer.Start();
            }
        }

        void NextCard(int index)
        {
            if (cards == null) return;
            var img = Assets.Image(string.Format("panel_{0:00}.jpg", index));
            if (img == null) return;
            card = index;
            var incoming = cards[1 - front];
            var outgoing = cards[front];
            front = 1 - front;
            incoming.Source = img;
            var scale = (ScaleTransform)incoming.RenderTransform;
            var zoom = new DoubleAnimation(1.0, 1.09, TimeSpan.FromSeconds(9.5));
            scale.BeginAnimation(ScaleTransform.ScaleXProperty, zoom);
            scale.BeginAnimation(ScaleTransform.ScaleYProperty, zoom);
            Panel.SetZIndex(incoming, 1); Panel.SetZIndex(outgoing, 0);
            incoming.BeginAnimation(OpacityProperty, new DoubleAnimation(0, 1, TimeSpan.FromMilliseconds(1100)));
            string text = index < Assets.Lore.Length ? Assets.Lore[index] : "";
            var hide = new DoubleAnimation(0, TimeSpan.FromMilliseconds(300));
            hide.Completed += (s, e) => { caption.Text = text; caption.BeginAnimation(OpacityProperty, new DoubleAnimation(1, TimeSpan.FromMilliseconds(700))); };
            caption.BeginAnimation(OpacityProperty, hide);
        }

        void ShowDone()
        {
            phase = Phase.Done;
            if (loreTimer != null) loreTimer.Stop();
            TaskbarItemInfo.ProgressState = TaskbarItemProgressState.None;
            Flash();
            var p = new StackPanel { Margin = new Thickness(0, 40, 0, 0) };
            if (uninstall)
            {
                vortex.Target = 0; vortex.PresenceTarget = 0;
                p.Children.Add(Kicker("O PORTAL FOI SELADO", Theme.Ember));
                p.Children.Add(Heading("Até a próxima", 44, FontWeights.Black));
                p.Children.Add(Heading("jornada.", 44, FontWeights.Black));
                p.Children.Add(Rule());
                p.Children.Add(Body("The World Gêneses foi removido. Os Guardiões voltaram a sonhar, e Valdora vai esperar pela sua volta."));
                var buttons = new StackPanel { Orientation = Orientation.Horizontal, Margin = new Thickness(0, 30, 0, 0) };
                var close = new Btn("FECHAR", true, Theme.Ember);
                close.Click += Close;
                buttons.Children.Add(close);
                p.Children.Add(buttons);
            }
            else
            {
                vortex.Burst(.9); vortex.Target = .55;
                Theme.Animate(emblem, OpacityProperty, 1, 900, 300);
                var pop = new DoubleAnimation(1.6, 1, TimeSpan.FromMilliseconds(1100)) { EasingFunction = new ElasticEase { Oscillations = 2, Springiness = 6 }, FillBehavior = FillBehavior.Stop };
                ((ScaleTransform)emblem.RenderTransform).BeginAnimation(ScaleTransform.ScaleXProperty, pop);
                ((ScaleTransform)emblem.RenderTransform).BeginAnimation(ScaleTransform.ScaleYProperty, pop);
                percent.BeginAnimation(OpacityProperty, new DoubleAnimation(0, TimeSpan.FromMilliseconds(400)));
                p.Children.Add(Kicker("O PORTAL ESTÁ ABERTO", Theme.Emerald));
                p.Children.Add(Heading("Acorde.", 76, FontWeights.Black));
                p.Children.Add(Rule());
                p.Children.Add(Body("Sem nome, sem memória. Só uma arma, a seiva a conceder, e um destino que começa a nascer.", 15, .95));
                var where = Theme.Text("Instalado em " + dir, Assets.Ui, 11.5, FontWeights.Normal, Theme.B(Theme.Muted, .75));
                where.Margin = new Thickness(0, 14, 0, 0);
                where.TextWrapping = TextWrapping.NoWrap; where.TextTrimming = TextTrimming.CharacterEllipsis; where.MaxWidth = 410;
                where.HorizontalAlignment = HorizontalAlignment.Left; where.ToolTip = dir;
                p.Children.Add(where);
                var buttons = new StackPanel { Orientation = Orientation.Horizontal, Margin = new Thickness(0, 32, 0, 0) };
                var play = new Btn("DESPERTAR EM VALDORA", true, Theme.Emerald);
                play.Click += Launch;
                var close = new Btn("FECHAR", false, Theme.Gold);
                close.Click += Close;
                buttons.Children.Add(play); buttons.Children.Add(close);
                p.Children.Add(buttons);
            }
            ShowPage(p);
        }

        void ShowFailed(string title, string message)
        {
            phase = Phase.Failed;
            if (loreTimer != null) loreTimer.Stop();
            TaskbarItemInfo.ProgressState = TaskbarItemProgressState.Error;
            vortex.Target = .15;
            Theme.Animate(emblem, OpacityProperty, 1, 600);
            percent.Text = "";
            var p = new StackPanel { Margin = new Thickness(0, 40, 0, 0) };
            p.Children.Add(Kicker("O PORTAL SE FECHOU", Theme.Danger));
            p.Children.Add(Heading(title, 40, FontWeights.Black));
            p.Children.Add(Rule());
            p.Children.Add(Body(message));
            var buttons = new StackPanel { Orientation = Orientation.Horizontal, Margin = new Thickness(0, 30, 0, 0) };
            var retry = new Btn("TENTAR DE NOVO", true, Theme.Gold);
            retry.Click += () => { seals.Set(uninstall ? 9 : 0, false); vortex.Target = uninstall ? .9 : .32; ShowWelcome(); };
            var close = new Btn("FECHAR", false, Theme.Gold);
            close.Click += Close;
            buttons.Children.Add(retry); buttons.Children.Add(close);
            p.Children.Add(buttons);
            ShowPage(p);
        }

        void Flash()
        {
            var burst = new DoubleAnimationUsingKeyFrames();
            burst.KeyFrames.Add(new LinearDoubleKeyFrame(.75, KeyTime.FromTimeSpan(TimeSpan.FromMilliseconds(120))));
            burst.KeyFrames.Add(new EasingDoubleKeyFrame(0, KeyTime.FromTimeSpan(TimeSpan.FromMilliseconds(1300)), new CubicEase { EasingMode = EasingMode.EaseOut }));
            flash.BeginAnimation(OpacityProperty, burst);
        }

        // o instalador roda como administrador; o jogo abre pelo Explorer, como o usuário comum
        void Launch()
        {
            string exe = Path.Combine(dir, Product.Exe);
            try { if (!opt.Demo) Process.Start("explorer.exe", "\"" + exe + "\""); } catch { }
            Close();
        }

        // ---- quadro a quadro

        double lastVortex;
        void OnFrame(object sender, EventArgs e)
        {
            double t = clock.Elapsed.TotalSeconds, dt = Math.Min(.05, t - lastFrame);
            if (dt < .004) return;
            lastFrame = t;
            if (hold != null) hold.Step(dt, t);
            if (phase == Phase.Welcome || phase == Phase.Done)
            {
                double breathe = 1 + .04 * Math.Sin(t * 1.6);
                var s = (ScaleTransform)emblem.RenderTransform;
                if (!s.HasAnimatedProperties) { s.ScaleX = breathe; s.ScaleY = breathe; }
            }
            if (phase == Phase.Working) Poll(dt, t);
            // o Vórtice a 30 quadros por segundo (02/11): a imagem toda é refeita no processador; a 60+ ele disputava com a
            // descompressão e a janela engasgava
            if (t - lastVortex >= 1.0 / 30)
            {
                vortex.Step(Math.Min(.1, t - lastVortex), t);
                lastVortex = t;
            }
            if (music != null) music.Step(dt, phase == Phase.Done ? .8 : 1);
        }

        void Poll(double dt, double t)
        {
            if (work.Cancelled) { phase = Phase.Failed; Close(); return; }
            if (work.Error != null) { ShowFailed("Algo deu errado", work.Error); return; }

            shown += (work.Progress - shown) * Math.Min(1, dt * 5);
            if (work.Finished && work.Progress - shown < .004) shown = 1;
            if (uninstall) { vortex.Target = .9 - .8 * shown; vortex.PresenceTarget = 1 - .6 * shown; }
            else vortex.Target = .5 + .75 * shown;

            int count = uninstall ? 9 - (int)Math.Floor(shown * 9 + 1e-4) : (int)Math.Floor(shown * 9 + 1e-4);
            if (count != seals.Lit)
            {
                int changed = seals.Set(count, true);
                if (changed >= 0)
                {
                    statusText.Text = uninstall ? "O Selo de " + Product.Regions[changed] + " adormeceu" : "O Selo de " + Product.Regions[changed] + " despertou";
                    statusText.Foreground = Theme.B(uninstall ? Theme.Ember : Theme.Emerald, .95);
                    lastText = t + 1.6;
                    if (!uninstall) vortex.Burst(.18);
                }
            }

            bar.Width = 384 * shown;
            percent.Text = uninstall ? "" : Math.Floor(shown * 100).ToString(Theme.Pt) + "%";
            TaskbarItemInfo.ProgressValue = shown;
            if (t > lastText)
            {
                lastText = t + .12;
                long done = Interlocked.Read(ref work.Done), total = work.Total;
                string stats = Math.Floor(shown * 100) + "%";
                if (total > 0) stats += "  ·  " + Theme.Bytes(done) + " de " + Theme.Bytes(total);
                double elapsed = t - workStarted;
                if (!uninstall && elapsed > 3 && done > 0 && done < total)
                {
                    double left = (total - done) / (done / elapsed);
                    stats += "  ·  " + (left < 50 ? "menos de 1 min" : "cerca de " + Math.Ceiling(left / 60) + " min");
                }
                statText.Text = stats;
                statusText.Text = string.IsNullOrEmpty(work.File) ? work.Status : work.Status + "  ·  " + work.File;
                statusText.Foreground = Theme.B(Theme.Muted, .75);
            }
            if (shown >= 1 && work.Finished) ShowDone();
        }

        void OnKey(object sender, KeyEventArgs e)
        {
            if (overlay.Visibility == Visibility.Visible)
            {
                if (e.Key == Key.Escape) HideOverlay();
                return;
            }
            if (e.Key == Key.Enter && phase == Phase.Welcome && hold != null) hold.Fire();
            else if (e.Key == Key.Escape) Close();
        }

        void OnClosing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            if (phase != Phase.Working) return;
            e.Cancel = true;
            if (uninstall) return;
            Confirm("Interromper a instalação?", "Os arquivos copiados até agora serão removidos. Você pode abrir o portal de novo quando quiser.", "INTERROMPER", () =>
            {
                work.Cts.Cancel();
                statusText.Text = "Desfazendo…";
            });
        }

        // ---- confirmação dentro da janela

        void Confirm(string title, string text, string yes, Action onYes)
        {
            overlay.Children.Clear();
            overlay.Children.Add(new Rectangle { Fill = Theme.B(Colors.Black, .62) });
            var card = new Border
            {
                Width = 440, HorizontalAlignment = HorizontalAlignment.Center, VerticalAlignment = VerticalAlignment.Center, CornerRadius = new CornerRadius(16),
                Background = Theme.B(Color.FromRgb(0x0E, 0x14, 0x1B)), BorderBrush = Theme.B(Theme.Gold, .4), BorderThickness = new Thickness(1), Padding = new Thickness(30, 26, 30, 26),
                Effect = new DropShadowEffect { Color = Colors.Black, BlurRadius = 40, ShadowDepth = 0, Opacity = .8 }
            };
            var p = new StackPanel();
            p.Children.Add(Heading(title, 22, FontWeights.Bold));
            var body = Body(text);
            body.Margin = new Thickness(0, 12, 0, 22);
            p.Children.Add(body);
            var buttons = new StackPanel { Orientation = Orientation.Horizontal };
            var y = new Btn(yes, true, Theme.Danger);
            y.Click += () => { HideOverlay(); onYes(); };
            var n = new Btn("CONTINUAR", false, Theme.Gold);
            n.Click += HideOverlay;
            buttons.Children.Add(n); buttons.Children.Add(y);
            p.Children.Add(buttons);
            card.Child = p;
            overlay.Children.Add(card);
            overlay.Opacity = 0;
            overlay.Visibility = Visibility.Visible;
            Theme.Animate(overlay, OpacityProperty, 1, 200);
        }

        void HideOverlay() { overlay.Visibility = Visibility.Collapsed; }
    }

    // ---------------------------------------------------------------- escolher pasta (o seletor moderno do Windows)

    static class FolderPicker
    {
        [ComImport, Guid("DC1C5A9C-E88A-4DDE-A5A1-60F82A20AEF7")]
        class FileOpenDialog { }

        [ComImport, Guid("42F85136-DB7E-439C-85F1-E4075D135FC8"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        interface IFileDialog
        {
            [PreserveSig] int Show(IntPtr parent);
            void SetFileTypes(uint count, IntPtr specs);
            void SetFileTypeIndex(uint index);
            void GetFileTypeIndex(out uint index);
            void Advise(IntPtr sink, out uint cookie);
            void Unadvise(uint cookie);
            void SetOptions(uint options);
            void GetOptions(out uint options);
            void SetDefaultFolder(IShellItem item);
            void SetFolder(IShellItem item);
            void GetFolder(out IShellItem item);
            void GetCurrentSelection(out IShellItem item);
            void SetFileName([MarshalAs(UnmanagedType.LPWStr)] string name);
            void GetFileName([MarshalAs(UnmanagedType.LPWStr)] out string name);
            void SetTitle([MarshalAs(UnmanagedType.LPWStr)] string title);
            void SetOkButtonLabel([MarshalAs(UnmanagedType.LPWStr)] string text);
            void SetFileNameLabel([MarshalAs(UnmanagedType.LPWStr)] string label);
            void GetResult(out IShellItem item);
        }

        [ComImport, Guid("43826D1E-E718-42EE-BC55-A1E261C37BFE"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        interface IShellItem
        {
            void BindToHandler(IntPtr pbc, ref Guid bhid, ref Guid riid, out IntPtr ppv);
            void GetParent(out IShellItem parent);
            void GetDisplayName(uint sigdn, [MarshalAs(UnmanagedType.LPWStr)] out string name);
            void GetAttributes(uint mask, out uint attributes);
            void Compare(IShellItem other, uint hint, out int order);
        }

        [DllImport("shell32.dll", CharSet = CharSet.Unicode, PreserveSig = false)]
        static extern void SHCreateItemFromParsingName(string path, IntPtr pbc, [In] ref Guid riid, out IShellItem item);

        public static string Pick(Window owner, string title, string start)
        {
            var dialog = (IFileDialog)new FileOpenDialog();
            try
            {
                dialog.SetOptions(0x20 | 0x40 | 0x800); // pastas, só sistema de arquivos, o caminho tem que existir
                dialog.SetTitle(title);
                dialog.SetOkButtonLabel("Abrir o portal aqui");
                try
                {
                    var iid = typeof(IShellItem).GUID;
                    IShellItem folder;
                    if (Directory.Exists(start)) { SHCreateItemFromParsingName(start, IntPtr.Zero, ref iid, out folder); dialog.SetFolder(folder); }
                }
                catch { }
                if (dialog.Show(new System.Windows.Interop.WindowInteropHelper(owner).Handle) != 0) return null;
                IShellItem result;
                dialog.GetResult(out result);
                string path;
                result.GetDisplayName(0x80058000, out path);
                return path;
            }
            finally { Marshal.ReleaseComObject(dialog); }
        }
    }
}
