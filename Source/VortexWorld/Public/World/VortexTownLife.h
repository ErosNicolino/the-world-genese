#pragma once

#include "GameFramework/Actor.h"
#include "VortexTownLife.generated.h"

class FJsonObject;
class UAnimSequence;
class UAudioComponent;
class USkeletalMeshComponent;
class USoundAttenuation;
class USoundBase;

/**
 * A vida da cidade (11/11, o usuário: "bichos na cidade e som de cada lugar"), enfeite em cada máquina que desenha (como
 * os pássaros), criada pelo AVortexTown com a lista "animals" da vila.json (Tools/Town/BuildVila.py).
 *
 * - Bichos (Cube World Kit da Quaternius, CC0; Tools/ImportTownLife.py): galinhas e pintinhos ciscando nos quintais,
 *   gatos, cachorros andando pelas ruas (só nas pedras: AVortexTown::MapKindAt), ovelhas e porcos no cercado. Andam até
 *   um ponto perto de casa, param, comem; galinhas, pintinhos e gatos fogem de quem chega perto.
 * - Sons dos bichos (Tools/Sfx/MakeTownSounds.py), de vez em quando e só com alguém perto: o galo canta de manhã, o
 *   cachorro late mais à noite.
 * - Sons dos lugares: o fogo e a bigorna na forja (nas horas de trabalho), a conversa na taverna (mais no almoço e à
 *   noite), o sino da capela às 6, 12 e 18 h, o cavalo no estábulo e o burburinho do mercado de dia.
 * Longe da câmera (> 50 m) os bichos não andam nem animam.
 */
UCLASS(Transient)
class VORTEXWORLD_API AVortexTownLife : public AActor
{
    GENERATED_BODY()

public:
    AVortexTownLife();
    void Init(const TSharedPtr<FJsonObject>& Plan);
    virtual void Tick(float DeltaSeconds) override;

private:
    enum class EAct : uint8 { Idle, Walk, Run, Eat };
    struct FKind
    {
        FName Name;
        float Scale = .2f, Walk = 60.f, Run = 260.f, Flee = 0.f, Sound = 20.f;
        TArray<USoundBase*> Sounds;
        UAnimSequence* Anims[4] = {};
    };
    struct FAnimal
    {
        int32 Kind = 0;
        FVector2D Home, At, Target;
        float Radius = 300.f, Yaw = 0.f, Timer = 0.f, SoundIn = 5.f;
        EAct Act = EAct::Idle;
        TWeakObjectPtr<USkeletalMeshComponent> Mesh;
    };
    struct FPlace
    {
        FName Kind;
        FVector At;
        float Timer = 0.f;
        int32 Rings = 0;
        TWeakObjectPtr<UAudioComponent> Loop;
    };
    TArray<FKind> Kinds;
    TArray<FAnimal> Animals;
    TArray<FPlace> Places;
    UPROPERTY(Transient) TObjectPtr<USoundAttenuation> Near;
    UPROPERTY(Transient) TObjectPtr<USoundAttenuation> Far;
    UPROPERTY(Transient) TArray<TObjectPtr<UObject>> Keep;
    float LastHour = -1.f;

    void SetAct(FAnimal& A, EAct Act);
    void PickTarget(FAnimal& A, const FVector2D& Away);
    float GroundAt(const FVector2D& At) const;
    void TickPlaces(float DeltaSeconds, float Hour, const FVector& Eye);
};
