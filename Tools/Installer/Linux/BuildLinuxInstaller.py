"""Instalador do The World Gêneses para Linux: um .run (script + o jogo em .tar.gz anexado).

python Tools/Installer/Linux/BuildLinuxInstaller.py <pasta do jogo Linux> <saída.run> <versão>

O tar é feito aqui (e não pelo tar do Windows) para gravar a permissão de executar no Vortex.sh, nos binários e
nas bibliotecas: sem ela o jogo não abre no Linux. O cabeçalho é Tools/Installer/Linux/portal_header.sh, gravado com
quebra de linha do Linux.
"""
import io
import pathlib
import sys
import tarfile

HERE = pathlib.Path(__file__).resolve().parent
ROOT = HERE.parents[2]


def executable(rel):
    name = rel.name
    return name.endswith((".sh", ".so")) or ".so." in name or ("Binaries" in rel.parts and "." not in name)


def main():
    game, out, version = pathlib.Path(sys.argv[1]), pathlib.Path(sys.argv[2]), sys.argv[3]
    files = [f for f in sorted(game.rglob("*")) if f.is_file() and not (f.parent == game and f.name.startswith("Manifest_"))
             and not f.name.endswith((".debug", ".sym"))]
    total = sum(f.stat().st_size for f in files)
    payload = io.BytesIO()
    with tarfile.open(fileobj=payload, mode="w:gz", compresslevel=6) as tar:
        for f in files:
            rel = f.relative_to(game)
            info = tar.gettarinfo(str(f), arcname=rel.as_posix())
            info.mode = 0o755 if executable(rel) else 0o644
            info.uid = info.gid = 0
            info.uname = info.gname = ""
            with open(f, "rb") as src:
                tar.addfile(info, src)
        icon = ROOT / "Tools" / "Installer" / "icon.png"
        info = tar.gettarinfo(str(icon), arcname="icon.png")
        info.mode = 0o644
        with open(icon, "rb") as src:
            tar.addfile(info, src)
    header = (HERE / "portal_header.sh").read_text(encoding="utf-8").replace("\r\n", "\n")
    header = header.replace("@VERSAO@", version).replace("@TAMANHO@", ("%.1f GB" % (total / 1024 ** 3)).replace(".", ","))
    if not header.endswith("__PACOTE__\n"):
        header = header.rstrip("\n") + "\n"
    out.parent.mkdir(parents=True, exist_ok=True)
    with open(out, "wb") as dst:
        dst.write(header.encode("utf-8"))
        dst.write(payload.getvalue())
    print("Instalador Linux: %s (%d MB, %d arquivos, jogo com %.2f GB)" % (out, out.stat().st_size // 2 ** 20, len(files), total / 1024 ** 3))


if __name__ == "__main__":
    main()
