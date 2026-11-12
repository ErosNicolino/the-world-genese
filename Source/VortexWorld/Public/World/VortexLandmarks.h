#pragma once

#include "GameFramework/Actor.h"
#include "VortexLandmarks.generated.h"

class UStaticMeshComponent;

/**
 * Coluna de fumaça feita de baforadas (esferas translúcidas, M_SmokePuff) que sobem, crescem, derivam
 * com o vento e somem; vista de longe (o vulcão de Brasafenda) ou de perto (as poças de lava).
 */
UCLASS()
class VORTEXWORLD_API AVortexSmokeColumn : public AActor
{
    GENERATED_BODY()
public:
    AVortexSmokeColumn();
    virtual void Tick(float DeltaSeconds) override;
    /** Largura e altura da coluna (cm), quantas baforadas, cor, opacidade e a duração de cada uma. */
    void Setup(float InWidth, float InHeight, int32 Count, const FLinearColor& Tint, float Opacity, float Life);
private:
    struct FPuff { TObjectPtr<UStaticMeshComponent> Ball; TObjectPtr<class UMaterialInstanceDynamic> Look; float Age = 0.f; FVector Drift = FVector::ZeroVector; };
    TArray<FPuff> Puffs;
    float Width = 1000.f, Height = 5000.f, Opacity = .5f, PuffLife = 10.f;
};

/**
 * Marcos do continente (ValleyWorld.Yggdrasil e ValleyWorld.RegionalVillages do Godot):
 * - a Yggdrasil, a Árvore do Mundo (~150 m) no coração de Verdalume, sempre montada (marco do
 *   horizonte), com a aldeia élfica em volta (casas em anel, arco de entrada, Poço da Lua, lanternas);
 * - o Acampamento das Dunas (Solcânion) e a Vila dos Caçadores (Verdalume), com praça de pedra e fogueira;
 * - os covis dos chefes regionais (Seryth nas dunas de vidro, Thorun no bosque antigo): círculo de
 *   obeliscos com runas.
 * As vilas e os covis montam quando alguém chega a 400 m. As peças usam as caixas de colisão do Godot.
 */
UCLASS()
class VORTEXWORLD_API AVortexLandmarks : public AActor
{
    GENERATED_BODY()
public:
    AVortexLandmarks();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    struct FSite { FString Id, Name; FVector2D Center; float Radius = 0.f; };
    /** Os lugares (cm): Yggdrasil, vilas e covis (o mapa e o terreno também usam). */
    static const TArray<FSite>& Sites();
    bool IsBuilt(int32 Site) const { return Parts.Contains(Site); }

private:
    TMap<int32, TArray<TObjectPtr<USceneComponent>>> Parts;
    /** A miragem da Yggdrasil (cópia translúcida e ondulante, M_YggMirage): só acesa quando a árvore está longe. */
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Mirage;
    void BuildSite(int32 Site);
    void DropSite(int32 Site);
    /** Acampamentos de bandidos (UVortexCreatureSubsystem::BanditCampAt): fogueira acesa, caixotes, carroça e
     *  troncos de assento, montados perto de alguém. Guardados em Parts com chaves a partir de CampKeyBase. */
    static constexpr int32 CampKeyBase = 100000;
    TMap<FIntPoint, int32> Camps;
    int32 NextCampKey = CampKeyBase;
    void BuildCamp(const FIntPoint& Cell, const FVector2D& At);
    void UpdateCamps(const TArray<FVector2D>& Players);
public:
    /** Testes: quantos acampamentos estão montados agora. */
    int32 BuiltCamps() const { return Camps.Num(); }
    /** O baú guardado do acampamento dessa quadra (a marca no save de quem abriu). */
    static FName CampChestId(const FIntPoint& Cell) { return FName(*FString::Printf(TEXT("camp:%d_%d"), Cell.X, Cell.Y)); }
private:
    /** Uma peça do Godot (town.json/elven_props.json) no ponto (m, Godot), assentada no alicerce. */
    void Place(int32 Site, const FString& Model, float X, float Z, float Yaw);
    TSharedPtr<class FJsonObject> Models;
};
