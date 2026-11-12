#pragma once

#include "GameFramework/Actor.h"
#include "VortexPortal.generated.h"

class UAudioComponent;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class UStaticMeshComponent;

/**
 * Portal de dungeon no estilo Solo Leveling: um rasgo oval em pé, com o redemoinho de pixels girando para dentro
 * (M_PortalSwirl, duas camadas em sentidos contrários), a borda de blocos acesos que tremem, faíscas em blocos
 * sendo puxadas em espiral, o círculo no chão, pedras flutuando, a luz pulsando e o zumbido em laço.
 * A cor sai do ranque: quanto mais escuro, mais difícil (E azul-claro, D azul, C índigo, B roxo, A vinho, S vermelho).
 * Entrar (E perto): leva até Destination, se houver; sem destino, avisa que a dungeon ainda está sendo feita
 * (DungeonId fica guardado para as dungeons do usuário).
 */
UCLASS()
class VORTEXWORLD_API AVortexPortal : public AActor
{
    GENERATED_BODY()
public:
    AVortexPortal();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /** E, D, C, B, A ou S. */
    UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_Rank, BlueprintReadOnly, Category = "Vortex|Portal")
    FString Rank = TEXT("E");
    /** A dungeon que este portal abre (para as dungeons que ainda vão ser feitas). */
    UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly, Category = "Vortex|Portal")
    FName DungeonId;
    /** Para onde leva (zero: ainda sem dungeon). */
    UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly, Category = "Vortex|Portal")
    FVector Destination = FVector::ZeroVector;

    /** Portal vermelho (S): quem entra não sai até vencer a dungeon; nos outros dá para voltar. As dungeons
     *  usam isto para decidir se o portal de saída existe antes do chefe. */
    bool IsNoEscape() const { return Rank == TEXT("S"); }
    static FLinearColor RankColor(const FString& Rank);
    /** Quanto o portal brilha (1 no E, cai até 0,45 no S: mais escuro, mais difícil). */
    static float RankGlow(const FString& Rank);
    /** Tamanho pelo ranque: o E é o normal (3,2 x 5,2 m); o vermelho (S) tem mais que o dobro. */
    static float RankSize(const FString& Rank);
    /** O portal mais perto de At (até Range cm), para a tecla E e o aviso na tela. */
    static AVortexPortal* FindNear(const UWorld* World, const FVector& At, float Range);
    /** Servidor: abre um portal (com o som de rasgar e o crescimento). */
    static AVortexPortal* Open(UWorld* World, const FVector& Ground, float Yaw, const FString& Rank, FName DungeonId = NAME_None);

private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Swirl;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> SwirlBack;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Floor;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> Rim;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> Sparks;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> Rocks;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> Light;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UAudioComponent> Hum;
    UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> Looks;
    float Age = 0.f, LightScale = 1.f, InstanceClock = 0.f;
    UFUNCTION() void OnRep_Rank();
    void ApplyRank();
    void PlaceInstances(bool bFirst);
};
