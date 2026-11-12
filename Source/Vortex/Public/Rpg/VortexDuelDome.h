#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VortexDuelDome.generated.h"

class UStaticMeshComponent;

/**
 * O domo de energia do duelo (02/11, pedido do usuário: os monstros atacavam quem duelava). Nasce no servidor quando o
 * duelo começa, entre os dois heróis, e some quando ele acaba. A regra fica no UVortexSocial (servidor): os dois não
 * saem, ninguém mais entra (heróis e criaturas são empurrados para fora) e de fora ninguém fere quem está dentro.
 * Aqui só a esfera translúcida (Tools/MakeDuelDome.py), replicada para todos verem.
 */
UCLASS()
class VORTEX_API AVortexDuelDome : public AActor
{
    GENERATED_BODY()
public:
    AVortexDuelDome();
    static constexpr float Radius = 900.f;
    /** O domo de duelo que contém um ponto (para os golpes de fora e para os bichos), ou nenhum. */
    static AVortexDuelDome* Containing(const UWorld* World, const FVector& At, float Margin = 0.f);
    bool Contains(const FVector& At, float Margin = 0.f) const;

private:
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Shell;
};
