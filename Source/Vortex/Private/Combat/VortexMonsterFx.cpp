#include "Combat/VortexMonsterFx.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "World/VortexTerrainData.h"
#include "World/VortexTerrainStreamer.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "UObject/StrongObjectPtr.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"

namespace
{
    struct FLook { float Life; TArray<FLinearColor> Colors; };

    // a cor de cada tom (do escuro ao claro) e quanto dura cada efeito
    const FLook& LookOf(FName Kind)
    {
        static const TMap<FName, FLook> Looks = {
            { TEXT("PoisonCloud"), { 2.2f, { FLinearColor(.06f, .2f, .03f), FLinearColor(.14f, .42f, .06f), FLinearColor(.32f, .7f, .14f) } } },
            { TEXT("PoisonPool"), { 2.6f, { FLinearColor(.05f, .18f, .03f), FLinearColor(.12f, .38f, .05f), FLinearColor(.4f, .85f, .2f) } } },
            { TEXT("Drip"), { .45f, { FLinearColor(.12f, .45f, .05f), FLinearColor(.3f, .8f, .14f), FLinearColor(.3f, .8f, .14f) } } },
            { TEXT("Tornado"), { 1.7f, { FLinearColor(.55f, .62f, .7f), FLinearColor(.8f, .86f, .92f), FLinearColor(1.f, 1.f, 1.f) } } },
            { TEXT("Dust"), { 1.f, { FLinearColor(.16f, .13f, .1f), FLinearColor(.3f, .25f, .19f), FLinearColor(.45f, .4f, .32f) } } },
            { TEXT("Splash"), { .8f, { FLinearColor(.1f, .14f, .05f), FLinearColor(.2f, .3f, .1f), FLinearColor(.35f, .5f, .2f) } } },
            { TEXT("Howl"), { 1.5f, { FLinearColor(.35f, .2f, .6f), FLinearColor(.6f, .45f, .95f), FLinearColor(.9f, .85f, 1.f) } } },
            { TEXT("Bite"), { .7f, { FLinearColor(.75f, .08f, .08f), FLinearColor(.95f, .9f, .8f), FLinearColor(1.f, 1.f, 1.f) } } },
            { TEXT("Headbutt"), { .4f, { FLinearColor(1.f, .75f, .15f), FLinearColor(1.f, .95f, .55f), FLinearColor(1.f, 1.f, 1.f) } } },
            { TEXT("GelPool"), { 2.2f, { FLinearColor(.1f, .4f, .3f), FLinearColor(.15f, .65f, .45f), FLinearColor(.3f, .95f, .7f) } } },
            { TEXT("GelDrip"), { .45f, { FLinearColor(.1f, .4f, .3f), FLinearColor(.15f, .65f, .45f), FLinearColor(.3f, .95f, .7f) } } },
            { TEXT("Swoop"), { .6f, { FLinearColor(.3f, .1f, .5f), FLinearColor(.6f, .3f, .9f), FLinearColor(.95f, .7f, 1.f) } } },
            { TEXT("Sonar"), { .9f, { FLinearColor(.1f, .5f, .6f), FLinearColor(.3f, .85f, .9f), FLinearColor(.8f, 1.f, 1.f) } } },
            { TEXT("Spikes"), { 1.3f, { FLinearColor(.2f, .12f, .06f), FLinearColor(.3f, .4f, .12f), FLinearColor(.6f, .75f, .3f) } } },
            { TEXT("Quake"), { 1.f, { FLinearColor(.2f, .19f, .17f), FLinearColor(.38f, .34f, .28f), FLinearColor(.55f, .5f, .4f) } } },
            { TEXT("Rubble"), { 1.f, { FLinearColor(.2f, .22f, .18f), FLinearColor(.34f, .4f, .3f), FLinearColor(.5f, .55f, .45f) } } },
            // 14/11: os golpes do Azhûr (UVortexVolcanicBoss::MulticastFx): só camadas e imagens, sem cubos
            { TEXT("SwSword1"), { 0.7f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("SwSword2"), { 0.7f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("SwSword3"), { 0.7f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("SwDagger1"), { 0.7f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("SwDagger2"), { 0.7f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("SwDagger3"), { 0.7f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("SwHammer1"), { 0.7f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("SwHammer2"), { 0.7f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("SwHammerSlam"), { 1.9f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("SwBow1"), { 0.7f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("SwBow3"), { 0.7f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("SwStaff1"), { 0.7f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("SwStaff3"), { 0.7f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("WaveTrail"), { 0.7f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("WaveHit"), { 0.7f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("Pwdash"), { 2.1f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("Pwgroundslam"), { 2.1f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("Pwruneseal"), { 2.1f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("Pwshockwave"), { 2.1f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("Pwwhirl"), { 2.1f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("Pworbit"), { 2.1f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("Pwflurry"), { 2.1f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("Pwsweep"), { 2.1f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("Pwreap"), { 2.1f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("Pwoverload"), { 2.1f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("Pwholyslam"), { 2.1f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("Pwtidewave"), { 2.1f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("Pwfirewall"), { 2.1f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("Pwheal"), { 2.1f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("Pwvolley"), { 2.1f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("Pwpierce"), { 2.1f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("Pwburst3"), { 2.1f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("Pwarrowrain"), { 2.1f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("Pwexplosive"), { 2.1f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("Pwfireball"), { 2.1f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("PwCharge"), { 2.1f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("PwBasic"), { 2.1f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("AzBeamHit"), { 2.6f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("AzHook"), { 1.7f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("AzHammer"), { 2.3f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("AzStomp"), { 1.9f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("AzSweep"), { 1.3f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("AzCombo"), { 1.5f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("AzCharge"), { 1.0f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("AzBurst"), { 2.5f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("AzFissure"), { 1.6f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("AzLaunch"), { 0.5f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("AzMeteor"), { 3.1f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("AzPillar"), { 1.9f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("AzShed"), { 2.1f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("AzUltimate"), { 3.6f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("AzWarn"), { 1.0f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("AzWarnRing"), { 1.0f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } } },
            { TEXT("Hurl"), { .7f, { FLinearColor(.7f, .7f, .7f), FLinearColor(.9f, .9f, .9f), FLinearColor(1.f, 1.f, 1.f) } } },
        };
        static const FLook Fallback = { 1.f, { FLinearColor::White, FLinearColor::White, FLinearColor::White } };
        const FLook* Look = Looks.Find(Kind);
        return Look ? *Look : Fallback;
    }

    /** A cor da névoa e do círculo de cada efeito. */
    FLinearColor TintOf(FName Kind)
    {
        if (Kind == TEXT("Tornado")) return FLinearColor(.6f, .55f, 1.f);
        if (Kind == TEXT("PoisonCloud") || Kind == TEXT("PoisonPool") || Kind == TEXT("Drip")) return FLinearColor(.28f, .75f, .12f);
        if (Kind == TEXT("Dust")) return FLinearColor(.55f, .43f, .3f);
        if (Kind == TEXT("Splash")) return FLinearColor(.3f, .42f, .16f);
        return FLinearColor::White;
    }

    float Bump(float T) { return FMath::Sin(FMath::Clamp(T, 0.f, 1.f) * UE_PI); }

    /** 13/11 (o usuário: "os efeitos podem ser bonitos, não precisa ser tudo quadrado", como os do Roblox): por cima das
     *  formas, as partículas com brilho de cada golpe (um sistema só de cada um) e em que instante elas saem (s). */
    const TCHAR* SparkOf(FName Kind, float& OutAt, float& OutScale)
    {
        OutAt = 0.f; OutScale = 1.f;
        if (Kind == TEXT("Howl")) { OutScale = 2.f; return TEXT("/Game/FreeParticle_SoftTofu/Niagara/NS_Sparkling_Moon.NS_Sparkling_Moon"); }
        if (Kind == TEXT("Bite")) { OutAt = .3f; OutScale = 1.4f; return TEXT("/Game/Vefects/Easy_Impact_Frames/VFX/Frames/Particles/Tests/NS_Impact_Frame_01.NS_Impact_Frame_01"); }
        if (Kind == TEXT("Headbutt")) return TEXT("/Game/Vefects/VFX_Anime_Star/VFX/Particles/NS_Anime_Star_01.NS_Anime_Star_01");
        if (Kind == TEXT("Sonar")) { OutScale = 1.5f; return TEXT("/Game/FreeParticle_SoftTofu/Niagara/NS_Sparkling_Noise.NS_Sparkling_Noise"); }
        if (Kind == TEXT("Spikes")) return TEXT("/Game/Vefects/VFX_Anime_Star/VFX/Particles/NS_Anime_Star_02_Vertical.NS_Anime_Star_02_Vertical");
        if (Kind == TEXT("Swoop")) return TEXT("/Game/ArrowTrail/FX/NS_ArrowTrail_Magic_2.NS_ArrowTrail_Magic_2");
        if (Kind == TEXT("Hurl")) return TEXT("/Game/ArrowTrail/FX/NS_ArrowTrail_Basic_02.NS_ArrowTrail_Basic_02");
        return nullptr;
    }

    /** 13/11, noite (o usuário aprovou o piloto no lobo): o golpe em camadas de partículas de verdade (ondas, clarão,
     *  faíscas), cada uma no seu instante. Quem tem camadas aqui não usa a partícula única de SparkOf. */
    struct FLayer { const TCHAR* Path; float At, Scale, Up; };
    TArrayView<const FLayer> LayersOf(FName Kind)
    {
        #define SHOCK TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves/")
        // o uivo: três ondas lilás que abrem rente ao chão (-75: o efeito nasce na altura do peito), o ar que treme e a lua em cima dele
        static const FLayer Howl[] = {
            { SHOCK TEXT("Particles/Color_Variation/VFX_Shockwave_01_Lavender_1s.VFX_Shockwave_01_Lavender_1s"), 0.f, 1.5f, -9999.f },
            { SHOCK TEXT("Particles/Color_Variation/VFX_Shockwave_01_Purple_Big_1s.VFX_Shockwave_01_Purple_Big_1s"), .2f, 1.6f, -9999.f },
            { SHOCK TEXT("Particles/Color_Variation/VFX_Shockwave_01_Lavender_Big_1s.VFX_Shockwave_01_Lavender_Big_1s"), .4f, 2.2f, -9999.f },
            { TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves_Distortion/Particles/VFX_Shockwave_Distortion_01_Big_1s.VFX_Shockwave_Distortion_01_Big_1s"), 0.f, 1.6f, -9999.f },
            { TEXT("/Game/FreeParticle_SoftTofu/Niagara/NS_Sparkling_Moon.NS_Sparkling_Moon"), 0.f, 2.f, 60.f },
        };
        // a mordida: quando a bocarra fecha, a onda vermelha de pé, a estrela de impacto e o ar que treme
        static const FLayer Bite[] = {
            { SHOCK TEXT("Particles/Color_Variation/VFX_Shockwave_Vertical_01_Red_1s.VFX_Shockwave_Vertical_01_Red_1s"), .3f, 1.f, 0.f },
            { TEXT("/Game/Vefects/VFX_Anime_Star/VFX/Particles/NS_Anime_Star_03.NS_Anime_Star_03"), .3f, 1.6f, 0.f },
            { TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves_Distortion/Particles/Vertical/VFX_Shockwave_Vertical_Distortion_01_Small_1s.VFX_Shockwave_Vertical_Distortion_01_Small_1s"), .3f, 1.f, 0.f },
        };
        const float GROUND = -9999.f;
        // 13/11, noite: os outros golpes no mesmo estilo (onda da cor do golpe + a partícula dele); GROUND = rente ao chão
        static const FLayer Headbutt[] = {
            { SHOCK TEXT("Particles/Color_Variation/VFX_Shockwave_Vertical_01_Yellow_Small_1s.VFX_Shockwave_Vertical_01_Yellow_Small_1s"), 0.0f, 0.8f, 0.0f },
            { TEXT("/Game/Vefects/VFX_Anime_Star/VFX/Particles/NS_Anime_Star_01.NS_Anime_Star_01"), 0.0f, 1.0f, 0.0f },
        };
        static const FLayer Sonar[] = {
            { SHOCK TEXT("Particles/Color_Variation/VFX_Shockwave_Vertical_01_Teal_Small_1s.VFX_Shockwave_Vertical_01_Teal_Small_1s"), 0.0f, 1.0f, 0.0f },
            { SHOCK TEXT("Particles/Color_Variation/VFX_Shockwave_Vertical_01_Teal_1s.VFX_Shockwave_Vertical_01_Teal_1s"), 0.13f, 1.0f, 0.0f },
            { SHOCK TEXT("Particles/Color_Variation/VFX_Shockwave_Vertical_01_Teal_Big_1s.VFX_Shockwave_Vertical_01_Teal_Big_1s"), 0.26f, 1.0f, 0.0f },
            { TEXT("/Game/FreeParticle_SoftTofu/Niagara/NS_Sparkling_Noise.NS_Sparkling_Noise"), 0.0f, 1.5f, 0.0f },
        };
        static const FLayer Swoop[] = {
            { SHOCK TEXT("Particles/Color_Variation/VFX_Shockwave_Vertical_01_Lavender_Small_1s.VFX_Shockwave_Vertical_01_Lavender_Small_1s"), 0.0f, 1.0f, 0.0f },
            { TEXT("/Game/ArrowTrail/FX/NS_ArrowTrail_Magic_2.NS_ArrowTrail_Magic_2"), 0.0f, 1.0f, 0.0f },
        };
        static const FLayer Spikes[] = {
            { SHOCK TEXT("Particles/Color_Variation/VFX_Shockwave_01_Green_Small_1s.VFX_Shockwave_01_Green_Small_1s"), 0.0f, 1.0f, GROUND },
            { TEXT("/Game/Vefects/VFX_Anime_Star/VFX/Particles/NS_Anime_Star_02_Vertical.NS_Anime_Star_02_Vertical"), 0.0f, 1.0f, GROUND },
        };
        static const FLayer Quake[] = {
            { SHOCK TEXT("Particles/VFX_Shockwave_02_White_Big_1s.VFX_Shockwave_02_White_Big_1s"), 0.0f, 2.0f, GROUND },
            { SHOCK TEXT("Particles/Color_Variation/VFX_Shockwave_01_Yellow_Big_1s.VFX_Shockwave_01_Yellow_Big_1s"), 0.1f, 1.4f, GROUND },
            { TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves_Distortion/Particles/VFX_Shockwave_Distortion_02_Big_1s.VFX_Shockwave_Distortion_02_Big_1s"), 0.0f, 2.0f, GROUND },
        };
        static const FLayer Hurl[] = {
            { SHOCK TEXT("Particles/Vertical/VFX_Shockwave_Vertical_01_White_Small_1s.VFX_Shockwave_Vertical_01_White_Small_1s"), 0.0f, 1.0f, 0.0f },
            { TEXT("/Game/ArrowTrail/FX/NS_ArrowTrail_Basic_02.NS_ArrowTrail_Basic_02"), 0.0f, 1.0f, 0.0f },
        };
        static const FLayer GelPool[] = {
            { SHOCK TEXT("Particles/VFX_Shockwave_02_White_Small_1s.VFX_Shockwave_02_White_Small_1s"), 0.0f, 1.0f, GROUND },
        };
        static const FLayer Splash[] = {
            { SHOCK TEXT("Particles/VFX_Shockwave_01_White_Small_1s.VFX_Shockwave_01_White_Small_1s"), 0.0f, 0.8f, GROUND },
        };
        static const FLayer PoisonPool[] = {
            { SHOCK TEXT("Particles/Color_Variation/VFX_Shockwave_01_Green_1s.VFX_Shockwave_01_Green_1s"), 0.0f, 0.8f, GROUND },
        };
        static const FLayer PoisonCloud[] = {
            { SHOCK TEXT("Particles/Color_Variation/VFX_Shockwave_01_Green_Big_1s.VFX_Shockwave_01_Green_Big_1s"), 0.0f, 1.2f, GROUND },
            { TEXT("/Game/FreeParticle_SoftTofu/Niagara/NS_Sparkling_Skull.NS_Sparkling_Skull"), 0.0f, 2.0f, 80.0f },
        };
        static const FLayer Dust[] = {
            { TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves_Distortion/Particles/VFX_Shockwave_Distortion_01_1s.VFX_Shockwave_Distortion_01_1s"), 0.0f, 1.0f, GROUND },
        };
        static const FLayer Tornado[] = {
            { SHOCK TEXT("Particles/Color_Variation/VFX_Shockwave_01_Purple_1s.VFX_Shockwave_01_Purple_1s"), 0.0f, 1.0f, GROUND },
            { TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves_Distortion/Particles/VFX_Shockwave_Distortion_01_Small_1s.VFX_Shockwave_Distortion_01_Small_1s"), 0.0f, 1.0f, GROUND },
        };
        static const FLayer Rubble[] = {
            { TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves_Distortion/Particles/VFX_Shockwave_Distortion_02_Small_1s.VFX_Shockwave_Distortion_02_Small_1s"), 0.0f, 1.0f, GROUND },
        };
        // 14/11: os golpes do Azhûr refeitos do zero (o usuário: "muito feias, não combina, crie do zero cada uma"): cada
        // golpe com as ondas dele; as imagens ficam em SpritesOf. UVortexVolcanicBoss::MulticastFx chama pelo nome "Az..."
        static const FLayer AzHook[] = {
            { TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves/Particles/Color_Variation/VFX_Shockwave_Vertical_01_Orange_1s.VFX_Shockwave_Vertical_01_Orange_1s"), 0.0f, 1.2f, 60.0f },
        };
        static const FLayer AzHammer[] = {
            { TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves/Particles/VFX_Shockwave_02_White_Big_1s.VFX_Shockwave_02_White_Big_1s"), 0.0f, 1.3f, GROUND },
            { TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves/Particles/Color_Variation/VFX_Shockwave_01_Orange_Big_1s.VFX_Shockwave_01_Orange_Big_1s"), 0.1f, 1.5f, GROUND },
            { TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves_Distortion/Particles/VFX_Shockwave_Distortion_02_Big_1s.VFX_Shockwave_Distortion_02_Big_1s"), 0.0f, 1.4f, GROUND },
        };
        static const FLayer AzStomp[] = {
            { TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves/Particles/Color_Variation/VFX_Shockwave_01_Yellow_Big_1s.VFX_Shockwave_01_Yellow_Big_1s"), 0.0f, 2.0f, GROUND },
            { TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves_Distortion/Particles/VFX_Shockwave_Distortion_01_Big_1s.VFX_Shockwave_Distortion_01_Big_1s"), 0.0f, 2.0f, GROUND },
        };
        static const FLayer AzCombo[] = {
            { TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves/Particles/Color_Variation/VFX_Shockwave_01_Red_Small_1s.VFX_Shockwave_01_Red_Small_1s"), 0.0f, 1.3f, GROUND },
        };
        static const FLayer AzBurst[] = {
            { TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves/Particles/Color_Variation/VFX_Shockwave_01_Orange_Big_1s.VFX_Shockwave_01_Orange_Big_1s"), 0.0f, 2.2f, GROUND },
            { TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves/Particles/Color_Variation/VFX_Shockwave_01_Red_Big_1s.VFX_Shockwave_01_Red_Big_1s"), 0.15f, 3.0f, GROUND },
            { TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves_Distortion/Particles/VFX_Shockwave_Distortion_01_Big_1s.VFX_Shockwave_Distortion_01_Big_1s"), 0.0f, 2.4f, GROUND },
            { TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Explosion.NS_Free_Spells_Explosion"), 0.0f, 2.0f, 0.0f },
        };
        static const FLayer AzMeteor[] = {
            { TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves/Particles/Color_Variation/VFX_Shockwave_01_Orange_1s.VFX_Shockwave_01_Orange_1s"), 0.0f, 1.4f, GROUND },
            { TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves_Distortion/Particles/VFX_Shockwave_Distortion_02_1s.VFX_Shockwave_Distortion_02_1s"), 0.0f, 1.3f, GROUND },
        };
        static const FLayer AzPillar[] = {
            { TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves/Particles/Color_Variation/VFX_Shockwave_Vertical_01_Orange_Small_1s.VFX_Shockwave_Vertical_01_Orange_Small_1s"), 0.0f, 1.0f, 80.0f },
            { TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves/Particles/Color_Variation/VFX_Shockwave_01_Red_Small_1s.VFX_Shockwave_01_Red_Small_1s"), 0.0f, 1.0f, GROUND },
        };
        static const FLayer AzShed[] = {
            { TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves/Particles/Color_Variation/VFX_Shockwave_01_Black_Big_1s.VFX_Shockwave_01_Black_Big_1s"), 0.0f, 1.6f, GROUND },
            { TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves/Particles/Color_Variation/VFX_Shockwave_01_Orange_Big_1s.VFX_Shockwave_01_Orange_Big_1s"), 0.12f, 1.8f, GROUND },
            { TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves_Distortion/Particles/VFX_Shockwave_Distortion_01_Big_1s.VFX_Shockwave_Distortion_01_Big_1s"), 0.0f, 1.7f, GROUND },
        };
        static const FLayer AzUltimate[] = {
            { TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves/Particles/Color_Variation/VFX_Shockwave_01_Orange_Big_1s.VFX_Shockwave_01_Orange_Big_1s"), 0.0f, 3.4f, GROUND },
            { TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves/Particles/Color_Variation/VFX_Shockwave_01_Red_Big_1s.VFX_Shockwave_01_Red_Big_1s"), 0.2f, 4.4f, GROUND },
            { TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves/Particles/Color_Variation/VFX_Shockwave_01_Yellow_Big_1s.VFX_Shockwave_01_Yellow_Big_1s"), 0.4f, 5.2f, GROUND },
            { TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves_Distortion/Particles/VFX_Shockwave_Distortion_01_Big_1s.VFX_Shockwave_Distortion_01_Big_1s"), 0.0f, 4.0f, GROUND },
            { TEXT("/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Explosion.NS_Free_Spells_Explosion"), 0.0f, 3.5f, 0.0f },
        };
        // 14/11: as cinco armas iniciais (o usuário: "as adagas feias e simples, entre outras") e o corte de longo alcance da espada
        static const FLayer SwSword3[] = {
            { TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves/Particles/Vertical/VFX_Shockwave_Vertical_01_White_Small_1s.VFX_Shockwave_Vertical_01_White_Small_1s"), 0.0f, 0.9f, 20.0f },
        };
        static const FLayer SwDagger3[] = {
            { TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves/Particles/Color_Variation/VFX_Shockwave_Vertical_01_Teal_Small_1s.VFX_Shockwave_Vertical_01_Teal_Small_1s"), 0.0f, 0.8f, 20.0f },
        };
        static const FLayer SwHammerSlam[] = {
            { TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves/Particles/Color_Variation/VFX_Shockwave_01_Yellow_Big_1s.VFX_Shockwave_01_Yellow_Big_1s"), 0.0f, 1.0f, GROUND },
            { TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves_Distortion/Particles/VFX_Shockwave_Distortion_02_1s.VFX_Shockwave_Distortion_02_1s"), 0.0f, 1.0f, GROUND },
        };
        static const FLayer SwStaff3[] = {
            { TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves/Particles/Color_Variation/VFX_Shockwave_Vertical_01_Purple_Small_1s.VFX_Shockwave_Vertical_01_Purple_Small_1s"), 0.0f, 0.8f, 30.0f },
        };
        static const FLayer WaveHit[] = {
            { TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves/Particles/Color_Variation/VFX_Shockwave_Vertical_01_Teal_1s.VFX_Shockwave_Vertical_01_Teal_1s"), 0.0f, 1.0f, 0.0f },
            { TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves_Distortion/Particles/Vertical/VFX_Shockwave_Vertical_Distortion_01_Small_1s.VFX_Shockwave_Vertical_Distortion_01_Small_1s"), 0.0f, 1.0f, 0.0f },
        };
        // 14/11: os poderes das espadas e dos arcos (um por tipo de poder; a cor vem do elemento da arma)
        static const FLayer Pwgroundslam[] = {
            { TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves/Particles/VFX_Shockwave_02_White_Big_1s.VFX_Shockwave_02_White_Big_1s"), 0.25f, 1.0f, GROUND },
            { TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves_Distortion/Particles/VFX_Shockwave_Distortion_02_1s.VFX_Shockwave_Distortion_02_1s"), 0.25f, 1.0f, GROUND },
        };
        static const FLayer Pwshockwave[] = {
            { TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves/Particles/Vertical/VFX_Shockwave_Vertical_01_White_1s.VFX_Shockwave_Vertical_01_White_1s"), 0.2f, 1.0f, 20.0f },
        };
        static const FLayer Pwholyslam[] = {
            { TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves/Particles/VFX_Shockwave_01_White_Big_1s.VFX_Shockwave_01_White_Big_1s"), 0.3f, 1.2f, GROUND },
        };
        static const FLayer Pwoverload[] = {
            { TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves_Distortion/Particles/VFX_Shockwave_Distortion_01_Small_1s.VFX_Shockwave_Distortion_01_Small_1s"), 0.0f, 1.0f, GROUND },
        };
        static const FLayer Pwexplosive[] = {
            { TEXT("/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves/Particles/Vertical/VFX_Shockwave_Vertical_01_White_Small_1s.VFX_Shockwave_Vertical_01_White_Small_1s"), 0.0f, 0.9f, 30.0f },
        };
        #undef SHOCK
        if (Kind == TEXT("Howl")) return Howl;
        if (Kind == TEXT("Bite")) return Bite;
        if (Kind == TEXT("Pwgroundslam")) return Pwgroundslam;
        if (Kind == TEXT("Pwshockwave")) return Pwshockwave;
        if (Kind == TEXT("Pwholyslam")) return Pwholyslam;
        if (Kind == TEXT("Pwoverload")) return Pwoverload;
        if (Kind == TEXT("Pwexplosive")) return Pwexplosive;
        if (Kind == TEXT("SwSword3")) return SwSword3;
        if (Kind == TEXT("SwDagger3")) return SwDagger3;
        if (Kind == TEXT("SwHammerSlam")) return SwHammerSlam;
        if (Kind == TEXT("SwStaff3")) return SwStaff3;
        if (Kind == TEXT("WaveHit")) return WaveHit;
        if (Kind == TEXT("AzHook")) return AzHook;
        if (Kind == TEXT("AzHammer")) return AzHammer;
        if (Kind == TEXT("AzStomp")) return AzStomp;
        if (Kind == TEXT("AzCombo")) return AzCombo;
        if (Kind == TEXT("AzBurst")) return AzBurst;
        if (Kind == TEXT("AzMeteor")) return AzMeteor;
        if (Kind == TEXT("AzPillar")) return AzPillar;
        if (Kind == TEXT("AzShed")) return AzShed;
        if (Kind == TEXT("AzUltimate")) return AzUltimate;
        if (Kind == TEXT("Headbutt")) return Headbutt;
        if (Kind == TEXT("Sonar")) return Sonar;
        if (Kind == TEXT("Swoop")) return Swoop;
        if (Kind == TEXT("Spikes")) return Spikes;
        if (Kind == TEXT("Quake")) return Quake;
        if (Kind == TEXT("Hurl")) return Hurl;
        if (Kind == TEXT("GelPool")) return GelPool;
        if (Kind == TEXT("Splash")) return Splash;
        if (Kind == TEXT("PoisonPool")) return PoisonPool;
        if (Kind == TEXT("PoisonCloud")) return PoisonCloud;
        if (Kind == TEXT("Dust")) return Dust;
        if (Kind == TEXT("Tornado")) return Tornado;
        if (Kind == TEXT("Rubble")) return Rubble;
        return {};
    }

    /** 13/11, noite: as imagens de efeito (Kenney Particle Pack, CC0; Tools/ImportFxSprites.py) em planos com transparência
     *  de verdade: seguram a cor de dia, quando as ondas de luz (LayersOf) quase somem no chão claro.
     *  Mode: 0 deitado no chão, 1 de frente para a câmera, 2 de pé, virado para onde o golpe aponta.
     *  From/To: o tamanho, em vezes o diâmetro do golpe; Travel: quanto avança (em raios) no modo 2. */
    struct FSprite { const TCHAR* Tex; float At, Life, From, To; FLinearColor Color; int32 Mode; float Up, Spin, Travel; bool bTint; };
    struct FLiveSprite { TWeakObjectPtr<UStaticMeshComponent> Mesh; TWeakObjectPtr<UMaterialInstanceDynamic> Mat; int32 Index = 0; };
    TArrayView<const FSprite> SpritesOf(FName Kind)
    {
        static const FLinearColor Lilac(2.2f, 1.2f, 4.f), Purple(1.1f, .3f, 2.4f), Blood(3.5f, .15f, .1f), White(4.f, 4.f, 4.f), Gold(4.f, 2.6f, .3f),
            Teal(.2f, 2.6f, 2.8f), Leaf(.5f, 2.2f, .25f), Venom(.35f, 1.6f, .1f), Earth(.42f, .3f, .18f), Sand(1.6f, 1.25f, .8f);
        static const FSprite Howl[] = {
            { TEXT("circle_03"), 0.f, .7f, .08f, 1.f, Lilac, 0, 0.f, 40.f, 0.f, false }, { TEXT("circle_03"), .25f, .7f, .08f, 1.f, Purple, 0, 0.f, -40.f, 0.f, false },
            { TEXT("twirl_01"), 0.f, .9f, .15f, .5f, Lilac, 0, 0.f, 260.f, 0.f, false }, { TEXT("flare_01"), 0.f, .5f, .1f, .35f, White, 1, 60.f, 0.f, 0.f, false },
        };
        // a mordida: o clarão quando a bocarra fecha, o arranhão vermelho por cima e o corte branco quando ela abre
        static const FSprite Bite[] = {
            { TEXT("flare_01"), .3f, .22f, .6f, 2.2f, White, 1, 0.f, 0.f, 0.f, false }, { TEXT("scratch_01"), .3f, .45f, .9f, 1.3f, Blood, 1, 0.f, 0.f, 0.f, false },
            { TEXT("slash_03"), .05f, .3f, .5f, 1.2f, White, 1, 60.f, 0.f, 0.f, false }, { TEXT("star_07"), .3f, .3f, .3f, 1.f, Blood, 1, 0.f, 200.f, 0.f, false },
        };
        static const FSprite Headbutt[] = { { TEXT("star_04"), 0.f, .28f, .6f, 2.4f, Gold, 1, 0.f, 0.f, 0.f, false }, { TEXT("circle_02"), 0.f, .3f, .4f, 2.f, White, 2, 0.f, 0.f, 0.f, false } };
        static const FSprite Sonar[] = {
            { TEXT("circle_02"), 0.f, .55f, .06f, .3f, Teal, 2, 0.f, 0.f, 1.f, false }, { TEXT("circle_02"), .13f, .55f, .06f, .3f, Teal, 2, 0.f, 0.f, 1.f, false },
            { TEXT("circle_02"), .26f, .55f, .06f, .3f, White, 2, 0.f, 0.f, 1.f, false },
        };
        static const FSprite Swoop[] = { { TEXT("slash_02"), 0.f, .35f, 1.5f, 3.5f, Lilac, 1, 0.f, 0.f, 0.f, false } };
        static const FSprite Spikes[] = { { TEXT("dirt_02"), 0.f, .6f, .5f, 1.3f, Earth, 0, 0.f, 0.f, 0.f, false }, { TEXT("circle_05"), 0.f, .4f, .3f, 1.1f, Leaf, 0, 0.f, 0.f, 0.f, false } };
        static const FSprite Quake[] = {
            { TEXT("circle_05"), 0.f, .6f, .1f, 1.1f, Earth, 0, 0.f, 0.f, 0.f, false }, { TEXT("circle_03"), .12f, .6f, .1f, 1.1f, Sand, 0, 0.f, 0.f, 0.f, false },
            { TEXT("dirt_02"), 0.f, .8f, .4f, 1.f, Earth, 0, 0.f, 30.f, 0.f, false },
        };
        static const FSprite Hurl[] = { { TEXT("twirl_02"), 0.f, .45f, 1.5f, 4.f, White, 1, 0.f, 500.f, 0.f, false } };
        static const FSprite Gel[] = { { TEXT("circle_02"), 0.f, .5f, .3f, 1.3f, Teal, 0, 0.f, 0.f, 0.f, true }, { TEXT("spark_04"), 0.f, .35f, .4f, 1.2f, White, 0, 0.f, 0.f, 0.f, false } };
        static const FSprite Splash[] = { { TEXT("circle_03"), 0.f, .4f, .2f, 1.f, White, 0, 0.f, 0.f, 0.f, false } };
        static const FSprite Poison[] = { { TEXT("magic_04"), 0.f, 1.6f, .5f, 1.f, Venom, 0, 0.f, 50.f, 0.f, false }, { TEXT("smoke_04"), 0.f, 1.2f, .4f, 1.1f, Leaf, 0, 0.f, -20.f, 0.f, false } };
        static const FSprite Dust[] = { { TEXT("dirt_02"), 0.f, .7f, .4f, 1.1f, Earth, 0, 0.f, 0.f, 0.f, false }, { TEXT("smoke_04"), 0.f, .8f, .4f, 1.f, Sand, 0, 0.f, 20.f, 0.f, false } };
        static const FSprite Tornado[] = { { TEXT("twirl_01"), 0.f, 1.6f, .7f, 1.1f, Lilac, 0, 0.f, 500.f, 0.f, false }, { TEXT("twirl_02"), 0.f, 1.6f, .5f, .9f, White, 0, 0.f, -380.f, 0.f, false } };
        static const FSprite Rubble[] = { { TEXT("circle_05"), 0.f, .6f, .3f, 1.f, Earth, 0, 0.f, 0.f, 0.f, false } };
        static const FLinearColor Fire(4.f, 1.4f, .15f), Ember(3.f, .45f, .05f), Scorch(.07f, .045f, .035f), Rune(.2f, 2.2f, 3.f);
        // 14/11: os golpes do Azhûr, um por um (magma e as runas ciano dele); AzCharge e os avisos usam o tempo do golpe (IsTimed)
        // Raio do Núcleo: onde o feixe toca o chão (a cada instante): a labareda, as faíscas e o rastro queimado que fica
        static const FSprite AzBeamHit[] = {
            { TEXT("scorch_01"), 0.f, 2.5f, .9f, 1.1f, Scorch, 0, 0.f, 0.f, 0.f, false }, { TEXT("flame_03"), 0.f, .45f, .8f, 1.8f, Fire, 1, 70.f, 0.f, 0.f, false },
            { TEXT("spark_06"), 0.f, .25f, .6f, 1.6f, Gold, 1, 50.f, 0.f, 0.f, false }, { TEXT("flare_01"), 0.f, .15f, .8f, 2.f, White, 1, 40.f, 0.f, 0.f, false },
        };
        static const FSprite AzHook[] = {
            { TEXT("scorch_01"), 0.0f, 1.6f, 0.7f, 1.1f, Scorch, 0, 0.f, 0.0f, 0.f, false },
            { TEXT("slash_04"), 0.0f, 0.45f, 0.6f, 1.5f, Fire, 0, 0.f, 0.0f, 0.f, false },
            { TEXT("slash_02"), 0.06f, 0.5f, 0.5f, 1.3f, Ember, 0, 0.f, 0.0f, 0.f, false },
            { TEXT("spark_06"), 0.0f, 0.35f, 0.4f, 1.3f, Gold, 1, 80.0f, 0.0f, 0.f, false },
            { TEXT("flare_01"), 0.0f, 0.18f, 0.3f, 1.1f, White, 1, 80.0f, 0.0f, 0.f, false },
        };
        static const FSprite AzHammer[] = {
            { TEXT("scorch_02"), 0.0f, 2.2f, 0.7f, 1.2f, Scorch, 0, 0.f, 0.0f, 0.f, false },
            { TEXT("circle_04"), 0.0f, 0.5f, 0.15f, 1.2f, Fire, 0, 0.f, 0.0f, 0.f, false },
            { TEXT("dirt_02"), 0.0f, 0.7f, 0.4f, 1.4f, Earth, 0, 0.f, 40.0f, 0.f, false },
            { TEXT("star_02"), 0.0f, 0.3f, 0.4f, 1.6f, Gold, 1, 100.0f, 0.0f, 0.f, false },
            { TEXT("flare_01"), 0.0f, 0.2f, 0.4f, 1.5f, White, 1, 100.0f, 0.0f, 0.f, false },
        };
        static const FSprite AzStomp[] = {
            { TEXT("scorch_03"), 0.0f, 1.8f, 0.5f, 0.9f, Scorch, 0, 0.f, 0.0f, 0.f, false },
            { TEXT("circle_01"), 0.0f, 0.6f, 0.1f, 1.1f, Sand, 0, 0.f, 0.0f, 0.f, false },
            { TEXT("circle_01"), 0.15f, 0.6f, 0.1f, 1.1f, Earth, 0, 0.f, 0.0f, 0.f, false },
            { TEXT("twirl_03"), 0.0f, 0.9f, 0.4f, 1.0f, Sand, 0, 0.f, 200.0f, 0.f, false },
            { TEXT("dirt_02"), 0.0f, 0.8f, 0.5f, 1.2f, Earth, 0, 0.f, -60.0f, 0.f, false },
        };
        static const FSprite AzSweep[] = {
            { TEXT("scorch_01"), 0.0f, 1.2f, 0.6f, 1.0f, Scorch, 0, 0.f, 0.0f, 0.f, false },
            { TEXT("flame_03"), 0.0f, 0.5f, 0.6f, 1.5f, Fire, 1, 110.0f, 0.0f, 0.f, false },
            { TEXT("spark_01"), 0.0f, 0.3f, 0.4f, 1.2f, Gold, 1, 90.0f, 0.0f, 0.f, false },
        };
        static const FSprite AzCombo[] = {
            { TEXT("scorch_02"), 0.0f, 1.4f, 0.6f, 1.0f, Scorch, 0, 0.f, 0.0f, 0.f, false },
            { TEXT("circle_02"), 0.0f, 0.4f, 0.2f, 1.2f, Ember, 0, 0.f, 0.0f, 0.f, false },
            { TEXT("star_04"), 0.0f, 0.28f, 0.4f, 1.5f, Fire, 1, 90.0f, 0.0f, 0.f, false },
            { TEXT("spark_06"), 0.0f, 0.3f, 0.4f, 1.2f, Gold, 1, 90.0f, 0.0f, 0.f, false },
        };
        static const FSprite AzCharge[] = {
            { TEXT("magic_05"), 0.0f, 1.0f, 1.3f, 0.35f, Rune, 0, 0.f, 140.0f, 0.f, false },
            { TEXT("magic_01"), 0.15f, 0.85f, 1.0f, 0.25f, Fire, 0, 0.f, -200.0f, 0.f, false },
            { TEXT("twirl_02"), 0.3f, 0.7f, 0.9f, 0.2f, Ember, 0, 0.f, 420.0f, 0.f, false },
            { TEXT("light_01"), 0.0f, 1.0f, 0.15f, 0.9f, Rune, 1, 60.0f, 0.0f, 0.f, false },
            { TEXT("flare_01"), 0.75f, 0.25f, 0.2f, 1.0f, White, 1, 60.0f, 0.0f, 0.f, false },
        };
        static const FSprite AzBurst[] = {
            { TEXT("smoke_07"), 0.0f, 2.4f, 0.5f, 1.1f, Scorch, 0, 0.f, 12.0f, 0.f, false },
            { TEXT("magic_01"), 0.0f, 0.9f, 0.15f, 1.0f, Rune, 0, 0.f, 90.0f, 0.f, false },
            { TEXT("circle_03"), 0.0f, 0.6f, 0.1f, 1.1f, Fire, 0, 0.f, 0.0f, 0.f, false },
            { TEXT("circle_03"), 0.2f, 0.6f, 0.1f, 1.1f, Ember, 0, 0.f, 0.0f, 0.f, false },
            { TEXT("fire_01"), 0.0f, 0.7f, 0.3f, 0.8f, Fire, 1, 220.0f, 0.0f, 0.f, false },
            { TEXT("flare_01"), 0.0f, 0.3f, 0.2f, 1.0f, White, 1, 200.0f, 0.0f, 0.f, false },
            { TEXT("star_07"), 0.05f, 0.45f, 0.2f, 0.8f, Gold, 1, 200.0f, 90.0f, 0.f, false },
        };
        static const FSprite AzFissure[] = {
            { TEXT("scorch_01"), 0.0f, 1.5f, 0.8f, 1.2f, Scorch, 0, 0.f, 0.0f, 0.f, false },
            { TEXT("flame_05"), 0.0f, 0.55f, 0.7f, 1.6f, Fire, 1, 90.0f, 0.0f, 0.f, false },
            { TEXT("spark_01"), 0.05f, 0.3f, 0.5f, 1.2f, Gold, 1, 110.0f, 0.0f, 0.f, false },
        };
        static const FSprite AzLaunch[] = {
            { TEXT("muzzle_02"), 0.0f, 0.25f, 0.5f, 1.6f, Fire, 1, 0.0f, 0.0f, 0.f, false },
            { TEXT("flare_01"), 0.0f, 0.2f, 0.4f, 1.4f, White, 1, 0.0f, 0.0f, 0.f, false },
            { TEXT("spark_06"), 0.0f, 0.35f, 0.4f, 1.3f, Gold, 1, 0.0f, 0.0f, 0.f, false },
        };
        static const FSprite AzMeteor[] = {
            { TEXT("scorch_03"), 0.0f, 3.0f, 0.8f, 1.2f, Scorch, 0, 0.f, 0.0f, 0.f, false },
            { TEXT("circle_05"), 0.0f, 0.5f, 0.2f, 1.3f, Fire, 0, 0.f, 0.0f, 0.f, false },
            { TEXT("fire_02"), 0.0f, 0.7f, 0.6f, 1.5f, Fire, 1, 120.0f, 0.0f, 0.f, false },
            { TEXT("smoke_01"), 0.2f, 1.6f, 0.6f, 1.6f, Scorch, 1, 220.0f, 0.0f, 0.f, false },
            { TEXT("flare_01"), 0.0f, 0.2f, 0.4f, 1.6f, White, 1, 80.0f, 0.0f, 0.f, false },
        };
        static const FSprite AzPillar[] = {
            { TEXT("scorch_02"), 0.0f, 1.8f, 0.7f, 1.1f, Scorch, 0, 0.f, 0.0f, 0.f, false },
            { TEXT("circle_02"), 0.0f, 0.5f, 0.2f, 1.3f, Fire, 0, 0.f, 0.0f, 0.f, false },
            { TEXT("flame_06"), 0.0f, 0.8f, 1.0f, 2.6f, Fire, 1, 220.0f, 0.0f, 0.f, false },
            { TEXT("flame_02"), 0.1f, 0.7f, 0.8f, 2.0f, Ember, 1, 160.0f, 0.0f, 0.f, false },
            { TEXT("spark_01"), 0.0f, 0.4f, 0.5f, 1.5f, Gold, 1, 260.0f, 0.0f, 0.f, false },
        };
        static const FSprite AzShed[] = {
            { TEXT("smoke_07"), 0.0f, 2.0f, 0.5f, 1.1f, Scorch, 0, 0.f, -14.0f, 0.f, false },
            { TEXT("dirt_02"), 0.0f, 0.8f, 0.3f, 1.2f, Earth, 0, 0.f, 50.0f, 0.f, false },
            { TEXT("circle_04"), 0.0f, 0.5f, 0.15f, 1.1f, Ember, 0, 0.f, 0.0f, 0.f, false },
            { TEXT("spark_06"), 0.0f, 0.45f, 0.3f, 1.2f, Fire, 1, 200.0f, 0.0f, 0.f, false },
            { TEXT("star_07"), 0.0f, 0.4f, 0.2f, 0.9f, Gold, 1, 200.0f, -120.0f, 0.f, false },
        };
        static const FSprite AzUltimate[] = {
            { TEXT("scorch_03"), 0.0f, 3.5f, 0.4f, 0.9f, Scorch, 0, 0.f, 0.0f, 0.f, false },
            { TEXT("magic_03"), 0.0f, 1.4f, 0.1f, 1.0f, Rune, 0, 0.f, 60.0f, 0.f, false },
            { TEXT("circle_03"), 0.0f, 0.7f, 0.05f, 1.1f, Fire, 0, 0.f, 0.0f, 0.f, false },
            { TEXT("circle_03"), 0.2f, 0.7f, 0.05f, 1.1f, Ember, 0, 0.f, 0.0f, 0.f, false },
            { TEXT("circle_03"), 0.4f, 0.7f, 0.05f, 1.1f, Gold, 0, 0.f, 0.0f, 0.f, false },
            { TEXT("fire_01"), 0.0f, 1.0f, 0.2f, 0.6f, Fire, 1, 400.0f, 0.0f, 0.f, false },
            { TEXT("flare_01"), 0.0f, 0.4f, 0.1f, 0.8f, White, 1, 300.0f, 0.0f, 0.f, false },
            { TEXT("star_02"), 0.05f, 0.5f, 0.1f, 0.5f, Gold, 1, 300.0f, 60.0f, 0.f, false },
        };
        static const FSprite AzWarn[] = {
            { TEXT("circle_04"), 0.0f, 1.0f, 1.0f, 1.0f, Ember, 0, 0.f, 0.0f, 0.f, false },
            { TEXT("magic_02"), 0.0f, 1.0f, 0.95f, 0.7f, Fire, 0, 0.f, 70.0f, 0.f, false },
            { TEXT("circle_02"), 0.6f, 0.4f, 0.2f, 1.0f, Gold, 0, 0.f, 0.0f, 0.f, false },
        };
        static const FSprite AzWarnRing[] = {
            { TEXT("circle_04"), 0.0f, 1.0f, 1.0f, 1.0f, Fire, 0, 0.f, 0.0f, 0.f, false },
            { TEXT("circle_04"), 0.0f, 1.0f, 0.46f, 0.46f, Ember, 0, 0.f, 0.0f, 0.f, false },
            { TEXT("magic_04"), 0.0f, 1.0f, 0.98f, 0.98f, Ember, 0, 0.f, -50.0f, 0.f, false },
        };
        static const FLinearColor Steel(2.2f, 2.6f, 3.2f), Arcane(2.2f, .8f, 4.f), Cyan(.3f, 1.6f, 4.f);
        // 14/11: as armas iniciais, golpe a golpe (modo 2 = de pé na frente do herói, avançando), e o corte de longo alcance
        static const FSprite SwSword1[] = {
            { TEXT("slash_01"), 0.0f, 0.22f, 0.8f, 1.5f, Steel, 2, 20.0f, 0.0f, 0.35f, false },
            { TEXT("spark_04"), 0.04f, 0.22f, 0.3f, 0.8f, White, 1, 30.0f, 0.0f, 0.f, false },
        };
        static const FSprite SwSword2[] = {
            { TEXT("slash_02"), 0.0f, 0.22f, 0.8f, 1.5f, Steel, 2, 35.0f, 0.0f, 0.35f, false },
            { TEXT("spark_04"), 0.04f, 0.22f, 0.3f, 0.8f, White, 1, 20.0f, 0.0f, 0.f, false },
        };
        static const FSprite SwSword3[] = {
            { TEXT("slash_04"), 0.0f, 0.28f, 1.0f, 2.0f, Steel, 2, 25.0f, 0.0f, 0.45f, false },
            { TEXT("slash_01"), 0.05f, 0.25f, 0.8f, 1.7f, White, 2, 25.0f, 0.0f, 0.5f, false },
            { TEXT("star_04"), 0.03f, 0.22f, 0.4f, 1.3f, White, 1, 30.0f, 0.0f, 0.f, false },
            { TEXT("spark_06"), 0.03f, 0.3f, 0.4f, 1.2f, Steel, 1, 30.0f, 0.0f, 0.f, false },
        };
        static const FSprite SwDagger1[] = {
            { TEXT("scratch_01"), 0.0f, 0.16f, 0.8f, 1.5f, Teal, 2, 20.0f, 0.0f, 0.3f, false },
            { TEXT("slash_03"), 0.02f, 0.16f, 0.7f, 1.4f, White, 2, 30.0f, 0.0f, 0.3f, false },
        };
        static const FSprite SwDagger2[] = {
            { TEXT("scratch_01"), 0.0f, 0.16f, 0.8f, 1.5f, White, 2, 35.0f, 500.0f, 0.3f, false },
            { TEXT("slash_03"), 0.02f, 0.16f, 0.7f, 1.4f, Teal, 2, 15.0f, 0.0f, 0.3f, false },
        };
        static const FSprite SwDagger3[] = {
            { TEXT("scratch_01"), 0.0f, 0.16f, 0.9f, 1.6f, Teal, 2, 25.0f, 0.0f, 0.3f, false },
            { TEXT("scratch_01"), 0.06f, 0.16f, 0.9f, 1.7f, White, 2, 35.0f, 600.0f, 0.4f, false },
            { TEXT("slash_03"), 0.12f, 0.2f, 1.0f, 2.0f, Teal, 2, 20.0f, 0.0f, 0.5f, false },
            { TEXT("star_04"), 0.12f, 0.2f, 0.4f, 1.3f, White, 1, 25.0f, 0.0f, 0.f, false },
            { TEXT("spark_06"), 0.1f, 0.28f, 0.4f, 1.2f, Teal, 1, 25.0f, 0.0f, 0.f, false },
        };
        static const FSprite SwHammer1[] = {
            { TEXT("slash_02"), 0.0f, 0.3f, 1.0f, 1.8f, Gold, 2, 30.0f, 0.0f, 0.3f, false },
            { TEXT("dirt_02"), 0.06f, 0.4f, 0.4f, 1.1f, Earth, 1, -40.0f, 0.0f, 0.f, false },
        };
        static const FSprite SwHammer2[] = {
            { TEXT("slash_04"), 0.0f, 0.3f, 1.0f, 1.8f, Gold, 2, 20.0f, 0.0f, 0.3f, false },
            { TEXT("dirt_02"), 0.06f, 0.4f, 0.4f, 1.1f, Earth, 1, -40.0f, 0.0f, 0.f, false },
        };
        static const FSprite SwHammerSlam[] = {
            { TEXT("scorch_02"), 0.0f, 1.8f, 0.7f, 1.1f, Scorch, 0, 0.f, 0.0f, 0.f, false },
            { TEXT("circle_05"), 0.0f, 0.45f, 0.15f, 1.2f, Gold, 0, 0.f, 0.0f, 0.f, false },
            { TEXT("dirt_02"), 0.0f, 0.7f, 0.4f, 1.3f, Earth, 0, 0.f, 60.0f, 0.f, false },
            { TEXT("star_02"), 0.0f, 0.3f, 0.4f, 1.5f, Gold, 1, 60.0f, 0.0f, 0.f, false },
            { TEXT("flare_01"), 0.0f, 0.18f, 0.4f, 1.4f, White, 1, 60.0f, 0.0f, 0.f, false },
        };
        static const FSprite SwBow1[] = {
            { TEXT("circle_02"), 0.0f, 0.2f, 0.3f, 1.3f, White, 2, 40.0f, 0.0f, 1.2f, false },
            { TEXT("spark_04"), 0.0f, 0.18f, 0.4f, 1.1f, White, 1, 40.0f, 0.0f, 0.f, false },
        };
        static const FSprite SwBow3[] = {
            { TEXT("circle_02"), 0.0f, 0.25f, 0.3f, 1.8f, Gold, 2, 40.0f, 0.0f, 1.6f, false },
            { TEXT("circle_02"), 0.05f, 0.25f, 0.3f, 1.4f, White, 2, 40.0f, 0.0f, 2.4f, false },
            { TEXT("star_02"), 0.0f, 0.22f, 0.5f, 1.8f, Gold, 1, 40.0f, 0.0f, 0.f, false },
            { TEXT("flare_01"), 0.0f, 0.15f, 0.5f, 1.6f, White, 1, 40.0f, 0.0f, 0.f, false },
        };
        static const FSprite SwStaff1[] = {
            { TEXT("magic_02"), 0.0f, 0.35f, 0.5f, 1.3f, Arcane, 2, 40.0f, 220.0f, 0.2f, false },
            { TEXT("flare_01"), 0.02f, 0.18f, 0.4f, 1.2f, White, 1, 40.0f, 0.0f, 0.f, false },
        };
        static const FSprite SwStaff3[] = {
            { TEXT("magic_05"), 0.0f, 0.5f, 0.6f, 2.0f, Arcane, 2, 40.0f, -160.0f, 0.2f, false },
            { TEXT("magic_02"), 0.06f, 0.4f, 0.5f, 1.5f, White, 2, 40.0f, 260.0f, 0.3f, false },
            { TEXT("light_01"), 0.0f, 0.4f, 0.5f, 2.0f, Arcane, 1, 40.0f, 0.0f, 0.f, false },
            { TEXT("star_07"), 0.05f, 0.3f, 0.4f, 1.4f, White, 1, 40.0f, 120.0f, 0.f, false },
        };
        static const FSprite WaveTrail[] = {
            { TEXT("trace_01"), 0.0f, 0.55f, 0.9f, 1.2f, Cyan, 0, 0.f, 0.0f, 0.f, false },
            { TEXT("dirt_02"), 0.0f, 0.5f, 0.4f, 1.0f, Earth, 0, 0.f, 80.0f, 0.f, false },
            { TEXT("spark_04"), 0.0f, 0.3f, 0.5f, 1.2f, White, 1, 40.0f, 0.0f, 0.f, false },
        };
        static const FSprite WaveHit[] = {
            { TEXT("circle_02"), 0.0f, 0.4f, 0.3f, 1.5f, Cyan, 0, 0.f, 0.0f, 0.f, false },
            { TEXT("star_04"), 0.0f, 0.25f, 0.6f, 2.2f, White, 1, 40.0f, 0.0f, 0.f, false },
            { TEXT("flare_01"), 0.0f, 0.18f, 0.6f, 2.4f, White, 1, 40.0f, 0.0f, 0.f, false },
            { TEXT("spark_06"), 0.0f, 0.35f, 0.6f, 2.0f, Cyan, 1, 40.0f, 0.0f, 0.f, false },
            { TEXT("slash_03"), 0.0f, 0.3f, 1.0f, 2.6f, Cyan, 2, 40.0f, 300.0f, 0.0f, false },
        };
        // 14/11: os poderes das espadas e dos arcos, um por tipo (bTint = a cor do elemento da arma, que vem em Custom)
        static const FSprite Pwdash[] = {
            { TEXT("trace_01"), 0.0f, 0.5f, 0.8f, 2.2f, White, 0, 0.0f, 0.0f, 0.0f, true },
            { TEXT("slash_04"), 0.0f, 0.3f, 0.8f, 1.6f, White, 2, 30.0f, 0.0f, 2.2f, true },
            { TEXT("spark_06"), 0.05f, 0.3f, 0.3f, 1.0f, White, 1, 30.0f, 0.0f, 0.0f, false },
        };
        static const FSprite Pwgroundslam[] = {
            { TEXT("scorch_02"), 0.25f, 1.8f, 0.6f, 1.1f, Scorch, 0, 0.0f, 0.0f, 0.0f, false },
            { TEXT("circle_05"), 0.25f, 0.5f, 0.1f, 1.2f, White, 0, 0.0f, 0.0f, 0.0f, true },
            { TEXT("dirt_02"), 0.25f, 0.7f, 0.4f, 1.3f, Earth, 0, 0.0f, 60.0f, 0.0f, false },
            { TEXT("star_02"), 0.25f, 0.3f, 0.3f, 1.3f, White, 1, 50.0f, 0.0f, 0.0f, false },
            { TEXT("magic_02"), 0.0f, 0.35f, 0.9f, 0.4f, White, 0, 0.0f, 200.0f, 0.0f, true },
        };
        static const FSprite Pwruneseal[] = {
            { TEXT("magic_05"), 0.0f, 1.5f, 0.3f, 1.1f, White, 0, 0.0f, 90.0f, 0.0f, true },
            { TEXT("magic_01"), 0.1f, 1.3f, 0.2f, 0.8f, White, 0, 0.0f, -140.0f, 0.0f, false },
            { TEXT("circle_04"), 0.2f, 0.8f, 0.2f, 1.2f, White, 0, 0.0f, 0.0f, 0.0f, true },
            { TEXT("light_01"), 0.2f, 0.8f, 0.3f, 1.0f, White, 1, 100.0f, 0.0f, 0.0f, true },
        };
        static const FSprite Pwshockwave[] = {
            { TEXT("circle_02"), 0.2f, 0.5f, 0.3f, 1.4f, White, 2, 30.0f, 0.0f, 2.5f, true },
            { TEXT("circle_02"), 0.3f, 0.5f, 0.3f, 1.2f, White, 2, 30.0f, 0.0f, 3.2f, false },
            { TEXT("circle_02"), 0.4f, 0.5f, 0.3f, 1.0f, White, 2, 30.0f, 0.0f, 4.0f, true },
            { TEXT("trace_01"), 0.2f, 0.6f, 0.8f, 2.0f, White, 0, 0.0f, 0.0f, 0.0f, true },
        };
        static const FSprite Pwwhirl[] = {
            { TEXT("twirl_01"), 0.0f, 0.9f, 0.5f, 1.2f, White, 0, 0.0f, 720.0f, 0.0f, true },
            { TEXT("twirl_03"), 0.05f, 0.8f, 0.4f, 1.0f, White, 0, 0.0f, -520.0f, 0.0f, false },
            { TEXT("circle_03"), 0.15f, 0.5f, 0.2f, 1.2f, White, 0, 0.0f, 0.0f, 0.0f, true },
            { TEXT("spark_04"), 0.1f, 0.4f, 0.4f, 1.2f, White, 1, 20.0f, 0.0f, 0.0f, false },
        };
        static const FSprite Pworbit[] = {
            { TEXT("magic_02"), 0.0f, 1.3f, 0.4f, 1.0f, White, 0, 0.0f, 220.0f, 0.0f, true },
            { TEXT("circle_01"), 0.0f, 0.6f, 0.2f, 1.1f, White, 0, 0.0f, 0.0f, 0.0f, false },
            { TEXT("star_07"), 0.1f, 0.7f, 0.3f, 0.9f, White, 1, 70.0f, 300.0f, 0.0f, true },
            { TEXT("light_02"), 0.0f, 0.6f, 0.3f, 1.0f, White, 1, 60.0f, 0.0f, 0.0f, true },
        };
        static const FSprite Pwflurry[] = {
            { TEXT("scratch_01"), 0.0f, 0.18f, 0.7f, 1.3f, White, 2, 30.0f, 0.0f, 0.4f, true },
            { TEXT("scratch_01"), 0.1f, 0.18f, 0.7f, 1.4f, White, 2, 20.0f, 500.0f, 0.5f, false },
            { TEXT("scratch_01"), 0.2f, 0.18f, 0.7f, 1.5f, White, 2, 40.0f, -400.0f, 0.6f, true },
            { TEXT("slash_03"), 0.3f, 0.22f, 0.8f, 1.8f, White, 2, 30.0f, 0.0f, 0.8f, false },
            { TEXT("spark_04"), 0.15f, 0.3f, 0.3f, 1.0f, White, 1, 30.0f, 0.0f, 0.0f, false },
        };
        static const FSprite Pwsweep[] = {
            { TEXT("slash_02"), 0.1f, 0.4f, 0.6f, 1.7f, White, 0, 0.0f, 0.0f, 0.0f, true },
            { TEXT("slash_04"), 0.15f, 0.4f, 0.5f, 1.5f, White, 0, 0.0f, 120.0f, 0.0f, false },
            { TEXT("circle_03"), 0.15f, 0.5f, 0.2f, 1.2f, White, 0, 0.0f, 0.0f, 0.0f, true },
            { TEXT("spark_06"), 0.15f, 0.3f, 0.3f, 1.1f, White, 1, 30.0f, 0.0f, 0.0f, false },
        };
        static const FSprite Pwreap[] = {
            { TEXT("slash_04"), 0.1f, 0.5f, 0.7f, 1.8f, White, 0, 0.0f, 360.0f, 0.0f, true },
            { TEXT("slash_01"), 0.15f, 0.3f, 0.8f, 1.8f, White, 2, 30.0f, 0.0f, 0.6f, true },
            { TEXT("star_04"), 0.2f, 0.25f, 0.3f, 1.2f, White, 1, 30.0f, 0.0f, 0.0f, false },
            { TEXT("circle_02"), 0.2f, 0.4f, 0.2f, 1.3f, White, 0, 0.0f, 0.0f, 0.0f, true },
        };
        static const FSprite Pwoverload[] = {
            { TEXT("magic_03"), 0.0f, 0.9f, 0.3f, 1.1f, White, 0, 0.0f, 260.0f, 0.0f, true },
            { TEXT("light_01"), 0.0f, 0.6f, 0.2f, 1.4f, White, 1, 50.0f, 0.0f, 0.0f, true },
            { TEXT("spark_01"), 0.05f, 0.4f, 0.4f, 1.4f, White, 1, 60.0f, 0.0f, 0.0f, false },
            { TEXT("star_07"), 0.1f, 0.4f, 0.3f, 1.1f, White, 1, 60.0f, 200.0f, 0.0f, true },
            { TEXT("circle_04"), 0.1f, 0.5f, 0.2f, 1.3f, White, 0, 0.0f, 0.0f, 0.0f, true },
        };
        static const FSprite Pwholyslam[] = {
            { TEXT("magic_01"), 0.0f, 1.2f, 0.3f, 1.0f, White, 0, 0.0f, 60.0f, 0.0f, true },
            { TEXT("circle_04"), 0.3f, 0.5f, 0.15f, 1.3f, White, 0, 0.0f, 0.0f, 0.0f, false },
            { TEXT("flare_01"), 0.3f, 0.25f, 0.4f, 1.8f, White, 1, 120.0f, 0.0f, 0.0f, false },
            { TEXT("light_02"), 0.0f, 0.9f, 0.3f, 1.2f, White, 1, 160.0f, 0.0f, 0.0f, true },
            { TEXT("scorch_01"), 0.3f, 1.4f, 0.5f, 0.9f, Scorch, 0, 0.0f, 0.0f, 0.0f, false },
        };
        static const FSprite Pwtidewave[] = {
            { TEXT("circle_01"), 0.1f, 0.7f, 0.4f, 1.6f, White, 2, 20.0f, 0.0f, 3.0f, true },
            { TEXT("circle_01"), 0.25f, 0.7f, 0.4f, 1.3f, White, 2, 20.0f, 0.0f, 3.6f, false },
            { TEXT("twirl_03"), 0.1f, 0.8f, 0.5f, 1.3f, White, 0, 0.0f, 160.0f, 0.0f, true },
            { TEXT("spark_04"), 0.15f, 0.4f, 0.4f, 1.2f, White, 1, 40.0f, 0.0f, 0.0f, false },
        };
        static const FSprite Pwfirewall[] = {
            { TEXT("scorch_01"), 0.1f, 1.8f, 0.6f, 1.1f, Scorch, 0, 0.0f, 0.0f, 0.0f, false },
            { TEXT("circle_02"), 0.1f, 0.5f, 0.2f, 1.2f, White, 0, 0.0f, 0.0f, 0.0f, true },
            { TEXT("flame_06"), 0.1f, 0.9f, 0.8f, 1.8f, White, 1, 130.0f, 0.0f, 0.0f, true },
            { TEXT("flame_02"), 0.2f, 0.8f, 0.6f, 1.4f, White, 1, 100.0f, 0.0f, 0.0f, false },
            { TEXT("spark_01"), 0.1f, 0.4f, 0.4f, 1.3f, White, 1, 160.0f, 0.0f, 0.0f, false },
        };
        static const FSprite Pwheal[] = {
            { TEXT("magic_04"), 0.0f, 1.3f, 0.4f, 1.0f, White, 0, 0.0f, 60.0f, 0.0f, true },
            { TEXT("circle_01"), 0.0f, 0.7f, 0.2f, 1.1f, White, 0, 0.0f, 0.0f, 0.0f, false },
            { TEXT("light_02"), 0.0f, 1.0f, 0.3f, 1.1f, White, 1, 60.0f, 0.0f, 0.0f, true },
            { TEXT("star_07"), 0.2f, 0.6f, 0.2f, 0.8f, White, 1, 130.0f, 180.0f, 0.0f, false },
        };
        static const FSprite Pwvolley[] = {
            { TEXT("magic_02"), 0.0f, 0.4f, 0.4f, 1.3f, White, 2, 40.0f, 220.0f, 0.4f, true },
            { TEXT("circle_02"), 0.05f, 0.3f, 0.3f, 1.5f, White, 2, 40.0f, 0.0f, 1.6f, false },
            { TEXT("circle_02"), 0.12f, 0.3f, 0.3f, 1.3f, White, 2, 40.0f, 0.0f, 2.2f, true },
            { TEXT("spark_04"), 0.0f, 0.25f, 0.4f, 1.1f, White, 1, 40.0f, 0.0f, 0.0f, false },
        };
        static const FSprite Pwpierce[] = {
            { TEXT("star_04"), 0.0f, 0.3f, 0.4f, 1.6f, White, 2, 40.0f, 0.0f, 2.5f, true },
            { TEXT("circle_02"), 0.0f, 0.25f, 0.3f, 1.2f, White, 2, 40.0f, 0.0f, 1.2f, false },
            { TEXT("trace_01"), 0.0f, 0.5f, 0.8f, 2.4f, White, 0, 0.0f, 0.0f, 0.0f, true },
            { TEXT("flare_01"), 0.0f, 0.15f, 0.4f, 1.4f, White, 1, 40.0f, 0.0f, 0.0f, false },
        };
        static const FSprite Pwburst3[] = {
            { TEXT("circle_02"), 0.0f, 0.22f, 0.3f, 1.2f, White, 2, 40.0f, 0.0f, 1.4f, true },
            { TEXT("circle_02"), 0.15f, 0.22f, 0.3f, 1.2f, White, 2, 40.0f, 0.0f, 1.4f, false },
            { TEXT("circle_02"), 0.3f, 0.22f, 0.3f, 1.2f, White, 2, 40.0f, 0.0f, 1.4f, true },
            { TEXT("spark_04"), 0.0f, 0.5f, 0.3f, 1.0f, White, 1, 40.0f, 0.0f, 0.0f, false },
        };
        static const FSprite Pwarrowrain[] = {
            { TEXT("magic_05"), 0.0f, 1.2f, 0.4f, 1.1f, White, 0, 0.0f, 80.0f, 0.0f, true },
            { TEXT("circle_04"), 0.1f, 0.6f, 0.2f, 1.2f, White, 0, 0.0f, 0.0f, 0.0f, false },
            { TEXT("light_01"), 0.0f, 0.8f, 0.4f, 1.4f, White, 1, 260.0f, 0.0f, 0.0f, true },
            { TEXT("star_02"), 0.1f, 0.4f, 0.3f, 1.1f, White, 1, 260.0f, 0.0f, 0.0f, false },
        };
        static const FSprite Pwexplosive[] = {
            { TEXT("circle_04"), 0.0f, 0.3f, 0.3f, 1.5f, White, 2, 40.0f, 0.0f, 1.2f, true },
            { TEXT("fire_01"), 0.0f, 0.5f, 0.4f, 1.2f, White, 1, 60.0f, 0.0f, 0.0f, true },
            { TEXT("spark_06"), 0.0f, 0.35f, 0.4f, 1.3f, White, 1, 50.0f, 0.0f, 0.0f, false },
            { TEXT("flare_01"), 0.0f, 0.15f, 0.4f, 1.5f, White, 1, 40.0f, 0.0f, 0.0f, false },
        };
        static const FSprite Pwfireball[] = {
            { TEXT("magic_01"), 0.0f, 0.5f, 0.4f, 1.4f, White, 2, 40.0f, 240.0f, 0.3f, true },
            { TEXT("fire_02"), 0.0f, 0.5f, 0.4f, 1.3f, White, 1, 60.0f, 0.0f, 0.0f, true },
            { TEXT("flare_01"), 0.0f, 0.18f, 0.4f, 1.6f, White, 1, 40.0f, 0.0f, 0.0f, false },
            { TEXT("spark_01"), 0.05f, 0.3f, 0.4f, 1.2f, White, 1, 60.0f, 0.0f, 0.0f, false },
        };
        static const FSprite PwCharge[] = {
            { TEXT("magic_02"), 0.0f, 0.7f, 1.0f, 0.4f, White, 0, 0.0f, 300.0f, 0.0f, true },
            { TEXT("light_01"), 0.0f, 0.7f, 0.2f, 0.9f, White, 1, 40.0f, 0.0f, 0.0f, true },
            { TEXT("spark_04"), 0.2f, 0.4f, 0.3f, 0.9f, White, 1, 50.0f, 0.0f, 0.0f, false },
        };
        static const FSprite PwBasic[] = {
            { TEXT("slash_02"), 0.0f, 0.25f, 0.8f, 1.6f, White, 2, 30.0f, 0.0f, 0.6f, true },
            { TEXT("spark_04"), 0.03f, 0.25f, 0.3f, 0.9f, White, 1, 30.0f, 0.0f, 0.0f, false },
        };
        static const TMap<FName, TArrayView<const FSprite>> All = {
            { TEXT("Pwdash"), Pwdash }, { TEXT("Pwgroundslam"), Pwgroundslam }, { TEXT("Pwruneseal"), Pwruneseal }, { TEXT("Pwshockwave"), Pwshockwave }, { TEXT("Pwwhirl"), Pwwhirl }, { TEXT("Pworbit"), Pworbit }, { TEXT("Pwflurry"), Pwflurry }, { TEXT("Pwsweep"), Pwsweep }, { TEXT("Pwreap"), Pwreap }, { TEXT("Pwoverload"), Pwoverload }, { TEXT("Pwholyslam"), Pwholyslam }, { TEXT("Pwtidewave"), Pwtidewave }, { TEXT("Pwfirewall"), Pwfirewall }, { TEXT("Pwheal"), Pwheal }, { TEXT("Pwvolley"), Pwvolley }, { TEXT("Pwpierce"), Pwpierce }, { TEXT("Pwburst3"), Pwburst3 }, { TEXT("Pwarrowrain"), Pwarrowrain }, { TEXT("Pwexplosive"), Pwexplosive }, { TEXT("Pwfireball"), Pwfireball }, { TEXT("PwCharge"), PwCharge }, { TEXT("PwBasic"), PwBasic },
            { TEXT("SwSword1"), SwSword1 }, { TEXT("SwSword2"), SwSword2 }, { TEXT("SwSword3"), SwSword3 }, { TEXT("SwDagger1"), SwDagger1 }, { TEXT("SwDagger2"), SwDagger2 }, { TEXT("SwDagger3"), SwDagger3 }, { TEXT("SwHammer1"), SwHammer1 }, { TEXT("SwHammer2"), SwHammer2 }, { TEXT("SwHammerSlam"), SwHammerSlam }, { TEXT("SwBow1"), SwBow1 }, { TEXT("SwBow3"), SwBow3 }, { TEXT("SwStaff1"), SwStaff1 }, { TEXT("SwStaff3"), SwStaff3 }, { TEXT("WaveTrail"), WaveTrail }, { TEXT("WaveHit"), WaveHit },
            { TEXT("AzBeamHit"), AzBeamHit }, { TEXT("AzHook"), AzHook }, { TEXT("AzHammer"), AzHammer }, { TEXT("AzStomp"), AzStomp }, { TEXT("AzSweep"), AzSweep }, { TEXT("AzCombo"), AzCombo }, { TEXT("AzCharge"), AzCharge }, { TEXT("AzBurst"), AzBurst }, { TEXT("AzFissure"), AzFissure }, { TEXT("AzLaunch"), AzLaunch }, { TEXT("AzMeteor"), AzMeteor }, { TEXT("AzPillar"), AzPillar }, { TEXT("AzShed"), AzShed }, { TEXT("AzUltimate"), AzUltimate }, { TEXT("AzWarn"), AzWarn }, { TEXT("AzWarnRing"), AzWarnRing },
            { TEXT("Howl"), Howl }, { TEXT("Bite"), Bite }, { TEXT("Headbutt"), Headbutt }, { TEXT("Sonar"), Sonar }, { TEXT("Swoop"), Swoop }, { TEXT("Spikes"), Spikes },
            { TEXT("Quake"), Quake }, { TEXT("Hurl"), Hurl }, { TEXT("GelPool"), Gel }, { TEXT("Splash"), Splash }, { TEXT("PoisonPool"), Poison }, { TEXT("PoisonCloud"), Poison },
            { TEXT("Dust"), Dust }, { TEXT("Tornado"), Tornado }, { TEXT("Rubble"), Rubble },
        };
        const TArrayView<const FSprite>* Found = All.Find(Kind);
        return Found ? *Found : TArrayView<const FSprite>();
    }

    /** 14/11: nestes a duração vem de quem chama (Spawn com InDelay negativo = a duração em s): os instantes e as durações
     *  das imagens são frações dela (a carga do núcleo dura o tempo do golpe; o aviso, o tempo do aviso). */
    bool IsTimed(FName Kind) { return Kind == TEXT("AzCharge") || Kind == TEXT("AzWarn") || Kind == TEXT("AzWarnRing"); }

    /** Os efeitos de 13/11 são só de cubos e cones: sem o círculo no chão nem as nuvens dos antigos. */
    bool IsPlain(FName Kind)
    {
        static const TSet<FName> Plain = { TEXT("Pwdash"), TEXT("Pwgroundslam"), TEXT("Pwruneseal"), TEXT("Pwshockwave"), TEXT("Pwwhirl"), TEXT("Pworbit"), TEXT("Pwflurry"), TEXT("Pwsweep"), TEXT("Pwreap"), TEXT("Pwoverload"), TEXT("Pwholyslam"), TEXT("Pwtidewave"), TEXT("Pwfirewall"), TEXT("Pwheal"), TEXT("Pwvolley"), TEXT("Pwpierce"), TEXT("Pwburst3"), TEXT("Pwarrowrain"), TEXT("Pwexplosive"), TEXT("Pwfireball"), TEXT("PwCharge"), TEXT("PwBasic"), TEXT("SwSword1"), TEXT("SwSword2"), TEXT("SwSword3"), TEXT("SwDagger1"), TEXT("SwDagger2"), TEXT("SwDagger3"), TEXT("SwHammer1"), TEXT("SwHammer2"), TEXT("SwHammerSlam"), TEXT("SwBow1"), TEXT("SwBow3"), TEXT("SwStaff1"), TEXT("SwStaff3"), TEXT("WaveTrail"), TEXT("WaveHit"), TEXT("AzBeamHit"), TEXT("AzHook"), TEXT("AzHammer"), TEXT("AzStomp"), TEXT("AzSweep"), TEXT("AzCombo"), TEXT("AzCharge"), TEXT("AzBurst"), TEXT("AzFissure"), TEXT("AzLaunch"), TEXT("AzMeteor"), TEXT("AzPillar"), TEXT("AzShed"), TEXT("AzUltimate"), TEXT("AzWarn"), TEXT("AzWarnRing"), TEXT("Howl"), TEXT("Bite"), TEXT("Headbutt"), TEXT("GelPool"), TEXT("GelDrip"), TEXT("Swoop"), TEXT("Sonar"),
            TEXT("Spikes"), TEXT("Quake"), TEXT("Rubble"), TEXT("Hurl") };
        return Plain.Contains(Kind);
    }

    /** 14/11 (o usuário: "use o monolith, pode refazer de todas as espadas e arcos"): os poderes ("Pw<tipo>") têm um efeito
     *  Niagara próprio, montado pelo Monolith (Tools/Fx/BuildPowerFx.py), em /Game/Vortex/Fx/Poderes/NS_<tipo>. Quando ele
     *  existe, entra no lugar das camadas e das imagens antigas do tipo; sem ele, valem as antigas. */
    UNiagaraSystem* PowerSystemOf(FName Kind)
    {
        static TMap<FName, TStrongObjectPtr<UNiagaraSystem>> Found;
        if (const TStrongObjectPtr<UNiagaraSystem>* Known = Found.Find(Kind)) return Known->Get();
        const FString Name = Kind.ToString();
        UNiagaraSystem* System = Name.StartsWith(TEXT("Pw")) ? LoadObject<UNiagaraSystem>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Fx/Poderes/NS_%s.NS_%s"), *Name, *Name), nullptr, LOAD_NoWarn) : nullptr;
        Found.Add(Kind, TStrongObjectPtr<UNiagaraSystem>(System));
        return System;
    }
}

// 13/11 (o usuário: "trava muito na primeira vez que usam os ataques"): tudo o que os efeitos dos monstros usam, para o
// AVortexFxWarmup (VortexHitFx.cpp) soltar uma vez na tela de carregamento. Ficam carregados a partida toda
namespace VortexMonsterFxWarm
{
    void Collect(TArray<UNiagaraSystem*>& Systems, TArray<UTexture2D*>& Sprites)
    {
        static TArray<TStrongObjectPtr<UObject>> Keep;
        static const TCHAR* Kinds[] = { TEXT("Pwdash"), TEXT("Pwgroundslam"), TEXT("Pwruneseal"), TEXT("Pwshockwave"), TEXT("Pwwhirl"), TEXT("Pworbit"), TEXT("Pwflurry"), TEXT("Pwsweep"), TEXT("Pwreap"), TEXT("Pwoverload"), TEXT("Pwholyslam"), TEXT("Pwtidewave"), TEXT("Pwfirewall"), TEXT("Pwheal"), TEXT("Pwvolley"), TEXT("Pwpierce"), TEXT("Pwburst3"), TEXT("Pwarrowrain"), TEXT("Pwexplosive"), TEXT("Pwfireball"), TEXT("PwCharge"), TEXT("PwBasic"), TEXT("SwSword1"), TEXT("SwSword2"), TEXT("SwSword3"), TEXT("SwDagger1"), TEXT("SwDagger2"), TEXT("SwDagger3"), TEXT("SwHammer1"), TEXT("SwHammer2"), TEXT("SwHammerSlam"), TEXT("SwBow1"), TEXT("SwBow3"), TEXT("SwStaff1"), TEXT("SwStaff3"), TEXT("WaveTrail"), TEXT("WaveHit"), TEXT("AzBeamHit"), TEXT("AzHook"), TEXT("AzHammer"), TEXT("AzStomp"), TEXT("AzSweep"), TEXT("AzCombo"), TEXT("AzCharge"), TEXT("AzBurst"), TEXT("AzFissure"), TEXT("AzLaunch"), TEXT("AzMeteor"), TEXT("AzPillar"), TEXT("AzShed"), TEXT("AzUltimate"), TEXT("AzWarn"), TEXT("AzWarnRing"), TEXT("PoisonCloud"), TEXT("PoisonPool"), TEXT("Drip"), TEXT("Tornado"), TEXT("Dust"), TEXT("Splash"), TEXT("Howl"), TEXT("Bite"),
            TEXT("Headbutt"), TEXT("GelPool"), TEXT("GelDrip"), TEXT("Swoop"), TEXT("Sonar"), TEXT("Spikes"), TEXT("Quake"), TEXT("Rubble"), TEXT("Hurl") };
        for (const TCHAR* Kind : Kinds)
        {
            if (UNiagaraSystem* Power = PowerSystemOf(Kind)) { Systems.AddUnique(Power); continue; }
            for (const FLayer& Layer : LayersOf(Kind))
                if (UNiagaraSystem* System = LoadObject<UNiagaraSystem>(nullptr, Layer.Path)) Systems.AddUnique(System);
            float At, Scale;
            if (const TCHAR* Path = SparkOf(Kind, At, Scale))
                if (UNiagaraSystem* System = LoadObject<UNiagaraSystem>(nullptr, Path)) Systems.AddUnique(System);
            for (const FSprite& S : SpritesOf(Kind))
                if (UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Combat/Sprites/T_Fx_%s.T_Fx_%s"), S.Tex, S.Tex))) Sprites.AddUnique(Tex);
        }
        if (Keep.IsEmpty())
        {
            for (UNiagaraSystem* System : Systems) Keep.Emplace(System);
            for (UTexture2D* Tex : Sprites) Keep.Emplace(Tex);
        }
    }
}

AVortexMonsterFx::AVortexMonsterFx()
{
    PrimaryActorTick.bCanEverTick = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
    SetActorEnableCollision(false);
}

AVortexMonsterFx* AVortexMonsterFx::Spawn(UWorld* World, FName InKind, const FVector& At, float InRadius, float InDelay, AActor* Follow, float Yaw, const FLinearColor& Tint)
{
    if (!World || World->GetNetMode() == NM_DedicatedServer) return nullptr;
    FActorSpawnParameters Params;
    Params.ObjectFlags |= RF_Transient;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AVortexMonsterFx* Fx = World->SpawnActor<AVortexMonsterFx>(At, FRotator(0.f, Yaw, 0.f), Params);
    if (!Fx) return nullptr;
    Fx->Custom = Tint;
    Fx->Kind = InKind;
    Fx->Radius = FMath::Clamp(InRadius, 40.f, 2000.f);   // (o Despertar do Núcleo do Azhûr tem 18 m)
    Fx->Delay = FMath::Max(0.f, InDelay);
    Fx->FollowActor = Follow;
    Fx->Life = InDelay < 0.f && IsTimed(InKind) ? -InDelay : LookOf(InKind).Life;
    Fx->Begin();
    return Fx;
}

void AVortexMonsterFx::Begin()
{
    UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    UMaterialInterface* Solid = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Combat/M_SolidColor.M_SolidColor"));
    // a cor própria (a gota cospe na cor dela): do escuro ao claro
    TArray<FLinearColor> Colors = LookOf(Kind).Colors;
    if (Custom.R >= 0.f && (Kind == TEXT("GelPool") || Kind == TEXT("GelDrip"))) Colors = { Custom * .45f, Custom * .75f, Custom };
    for (const FLinearColor& Color : Colors)
    {
        UInstancedStaticMeshComponent* Shade = NewObject<UInstancedStaticMeshComponent>(this);
        Shade->SetupAttachment(GetRootComponent());
        Shade->SetUsingAbsoluteLocation(true);
        Shade->SetUsingAbsoluteRotation(true);
        Shade->SetStaticMesh(Cube);
        Shade->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Shade->SetCastShadow(false);
        Shade->bAffectDistanceFieldLighting = false;
        if (Solid)
        {
            UMaterialInstanceDynamic* Tint = UMaterialInstanceDynamic::Create(Solid, this);
            Tint->SetVectorParameterValue(TEXT("Color"), Color);
            Shade->SetMaterial(0, Tint);
        }
        Shade->RegisterComponent();
        Shades.Add(Shade);
    }
    // os pedaços de cada efeito
    auto Add = [this](int32 Count, TFunction<void(FBit&, int32)> Make)
    {
        for (int32 i = 0; i < Count; ++i)
        {
            FBit Bit;
            Bit.Shade = FMath::RandRange(0, Shades.Num() - 1);
            Make(Bit, i);
            Bits.Add(Bit);
        }
    };
    if (Kind == TEXT("PoisonCloud"))
        Add(16, [this](FBit& B, int32) { B.Dir = FVector(FMath::VRand().GetSafeNormal2D()); B.A = Radius * FMath::FRandRange(.25f, 1.f);
            B.B = FMath::FRandRange(20.f, 110.f); B.C = FMath::FRandRange(0.f, .35f); B.Size = FMath::FRandRange(22.f, 46.f); });
    else if (Kind == TEXT("PoisonPool") || Kind == TEXT("GelPool"))
    {
        Add(22, [this](FBit& B, int32) { B.Dir = FVector(FMath::VRand().GetSafeNormal2D()); B.A = Radius * FMath::Sqrt(FMath::FRand());
            B.C = -1.f; B.Size = FMath::FRandRange(22.f, 38.f); });
        Add(12, [this](FBit& B, int32) { B.Dir = FVector(FMath::VRand().GetSafeNormal2D()); B.A = Radius * .8f * FMath::FRand();
            B.C = FMath::FRandRange(0.f, .8f); B.Size = FMath::FRandRange(6.f, 12.f); B.Shade = 2; });
    }
    else if (Kind == TEXT("Drip") || Kind == TEXT("GelDrip"))
        Add(1, [](FBit& B, int32) { B.Size = FMath::FRandRange(7.f, 11.f); });
    else if (Kind == TEXT("Tornado"))
    {
        // os pedaços que o vento leva, subindo em espiral (o raio abre com a altura); as pedrinhas giram em volta da base
        Add(34, [](FBit& B, int32 i) { B.A = FMath::FRand(); B.B = FMath::FRandRange(0.f, UE_TWO_PI); B.C = FMath::FRandRange(.85f, 1.25f);
            B.Size = FMath::FRandRange(12.f, 26.f); B.Shade = 1 + (i % 2); });
        Add(22, [](FBit& B, int32) { B.A = -1.f; B.B = FMath::FRandRange(0.f, UE_TWO_PI); B.C = FMath::FRandRange(.8f, 1.2f);
            B.Size = FMath::FRandRange(8.f, 16.f); B.Shade = 0; });
    }
    else if (Kind == TEXT("Dust"))
        Add(18, [this](FBit& B, int32) { B.Dir = FVector(FMath::VRand().GetSafeNormal2D()); B.A = Radius * FMath::FRandRange(.6f, 1.1f);
            B.B = FMath::FRandRange(30.f, 140.f); B.Size = FMath::FRandRange(10.f, 26.f); });
    else if (Kind == TEXT("Splash"))
        Add(26, [this](FBit& B, int32) { B.Dir = FVector(FMath::VRand().GetSafeNormal2D()); B.A = Radius * FMath::FRandRange(.4f, 1.f);
            B.B = FMath::FRandRange(60.f, 180.f); B.Size = FMath::FRandRange(7.f, 14.f); });
    // ---- 13/11: os efeitos novos (um por golpe)
    else if (Kind == TEXT("Howl"))        // três anéis que abrem, um atrás do outro
        Add(0, [](FBit&, int32) {});      // 13/11, noite: as ondas são partículas (LayersOf); as barras de cubos saíram
    else if (Kind == TEXT("Bite"))        // o sangue que espirra quando a bocarra fecha (as presas são cones, abaixo)
        Add(16, [this](FBit& B, int32) { B.Dir = FMath::VRand(); B.A = Radius * FMath::FRandRange(.5f, 1.3f); B.Size = FMath::FRandRange(8.f, 16.f); B.Shade = 0; });
    else if (Kind == TEXT("Headbutt"))    // a estrela de impacto: raios no plano de frente
        Add(12, [](FBit& B, int32 i) { B.B = i * UE_TWO_PI / 12.f + FMath::FRandRange(-.12f, .12f); B.A = i % 2 ? 1.f : .6f; B.Size = FMath::FRandRange(9.f, 14.f); B.Shade = i % 3; });
    else if (Kind == TEXT("Swoop") || Kind == TEXT("Hurl"))   // o rastro: cada pedaço um pouco mais atrás no caminho
        Add(16, [](FBit& B, int32 i) { B.A = float(i); B.Dir = FMath::VRand() * 14.f; B.Size = 30.f - i * 1.5f; B.Shade = 2 - FMath::Min(2, i / 5); });
    else if (Kind == TEXT("Sonar"))       // quatro arcos de som que avançam
        Add(44, [](FBit& B, int32 i) { B.A = float(i / 11); B.B = FMath::DegreesToRadians((i % 11 - 5) * 8.f); B.Size = 13.f; B.Shade = FMath::Min(2, i / 11); });
    else if (Kind == TEXT("Spikes"))      // a terra que pula em volta dos espinhos (cones, abaixo)
        Add(16, [this](FBit& B, int32) { B.Dir = FVector(FMath::VRand().GetSafeNormal2D()); B.A = Radius * FMath::FRandRange(.3f, 1.f);
            B.B = FMath::FRandRange(40.f, 130.f); B.Size = FMath::FRandRange(8.f, 18.f); B.Shade = 0; });
    else if (Kind == TEXT("Quake"))       // as pedras do chão: pulam quando a onda passa por elas
        Add(52, [this](FBit& B, int32) { B.Dir = FVector(FMath::VRand().GetSafeNormal2D()); B.A = Radius * FMath::Sqrt(FMath::FRand());
            B.B = FMath::FRandRange(45.f, 120.f); B.C = FMath::FRandRange(0.f, 360.f); B.Size = FMath::FRandRange(18.f, 40.f); });
    else if (Kind == TEXT("Rubble"))      // os pedaços que saem do chão e se juntam nas mãos
        Add(14, [this](FBit& B, int32) { B.Dir = FVector(FMath::VRand().GetSafeNormal2D()); B.A = Radius * FMath::FRandRange(.4f, 1.f);
            B.C = FMath::FRandRange(0.f, .3f); B.Size = FMath::FRandRange(16.f, 34.f); });
    if (Kind == TEXT("Bite") || Kind == TEXT("Spikes"))
    {
        Cones = NewObject<UInstancedStaticMeshComponent>(this);
        Cones->SetupAttachment(GetRootComponent());
        Cones->SetUsingAbsoluteLocation(true);
        Cones->SetUsingAbsoluteRotation(true);
        Cones->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone")));
        Cones->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Cones->SetCastShadow(false);
        if (Solid)
        {
            UMaterialInstanceDynamic* Tint = UMaterialInstanceDynamic::Create(Solid, this);
            Tint->SetVectorParameterValue(TEXT("Color"), Kind == TEXT("Bite") ? FLinearColor(3.5f, 3.4f, 3.f) : FLinearColor(.34f, .42f, .14f));   // as presas brilham (brancas na neve sumiam)
            Cones->SetMaterial(0, Tint);
        }
        Cones->RegisterComponent();
        if (Kind == TEXT("Bite"))
        {
            // sete presas em cima e sete embaixo; as duas dos cantos (os caninos) são as maiores
            for (int32 Row = 0; Row < 2; ++Row)
                for (int32 i = 0; i < 7; ++i)
                {
                    FBit B;
                    B.A = Row ? -1.f : 1.f;
                    B.B = (i - 3) / 3.f;
                    B.Size = FMath::Abs(i - 3) == 2 ? 1.f : FMath::Abs(i - 3) == 3 ? .5f : .62f;
                    ConeBits.Add(B);
                }
        }
        else
            for (int32 i = 0; i < 10; ++i)
            {
                FBit B;
                B.Dir = FVector(FMath::VRand().GetSafeNormal2D());
                B.A = i == 0 ? 0.f : Radius * FMath::FRandRange(.25f, .85f);
                B.B = i == 0 ? 190.f : FMath::FRandRange(90.f, 160.f);
                B.C = i == 0 ? 0.f : FMath::FRandRange(.4f, 1.f);
                ConeBits.Add(B);
            }
        for (int32 i = 0; i < ConeBits.Num(); ++i) Cones->AddInstance(FTransform(FVector::ZeroVector), true);
    }
    for (const FBit& Bit : Bits) Shades[Bit.Shade]->AddInstance(FTransform(FVector::ZeroVector), true);
    if (Kind != TEXT("Drip") && !IsPlain(Kind))
    {
        const FLinearColor Tint = TintOf(Kind);
        auto Fx = [&](const TCHAR* Name, float Strength) -> UMaterialInstanceDynamic*
        {
            UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Combat/%s.%s"), Name, Name));
            UMaterialInstanceDynamic* Mat = Base ? UMaterialInstanceDynamic::Create(Base, this) : nullptr;
            if (Mat)
            {
                Mat->SetVectorParameterValue(TEXT("Color"), Tint);
                Mat->SetScalarParameterValue(TEXT("Strength"), Strength);
                Mat->SetScalarParameterValue(TEXT("Seed"), FMath::FRand());
                Fading.Add(Mat);
                FadingStrength.Add(Strength);
            }
            return Mat;
        };
        auto Piece = [&](const TCHAR* Shape, UMaterialInterface* Mat)
        {
            UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
            C->SetupAttachment(GetRootComponent());
            C->SetUsingAbsoluteLocation(true);
            C->SetUsingAbsoluteRotation(true);
            C->SetUsingAbsoluteScale(true);
            C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, Shape));
            C->SetMaterial(0, Mat);
            C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            C->SetCastShadow(false);
            C->RegisterComponent();
            return C;
        };
        GroundRing = Piece(TEXT("/Engine/BasicShapes/Plane.Plane"), Fx(TEXT("M_DropRing"), Kind == TEXT("Tornado") ? 1.1f : .9f));
        if (Kind == TEXT("Tornado"))
        {
            Funnels.Add(Piece(TEXT("/Engine/BasicShapes/Cone.Cone"), Fx(TEXT("M_FxFunnel"), .55f)));
            Funnels.Add(Piece(TEXT("/Engine/BasicShapes/Cone.Cone"), Fx(TEXT("M_FxFunnel"), .85f)));
        }
        Puffs = NewObject<UInstancedStaticMeshComponent>(this);
        Puffs->SetupAttachment(GetRootComponent());
        Puffs->SetUsingAbsoluteLocation(true);
        Puffs->SetUsingAbsoluteRotation(true);
        Puffs->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
        Puffs->SetMaterial(0, Fx(TEXT("M_FxPuff"), Kind == TEXT("PoisonCloud") ? .55f : .34f));
        Puffs->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Puffs->SetCastShadow(false);
        Puffs->RegisterComponent();
        const int32 Count = Kind == TEXT("PoisonCloud") ? 26 : Kind == TEXT("Tornado") ? 18 : Kind == TEXT("Dust") ? 20 : Kind == TEXT("PoisonPool") ? 12 : 10;
        for (int32 i = 0; i < Count; ++i)
        {
            FBit P;
            P.Dir = FVector(FMath::VRand().GetSafeNormal2D());
            P.A = Radius * (Kind == TEXT("PoisonCloud") ? FMath::Sqrt(FMath::FRand()) : FMath::FRandRange(.55f, 1.f));
            P.B = Kind == TEXT("PoisonPool") ? FMath::FRandRange(6.f, 22.f) : FMath::FRandRange(20.f, 120.f);
            P.C = FMath::FRandRange(0.f, .3f);
            P.Size = FMath::Clamp(Radius * FMath::FRandRange(.14f, .26f), 30.f, 85.f);   // grandes demais tapavam a visão
            PuffBits.Add(P);
            Puffs->AddInstance(FTransform(FQuat::Identity, GetActorLocation(), FVector(.001f)), true);
        }
    }
    SetActorHiddenInGame(Delay > 0.f);
    Tick(0.f);
}

FTransform AVortexMonsterFx::PlacePuff(const FBit& Bit, float T) const
{
    // as nuvens: saem do centro, sobem devagar, incham e somem encolhendo
    const FVector Center(FollowActor.IsValid() ? FollowActor->GetActorLocation().X : GetActorLocation().X, FollowActor.IsValid() ? FollowActor->GetActorLocation().Y : GetActorLocation().Y, BaseZ);
    const float Local = FMath::Clamp((T - Bit.C) / (1.f - Bit.C), 0.f, 1.f);
    const float Out = 1.f - FMath::Square(1.f - Local);
    FVector Dir = Bit.Dir;
    if (Kind == TEXT("Tornado")) Dir = Bit.Dir.RotateAngleAxis(Age * 340.f, FVector::UpVector);   // a poeira roda em volta da base
    const float Size = Bit.Size * (.45f + .8f * Local) * Bump(FMath::Min(1.f, Local * .85f + .1f)) / 50.f;
    return FTransform(FQuat::Identity, Center + Dir * Bit.A * Out + FVector(0.f, 0.f, 10.f + Bit.B * Local), FVector(FMath::Max(Size, .001f)));
}

FTransform AVortexMonsterFx::Place(const FBit& Bit, float T) const
{
    const FVector Center = FollowActor.IsValid() ? FollowActor->GetActorLocation() : GetActorLocation();
    const float Fade = FMath::Clamp((1.f - T) * 4.f, 0.f, 1.f);
    if (Kind == TEXT("PoisonCloud"))
    {
        // sai do centro, se espalha e sobe devagar; incha e murcha
        const float Local = FMath::Clamp((T - Bit.C) / (1.f - Bit.C), 0.f, 1.f);
        const float Out = 1.f - FMath::Square(1.f - Local);
        const FVector At = Center + Bit.Dir * Bit.A * Out + FVector(0.f, 0.f, 15.f + Bit.B * Local);
        return FTransform(FRotator(Age * 40.f, Bit.A, Age * 30.f), At, FVector(Bit.Size * Bump(Local * .9f + .1f) * Fade / 100.f));
    }
    if (Kind == TEXT("PoisonPool") || Kind == TEXT("GelPool"))
    {
        if (Bit.C < 0.f)   // a poça: placas baixas no chão que crescem e somem
            return FTransform(FRotator(0.f, Bit.A * 7.f, 0.f), Center + Bit.Dir * Bit.A + FVector(0.f, 0.f, 2.f),
                FVector(Bit.Size, Bit.Size, 3.f) * FMath::Min(1.f, T * 6.f) * Fade / 100.f);
        // as bolhas: sobem e estouram
        const float Cycle = FMath::Fmod(T * 2.5f + Bit.C, 1.f);
        return FTransform(FRotator::ZeroRotator, Center + Bit.Dir * Bit.A + FVector(0.f, 0.f, 4.f + Cycle * 30.f),
            FVector(Bit.Size * Bump(Cycle) * Fade / 100.f));
    }
    if (Kind == TEXT("Drip") || Kind == TEXT("GelDrip"))
        return FTransform(FRotator::ZeroRotator, Center - FVector(0.f, 0.f, 90.f * T * T), FVector(Bit.Size * (1.f - T) / 100.f));
    if (Kind == TEXT("Tornado"))
    {
        const float Grow = FMath::Min(1.f, T * 5.f) * Fade;
        const float Height = FMath::Clamp(Radius * .85f, 240.f, 480.f);
        const float Up = BaseZ - Center.Z;   // do chão, mesmo com o morcego voando
        if (Bit.A < 0.f)
        {
            const float Angle = Bit.B + Age * 6.f * Bit.C;
            return FTransform(FRotator(0.f, Angle * 57.3f, 0.f), Center + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * Radius * .7f
                + FVector(0.f, 0.f, Up + 12.f + 10.f * FMath::Sin(Age * 9.f + Bit.B)), FVector(Bit.Size * Grow / 100.f));
        }
        // a altura do cubo sobe (e volta para baixo): o vento subindo pelo funil
        const float H = FMath::Fmod(Bit.A + Age * .55f * Bit.C, 1.f);
        const float R = FMath::Lerp(.1f, .56f, H * H) * Radius;
        const float Angle = Bit.B + Age * (10.f + 6.f * (1.f - H)) * Bit.C;
        const FVector At = Center + FVector(FMath::Cos(Angle) * R, FMath::Sin(Angle) * R, Up + H * Height);
        return FTransform(FRotator(0.f, Angle * 57.3f, 20.f), At, FVector(Bit.Size * (.6f + .4f * H) * Grow * Bump(H * .9f + .05f) / 100.f));
    }
    if (Kind == TEXT("Dust") || Kind == TEXT("Splash"))
    {
        // saem em anel, sobem num arco e caem; a poeira fica mais tempo no ar
        const float Out = 1.f - FMath::Square(1.f - T);
        const FVector At = Center + Bit.Dir * Bit.A * Out + FVector(0.f, 0.f, Bit.B * Bump(T) + 5.f);
        return FTransform(FRotator(Age * 300.f, Bit.A, 0.f), At, FVector(Bit.Size * Fade / 100.f));
    }
    // ---- 13/11: os efeitos novos. Front: para onde o efeito aponta; Right: o lado
    const FVector Front = GetActorForwardVector().GetSafeNormal2D(), Right(-Front.Y, Front.X, 0.f), Up = FVector::UpVector;
    if (Kind == TEXT("Howl"))
    {
        const float Local = FMath::Clamp((T - Bit.A * .16f) / .6f, 0.f, 1.f);
        const float Wide = Radius * Local;
        // cada pedaço estica de lado até encostar no vizinho: o anel fica fechado (cubinhos soltos não se liam como anel)
        const float Thick = Bit.Size * (Local <= 0.f ? 0.f : 1.15f - Local);
        return FTransform(FRotator(0.f, FMath::RadiansToDegrees(Bit.B), 0.f), Center + FVector(FMath::Cos(Bit.B), FMath::Sin(Bit.B), 0.f) * Wide + Up * (20.f + Wide * .12f),
            FVector(Thick, Local <= 0.f ? 0.f : FMath::Max(Thick, Wide * UE_TWO_PI / 16.f * .9f), Thick) / 100.f);
    }
    if (Kind == TEXT("Bite"))
    {
        // espirra na hora em que a bocarra fecha (T = .45)
        const float Local = FMath::Clamp((T - .45f) / .5f, 0.f, 1.f);
        return FTransform(FRotator(Age * 400.f, 0.f, Age * 300.f), Center + Bit.Dir * Bit.A * (1.f - FMath::Square(1.f - Local)) - Up * 60.f * Local * Local,
            FVector(Bit.Size * (Local <= 0.f ? 0.f : 1.f - Local) / 100.f));
    }
    if (Kind == TEXT("Headbutt"))
    {
        const FVector Dir = Right * FMath::Cos(Bit.B) + Up * FMath::Sin(Bit.B);
        const float Out = 1.f - FMath::Square(1.f - T);
        return FTransform(FRotationMatrix::MakeFromX(Dir).ToQuat(), Center + Dir * Radius * Bit.A * (.25f + .75f * Out),
            FVector(Bit.Size * 3.2f, Bit.Size * .55f, Bit.Size * .55f) * (1.f - T) / 100.f);
    }
    if (Kind == TEXT("Swoop") || Kind == TEXT("Hurl"))
    {
        const int32 Back = FMath::Min(Trail.Num() - 1, FMath::RoundToInt(Bit.A * 1.6f));
        if (Back < 0) return FTransform(FQuat::Identity, Center, FVector(.001f));
        return FTransform(FRotator(Age * 200.f, Bit.A * 40.f, 0.f), Trail[Back] + Bit.Dir, FVector(FMath::Max(2.f, Bit.Size) * Fade / 100.f));
    }
    if (Kind == TEXT("Sonar"))
    {
        const float Local = FMath::Clamp((T - Bit.A * .13f) / .55f, 0.f, 1.f);
        const FVector Dir = Front * FMath::Cos(Bit.B) + Right * FMath::Sin(Bit.B);
        return FTransform(FRotationMatrix::MakeFromX(Dir).ToQuat(), Center + Dir * (60.f + Radius * Local),
            FVector(Bit.Size * .5f, Bit.Size * (1.f + 2.f * Local), Bit.Size * (1.f + 2.f * Local)) * (Local <= 0.f ? 0.f : 1.f - Local * Local) / 100.f);
    }
    if (Kind == TEXT("Spikes"))
    {
        const float Out = 1.f - FMath::Square(1.f - T);
        return FTransform(FRotator(Age * 300.f, Bit.A, 0.f), FVector(Center.X, Center.Y, BaseZ) + Bit.Dir * Bit.A * (.4f + .6f * Out) + Up * (Bit.B * Bump(T) + 5.f),
            FVector(Bit.Size * Fade / 100.f));
    }
    if (Kind == TEXT("Quake"))
    {
        // a onda anda do centro para fora; a pedra pula quando ela passa e assenta depois
        const float Wave = Radius * T * 1.3f;
        const float Lift = FMath::Max(0.f, 1.f - FMath::Abs(Bit.A - Wave) / (Radius * .22f));
        return FTransform(FRotator(Bit.C + Lift * 60.f, Bit.C, Lift * 40.f), FVector(Center.X, Center.Y, BaseZ) + Bit.Dir * Bit.A + Up * (Bit.B * Lift + 4.f),
            FVector(Bit.Size * (.15f + .85f * Lift) * Fade / 100.f));
    }
    if (Kind == TEXT("Rubble"))
    {
        // 13/11: seguem as mãos do clipe Pickup do golem: juntam-se nelas no chão, à frente (até .55 s), e sobem com
        // elas até acima da cabeça (.70 a .90 s, medido no clipe gravado). shortcut: os tempos são os do clipe; se o clipe mudar, acertar aqui
        const float Local = FMath::SmoothStep(0.f, 1.f, FMath::Clamp((T - Bit.C) / (.55f - Bit.C), 0.f, 1.f));
        const FVector Feet(Center.X, Center.Y, BaseZ), From = Feet + Bit.Dir * Bit.A;
        const FVector To = FMath::Lerp(Feet + Front * 95.f + Up * 35.f, Feet + Front * 45.f + Up * 300.f, FMath::SmoothStep(.70f, .90f, T));
        return FTransform(FRotator(Age * 250.f, Bit.A, Age * 180.f), FMath::Lerp(From, To, Local) + Up * 40.f * Bump(Local),
            FVector(Bit.Size * (T > .88f ? FMath::Max(0.f, (1.f - T) / .12f) : .4f + .6f * Local) / 100.f));
    }
    return FTransform(Center);
}

FTransform AVortexMonsterFx::PlaceCone(const FBit& Bit, float T) const
{
    const FVector Center = FollowActor.IsValid() ? FollowActor->GetActorLocation() : GetActorLocation();
    const FVector Front = GetActorForwardVector().GetSafeNormal2D(), Right(-Front.Y, Front.X, 0.f), Up = FVector::UpVector;
    if (Kind == TEXT("Bite"))
    {
        // a bocarra: aparece aberta, escancara, fecha de uma vez (T = .45) e some
        const float Open = T < .3f ? FMath::Lerp(.45f, 1.f, T / .3f) : T < .45f ? FMath::Lerp(1.f, .08f, FMath::Square((T - .3f) / .15f)) : .08f;
        const float Show = FMath::Min(1.f, T * 12.f) * FMath::Clamp((1.f - T) / .25f, 0.f, 1.f);
        const float Tall = Radius * .75f * Bit.Size, Wide = Radius * .26f * (.6f + .4f * Bit.Size);
        // a fileira é curva: as presas do meio ficam mais para dentro da boca (mais altas em cima, mais baixas embaixo)
        const float Arch = Radius * .18f * (1.f - Bit.B * Bit.B);
        const FVector At = Center + Right * Bit.B * Radius * .95f + Up * Bit.A * (Radius * .85f * Open + Tall * .5f + Arch);
        return FTransform(FRotator(Bit.A > 0.f ? 180.f : 0.f, 0.f, 0.f), At, FVector(Wide, Wide, Tall) * Show / 100.f);
    }
    // os espinhos: sobem de uma vez (cada um no seu tempo), ficam e afundam
    const float Rise = FMath::Clamp((T - Bit.C * .1f) / .1f, 0.f, 1.f) * FMath::Clamp((1.f - T) / .2f, 0.f, 1.f);
    const float Tall = Bit.B * (1.f - FMath::Square(1.f - Rise));
    const FVector Lean = Bit.Dir * Bit.C * 14.f;
    return FTransform(FRotationMatrix::MakeFromZ(Up * 100.f + Lean).ToQuat(), FVector(Center.X, Center.Y, BaseZ) + Bit.Dir * Bit.A + Up * (Tall * .5f - 4.f),
        FVector(Bit.B * .3f, Bit.B * .3f, FMath::Max(1.f, Tall)) / 100.f);
}

void AVortexMonsterFx::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (Delay > 0.f)
    {
        Delay -= DeltaSeconds;
        if (Delay > 0.f) return;
        SetActorHiddenInGame(false);
    }
    else Age += DeltaSeconds;
    if (Age > Life) { Destroy(); return; }
    const float T = Age / Life;
    // a altura do chão embaixo do efeito (o morcego voa; o círculo e o funil nascem do chão)
    const FVector Over = FollowActor.IsValid() ? FollowActor->GetActorLocation() : GetActorLocation();
    // o terreno, não o primeiro obstáculo: embaixo de árvore, a linha batia na copa e o círculo nascia acima do morcego
    const TSharedPtr<FVortexTerrainData> Terrain = AVortexTerrainStreamer::SharedData();
    BaseZ = Terrain ? Terrain->GroundAt(Over.X, Over.Y) : Over.Z - 90.f;
    const float Open = FMath::Min(1.f, T * 5.f) * FMath::Clamp((1.f - T) * 4.f, 0.f, 1.f);
    for (int32 m = 0; m < Fading.Num(); ++m) if (Fading[m]) Fading[m]->SetScalarParameterValue(TEXT("Strength"), FadingStrength[m] * Open);
    if (GroundRing)
    {
        const float Size = FMath::Max(.01f, 2.f * Radius / 100.f * FMath::Min(1.f, T * 6.f) * (1.f + .12f * Bump(T * 6.f)));
        GroundRing->SetWorldLocation(FVector(Over.X, Over.Y, BaseZ + 5.f));
        GroundRing->SetWorldScale3D(FVector(Size, Size, 1.f));
    }
    for (int32 f = 0; f < Funnels.Num(); ++f)
    {
        // o cone de ponta para baixo: bem largo em cima, do tamanho da área de dano
        const float Tall = FMath::Clamp(Radius * .85f, 240.f, 480.f) * (f ? .82f : 1.f);
        const float Wide = 2.f * Radius * (f ? .34f : .56f) / 100.f * FMath::Max(.02f, Open);
        const FVector Sway(FMath::Sin(Age * 3.1f + f) * 14.f, FMath::Cos(Age * 2.6f + f) * 14.f, 0.f);
        Funnels[f]->SetWorldLocationAndRotation(FVector(Over.X, Over.Y, BaseZ + Tall * .5f) + Sway, FRotator(180.f, Age * (f ? -160.f : 120.f), 0.f));
        Funnels[f]->SetWorldScale3D(FVector(Wide, Wide, Tall / 100.f));
    }
    if (Puffs && PuffBits.Num())
    {
        TArray<FTransform> Clouds;
        Clouds.Reserve(PuffBits.Num());
        for (const FBit& Bit : PuffBits) Clouds.Add(PlacePuff(Bit, T));
        Puffs->BatchUpdateInstancesTransforms(0, Clouds, true, true, true);
    }
    // as partículas do golpe: uma vez, no instante dele (os rastros vão presos em quem o efeito segue)
    // as camadas: cada uma quando a idade passa pelo instante dela (LastAge: a idade no quadro anterior)
    static TMap<TWeakObjectPtr<const AVortexMonsterFx>, float> LastAges;
    const float LastAge = LastAges.FindRef(this);
    LastAges.Add(this, Age);
    if (LastAges.Num() > 256) for (auto It = LastAges.CreateIterator(); It; ++It) if (!It.Key().IsValid()) It.RemoveCurrent();
    UNiagaraSystem* Power = PowerSystemOf(Kind);
    if (Power && LastAge <= 0.f && Age > 0.f)
        if (UNiagaraComponent* Made = UNiagaraFunctionLibrary::SpawnSystemAttached(Power, GetRootComponent(), NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, FVector::OneVector,
                EAttachLocation::KeepRelativeOffset, true, ENCPoolMethod::None))
            if (Custom.R >= 0.f) Made->SetVariableLinearColor(TEXT("Color"), Custom);   // a cor do elemento da arma
    const TArrayView<const FLayer> FxLayers = Power ? TArrayView<const FLayer>() : LayersOf(Kind);
    for (const FLayer& Layer : FxLayers)
        if (LastAge <= Layer.At && Age > Layer.At)
            if (UNiagaraSystem* System = LoadObject<UNiagaraSystem>(nullptr, Layer.Path))
                UNiagaraFunctionLibrary::SpawnSystemAttached(System, GetRootComponent(), NAME_None, FVector(0.f, 0.f, Layer.Up < -9000.f ? BaseZ + 8.f - GetActorLocation().Z : Layer.Up), FRotator::ZeroRotator,
                    FVector(Layer.Scale), EAttachLocation::KeepRelativeOffset, true, ENCPoolMethod::None);
    // as imagens: nascem no instante de cada uma, crescem e somem (o estado fica aqui para não mexer no .h)
    {
        static TMap<TWeakObjectPtr<const AVortexMonsterFx>, TArray<FLiveSprite>> LiveSprites;
        if (LiveSprites.Num() > 256) for (auto It = LiveSprites.CreateIterator(); It; ++It) if (!It.Key().IsValid()) It.RemoveCurrent();
        TArray<FLiveSprite>& Live = LiveSprites.FindOrAdd(this);
        const TArrayView<const FSprite> Sprites = Power ? TArrayView<const FSprite>() : SpritesOf(Kind);
        const float Span = IsTimed(Kind) ? Life : 1.f;
        for (int32 i = 0; i < Sprites.Num(); ++i)
        {
            const FSprite& S = Sprites[i];
            if (!(LastAge <= S.At * Span && Age > S.At * Span)) continue;
            UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Vortex/Combat/M_FxSprite.M_FxSprite"));
            UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Combat/Sprites/T_Fx_%s.T_Fx_%s"), S.Tex, S.Tex));
            if (!Base || !Tex) continue;
            UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
            C->SetupAttachment(GetRootComponent());
            C->SetUsingAbsoluteLocation(true);
            C->SetUsingAbsoluteRotation(true);
            C->SetUsingAbsoluteScale(true);
            C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));
            C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            C->SetCastShadow(false);
            C->TranslucencySortPriority = 10 + i;
            UMaterialInstanceDynamic* Mat = UMaterialInstanceDynamic::Create(Base, this);
            Mat->SetTextureParameterValue(TEXT("Tex"), Tex);
            Mat->SetVectorParameterValue(TEXT("Color"), S.bTint && Custom.R >= 0.f ? Custom * 3.f : S.Color);
            Mat->SetScalarParameterValue(TEXT("Strength"), 0.f);
            C->SetMaterial(0, Mat);
            C->SetWorldScale3D(FVector(.001f));
            C->RegisterComponent();
            FLiveSprite One;
            One.Mesh = C; One.Mat = Mat; One.Index = i;
            Live.Add(One);
        }
        const APlayerController* Viewer = GetWorld()->GetFirstPlayerController();
        const FVector Eye = Viewer && Viewer->PlayerCameraManager ? Viewer->PlayerCameraManager->GetCameraLocation() : Over + FVector(0.f, 0.f, 500.f);
        const FVector Ahead = GetActorForwardVector().GetSafeNormal2D();
        for (int32 k = Live.Num() - 1; k >= 0; --k)
        {
            UStaticMeshComponent* C = Live[k].Mesh.Get();
            const FSprite& S = Sprites[Live[k].Index];
            const float U = FMath::Clamp((Age - S.At * Span) / (S.Life * Span), 0.f, 1.f);
            if (!C || U >= 1.f) { if (C) C->DestroyComponent(); Live.RemoveAtSwap(k); continue; }
            const float Ease = 1.f - FMath::Square(1.f - U);
            const float Size = 2.f * Radius * FMath::Lerp(S.From, S.To, Ease) / 100.f;
            if (UMaterialInstanceDynamic* Mat = Live[k].Mat.Get())
                Mat->SetScalarParameterValue(TEXT("Strength"), IsTimed(Kind) ? FMath::Min(1.f, U * 6.f) * (.45f + .55f * U) : FMath::Min(1.f, U * 10.f) * FMath::Pow(1.f - U, 1.3f));
            FVector At = Over + FVector(0.f, 0.f, S.Up);
            FQuat Turn;
            if (S.Mode == 0)
            {
                At = FVector(Over.X, Over.Y, BaseZ + 6.f + Live[k].Index * 1.5f);
                Turn = FRotator(0.f, GetActorRotation().Yaw + S.Spin * Age + Live[k].Index * 37.f, 0.f).Quaternion();
            }
            else
            {
                // o plano tem a frente no +Z: aponta para a câmera (1) ou para onde o golpe vai (2)
                const FVector Face = S.Mode == 1 ? (Eye - At).GetSafeNormal() : Ahead;
                Turn = FQuat(Face, FMath::DegreesToRadians(S.Spin * Age)) * FRotationMatrix::MakeFromZX(Face, FVector::UpVector).ToQuat();
                At += S.Mode == 1 ? Face * 30.f : Ahead * (60.f + S.Travel * Radius * Ease);
            }
            C->SetWorldLocationAndRotation(At, Turn);
            C->SetWorldScale3D(FVector(Size, Size, 1.f));
        }
    }
    float SparkAt = 0.f, SparkScale = 1.f;
    if (const TCHAR* Path = bSparked || FxLayers.Num() ? nullptr : SparkOf(Kind, SparkAt, SparkScale); Path && Age >= SparkAt)
    {
        bSparked = true;
        if (UNiagaraSystem* System = LoadObject<UNiagaraSystem>(nullptr, Path))
        {
            // sempre presas neste efeito: somem com ele (soltas no mundo, as que são em laço ficavam no ar para sempre);
            // os rastros andam junto com quem o efeito segue (abaixo)
            UNiagaraFunctionLibrary::SpawnSystemAttached(System, GetRootComponent(), NAME_None,
                FVector(0.f, 0.f, Kind == TEXT("Spikes") ? BaseZ + 10.f - GetActorLocation().Z : 0.f), FRotator::ZeroRotator, FVector(SparkScale),
                EAttachLocation::KeepRelativeOffset, true, ENCPoolMethod::None);
        }
    }
    // o caminho de quem o efeito segue (o morcego no rasante, o herói arremessado): o mais novo na frente
    if (Kind == TEXT("Swoop") || Kind == TEXT("Hurl"))
    {
        SetActorLocation(Over);
        Trail.Insert(Over, 0);
        if (Trail.Num() > 40) Trail.SetNum(40);
    }
    if (Cones && ConeBits.Num())
    {
        TArray<FTransform> Fangs;
        Fangs.Reserve(ConeBits.Num());
        for (const FBit& Bit : ConeBits) Fangs.Add(PlaceCone(Bit, T));
        Cones->BatchUpdateInstancesTransforms(0, Fangs, true, true, true);
    }
    TArray<TArray<FTransform>> Poses;
    Poses.SetNum(Shades.Num());
    for (const FBit& Bit : Bits) Poses[Bit.Shade].Add(Place(Bit, T));
    for (int32 s = 0; s < Shades.Num(); ++s)
        if (Poses[s].Num()) Shades[s]->BatchUpdateInstancesTransforms(0, Poses[s], true, true);
}

// 13/11: o catálogo de partículas (para escolher sem ser às cegas). "Vortex.Fx star,shockwave" (ou /fx no chat) põe em
// fila, à frente do herói, todo sistema Niagara cujo nome contém um dos pedaços, e solta cada um de novo a cada 2,5 s
// por 20 s. O log diz a ordem (VORTEX_FX n nome): 6 por fileira, da esquerda para a direita, a 1ª fileira a mais perto.
// shortcut: só na máquina de quem digita (é ferramenta de teste)
#include "AssetRegistry/AssetRegistryModule.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "TimerManager.h"

static FAutoConsoleCommandWithWorldAndArgs GVortexFxCatalog(TEXT("Vortex.Fx"), TEXT("Catálogo de partículas: Vortex.Fx pedaço1,pedaço2"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
    {
        const APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
        const APawn* Hero = Player ? Player->GetPawn() : nullptr;
        if (!Hero || Args.IsEmpty()) return;
        TArray<FString> Wanted;
        FString::Join(Args, TEXT(",")).ParseIntoArray(Wanted, TEXT(","));
        TArray<FAssetData> All;
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get().GetAssetsByClass(UNiagaraSystem::StaticClass()->GetClassPathName(), All);
        All.Sort([](const FAssetData& A, const FAssetData& B) { return A.AssetName.LexicalLess(B.AssetName); });
        TArray<FSoftObjectPath> Found;
        for (const FAssetData& Asset : All)
            if (Wanted.ContainsByPredicate([&](const FString& W) { return Asset.AssetName.ToString().Contains(W) || Asset.AssetName.ToString() == W; }))
                Found.Add(Asset.GetSoftObjectPath());
        const FVector Forward = Hero->GetActorForwardVector().GetSafeNormal2D(), Right(-Forward.Y, Forward.X, 0.f);
        const FVector Base = Hero->GetActorLocation() + Forward * 500.f;
        for (int32 i = 0; i < Found.Num(); ++i)
        {
            UE_LOG(LogTemp, Display, TEXT("VORTEX_FX %d %s"), i, *Found[i].GetAssetName());
            UNiagaraSystem* System = Cast<UNiagaraSystem>(Found[i].TryLoad());
            if (!System) continue;
            const FVector At = Base + Right * ((i % 6) - 2.5f) * 400.f + Forward * (i / 6) * 450.f;   // grade: 6 por fileira, a 1ª fileira é a mais perto
            for (int32 k = 0; k < 8; ++k)
            {
                FTimerHandle Handle;
                World->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(World, [World, System, At]()
                {
                    UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, System, At);
                }), .05f + k * 2.5f, false);
            }
        }
    }));
