#pragma once

#include "Async/Future.h"
#include "CanvasItem.h"
#include "Framework/VortexHudBase.h"
#include "Framework/VortexPings.h"
#include "VortexAimHUD.generated.h"

class UMaterialInstanceDynamic;
class USceneCaptureComponent2D;
class UTexture2D;
class UTextureRenderTarget2D;
class AVortexCharacter;
class AVortexWolf;
struct FSlateFontInfo;

/**
 * HUD do jogo no estilo cartoon das janelas (Tools/MakeHudCartoon.py e ImportHudCartoon.py; o desenho fica em
 * VortexAimHUDCartoon.cpp). Tudo é desenhado no Canvas e escala com a altura da tela (base 1080p):
 * - canto superior esquerdo: retrato do herói (captura ao vivo do rosto), nome, nível, vida (que
 *   escorre e pisca quando baixa), mana e XP;
 * - canto superior direito: minimapa do vale feito do próprio terreno (relevo, água, árvores),
 *   norte para cima, com o herói, a cidade, NPCs (com ! e ? das missões), o grupo e só as criaturas
 *   que você viu; dia, área e relógio na moldura. Embaixo dele, a missão acompanhada.
 *   M abre o mapa do vale inteiro; o alcance do minimapa fica nas opções (Interface);
 * - abaixo do retrato: os efeitos ativos (contra-ataque, Fonte, cidade) e o grupo (vida e nível);
 * - no mundo: nome, título, marca de missão e falas dos NPCs; "[E] Falar"; nome e seta dos
 *   companheiros (na borda da tela quando estão fora dela);
 * - centro em cima: a criatura em combate mais perto (retrato, nível, vida e o que ela está fazendo);
 *   chefes ficam com a barra até a luta acabar;
 * - centro embaixo: golpe (botão esquerdo), especial da arma (direito, com recarga), habilidades
 *   1 a 4, poções Q (cura) e R (mana) e as cargas de esquiva, acendendo quando a tecla é usada;
 * - números de dano sobre quem levou o golpe; mira no centro.
 * Vortex.HUD 0/1 esconde/mostra (fica gravado).
 */
/** O desenho da névoa do mapa (nuvens pintadas): o mesmo na textura do continente e no detalhe por bloco. */
namespace VortexMapPaint
{
    /** F = quanto o ponto já foi visto (0..1); X, Y e Cell em pixels do mapa do continente (as nuvens batem nos dois). */
    VORTEXUI_API FColor CloudOver(const FColor& Base, float F, float X, float Y, float Cell);
}

UCLASS()
class VORTEXUI_API AVortexAimHUD : public AVortexHudBase
{
    GENERATED_BODY()
public:
    AVortexAimHUD();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void DrawHUD() override;

    // medições/testes
    virtual bool HasMinimap() const override { return MapTexture != nullptr; }
    virtual const AVortexWolf* GetTargetWolf() const override;
    /** M: o mapa do mundo em tela cheia (abre centrado no herói; o mouse aparece e o herói para). */
    virtual void ToggleWorldMap() override;
    virtual bool IsWorldMapOpen() const override { return bWorldMap; }
    /** Roda do mouse com o mapa aberto: aproxima (no ponto embaixo do mouse) ou afasta. */
    virtual void ZoomWorldMap(float Steps) override;
    /** O marcador do jogador (clique direito no mapa): aparece no mapa, no minimapa e na bússola. */
    virtual bool HasMapMarker() const override { return bHasMarker; }
    virtual FVector2D GetMapMarker() const override { return Marker; }
    virtual void SetMapMarker(const FVector2D& At) override { Marker = At; bHasMarker = true; }
    /** Testes: o mapa local do minimapa (1 ponto por bloco) já foi montado. */
    virtual bool HasLocalMap() const override { return LocalMap != nullptr && bLocalReady; }
    virtual bool HasDetailMap() const override { return DetailMap != nullptr && bDetailReady; }
    virtual float GetMapZoom() const override { return MapZoom; }
    /** Centraliza o mapa grande num ponto (o botão CENTRALIZAR usa o herói). */
    virtual void CenterWorldMap(const FVector2D& At) override { MapCenter = At; }
    /** Parte do mapa já descoberta (0 a 1). */
    virtual float ExploredFraction() const override { return SeenTotal > 0 ? float(SeenCount) / SeenTotal : 0.f; }
    /** Mapa de viajante: copia para a carta uma rota sinuosa de Hundreds x 100 m (perto da cidade, ou
     *  longe a partir de 900 m, com um trecho grande no fim). Devolve quantos pedaços eram novos. */
    virtual int32 RevealTravelerRoute(int32 Hundreds, int32 Seed, const FVector& Hero) override;
    /** Testes: o pedaço daquele ponto já saiu da névoa. */
    virtual bool IsDiscovered(const FVector& At) const override;
    /** Testes: abre a tela de carregamento como na saída do Estádio. */
    virtual void DebugBeginLoading() override { StartLoading(true); }
    /** O Aplicar das opções: a tela de carregamento cobre a troca até o chão em volta remontar. */
    virtual void BeginApplyLoading() override { StartLoading(true); bApplyingOptions = true; }
    virtual bool IsLoadingScreen() const override { return LoadingFade > 0.f; }
    virtual void ExportPortraits(const FString& Prefix) const override;

private:
    UPROPERTY(Transient) TMap<FName, TObjectPtr<UTexture2D>> Textures;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> HeroPortrait;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> TargetPortrait;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> Minimap;
    UPROPERTY(Transient) TObjectPtr<UTextureRenderTarget2D> HeroTarget;
    UPROPERTY(Transient) TObjectPtr<UTextureRenderTarget2D> WolfTarget;
    UPROPERTY(Transient) TObjectPtr<USceneCaptureComponent2D> Capture;
    UPROPERTY(Transient) TObjectPtr<UTexture2D> MapTexture;
    UPROPERTY(Transient) TObjectPtr<class UFont> TextFont;
    // ---- arte cartoon (/Game/Vortex/UI/HudCartoon)
    UPROPERTY(Transient) TMap<FName, TObjectPtr<UTexture2D>> ToonTextures;
    UTexture2D* ToonTex(FName Name);
    /** Painel de 9 partes: os cantos ficam (Corner px na tela), o meio estica. */
    void Slice(FName Name, float X, float Y, float W, float H, float Corner, const FLinearColor& Tint = FLinearColor::White);
    /** Peça centrada em (CX, CY), de lado Size, girada Degrees (horário). */
    void Sprite(FName Name, float CX, float CY, float Size, float Degrees = 0.f, const FLinearColor& Tint = FLinearColor::White);
    /** Barra cartoon: fundo escuro e o enchimento tingido (Fraction de 0 a 1). */
    void ToonBar(float X, float Y, float W, float H, float Fraction, const FLinearColor& Color);
    /** Texto na letra de desenho animado com contorno escuro, centrado na altura da caixa (Align 0/1/2). */
    void Toon(const FString& Text, float X, float Y, float W, float H, float Size, const FLinearColor& Color, int32 Align = 0);
    FVector2D ToonSize(const FString& Text, float Size) const;
    /** Os textos cartoon esperam e são desenhados todos juntos no fim (FlushToon): intercalados com os painéis, cada
     *  troca de textura virava um lote a mais na placa de vídeo (a HUD nova custava ~2 ms de render por isso). */
    TArray<FCanvasTextItem> PendingToon;
    void FlushToon();
    // ---- mapa local (1 ponto por bloco de 0,75 m): o do minimapa em volta do herói e o de detalhe do mapa grande
    UPROPERTY(Transient) TObjectPtr<UTexture2D> LocalMap;
    UPROPERTY(Transient) TObjectPtr<UTexture2D> DetailMap;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> MinimapToon;
    static constexpr int32 LocalCells = 512;
    static constexpr int32 DetailCells = 2048;   // 10/11: 1,5 km de detalhe (antes 768 m: a borda da vista ficava borrada)
    FIntPoint LocalOrigin = FIntPoint::ZeroValue, DetailOrigin = FIntPoint::ZeroValue, LocalJobOrigin = FIntPoint::ZeroValue, DetailJobOrigin = FIntPoint::ZeroValue;
    TFuture<TArray<FColor>> LocalJob, DetailJob;
    bool bLocalReady = false, bDetailReady = false;
    float DetailSeenCount = -1.f;
    void UpdateLocalMaps(AVortexCharacter* Hero);
    void UploadWhole(UTexture2D*& Texture, int32 Size, const TArray<FColor>& Pixels);
    // ---- mapa grande: centro (cm do mundo), arrastar, marcador e filtros da legenda
    FVector2D MapCenter = FVector2D::ZeroVector, Marker = FVector2D::ZeroVector, DragMouse = FVector2D::ZeroVector, DragCenter = FVector2D::ZeroVector;
    bool bHasMarker = false, bDragging = false;
    FBox2D MapView = FBox2D(ForceInit);
    uint8 MapShow = 0xFF;

    struct FDamageNumber { FVector Location; float Damage; float Born; bool bOnHero; bool bCrit = false; float Drift = 0.f; };
    TArray<FDamageNumber> DamageNumbers;
    void DrawStadium(class AVortexCharacter* Hero);
    /** Os comandos do jogo (03/11): sempre no Estádio do Despertar (onde o herói novo escolhe a arma); F1 mostra e
     *  esconde em qualquer lugar. */
    void DrawControls(class AVortexCharacter* Hero, float Alpha);
    /** 03/11: na mira, o anel da carga do arco; o inimigo travado (cajado, Caps Lock); quem a cura do cajado vai curar. */
    void DrawWeaponAim(class AVortexCharacter* Hero);
    bool bShowControls = false;
    float FpsShown = 0.f;

    /** A criatura em combate mais perto (lobo, gota, golem, chefe...). */
    TWeakObjectPtr<AActor> TargetCreature;
    /** Quando cada criatura foi vista por último (só as vistas aparecem no mapa). */
    TMap<TWeakObjectPtr<AActor>, float> SeenAt;
    float SpotClock = 0.f;
    bool bWorldMap = false;
    TWeakObjectPtr<AVortexCharacter> BoundHero;
    FBox2D MapBounds = FBox2D(ForceInit);
    bool bMapRequested = false;
    FIntPoint MapSize = FIntPoint::ZeroValue;
    TFuture<TArray<FColor>> MapJob;
    // ---- mapa descoberto aos poucos: névoa por pedaço de 18 m (raio de 70 m em volta do herói),
    // guardada no save do herói; a textura do mapa é refeita com a névoa (bordas suaves) ao descobrir
    TSharedPtr<const TArray<FColor>> MapBase;
    /** Células por pixel do mapa (4 no continente) e pedaços de terra do mapa (para o "explorado"). */
    int32 MapStep = 1;
    TSharedPtr<int32, ESPMode::ThreadSafe> MapLand;
    TArray<uint8> Seen;
    FIntPoint SeenOrigin = FIntPoint::ZeroValue, SeenSize = FIntPoint::ZeroValue;
    int32 SeenCount = 0, SeenTotal = 0, CellsPerChunk = 24;
    bool bSeenReady = false, bFogDirty = false, bSeenUnsaved = false;
    float DiscoverClock = 0.f, FogClock = 0.f, SeenSaveClock = 0.f;
    /** Só o retângulo que mudou é refeito e enviado (em pedaços do mapa; vazio = nada a fazer). */
    FIntRect FogDirty = FIntRect(0, 0, 0, 0);
    struct FFogPatch { FIntRect Rect; TArray<FColor> Pixels; };
    TFuture<FFogPatch> FogJob;
    struct FMapPoi { FVector2D At; FString Name; };
    TArray<FMapPoi> MapPois;
    float MapZoom = 1.f;
    void UpdateDiscovery(AVortexCharacter* Hero, float DeltaSeconds);
    void UploadMap(const TArray<FColor>& Pixels);
    void UploadMapRect(const FIntRect& Rect, const TArray<FColor>& Pixels);
    void MarkFog(const FIntPoint& Cell);
    void SaveDiscovery();
    float Scale = 1.f;
    float HeroCaptureTime = 0.f, WolfCaptureTime = 0.f;
    /** O retrato do herói é a foto parada (Saved/Portraits); já está na HUD. */
    bool bHeroPortraitFixed = false;
    /** Retrato do alvo: fotografado uma vez ao trocar de criatura, nunca atualizado em tempo real. */
    bool bTargetPortraitReady = false;
    float HealthShown = 1.f, TargetHealthShown = 1.f, TargetSeenTime = -100.f, TargetAlpha = 0.f;
    float LastHeroHealth = -1.f;

    UFUNCTION()
    void OnHeroHit(AActor* InstigatorActor, AActor* TargetActor, float Damage);

    UTexture2D* Tex(FName Name);
    void Image(FName Name, float X, float Y, float W, float H, float Fill = 1.f, const FLinearColor& Tint = FLinearColor::White);
    void Label(const FString& Text, float X, float Y, float W, float H, float Size, const FLinearColor& Color,
        int32 Align = 0, bool bBold = true);
    FVector2D Measure(const FString& Text, const FSlateFontInfo& Font) const;
    void Disc(float CX, float CY, float Radius, const FLinearColor& Fill, const FLinearColor& Ring);

    void DrawPlayer(AVortexCharacter* Hero, float DeltaSeconds);
    void DrawMinimap(AVortexCharacter* Hero);
    void DrawTarget(AVortexCharacter* Hero, float DeltaSeconds);
    void DrawHotbar(AVortexCharacter* Hero);
    void DrawDamage();
    /** Texto de desenho animado (Luckiest Guy, contorno grosso): os números de dano. */
    void CartoonText(const FString& Text, float CX, float CY, float Size, const FLinearColor& Fill, const FLinearColor& Outline, float Alpha);
    /** O nome do lugar onde o herói entrou (região, cidade, vila): letras que caem quicando no alto da tela. */
    void DrawPlaceBanner(AVortexCharacter* Hero, float DeltaSeconds);
    /** Estádio: o pedestal do cartão (o nome pula quando muda) e quando mudou. */
    FName StadiumCardType;
    float StadiumCardAt = -100.f;
    FString PlaceShown, PlacePending, BannerTitle, BannerSub;
    float PlaceCheckAt = 0.f, PlacePendingSince = 0.f, BannerAt = -100.f;
    FLinearColor BannerTint = FLinearColor::White;
    TMap<FString, float> PlaceBannerTimes;
    /** Troca de foco do golpe de perto com uma virada rápida do mouse (sem marca na tela). */
    void DrawFocus(AVortexCharacter* Hero, float DeltaSeconds);
    // virada rápida do mouse (troca o foco): giro somado nos últimos ~0,1 s e o último ângulo da câmera
    float FlickYaw = 0.f, FlickLastYaw = 0.f, FlickReadyAt = 0.f;
    void DrawCrosshair();
    /** Bússola no alto da tela: pontos cardeais, objetivo, cidade, Caldeira, grupo e marcações. */
    void DrawCompass(AVortexCharacter* Hero);
    /** Marcações do Z no mundo (e na borda da tela quando estão fora dela). */
    void DrawPings(AVortexCharacter* Hero);
    /** Segurando B: a roda de emotes e danças no meio da tela. */
    void DrawEmoteWheel(AVortexCharacter* Hero);
    /** Brilho dos coletáveis perto (14 m) e o aviso do E: colher, dar fruta ao cavalo, montar, desmontar. */
    void DrawInteractables(AVortexCharacter* Hero);
    // o aviso do E e os brilhos dos coletáveis: as buscas (moradores, jogadores, baús, cavalos, coletáveis) varriam
    // os atores a cada quadro; agora a cada 0,12 s / 0,25 s (o desenho continua a cada quadro)
    double InteractCheckAt = -1.0, SparkleCheckAt = -1.0;
    FString InteractPrompt;
    FLinearColor InteractColor = FLinearColor::White;
    bool bInteractTalk = false;
    struct FSparkle { FVector At = FVector::ZeroVector; float Seed = 0.f, Rise = 35.f; };
    TArray<FSparkle> Sparkles;
    void DrawNotices(AVortexCharacter* Hero);
    void DrawQuestTracker(AVortexCharacter* Hero);
    void DrawNpcTags(AVortexCharacter* Hero);
    /** Tela de morte: faixas de cinema, "VOCÊ CAIU", quem venceu e a contagem para renascer. */
    void DrawDeath(AVortexCharacter* Hero);
    /** Tela de carregamento (ao sair do Estádio e ao entrar): cobre tudo até o chão em volta estar montado. */
    bool DrawLoading(AVortexCharacter* Hero, float DeltaSeconds);
    /** 08/11: abre a tela já cobrindo tudo (bSnap: sem o clarear de 0,17 s, em que o mundo pela metade aparecia). */
    void StartLoading(bool bSnap);
    UPROPERTY(Transient) TObjectPtr<UTexture2D> LoadingArt;
    FVector LastHeroAt = FVector::ZeroVector;
    bool bEnteredWorld = false;
    bool bWasAwakening = false, bLoading = false, bApplyingOptions = false;
    float LoadingAge = 0.f, LoadingFade = 0.f;
    int32 LoadingPeak = 1, LoadingTip = 0;
    FString LastKiller;
    /** Placa sobre a cabeça das criaturas perto: nome e nível na cor da dificuldade, vida com o rastro do dano. */
    void DrawCreaturePlates(AVortexCharacter* Hero, float DeltaSeconds);
    struct FPlateState
    {
        float Shown = 1.f, Chip = 1.f, Alpha = 0.f, Seen = -100.f;
        // animação: o "pulo" ao aparecer, o golpe (tremida e brilho), a posição suavizada na tela
        float Pop = 0.f, LastFraction = 1.f, HitAt = -100.f;
        FVector2D Pos = FVector2D::ZeroVector;
    };
    TMap<TWeakObjectPtr<AActor>, FPlateState> Plates;
    void DrawParty(AVortexCharacter* Hero);
    void DrawBuffs(AVortexCharacter* Hero);
    void DrawWorldMap(AVortexCharacter* Hero);
    void UpdateSpotted(AVortexCharacter* Hero, float DeltaSeconds);
    bool WasSeen(const AActor* Actor) const;
    /** Pinta uma marca do mapa (criatura, NPC, grupo, missão, cidade) no ponto da tela. */
    void MapMark(const FVector2D& At, float Radius, const FLinearColor& Color, int32 Shape = 0);

    void UpdatePortraits(AVortexCharacter* Hero);
    void CaptureFace(AActor* Actor, UTextureRenderTarget2D* Target, float Distance, float FieldOfView);
    void RequestMinimap();
    AActor* FindCombatCreature(const AVortexCharacter* Hero) const;
    float LastDrawTime = -1.f;
};
