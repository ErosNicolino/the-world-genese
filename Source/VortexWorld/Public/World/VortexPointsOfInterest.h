#pragma once

#include "GameFramework/Actor.h"
#include "VortexPointsOfInterest.generated.h"

class UProceduralMeshComponent;
class UPointLightComponent;
class UMaterialInterface;

/** Os tipos de ponto de interesse espalhados pelo continente. */
enum class EVortexPoi : uint8 { Ruins, Tower, Shrine, Mine, Graveyard, Caravan, Dock, Count };

/** Um ponto de interesse sorteado numa quadra (o mesmo em todas as máquinas: sai da posição). */
struct FVortexPoi
{
    EVortexPoi Kind = EVortexPoi::Ruins;
    FName Id;
    FIntPoint Cell = FIntPoint::ZeroValue;
    FVector2D At = FVector2D::ZeroVector;   // cm
    float Yaw = 0.f;                         // graus
    int32 Region = 1;
    uint8 Variant = 0;                       // santuário: 0 força, 1 vida, 2 sorte; ruínas: 0 baú, 1 inscrição
    bool bRare = false;
};

/**
 * Pontos de interesse ("tem pouca construção, ruínas e essas coisas espalhadas pelo jogo"): uma quadra de 72 m em
 * cada ~2,5 ganha um, no estilo da região (pedra clara no vale, arenito no deserto, basalto no vulcão, musgo no
 * pântano, osso e pedra roxa em Morthúria...). Montados em blocos com colisão quando alguém chega a 200 m.
 * - Ruínas: piso quebrado, muros e colunas (algumas caídas), entulho; um baú escondido ou uma pedra com inscrição.
 * - Torre de vigia: escada em volta até o topo; lá em cima o mapa revela 300 m em volta.
 * - Santuário: obelisco com runa acesa; E dá uma bênção (força, vida ou sorte) por 5 minutos.
 * - Mina abandonada: morro com a boca escura, escoras, trilho e carrinho de minério; baú guardado por goblins.
 * - Cemitério: muro baixo, lápides, cruzes, árvore seca e mausoléu; à noite os mortos levantam; baú guardado.
 * - Caravana tombada: carroça quebrada, roda caída, caixotes, barris e sacos; um baú pequeno.
 * - Cais de pesca: píer de tábuas entrando na água, barco a remo, barris.
 * Os baús usam os de AVortexWorldFeatures (abre com E, saque do servidor, aberto no save de cada herói).
 */
UCLASS()
class VORTEXWORLD_API AVortexPointsOfInterest : public AActor
{
    GENERATED_BODY()
public:
    AVortexPointsOfInterest();
    virtual void Tick(float DeltaSeconds) override;

    static constexpr float CellSize = 7200.f;
    /** O ponto de interesse da quadra, se houver (sorteio fixo pela posição; chão seco, longe da cidade e dos bandidos). */
    static bool PoiAt(const FIntPoint& Cell, FVortexPoi& Out);
    static FString KindName(EVortexPoi Kind);
    /** Portal de dungeon espalhado (1 a cada ~14 quadras sem outra coisa): azul é comum, vermelho é raro
     *  (E 40%, D 25%, C 16%, B 10%, A 6%, S 3%). O servidor abre quando alguém chega perto. */
    static bool GateAt(const FIntPoint& Cell, FVector2D& OutAt, FString& OutRank, float& OutYaw);
    /** Teia: em pé (entre dois galhos) ou no chão. Width em m. */
    struct FWebSpot { FVector2D At = FVector2D::ZeroVector; float Yaw = 0.f, Width = 3.f; bool bGround = false; };
    /** Teias espalhadas (mais nas matas, no pântano e em Morthúria): 2 a 4 juntas, com aranhas penduradas por perto. */
    static bool WebsAt(const FIntPoint& Cell, TArray<FWebSpot>& Out);
    /** Alguém neste ponto está preso numa teia (anda bem devagar e não pula). */
    bool IsInWeb(const FVector& At) const;
    /** Um golpe saindo de From para Forward: corta a teia que acertar (3 golpes); true se acertou alguma. */
    bool SlashWeb(const FVector& From, const FVector& Forward);
    /** Teias fixas (dungeons dos portais): ficam até RemovePinnedWebs, no chão GroundZ (cm). */
    void AddPinnedWebs(const FIntPoint& Key, const TArray<FWebSpot>& Spots, float GroundZ);
    void RemovePinnedWebs(const FIntPoint& Key);
    /** Ponto (m, no espaço do ponto: X para o lado, Z para trás; a frente é -Z) no mundo (cm). */
    static FVector2D LocalToWorld(const FVortexPoi& Poi, float LX, float LZ);
    static AVortexPointsOfInterest* Find(const UWorld* World);

    /** Santuário montado perto (até Range cm) para a tecla E. */
    const FVortexPoi* NearestShrine(const FVector& At, float Range) const;
    /** Pedra com inscrição perto (ruínas). */
    const FVortexPoi* NearestLore(const FVector& At, float Range) const;
    /** O herói está no topo de uma torre de vigia (o mapa revela longe). */
    bool IsOnTowerTop(const FVector& At) const;
    /** Texto da inscrição (uma frase da história do mundo, sorteada pela pedra). */
    static FString LoreText(const FVortexPoi& Poi);
    static FString BlessingName(uint8 Variant);
    static FName BlessingKind(uint8 Variant);
    int32 BuiltCount() const { return Built.Num(); }

private:
    struct FBuilt
    {
        FVortexPoi Poi;
        TArray<TObjectPtr<USceneComponent>> Parts;
        TWeakObjectPtr<UPointLightComponent> Light;
        TWeakObjectPtr<UProceduralMeshComponent> Crystal;
        float TowerTopZ = 0.f;
    };
    TMap<FIntPoint, FBuilt> Built;
    TMap<FIntPoint, TWeakObjectPtr<class AVortexPortal>> Gates;
    struct FWeb { FWebSpot Spot; float GroundZ = 0.f; TWeakObjectPtr<UProceduralMeshComponent> Mesh; int32 Hits = 0; double CutUntil = -1.0; };
    TMap<FIntPoint, TArray<FWeb>> Webs;
    TSet<FIntPoint> PinnedWebs;
    void BuildWebs(const FIntPoint& Cell, const TArray<FWebSpot>& Spots, TOptional<float> FixedGroundZ = {});
    UPROPERTY(Transient) TArray<TObjectPtr<USceneComponent>> Keep;
    UPROPERTY(Transient) TObjectPtr<UMaterialInterface> Material;
    float Clock = 0.f, Since = 0.f;
    void Build(const FVortexPoi& Poi);
    void Drop(const FIntPoint& Cell);
};
