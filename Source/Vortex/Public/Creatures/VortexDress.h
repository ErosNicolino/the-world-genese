#pragma once

#include "Components/ActorComponent.h"
#include "Framework/VortexLook.h"
#include "VortexDress.generated.h"

class USkeletalMeshComponent;
class UMaterialInstanceDynamic;
class UTexture2D;

/**
 * Veste um personagem com o guarda-roupa do herói (as mesmas peças, paleta e tingimento do
 * AVortexNpc::ApplyLook), em cima de uma malha base do aventureiro que só anima: cada peça copia a
 * pose dela. Usado pelos bandidos (AVortexMonster humanoides). Só visual: cada máquina monta o seu.
 */
UCLASS(ClassGroup=(Vortex))
class VORTEX_API UVortexDressComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    /** Veste Base com Look (some com a malha base, que continua animando). */
    void Apply(USkeletalMeshComponent* Base, const FVortexLook& Look);

    /** Roupa de bandido do Godot (TownFolk.BanditLook): escura e gasta; o chefe de malha cor de vinho. */
    static FVortexLook BanditLook(int32 Seed, bool bLeader);
    /** Morto-vivo dos cemitérios: a raça morto-vivo do criador de heróis, em trapos escuros. */
    static FVortexLook UndeadLook(int32 Seed);
    /** Esqueleto dos portais: a raça morto-vivo toda em cor de osso (pele, roupa e botas). */
    static FVortexLook SkeletonLook(int32 Seed);

private:
    UPROPERTY(Transient) TArray<TObjectPtr<USkeletalMeshComponent>> Parts;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> LookMaterial;
    UPROPERTY(Transient) TMap<FName, TObjectPtr<UMaterialInstanceDynamic>> Tints;
    UPROPERTY(Transient) TObjectPtr<UTexture2D> Palette;
    void SetPart(USkeletalMeshComponent* Base, FName PartName, FName PieceId);
};
