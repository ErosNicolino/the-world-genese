#pragma once

#include "GameFramework/Actor.h"
#include "VortexWildlife.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UStaticMesh;

/**
 * Fauna e flora da água em volta da câmera ("pássaros e peixes, e plantas de diferentes tipos"), em cada máquina que
 * desenha (criada junto da AVortexForestLife; só enfeite, não passa pela rede). Tudo de blocos (M_LifeBlocks).
 * - Pássaros, um por região: pardal (Valdora, Aurivale), tiziu-de-brasa (Brasafenda), andorinha-da-tempestade (Raikor),
 *   pardal-da-neve (Nivaluz), abutre (Solcânion, só no alto), garça (Lodonegro, na água rasa), corvo (Morthúria, também
 *   de noite), pássaro-lume (Verdalume, cauda acesa) e gaivota (no mar e na costa).
 *   - Bandos no céu dando voltas devagar (os grandes planam).
 *   - Grupos no chão: pulam, bicam o chão e piam; quando alguém chega perto (~6,5 m) levantam voo juntos, com o bater
 *     de asas, e voltam a pousar depois em outro lugar, fora da vista.
 *   - De noite somem (menos os corvos).
 * - Peixes, um por região (truta, carpa-dourada, peixe-gelo, bagre-do-lodo, peixe-lume, peixe-brasa, sardinha no mar;
 *   1 cardume em 20 tem um peixe dourado): cardumes nadando na água funda, fogem de quem chega perto e às vezes um pula
 *   para fora e cai com respingo ("plic").
 * - Plantas da água: vitórias-régias na água rasa (umas com flor; mais no pântano) e algas no fundo (verdes, escuras no
 *   lodo, vermelhas na brasa, acesas em Verdalume).
 * - Cogumelos soltam uma nuvem de esporos quando o herói passa correndo por cima (os acesos soltam esporos acesos).
 * -VortexNoWildlife desliga.
 */
UCLASS(Transient)
class VORTEXWORLD_API AVortexWildlife : public AActor
{
    GENERATED_BODY()

public:
    AVortexWildlife();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    static AVortexWildlife* Find(const UWorld* World);

    // testes e medições
    int32 SkyBirds() const;
    int32 GroundBirds() const;
    int32 FleeingBirds() const;
    int32 FishCount() const { return Fish.Num(); }
    int32 JumpingFish() const;
    int32 LilyPads() const { return PadCount; }
    int32 Seaweeds() const { return WeedCount; }
    int32 Puffs() const { return Particles.Num(); }
    /** Centro do grupo de pássaros no chão mais perto (ZeroVector: nenhum). */
    FVector NearestGroundFlock(const FVector& From) const;
    /** Centro do cardume mais perto (ZeroVector: nenhum). */
    FVector NearestSchool(const FVector& From) const;
    /** Vitória-régia mais perto (ZeroVector: nenhuma). */
    FVector NearestPad(const FVector& From) const;
    /** Testes: põe um grupo no chão e um cardume perto de At (se houver água ali perto) agora. */
    void DebugSpawnNear(const FVector& At);
    /** Testes: solta os esporos de um cogumelo em At. */
    void DebugPuff(const FVector& At, bool bGlow);
    FString Describe() const;

    // as espécies (cores e tamanhos no .cpp)
    enum EBird : uint8 { Sparrow, EmberFinch, StormSwallow, SnowSparrow, Vulture, Heron, Crow, Glowbird, Gull, BirdKinds };
    enum EFish : uint8 { Trout, Koi, IceFish, MudCatfish, Glowfish, EmberFish, Sardine, GoldFish, FishKinds };
    static FString BirdName(int32 Kind);
    static FString FishName(int32 Kind);

private:
    struct FBird
    {
        uint8 Kind = Sparrow;
        uint8 Mode = 0;            // 0 no céu, 1 no chão, 2 fugindo
        FVector Pos = FVector::ZeroVector, Vel = FVector::ZeroVector;
        float Slot = 0.f, Flap = 0.f, Yaw = 0.f, Roll = 0.f, Peck = 0.f, Timer = 0.f;
        FVector HopFrom = FVector::ZeroVector, HopTo = FVector::ZeroVector;
        float HopT = 1.f;
        int32 Group = 0;
    };
    struct FFlock
    {
        uint8 Kind = Sparrow;
        bool bSky = true, bActive = false, bFled = false;
        FVector Center = FVector::ZeroVector, Drift = FVector::ZeroVector;
        float Radius = 1500.f, Height = 2500.f, Angle = 0.f, Spin = .2f, Timer = 0.f, NextCall = 0.f;
    };
    struct FFishy
    {
        uint8 Kind = Trout;
        FVector Pos = FVector::ZeroVector, Vel = FVector::ZeroVector, Offset = FVector::ZeroVector;
        float Phase = 0.f, Yaw = 0.f, JumpT = -1.f;
        FVector JumpFrom = FVector::ZeroVector, JumpTo = FVector::ZeroVector;
        int32 School = 0;
    };
    struct FSchool
    {
        bool bActive = false;
        FVector Center = FVector::ZeroVector, Target = FVector::ZeroVector;
        float Surface = 0.f, Depth = 0.f, Rush = 0.f, NextJump = 0.f, Retarget = 0.f;
    };
    struct FParticle { int32 Look = 0; FVector Pos = FVector::ZeroVector, Vel = FVector::ZeroVector; float Life = 0.f, MaxLife = 1.f, Size = 1.f; };

    struct FDraw { TObjectPtr<UInstancedStaticMeshComponent> Ism; int32 Count = 0; };
    TArray<FDraw> BirdBody, BirdWingL, BirdWingR, FishBody, FishTail;
    FDraw Pads, PadFlowers, PadGlowFlowers, WeedGreen, WeedDark, WeedRed, WeedGlow;
    TArray<FDraw> PuffDraw;   // esporos (marrom, vermelho, aceso, verde) e respingo
    UPROPERTY(Transient) TArray<TObjectPtr<UObject>> Keep;
    UPROPERTY(Transient) TObjectPtr<UMaterialInterface> Material;

    TArray<FBird> Birds;
    TArray<FFlock> Flocks;
    TArray<FFishy> Fish;
    TArray<FSchool> Schools;
    TArray<FParticle> Particles;
    TMap<FIntVector, double> PuffedAt;
    FVector PlantsCenter = FVector(1.e9f);
    int32 PadCount = 0, WeedCount = 0;
    bool bSeeded = false;
    TArray<FVector> PadSpots;
    float Daylight = 1.f, Clock = 0.f, ScareTimer = 0.f, SporeTimer = 0.f;
    TArray<FVector> Scarers;   // personagens perto (atualizado 5 vezes por segundo)
    FRandomStream Rng = FRandomStream(4242);

    FDraw MakeDraw(UStaticMesh* Mesh, float Cull, bool bShadow);
    void Flush(FDraw& Draw, const TArray<FTransform>& Transforms);
    FVector CameraLocation(FVector* Forward = nullptr) const;
    bool IsNight() const { return Daylight < .25f; }
    uint8 BirdFor(const FVector& At, bool bSky) const;
    uint8 FishFor(const FVector& At) const;
    bool FindLand(const FVector& Camera, const FVector& Forward, float MinD, float MaxD, bool bBehind, FVector& Out, bool bShallowOk) const;
    bool FindWater(const FVector& Camera, float MinD, float MaxD, float MinDepth, FVector& Out, float& Surface, float& Depth) const;
    void SpawnFlock(int32 Index, const FVector& Camera, const FVector& Forward, bool bSky, bool bFirst);
    void SpawnSchool(int32 Index, const FVector& Camera, bool bFirst);
    void UpdateBirds(float Dt, const FVector& Camera, const FVector& Forward);
    void UpdateFish(float Dt, const FVector& Camera);
    void UpdatePlants(const FVector& Camera);
    void UpdateSpores(float Dt);
    void Emit(int32 Look, const FVector& At, int32 Count, float Up, float Out, float Life, float Size);
    void DrawAll();
};
