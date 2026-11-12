#pragma once

#include "CoreMinimal.h"

/** Como um golpe de criatura acerta (ValleyMonster.Moves.cs e RegionalBoss.cs do Godot). */
enum class EVortexMoveKind : uint8
{
    Strike,     // golpe à frente: alcance e cone (mordida, soco, porrete, língua)
    Area,       // em volta da criatura no impacto (esporos, guincho, pancada no chão)
    Lunge,      // salta/avança e acerta onde cai (bote, mergulho, salto da gota e do sapo)
    Lob,        // arremessa em arco até onde o herói estava (pedra, veneno)
    Hop,        // pulo para trás (o cogumelo guardando distância)
    Pattern,    // chefes: marcas no chão que enchem e explodem (e às vezes uma investida)
    Shoot,      // flecha de verdade (AVortexProjectile) até o herói: o batedor dos bandidos
};

/** Uma marca no chão de um golpe de chefe. Anchor: 0 = a própria criatura, 1 = à frente, 2 = no herói. */
struct FVortexMark
{
    uint8 Anchor = 0;
    float Ahead = 0.f;       // cm à frente (Anchor 1) ou de lado do herói (Anchor 2, com SideAngle)
    float Radius = 300.f;    // cm
    float Delay = 1.f;       // s até explodir
    float Scale = 1.f;       // do dano
    bool bRing = false;      // anel: o miolo é seguro
    int32 MinPhase = 1;      // só a partir desta fase (chefes ficam mais perigosos)
    float SideAngle = 0.f;   // graus (marca deslocada para o lado)
};

struct FVortexMonsterMove
{
    FName Id, Clip;
    EVortexMoveKind Kind = EVortexMoveKind::Strike;
    float Impact = .35f;         // s do começo até o golpe valer
    float MinRange = 0.f, MaxRange = 250.f;   // cm: quando escolher este golpe
    float Reach = 250.f;         // cm: alcance do acerto (Strike/Lunge) ou raio (Area/Lob)
    float FrontDot = -1.f;       // Strike: só acerta quem está à frente
    float Scale = 1.f;           // do dano da criatura
    float OwnCooldown = 0.f;     // recarga só deste golpe
    float After = 1.f;           // espera depois do golpe (todos os golpes)
    float Retreat = 0.f;         // recua por esse tempo depois
    float LungeFrom = -1.f;      // Lunge: quando o corpo sai do lugar (s)
    float LungeDistance = 0.f;   // Lunge: até onde vai (cm, no máximo até o herói)
    float Flight = .8f;          // Lob: tempo de voo
    float Stun = 0.f;            // atordoa o herói? (não usado ainda: heróis não ficam atordoados)
    bool bMagic = false;         // a Resistência corta
    bool bTelegraph = false;     // Area/Lunge: marca o chão antes
    bool bAtTarget = false;      // 10/11, Area: a marca (e o golpe) nasce sob o herói, não sob o monstro (as raízes do cogumelo)
    float Rush = 0.f;            // Pattern: investida para a frente (cm)
    bool bLeavesLava = false;    // Pattern: cada marca vira poça de lava (e cai um meteoro nela)
    float Duration = 1.f;        // Pattern: tempo da ação inteira
    TArray<FVortexMark> Marks;   // Pattern
    FString Label;               // o que aparece na barra do chefe ("PINÇAS · afaste-se da frente")
};

/** Uma espécie do Godot: tamanho, ritmo, vida, dano, golpes e o que acontece ao vencê-la. */
struct VORTEX_API FVortexSpeciesDef
{
    FName Species;               // "Slime", "Golem"... (o nome do arquivo do Godot)
    FString Title;
    float Radius = 40.f, Height = 120.f;   // cm (cápsula)
    float Speed = 220.f;         // cm/s andando atrás do herói
    float Accel = 1600.f;        // cm/s²
    float Sight = 1200.f;        // cm
    float Hits = 3.f;            // golpes de um herói do mesmo nível para vencer (Godot ConfigureLevel)
    float BaseDamage = 10.f;
    float CloseEnough = 0.f;     // cm: para de avançar nessa distância (cogumelo guarda distância)
    bool bPassive = false;       // animal: pasta e foge
    float Scare = 0.f;           // cm: animal foge se o herói chega a essa distância
    bool bFlying = false;        // morcego: paira acima do chão
    bool bClimber = false;       // aranha: sobe paredes (e árvores), anda no teto e desce pendurada no fio
    bool bFearful = false;       // foge um pouco com pouca vida
    bool bMagic = false;         // os golpes dela são mágicos
    bool bBoss = false;          // chefe: barra grande, fases, não fica atordoado nem é empurrado
    float BossHealth = 0.f;      // chefes: vida fixa (não pelo nível)
    float Leash = 4000.f;        // cm da casa: volta se o herói foge para além disso
    int32 DefaultLevel = 1;
    FName IdleClip = TEXT("Idle"), WalkClip = TEXT("Walk"), RunClip, HitClip = TEXT("Hit"), DeathClip = TEXT("Death");
    FName GrazeClip, AlertClip, AwakenClip;
    FName HitSound, StepSound;
    FLinearColor Splash = FLinearColor(.48f, .02f, .03f);   // respingo ao apanhar
    FLinearColor Telegraph = FLinearColor(1.f, .45f, .2f);  // cor das marcas no chão
    TArray<FVortexMonsterMove> Moves;
    /** O que o golpe de perto deixa no herói: "burn" (queima) ou "poison" (veneno). */
    FName OnHit;
    // ---- humanoides (bandidos): o aventureiro com roupa e arma de verdade, como no Godot
    bool bHumanoid = false;
    /** Tipo de arma (DA_<tipo>): modelo, osso da mão, posturas (Idle/Walk/Run) e golpes (Swing0..). */
    FName Weapon;
    /** Escudo na mão esquerda: defende pela frente (40% do dano passa) quando não está golpeando. */
    bool bShield = false;
    /** Tamanho do corpo (o brutamontes é 1,45x maior em tudo). */
    float BodyScale = 1.f;
    /** Malha e animações de outra espécie (as variações do goblin usam as do Goblin). Vazio: as dela mesma. */
    FName Model;
    /** Guarda distância: recua se o herói chega mais perto que isso (cm; o arqueiro). */
    float KeepAway = 0.f;

    static const FVortexSpeciesDef* Find(FName Species);
    static const TArray<FName>& All();
};
