#!/bin/sh
# The World Gêneses: instalador para Linux ("o Portal"). O jogo vai anexado depois da linha __PACOTE__.
#   sh TheWorldGeneses_*_Linux.run              instala (com janelas, se o zenity existir; senão, no terminal)
#   sh TheWorldGeneses_*_Linux.run --pasta DIR  instala em DIR
#   sh TheWorldGeneses_*_Linux.run --texto      sem janelas
# Instala para o usuário (sem sudo): ~/.local/share/TheWorldGeneses, atalho no menu e Desinstalar.sh na pasta.
set -e
NOME="The World Gêneses"
VERSAO="@VERSAO@"
DESTINO="${XDG_DATA_HOME:-$HOME/.local/share}/TheWorldGeneses"
APPS="${XDG_DATA_HOME:-$HOME/.local/share}/applications"
TAMANHO="@TAMANHO@"
JANELAS=1
while [ $# -gt 0 ]; do
    case "$1" in
        --pasta) DESTINO="$2"; shift ;;
        --texto) JANELAS=0 ;;
    esac
    shift
done
command -v zenity >/dev/null 2>&1 && [ -n "$DISPLAY$WAYLAND_DISPLAY" ] || JANELAS=0

roxo=$(printf '\033[38;5;135m'); verde=$(printf '\033[38;5;48m'); ouro=$(printf '\033[38;5;179m'); fim=$(printf '\033[0m')
vortice() {
    printf '%s' "$roxo"
    cat <<'ARTE'
                 .  *   .        .   *
          .   ___---~~~~~~---___    .
       *   .-~  .-~~~~~~~~~~-.  ~-.     *
         /    /    .-~~-.    \    \
        |    |    ( (()) )    |    |
         \    \    `-..-'    /    /    .
      .    `-._ `-.______.-' _.-'  *
               ~~--______--~~
ARTE
    printf '%s' "$fim"
    printf '%s        THE WORLD GÊNESES  ·  o Portal  ·  %s%s\n' "$ouro" "$VERSAO" "$fim"
    printf '%s   Antes do nome, antes da dor, havia o mar… havia Gênese.%s\n\n' "$verde" "$fim"
}

aviso() { if [ $JANELAS = 1 ]; then zenity --info --title="$NOME" --width=420 --text="$1" 2>/dev/null || true; else printf '%s\n' "$1"; fi; }
erro() { if [ $JANELAS = 1 ]; then zenity --error --title="$NOME" --width=420 --text="$1" 2>/dev/null || true; else printf 'ERRO: %s\n' "$1" >&2; fi; exit 1; }

[ $JANELAS = 1 ] || vortice
if [ $JANELAS = 1 ]; then
    zenity --question --title="$NOME · o Portal" --width=460 --ok-label="Abrir o portal" --cancel-label="Agora não" \
        --text="<b>Bem-vindo a Valdora.</b>\n\nAntes do nome, antes da dor, havia o mar… havia Gênese.\n\nO jogo será instalado em:\n<tt>$DESTINO</tt>\n\nOcupa $TAMANHO." 2>/dev/null || exit 0
else
    printf 'Instalar em %s (%s)? [S/n] ' "$DESTINO" "$TAMANHO"
    read -r resposta || resposta=s
    case "$resposta" in n|N) exit 0 ;; esac
fi

LINHA=$(awk '/^__PACOTE__$/ { print NR + 1; exit 0; }' "$0")
mkdir -p "$DESTINO" || erro "Sem permissão para criar $DESTINO."
# os nove Selos acendem conforme os arquivos chegam
if [ $JANELAS = 1 ]; then
    ( tail -n +"$LINHA" "$0" | tar -xzf - -C "$DESTINO" & pid=$!; n=0
      for regiao in Valdora Brasafenda Raikor Nivaluz Solcânion Aurivale Lodonegro Morthúria Verdalume; do
          n=$((n + 11)); echo "$n"; echo "# O Selo de $regiao despertou…"
          kill -0 $pid 2>/dev/null && sleep 2
      done
      wait $pid; echo 100; echo "# O portal está aberto." ) |
        zenity --progress --title="$NOME · o portal está se abrindo" --width=460 --auto-close --no-cancel 2>/dev/null
else
    printf '%sAbrindo o portal…%s ' "$verde" "$fim"
    tail -n +"$LINHA" "$0" | tar -xzf - -C "$DESTINO"
    printf '%s✦ os nove Selos despertaram%s\n' "$ouro" "$fim"
fi
[ -x "$DESTINO/Vortex.sh" ] || erro "A instalação não terminou: falta $DESTINO/Vortex.sh."

# atalho no menu de aplicativos e o desinstalador
mkdir -p "$APPS"
cat > "$APPS/TheWorldGeneses.desktop" <<DESKTOP
[Desktop Entry]
Type=Application
Name=$NOME
Comment=Sua jornada começa em Valdora
Exec="$DESTINO/Vortex.sh"
Path=$DESTINO
Icon=$DESTINO/icon.png
Terminal=false
Categories=Game;RolePlaying;
DESKTOP
chmod +x "$APPS/TheWorldGeneses.desktop"
cat > "$DESTINO/Desinstalar.sh" <<DESINSTALAR
#!/bin/sh
# Sela o portal: apaga o jogo e o atalho (os saves ficam em ~/.config/Epic/Vortex).
rm -f "$APPS/TheWorldGeneses.desktop"
rm -rf "$DESTINO"
echo "O portal foi selado. Valdora vai esperar pela sua volta."
DESINSTALAR
chmod +x "$DESTINO/Desinstalar.sh"
command -v update-desktop-database >/dev/null 2>&1 && update-desktop-database "$APPS" >/dev/null 2>&1 || true

if [ $JANELAS = 1 ]; then
    zenity --question --title="$NOME" --width=420 --ok-label="Despertar em Valdora" --cancel-label="Fechar" \
        --text="<b>Acorde.</b>\n\nSem nome, sem memória. Só uma arma, a seiva a conceder, e um destino que começa a nascer.\n\nO jogo está no menu de aplicativos." 2>/dev/null &&
        (cd "$DESTINO" && nohup ./Vortex.sh >/dev/null 2>&1 &)
else
    printf '\n%sAcorde.%s O jogo está no menu de aplicativos, ou rode: %s/Vortex.sh\n' "$ouro" "$fim" "$DESTINO"
fi
exit 0
__PACOTE__
