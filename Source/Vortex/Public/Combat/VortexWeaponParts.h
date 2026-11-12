#pragma once

#include "Components/ActorComponent.h"
#include "VortexWeaponParts.generated.h"

class UStaticMeshComponent;
class UInstancedStaticMeshComponent;

/**
 * Peças da arma com movimento próprio (REVISAO_ARMAS G7), só desenho, em cada máquina:
 * - grimório: a capa abre na lombada e três folhas passam rapidinho a cada golpe; fecha parado;
 * - chicote: tira de couro comprida (corda simulada), balança, e estica e estala no golpe;
 * - corrente com lâmina: elos que balançam e se esticam até o alcance no golpe, a lâmina na ponta;
 * - arco: a corda (o modelo não tem) presa nas duas pontas; segurando o clique ela vai até a outra mão,
 *   ao soltar volta e vibra um instante.
 * Vale para a arma no corpo e para a arma da primeira pessoa (cada uma com a sua corda).
 */
UCLASS(ClassGroup=(Vortex))
class VORTEX_API UVortexWeaponParts : public UActorComponent
{
    GENERATED_BODY()
public:
    UVortexWeaponParts();
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    struct FHolder
    {
        TWeakObjectPtr<UStaticMeshComponent> Weapon;
        TWeakObjectPtr<UStaticMeshComponent> Cover;
        TArray<TWeakObjectPtr<UStaticMeshComponent>> Pages;
        TWeakObjectPtr<UInstancedStaticMeshComponent> Rope;
        TWeakObjectPtr<UStaticMeshComponent> Nocked;   // 03/11: a flecha na corda enquanto puxa
        TWeakObjectPtr<UStaticMeshComponent> Tip;
        TArray<FVector> Points, Old;
        FName Family;
        // arco: as pontas (espaço da arma, achadas nos vértices) e a vibração da corda
        bool bTipsFound = false;
        float ViewDraw = 0.f;   // primeira pessoa: 0 descansando, 1 puxando
        FVector TipA = FVector::ZeroVector, TipB = FVector::ZeroVector;
        FVector Nock = FVector::ZeroVector, NockSpeed = FVector::ZeroVector;
    };
    TArray<FHolder> Holders;
    int32 LastSerial = -1, LastSwing = INDEX_NONE;
    float SwingAge = 10.f, Open = 0.f;

    void Tick(FHolder& H, class AVortexCharacter* Hero, float DeltaTime);
    void Clear(FHolder& H);
    void TickBook(FHolder& H, float DeltaTime);
    void TickRope(FHolder& H, class AVortexCharacter* Hero, float DeltaTime);
    void TickBowString(FHolder& H, class AVortexCharacter* Hero, float DeltaTime);
    /** 0..1: o quanto o golpe agora estica a corda até o alcance. */
    float StrikeAlpha(const class AVortexCharacter* Hero) const;
};
