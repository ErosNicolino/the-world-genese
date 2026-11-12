#pragma once

#include "Components/ActorComponent.h"
#include "VortexWetness.generated.h"

/**
 * O herói molhado (03/11), só desenho, em cada máquina. Entrando na água, guarda até que altura do corpo ela chegou
 * (andando no raso só as pernas; nadando, tudo). Saindo, seca aos poucos (~45 s).
 * Passa para as malhas do herói (corpo, roupas, cabelo) pelos dados primitivos 30 (a linha da água no mundo, cm) e
 * 31 (o quanto está molhado). Os materiais escurecem e brilham abaixo da linha (Tools/AddWetLook.py).
 */
UCLASS(ClassGroup=(Vortex))
class VORTEX_API UVortexWetness : public UActorComponent
{
    GENERATED_BODY()
public:
    UVortexWetness();
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    float WetHeight = 0.f;      // cm acima dos pés
    float Wetness = 0.f;        // 0 seco, 1 encharcado
    float SentLine = -1e9f, SentWetness = -1.f;
};
