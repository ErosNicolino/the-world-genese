#pragma once

#include "UObject/Object.h"
#include "VortexPlayerSettings.generated.h"

/**
 * Preferências de controle de cada jogador nesta máquina, gravadas em
 * Saved/Config/<plataforma>/GameUserSettings.ini (seção /Script/Vortex.VortexPlayerSettings).
 * A sensibilidade multiplica a do DefaultInput.ini (eixos MouseX/MouseY).
 * No jogo: [ diminui, ] aumenta. Console: Vortex.Mouse.Sensibilidade, Vortex.Mouse.Mira,
 * Vortex.Mouse.InverterY.
 * HUD: M troca o alcance do minimapa (40, 70 ou 120 m); console Vortex.HUD 0/1 esconde/mostra a HUD.
 */
UCLASS(config = GameUserSettings)
class VORTEXWORLD_API UVortexPlayerSettings : public UObject
{
    GENERATED_BODY()

public:
    static UVortexPlayerSettings* Get() { return GetMutableDefault<UVortexPlayerSettings>(); }
    /** 02/11: a classe mudou do módulo Vortex para o VortexWorld e a seção do GameUserSettings.ini mudou de nome: na
     *  primeira vez, copia as opções da seção antiga ([/Script/Vortex.VortexPlayerSettings]) para não voltar ao padrão. */
    virtual void PostInitProperties() override;

    static constexpr float MinSensitivity = 0.1f;
    static constexpr float MaxSensitivity = 3.f;
    static constexpr float SensitivityStep = 0.1f;

    // 1 = a velocidade antiga; o padrão gira a câmera mais devagar
    UPROPERTY(config, EditAnywhere, Category = "Mouse", meta = (ClampMin = "0.1", ClampMax = "3.0"))
    float MouseSensitivity = 0.6f;

    // multiplicador extra enquanto o botão direito segura a mira
    UPROPERTY(config, EditAnywhere, Category = "Mouse", meta = (ClampMin = "0.1", ClampMax = "2.0"))
    float AimSensitivityScale = 0.7f;

    UPROPERTY(config, EditAnywhere, Category = "Mouse")
    bool bInvertMouseY = false;

    // raio do minimapa em metros (M alterna entre os três)
    UPROPERTY(config, EditAnywhere, Category = "HUD")
    float MinimapRadius = 100.f;   // 10/11: 60, 100 ou 160 m (antes 40, 70, 120: o usuário achou perto demais)

    // o minimapa gira com a câmera (o N anda na borda); desligado, o norte fica sempre para cima
    UPROPERTY(config, EditAnywhere, Category = "HUD")
    bool bMinimapRotates = true;

    UPROPERTY(config, EditAnywhere, Category = "HUD")
    bool bShowHud = true;

    // contador de FPS no canto do HUD
    UPROPERTY(config, EditAnywhere, Category = "HUD")
    bool bShowFps = false;

    // marcação de foco: o inimigo para onde o golpe de perto vai (desligada, a tela fica livre)
    UPROPERTY(config, EditAnywhere, Category = "HUD")
    bool bShowFocusMarker = true;

    // a resolução interna padrão (75%, o TSR reconstrói a imagem) já foi posta na primeira vez
    UPROPERTY(config)
    bool bRenderDefaultsApplied = false;

    // versão dos padrões de imagem já aplicados (2: sombras clássicas, Alpha 1.0)
    UPROPERTY(config)
    int32 RenderDefaultsVersion = 0;

    // tipo de sombra do sol: 0 leves (clássicas, ~15% mais FPS), 1 detalhadas (sombras virtuais da Unreal 5)
    UPROPERTY(config) int32 ShadowMode = 0;

    // neblina baixa nos vales, na água e nas matas (pacote FogArea; liga a névoa volumétrica numa grade mais leve)
    UPROPERTY(config) bool bValleyMist = false;

    // resolução dinâmica: a resolução interna baixa sozinha (até 50%) quando o FPS cai abaixo do alvo
    UPROPERTY(config) bool bDynamicResolution = true;
    UPROPERTY(config) int32 DynamicResolutionTarget = 60;

    // super resolução: 0 TSR (com a resolução dinâmica), 1 DLSS Qualidade, 2 DLSS Equilibrado, 3 DLSS Desempenho, 4 DLAA
    // (DLSS e DLAA só em placas RTX e com o plugin da NVIDIA em Plugins/DLSS)
    UPROPERTY(config) int32 Upscaler = 0;

    // ---- gráficos (Framework/VortexGraphics): o padrão é o visual simples, sem "shaders"; a opção
    // Shaders liga os seis efeitos abaixo de uma vez, e os avançados deixam ligar um por um
    UPROPERTY(config) bool bShaderGI = false;             // luz global do Lumen
    UPROPERTY(config) bool bShaderReflections = false;    // reflexos do Lumen
    UPROPERTY(config) bool bShaderLightShafts = false;    // raios de sol
    UPROPERTY(config) bool bShaderMotionBlur = false;     // desfoque de movimento
    UPROPERTY(config) bool bShaderBloom = false;          // brilho em volta das luzes
    UPROPERTY(config) bool bShaderVolumetricFog = false;  // névoa volumétrica
    UPROPERTY(config) bool bAmbientOcclusion = true;
    /** 0 baixa (opaca), 1 média (transparente parada), 2 alta (marolinhas), 3 épica (refração e ondas). */
    UPROPERTY(config) int32 WaterQuality = 2;
    /** Grama: 0 curta, 1 média, 2 longa (a de antes), 3 máxima. */
    UPROPERTY(config) int32 GrassDistance = 2;
    /** Distância de visualização (árvores, plantas, pedras): 0 curta ... 3 máxima (a de antes). */
    UPROPERTY(config) int32 ViewDistance = 3;
    /** Densidade da vegetação (grama): 0 baixa (35%), 1 média (60%), 2 alta (80%), 3 máxima. */
    UPROPERTY(config) int32 GrassDensity = 3;
    UPROPERTY(config) float FieldOfView = 75.f;
    /** 0 TSR, 1 TAA, 2 FXAA, 3 desligado. */
    UPROPERTY(config) int32 AntiAliasingMethod = 0;
    /** 0 desligada, 1 leve, 2 média, 3 forte. */
    UPROPERTY(config) int32 Sharpen = 0;
    /** Filtro anisotrópico: 0 desligado, 1 = 4x, 2 = 8x, 3 = 16x (texturas nítidas vistas de lado). */
    UPROPERTY(config) int32 Anisotropy = 3;
    /** Bordas do TSR: histórico em 100%, 150% ou 200% (quinas dos blocos mais lisas). */
    UPROPERTY(config) int32 EdgeQuality = 1;
    /** 11/11: o modelo do DLSS. 0 automático (o leve nas RTX 20 e 30), 1 leve (preset E: 0,8 ms numa RTX 2060 SUPER em
     *  1080p), 2 nítido (o padrão do plugin, mais estável em movimento: 3,5 ms na mesma placa). */
    UPROPERTY(config) int32 DlssModel = 0;
    /** Brilho da imagem (compensação da exposição): 0 mais escuro ... 1 o calibrado antigo ... 5 máximo. */
    UPROPERTY(config) int32 Brightness = 3;
    /** Exposição extra (EV) de cada nível de Brightness. */
    static float BrightnessEV(int32 Level)
    {
        static const float Steps[] = { -.5f, 0.f, .4f, .8f, 1.2f, 1.6f };
        return Steps[FMath::Clamp(Level, 0, int32(UE_ARRAY_COUNT(Steps)) - 1)];
    }

    // ---- áudio (VortexAudio::ApplyVolumes): 0 a 1 por categoria; o geral multiplica todos
    UPROPERTY(config) float VolumeMaster = 1.f;
    UPROPERTY(config) float VolumeMusic = 1.f;
    UPROPERTY(config) float VolumeEffects = 1.f;
    UPROPERTY(config) float VolumeAmbience = 1.f;
    UPROPERTY(config) float VolumeInterface = 1.f;

    /** Heróis (nome do arquivo do save) que já viram a HQ de abertura neste computador. IntroSeen era a lista da
     *  versão antiga, que passava sozinha com espaço e cliques: quem estava nela vê a abertura nova de novo. */
    UPROPERTY(config) TArray<FString> IntroSeen;
    UPROPERTY(config) TArray<FString> IntroWatched;
    bool HasWatchedIntro(const FString& Hero) const { return IntroWatched.Contains(Hero); }
    void MarkIntroSeen(const FString& Hero) { IntroWatched.AddUnique(Hero); SaveConfig(); }

    void SetShowFps(bool bShow) { bShowFps = bShow; SaveConfig(); }
    void SetShowFocusMarker(bool bShow) { bShowFocusMarker = bShow; SaveConfig(); }
    void SetMinimapRotates(bool bOn) { bMinimapRotates = bOn; SaveConfig(); }

    void CycleMinimapRadius();
    void SetShowHud(bool bShow);
    void SetMouseSensitivity(float Value);
    void StepMouseSensitivity(int32 Steps) { SetMouseSensitivity(MouseSensitivity + Steps * SensitivityStep); }
    void SetAimSensitivityScale(float Value);
    void SetInvertMouseY(bool bInvert);
};
