#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VortexPowerFx.generated.h"

class ACharacter;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UProceduralMeshComponent;

/**
 * O efeito do poder de cada espada e de cada arco (07/11, o usuário: "criar um para cada do zero, não reutilizar"):
 * montado aqui com peças feitas em código (o corte no ar, o anel de blocos, o círculo de runas, os pilares, os espinhos
 * que saem do chão, os estilhaços, os orbes, a chuva, a espiral, o feixe, as penas, o raio e o rastro na lâmina), na
 * composição e nas cores de cada arma (Data/Rpg/power_fx.json, Tools/Weapons/BuildPowerFx.py). Só visual, em cada
 * máquina (VortexSwords::OnSpecialPose); some sozinho.
 */
UCLASS()
class VORTEX_API AVortexPowerFx : public AActor
{
    GENERATED_BODY()

public:
    AVortexPowerFx();
    /** O efeito do item (o id da espada ou do arco) no herói; nada se o item não tem efeito. */
    static AVortexPowerFx* Spawn(ACharacter* Hero, FName Item);
    /** 07/11: tem receita (os poderes de classe: "classe_<id>") e o instante do golpe dela (o da animação, em segundos). */
    static bool Has(FName Key);
    static float HitTime(FName Key);
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

    struct FBit
    {
        FVector Pos = FVector::ZeroVector, Vel = FVector::ZeroVector;
        FRotator Rot = FRotator::ZeroRotator, Spin = FRotator::ZeroRotator;
        float Size = 1.f, Delay = 0.f, Phase = 0.f;
    };
    struct FPart
    {
        FName Type;
        TMap<FName, float> V;
        float Delay = 0.f, Life = 1.f;
        bool bStarted = false;
        TArray<FBit> Bits;
        UInstancedStaticMeshComponent* Ism = nullptr;   // presos ao ator (OwnedComponents): o GC não leva
        TArray<UInstancedStaticMeshComponent*> Extra;   // a onda: a água clara e a espuma
        float NextSpawn = 0.f;
        UProceduralMeshComponent* Arc = nullptr;
        UMaterialInstanceDynamic* Look = nullptr;
        float Value(const TCHAR* Key, float Default = 0.f) const { const float* F = V.Find(FName(Key)); return F ? *F : Default; }
    };

    /** As texturas animadas do EffectCraft (Data/VFX/Flipbooks, Tools/VFX/MakeFlipbooks.py): agua, poeira, brilho... */
    struct FSprite
    {
        int32 Tex = 0;
        FVector Pos = FVector::ZeroVector, Vel = FVector::ZeroVector;
        float Size = 100.f, Start = 0.f, Life = .6f, Gravity = 0.f;
        FLinearColor Tint = FLinearColor::White;
    };

private:
    void AddSprite(int32 Tex, const FVector& Pos, const FVector& Vel, float Size, float Life, float Delay = 0.f, float Gravity = 0.f);
    void TickSprites();
    void TickWave(FPart& Part, float T);
    void TickCracks(FPart& Part, float T);
    void StartPart(FPart& Part);
    void TickPart(FPart& Part, float T);
    FVector Aim() const;
    void SetHeroHidden(bool bHide);

    TArray<FPart> Parts;
    TArray<FLinearColor> Colors;
    TWeakObjectPtr<ACharacter> Hero;
    FVector Origin = FVector::ZeroVector, Forward = FVector::ForwardVector, Right = FVector::RightVector;
    FVector AimAt = FVector::ZeroVector;                      // onde a mira do herói bate no chão (as runas, a chuva, a explosão)
    TArray<TWeakObjectPtr<USceneComponent>> HiddenParts;      // "vanish": o que estava à mostra no herói e foi escondido
    float Ground = 0.f, Age = 0.f, Lifetime = 2.f;
    TArray<FSprite> Sprites;
    TMap<int32, UInstancedStaticMeshComponent*> SpriteMeshes;
    TArray<FVector> TrailPoints;
    TArray<float> TrailAges;
};
