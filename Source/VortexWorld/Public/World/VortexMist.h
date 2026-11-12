#pragma once

#include "GameFramework/Actor.h"
#include "VortexMist.generated.h"

/**
 * Neblina baixa pelo mundo, com o pacote FogArea (copiado do VortexReferencia): manchas de névoa com
 * ruído (BP_FogArea) nos vales, na beira da água e nas matas, em volta de quem joga.
 * - O mundo é dividido em quadras de 90 m; cada quadra sorteia (sempre igual) se tem neblina, onde e o tamanho.
 * - Só as quadras a até ~210 m do jogador ganham a névoa; as que ficam para trás voltam para a fila.
 * - Só visual (o servidor dedicado não monta).
 */
UCLASS()
class VORTEXWORLD_API AVortexMist : public AActor
{
    GENERATED_BODY()
public:
    AVortexMist();
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

    static constexpr float Cell = 9000.f;     // cm
    static constexpr float Reach = 21000.f;   // cm (a névoa volumétrica vai até 220 m)
    int32 VisibleCount() const { return Patches.Num(); }

private:
    UPROPERTY(Transient) TSubclassOf<AActor> FogClass;
    TMap<FIntPoint, TWeakObjectPtr<AActor>> Patches;
    TSet<FIntPoint> Empty;   // quadras sorteadas sem neblina (não sorteia de novo)
    bool bLoaded = false;
    bool Roll(const FIntPoint& Key, FTransform& Out) const;
};
