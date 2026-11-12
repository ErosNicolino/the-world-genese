#pragma once

#include "GameFramework/Actor.h"
#include "Framework/VortexLook.h"
#include "VortexMenuStage.generated.h"

class AVortexCharacter;
class AVortexTerrainStreamer;
class UAudioComponent;
class UCameraComponent;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;

UENUM()
enum class EVortexStageMode : uint8 { Home, Heroes, Races, Creator, Lobby };

/**
 * O "mundinho" atrás do menu (L_Menu): um canto de verdade do vale, perto do lago, com o terreno,
 * a grama, as árvores, o céu, as nuvens, as borboletas e os sons do jogo, e a mesma luz e
 * exposição calibradas do mapa do jogo (sol, céu, névoa e exposição criados aqui, e o dia parado
 * num fim de manhã). O herói de verdade fica em pé no chão, sobre um círculo de runas na cor da
 * raça. O menu Slate fica transparente por cima.
 *
 * Câmera livre (estilo Cube World): arrastar gira em volta do herói, a roda aproxima, o botão
 * direito sobe e desce; "Rosto" aproxima no rosto. Cada tela do menu tem um enquadramento.
 * Também toca a música tema e os sons da interface.
 */
UCLASS()
class VORTEX_API AVortexMenuStage : public AActor
{
    GENERATED_BODY()
public:
    AVortexMenuStage();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    void SetMode(EVortexStageMode NewMode);
    void ShowLook(const FVortexLook& Look);
    void SetHeroVisible(bool bVisible);
    /** Câmera em volta do herói (graus). */
    void Orbit(float YawDegrees, float PitchDegrees);
    /** Sobe/desce o ponto que a câmera olha. */
    void Pan(float Centimeters);
    void Zoom(float Steps) { ZoomLevel = FMath::Clamp(ZoomLevel + Steps * .12f, -.5f, 1.f); }
    void AddYaw(float Degrees) { Orbit(Degrees, 0.f); }
    void ResetView();
    void SetFaceFocus(bool bFace) { bFaceFocus = bFace; }
    bool IsFaceFocus() const { return bFaceFocus; }
    /** Herói criado: giro, golpe da espada e o círculo explode em luz. */
    void Celebrate();
    /** Reveal da raça: um pulso de luz no círculo. */
    void Pulse();
    void PlayUi(FName Sound, float Volume = 1.f);
    AVortexCharacter* GetHero() const { return Hero; }
    /**
     * Sala de espera: os heróis de todos (até 8) lado a lado em arco, o primeiro no centro. Quem
     * acabou de ficar pronto dá uma volta e um golpe.
     */
    void ShowLobby(const TArray<FVortexLook>& Looks, const TArray<bool>& Ready);
    /** O mundo do menu já está rodando e o chão em volta do herói já foi montado. */
    bool IsReady() const { return bPlaced && Clock - PlacedAt > .6f; }

private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Ring;
    /** Luz suave que acompanha a câmera nas telas do herói (o lado de sombra do sol ficava escuro demais). */
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UPointLightComponent> Fill;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> RingMaterial;
    UPROPERTY(Transient) TObjectPtr<AVortexCharacter> Hero;
    UPROPERTY(Transient) TObjectPtr<AVortexTerrainStreamer> Streamer;
    UPROPERTY(Transient) TObjectPtr<UAudioComponent> Music;
    UPROPERTY(Transient) TMap<FName, TObjectPtr<class USoundBase>> Sounds;

    void SpawnWorld();
    void PlaceHero();
    AVortexCharacter* SpawnStageHero(const FVector& Where);
    FVector LobbySpot(int32 Index) const;

    struct FLobbyMember
    {
        TWeakObjectPtr<AVortexCharacter> Character;
        float SpinAt = -10.f;
        bool bReady = false, bSwung = true;
        FString LookKey;   // visual mostrado (troca só quando muda)
    };
    TArray<FLobbyMember> Members;
    UPROPERTY(Transient) TArray<TObjectPtr<AVortexCharacter>> LobbyParty;

    EVortexStageMode Mode = EVortexStageMode::Home;
    FLinearColor RingColor = FLinearColor(1.f, .75f, .3f), RingTarget = RingColor;
    // câmera: giro, inclinação e distância desejados e mostrados
    float BaseYaw = 0.f, CamYaw = 0.f, CamPitch = -8.f, YawShown = 0.f, PitchShown = -8.f;
    float PanZ = 0.f, PanShown = 0.f, ZoomLevel = 0.f, ZoomShown = 0.f, FaceShown = 0.f, ShiftShown = 0.f, DistShown = 420.f;
    float CelebrateTime = -10.f, PulseTime = -10.f, Clock = 0.f, PlacedAt = 0.f, HeroScale = 1.f;
    bool bFaceFocus = false, bHeroVisible = true, bSwung = false, bPlaced = false;
    FVector Feet = FVector::ZeroVector;
    // visual pedido antes do herói existir (o chão ainda carregando)
    FVortexLook PendingLook;
    bool bHasPendingLook = false;
};
