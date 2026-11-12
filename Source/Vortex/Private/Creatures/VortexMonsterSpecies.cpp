#include "Creatures/VortexMonsterSpecies.h"

// As espécies do Godot (Art/ValleyMonster.*.cs, RegionalBoss.cs, RuneColossus.cs), em centímetros.
// Os tempos de impacto são os dos clipes (ValleyMonster.Moves.cs: Info); os alcances e as recargas,
// os de ChooseMove/ResolveMove.
namespace
{
    FVortexMonsterMove Move(const TCHAR* Id, const TCHAR* Clip, EVortexMoveKind Kind, float Impact, float MinRange, float MaxRange,
        float Reach, float Scale, float After)
    {
        FVortexMonsterMove M;
        M.Id = Id;
        M.Clip = Clip;
        M.Kind = Kind;
        M.Impact = Impact;
        M.MinRange = MinRange;
        M.MaxRange = MaxRange;
        M.Reach = Reach;
        M.Scale = Scale;
        M.After = After;
        return M;
    }

    FVortexMark Mark(uint8 Anchor, float Ahead, float Radius, float Delay, float Scale = 1.f, bool bRing = false, int32 MinPhase = 1, float Side = 0.f)
    {
        FVortexMark K;
        K.Anchor = Anchor; K.Ahead = Ahead; K.Radius = Radius; K.Delay = Delay; K.Scale = Scale; K.bRing = bRing; K.MinPhase = MinPhase; K.SideAngle = Side;
        return K;
    }

    TMap<FName, FVortexSpeciesDef> Build()
    {
        TMap<FName, FVortexSpeciesDef> All;
        auto Add = [&All](FVortexSpeciesDef D) { All.Add(D.Species, MoveTemp(D)); };
        using K = EVortexMoveKind;

        // Gotas (o slime novo, VortexSlimeLook): andam aos pulos; o bote estica a gelatina até o herói (alcança ~2,6 m),
        // de longe um salto esmagador. Quatro tipos, pela região: a de jade (comum), a de brasa (Brasafenda, queima),
        // a peçonhenta (Lodonegro e Morthúria, envenena e cospe veneno) e a de ferro (Raikor e Solcânion, lenta e dura)
        auto Gota = [&](const TCHAR* Id, const TCHAR* Title, float Speed, float Hits, float Damage, const FLinearColor& Splash, FName OnHit)
        {
            FVortexSpeciesDef D; D.Species = Id; D.Title = Title;
            D.Radius = 48.f; D.Height = 96.f; D.Speed = Speed; D.Accel = 3000.f; D.Hits = Hits; D.BaseDamage = Damage;
            D.bFearful = OnHit.IsNone(); D.bMagic = true; D.HitSound = TEXT("slime_land"); D.StepSound = TEXT("slime_hop");
            D.Splash = Splash; D.Telegraph = Splash; D.OnHit = OnHit;
            FVortexMonsterMove Leap = Move(TEXT("Leap"), TEXT("Leap"), K::Lunge, .9f, 280.f, 650.f, 210.f, 1.35f, 1.3f);
            Leap.LungeFrom = .42f; Leap.LungeDistance = 550.f; Leap.OwnCooldown = 5.5f; Leap.bTelegraph = true; Leap.bMagic = true;
            D.Moves.Add(Leap);
            FVortexMonsterMove Tackle = Move(TEXT("Tackle"), TEXT("Attack"), K::Strike, .38f, 0.f, 240.f, 250.f, 1.f, 1.2f);
            Tackle.FrontDot = .5f; Tackle.bMagic = true;
            D.Moves.Add(Tackle);
            return D;
        };
        Add(Gota(TEXT("Slime"), TEXT("GOTA DE JADE"), 220.f, 3.f, 10.f, FLinearColor(.18f, .85f, .55f), NAME_None));
        Add(Gota(TEXT("SlimeFire"), TEXT("GOTA DE BRASA"), 240.f, 3.2f, 12.f, FLinearColor(1.f, .45f, .1f), TEXT("burn")));
        {
            FVortexSpeciesDef D = Gota(TEXT("SlimePoison"), TEXT("GOTA PEÇONHENTA"), 210.f, 3.f, 10.f, FLinearColor(.7f, .25f, 1.f), TEXT("poison"));
            FVortexMonsterMove Spit = Move(TEXT("Spit"), TEXT("Attack"), K::Lob, .4f, 380.f, 900.f, 170.f, .8f, 1.4f);
            Spit.OwnCooldown = 4.f; Spit.bMagic = true; Spit.Flight = .8f;
            D.Moves.Add(Spit);
            Add(D);
        }
        {
            // ferro: o dobro de vida, mais devagar, e uma pancada no chão em volta (encurralado)
            FVortexSpeciesDef D = Gota(TEXT("SlimeIron"), TEXT("GOTA DE FERRO"), 150.f, 6.f, 15.f, FLinearColor(.75f, .75f, .8f), NAME_None);
            D.bMagic = false; D.HitSound = TEXT("golem_slam");
            for (FVortexMonsterMove& M : D.Moves) M.bMagic = false;
            FVortexMonsterMove Slam = Move(TEXT("Slam"), TEXT("Slam"), K::Area, .55f, 0.f, 200.f, 260.f, 1.3f, 1.6f);
            Slam.OwnCooldown = 6.f; Slam.bTelegraph = true;
            D.Moves.Add(Slam);
            Add(D);
        }
        {   // Cogumelo Errante: guarda distância, cospe veneno, pula para trás e solta esporos encurralado
            FVortexSpeciesDef D; D.Species = TEXT("Mushroom"); D.Title = TEXT("COGUMELO ERRANTE");
            D.Radius = 45.f; D.Height = 129.f; D.Speed = 220.f; D.Accel = 900.f; D.Hits = 3.5f; D.BaseDamage = 10.f; D.CloseEnough = 650.f;
            D.bFearful = true; D.bMagic = true; D.HitClip = TEXT("Tremble"); D.HitSound = TEXT("poison_smoke");
            D.Splash = FLinearColor(.55f, .9f, .25f); D.Telegraph = FLinearColor(.6f, 1.f, .25f);
            FVortexMonsterMove Spores = Move(TEXT("Spores"), TEXT("Spores"), K::Area, .55f, 0.f, 290.f, 290.f, .8f, .5f);
            Spores.OwnCooldown = 7.f; Spores.bMagic = true; Spores.bTelegraph = true;
            D.Moves.Add(Spores);
            FVortexMonsterMove Hop = Move(TEXT("Hop"), TEXT("Hop"), K::Hop, .48f, 0.f, 330.f, 0.f, 0.f, .35f);
            Hop.OwnCooldown = 2.2f; Hop.LungeFrom = .1f; Hop.LungeDistance = 240.f;
            D.Moves.Add(Hop);
            FVortexMonsterMove Spit = Move(TEXT("Spit"), TEXT("Attack"), K::Lob, .45f, 250.f, 850.f, 170.f, 1.f, 1.8f);
            Spit.Flight = .6f; Spit.bMagic = true;
            D.Moves.Add(Spit);
            // 10/11 (o Cogumelo Corrompido do Meshy): Raízes Corrompidas, o chão marca embaixo do herói e as raízes estouram ali
            FVortexMonsterMove Roots = Move(TEXT("Roots"), TEXT("Roots"), K::Area, 1.05f, 300.f, 900.f, 220.f, 1.3f, 1.f);
            Roots.OwnCooldown = 9.f; Roots.bMagic = true; Roots.bTelegraph = true; Roots.bAtTarget = true;
            D.Moves.Add(Roots);
            // Nuvem Corrompida: perto, um anel grande em volta dele vira uma nuvem de esporos (recarga longa)
            FVortexMonsterMove Burst = Move(TEXT("Burst"), TEXT("Burst"), K::Area, .95f, 0.f, 420.f, 450.f, 1.1f, 1.2f);
            Burst.OwnCooldown = 14.f; Burst.bMagic = true; Burst.bTelegraph = true;
            D.Moves.Add(Burst);
            Add(D);
        }
        {   // Golem Musgoso: lento e pesado; soco, pancada no chão em área e pedra arremessada
            FVortexSpeciesDef D; D.Species = TEXT("Golem"); D.Title = TEXT("GOLEM MUSGOSO");
            D.Radius = 65.f; D.Height = 235.f; D.Speed = 150.f; D.Accel = 500.f; D.Sight = 1700.f; D.Hits = 6.f; D.BaseDamage = 22.f;
            D.HitSound = TEXT("golem_slam"); D.StepSound = TEXT("golem_step"); D.Splash = FLinearColor(.36f, .4f, .3f);
            D.Telegraph = FLinearColor(.6f, 1.f, .8f);
            FVortexMonsterMove Slam = Move(TEXT("Slam"), TEXT("Slam"), K::Area, .65f, 0.f, 340.f, 360.f, 1.15f, 1.6f);
            Slam.OwnCooldown = 6.5f; Slam.bTelegraph = true;
            D.Moves.Add(Slam);
            FVortexMonsterMove Punch = Move(TEXT("Punch"), TEXT("Attack"), K::Strike, .55f, 0.f, 290.f, 300.f, 1.f, 1.4f);
            Punch.FrontDot = .3f;
            D.Moves.Add(Punch);
            FVortexMonsterMove Throw = Move(TEXT("Throw"), TEXT("Throw"), K::Lob, .95f, 550.f, 1700.f, 210.f, 1.2f, 1.2f);
            Throw.OwnCooldown = 7.f; Throw.Flight = 1.f;
            D.Moves.Add(Throw);
            Add(D);
        }
        {   // Morcego de Ametista: bate e foge; picada, mergulho e guincho em volta
            FVortexSpeciesDef D; D.Species = TEXT("Bat"); D.Title = TEXT("MORCEGO DE AMETISTA");
            D.Radius = 40.f; D.Height = 100.f; D.Speed = 320.f; D.Accel = 1100.f; D.Sight = 1600.f; D.Hits = 3.f; D.BaseDamage = 10.f;
            D.bFlying = true; D.bFearful = true; D.bMagic = true; D.IdleClip = TEXT("Fly"); D.WalkClip = TEXT("Fly");
            D.HitSound = TEXT("bat_screech"); D.Splash = FLinearColor(.45f, .3f, .6f); D.Telegraph = FLinearColor(.7f, .5f, 1.f);
            FVortexMonsterMove Screech = Move(TEXT("Screech"), TEXT("Screech"), K::Area, .5f, 0.f, 580.f, 500.f, .55f, 1.f);
            Screech.OwnCooldown = 9.f; Screech.bMagic = true; Screech.bTelegraph = true;
            D.Moves.Add(Screech);
            FVortexMonsterMove Nip = Move(TEXT("Nip"), TEXT("Attack"), K::Strike, .34f, 0.f, 170.f, 200.f, .8f, .9f);
            Nip.Retreat = .8f; Nip.bMagic = true;
            D.Moves.Add(Nip);
            FVortexMonsterMove Dive = Move(TEXT("Dive"), TEXT("Dive"), K::Lunge, .55f, 170.f, 520.f, 270.f, 1.f, 1.8f);
            Dive.LungeFrom = .15f; Dive.LungeDistance = 300.f; Dive.Retreat = 1.3f; Dive.bMagic = true;
            D.Moves.Add(Dive);
            Add(D);
        }
        {   // Sapo-Gigante do Brejo: lingüada que alcança 4,6 m e salto de barriga
            FVortexSpeciesDef D; D.Species = TEXT("Frog"); D.Title = TEXT("SAPO-GIGANTE DO BREJO");
            D.Radius = 36.f; D.Height = 95.f; D.Speed = 280.f; D.Accel = 800.f; D.Hits = 4.6f; D.BaseDamage = 13.f; D.bMagic = true;
            D.RunClip = TEXT("Run"); D.HitSound = TEXT("slime_land"); D.Splash = FLinearColor(.4f, .6f, .2f); D.Telegraph = FLinearColor(1.f, .8f, .3f);
            FVortexMonsterMove Leap = Move(TEXT("Leap"), TEXT("Leap"), K::Lunge, .9f, 320.f, 850.f, 210.f, 1.35f, 1.3f);
            Leap.LungeFrom = .42f; Leap.LungeDistance = 550.f; Leap.OwnCooldown = 5.5f; Leap.bTelegraph = true; Leap.bMagic = true;
            D.Moves.Add(Leap);
            FVortexMonsterMove Tongue = Move(TEXT("Tongue"), TEXT("Attack"), K::Strike, .36f, 0.f, 440.f, 460.f, .9f, 1.3f);
            Tongue.FrontDot = .55f; Tongue.bMagic = true;
            D.Moves.Add(Tongue);
            Add(D);
        }
        {   // Goblin: em bandos com um chefe maior; porrete de perto e pedras de longe
            FVortexSpeciesDef D; D.Species = TEXT("Goblin"); D.Title = TEXT("GOBLIN");
            D.Radius = 30.f; D.Height = 101.f; D.Speed = 410.f; D.Accel = 1600.f; D.Hits = 2.6f; D.BaseDamage = 9.f;
            D.RunClip = TEXT("Run"); D.HitSound = TEXT("creature_hit");
            FVortexMonsterMove Rock = Move(TEXT("Rock"), TEXT("Throw"), K::Lob, .55f, 500.f, 1400.f, 210.f, 1.2f, 1.f);
            Rock.OwnCooldown = 5.5f; Rock.Flight = .9f;
            D.Moves.Add(Rock);
            FVortexMonsterMove Club = Move(TEXT("Club"), TEXT("Attack"), K::Strike, .38f, 0.f, 220.f, 230.f, 1.f, .95f);
            Club.FrontDot = .2f;
            D.Moves.Add(Club);
            Add(D);
        }
        // as duas variações do goblin (o modelo do Goblin; XP, saque e missões contam como Goblin). Os três tipos
        // das imagens novas do usuário: o gordo (Brutamontes), o forte (o Goblin de sempre) e o magro (Ladino)
        {   // Brutamontes: maior, lento e resistente; porretada pesada que empurra, sem pedras
            FVortexSpeciesDef D; D.Species = TEXT("GoblinBrute"); D.Title = TEXT("GOBLIN BRUTAMONTES"); D.Model = TEXT("Goblin");
            D.Radius = 38.f; D.Height = 128.f; D.BodyScale = 1.3f; D.Speed = 300.f; D.Accel = 1100.f; D.Hits = 4.4f; D.BaseDamage = 13.f;
            D.RunClip = TEXT("Run"); D.HitSound = TEXT("creature_hit");
            FVortexMonsterMove Club = Move(TEXT("HeavyClub"), TEXT("Attack"), K::Strike, .5f, 0.f, 250.f, 260.f, 1.45f, 1.3f);
            Club.FrontDot = .1f; Club.Stun = .3f;
            D.Moves.Add(Club);
            Add(D);
        }
        {   // Ladino: menor e rápido; pica e recua, joga pedras com mais frequência
            FVortexSpeciesDef D; D.Species = TEXT("GoblinScout"); D.Title = TEXT("GOBLIN LADINO"); D.Model = TEXT("Goblin");
            D.Radius = 26.f; D.Height = 88.f; D.BodyScale = .85f; D.Speed = 520.f; D.Accel = 2200.f; D.Hits = 1.8f; D.BaseDamage = 7.f;
            D.RunClip = TEXT("Run"); D.HitSound = TEXT("creature_hit"); D.bFearful = true;
            FVortexMonsterMove Rock = Move(TEXT("Rock"), TEXT("Throw"), K::Lob, .45f, 450.f, 1400.f, 200.f, 1.f, .9f);
            Rock.OwnCooldown = 3.5f; Rock.Flight = .8f;
            D.Moves.Add(Rock);
            FVortexMonsterMove Stab = Move(TEXT("Stab"), TEXT("Attack"), K::Strike, .28f, 0.f, 200.f, 210.f, .75f, .7f);
            Stab.FrontDot = .2f; Stab.Retreat = .7f;
            D.Moves.Add(Stab);
            Add(D);
        }
        // Bandidos (ValleyMonster.Bandit.cs do Godot): o boneco do aventureiro, roupa escura e arma de
        // verdade, em bandos de 3 a 4. Golpes com os clipes da própria arma (Swing0..2 = os do DA).
        {   // espada (o capitão e o ladrão): golpes 1 e 2 alternados, o 3º pesado; de alguns metros, estocada
            FVortexSpeciesDef D; D.Species = TEXT("Bandit"); D.Title = TEXT("LADRÃO EMBOSCADOR");
            D.bHumanoid = true; D.Weapon = TEXT("espada_de_cinzas");
            D.Radius = 30.f; D.Height = 140.f; D.Speed = 430.f; D.Accel = 2000.f; D.Hits = 4.6f; D.BaseDamage = 12.f;
            D.RunClip = TEXT("Run"); D.HitSound = TEXT("creature_hit"); D.Sight = 1600.f; D.Splash = FLinearColor(.5f, .03f, .03f);
            FVortexMonsterMove Charge = Move(TEXT("Charge"), TEXT("Swing2"), K::Lunge, .45f, 380.f, 650.f, 230.f, 1.2f, 1.1f);
            Charge.LungeFrom = .12f; Charge.LungeDistance = 420.f; Charge.OwnCooldown = 4.f; Charge.FrontDot = .2f;
            D.Moves.Add(Charge);
            FVortexMonsterMove Heavy = Move(TEXT("Heavy"), TEXT("Swing2"), K::Strike, .42f, 0.f, 230.f, 250.f, 1.4f, 1.f);
            Heavy.OwnCooldown = 3.f; Heavy.FrontDot = .2f;
            D.Moves.Add(Heavy);
            FVortexMonsterMove Slash = Move(TEXT("Slash"), TEXT("Swing0"), K::Strike, .3f, 0.f, 230.f, 240.f, 1.f, .7f);
            Slash.FrontDot = .2f;
            D.Moves.Add(Slash);
            FVortexMonsterMove Slash2 = Move(TEXT("Slash2"), TEXT("Swing1"), K::Strike, .3f, 0.f, 230.f, 240.f, 1.f, .7f);
            Slash2.FrontDot = .2f; Slash2.OwnCooldown = 1.2f;
            D.Moves.Insert(Slash2, 2);
            Add(D);
            // guerreiro de escudo: a espada e o escudo; defende pela frente
            // 11/11: o cavaleiro renegado, de cota de malha (VortexMonster.cpp: BanditBlockLook)
            D.Species = TEXT("BanditShield"); D.Title = TEXT("CAVALEIRO RENEGADO"); D.Weapon = TEXT("espada_escudo"); D.bShield = true;
            D.Speed = 380.f; D.Hits = 5.2f;
            Add(D);
        }
        {   // marreta (o brutamontes, 1,45x maior): golpes lentos e fortes; o 3º é uma pancada no chão
            FVortexSpeciesDef D; D.Species = TEXT("BanditBrute"); D.Title = TEXT("SAQUEADOR BRUTAMONTES");
            // 11/11: o gordo (o corpo 1,38x mais largo que alto, VortexMonster.cpp: BanditBlockLook): 1,15 x 1,08 de altura
            D.bHumanoid = true; D.Weapon = TEXT("martelo_de_bram"); D.BodyScale = 1.15f;
            D.Radius = 48.f; D.Height = 174.f; D.Speed = 330.f; D.Accel = 1400.f; D.Hits = 6.f; D.BaseDamage = 17.f;
            D.RunClip = TEXT("Run"); D.HitSound = TEXT("creature_hit"); D.Sight = 1500.f; D.Splash = FLinearColor(.5f, .03f, .03f);
            FVortexMonsterMove Slam = Move(TEXT("Slam"), TEXT("Swing2"), K::Area, .62f, 0.f, 300.f, 320.f, 1.5f, 1.3f);
            Slam.OwnCooldown = 4.5f; Slam.bTelegraph = true;
            D.Moves.Add(Slam);
            FVortexMonsterMove Smash = Move(TEXT("Smash"), TEXT("Swing0"), K::Strike, .45f, 0.f, 280.f, 290.f, 1.f, 1.f);
            Smash.FrontDot = .2f;
            D.Moves.Add(Smash);
            Add(D);
        }
        {   // arco (o batedor): guarda distância, recua se o herói chega perto e atira flechas de verdade
            // 11/11: o pequeno (0,84 do tamanho: VortexMonster.cpp: BanditBlockLook)
            FVortexSpeciesDef D; D.Species = TEXT("BanditArcher"); D.Title = TEXT("BATEDOR BANDIDO");
            D.bHumanoid = true; D.Weapon = TEXT("arco_de_betula");
            D.Radius = 26.f; D.Height = 118.f; D.Speed = 410.f; D.Accel = 2000.f; D.Hits = 3.6f; D.BaseDamage = 11.f;
            D.RunClip = TEXT("Run"); D.HitSound = TEXT("creature_hit"); D.Sight = 2400.f; D.CloseEnough = 1200.f; D.KeepAway = 650.f;
            D.Splash = FLinearColor(.5f, .03f, .03f);
            FVortexMonsterMove Shoot = Move(TEXT("Shoot"), TEXT("Swing0"), K::Shoot, .36f, 250.f, 2300.f, 2400.f, 1.f, 1.3f);
            D.Moves.Add(Shoot);
            FVortexMonsterMove Kick = Move(TEXT("Kick"), TEXT("Swing1"), K::Strike, .3f, 0.f, 200.f, 220.f, .7f, .9f);
            Kick.FrontDot = .2f;
            D.Moves.Add(Kick);
            Add(D);
        }
        // Mortos-vivos dos cemitérios (pontos de interesse): o boneco do aventureiro na raça morto-vivo, em trapos;
        // mais lentos e mais duros que os bandidos. Um de dia guarda o mausoléu; à noite levantam os outros.
        {
            FVortexSpeciesDef D; D.Species = TEXT("Undead"); D.Title = TEXT("MORTO-VIVO ERRANTE");
            D.bHumanoid = true; D.Weapon = TEXT("espada_de_cinzas");
            D.Radius = 30.f; D.Height = 140.f; D.Speed = 300.f; D.Accel = 1500.f; D.Hits = 5.6f; D.BaseDamage = 13.f;
            D.RunClip = TEXT("Run"); D.HitSound = TEXT("creature_hit"); D.Sight = 1500.f; D.Splash = FLinearColor(.25f, .3f, .2f);
            FVortexMonsterMove Heavy = Move(TEXT("Heavy"), TEXT("Swing2"), K::Strike, .5f, 0.f, 230.f, 250.f, 1.4f, 1.1f);
            Heavy.OwnCooldown = 3.f; Heavy.FrontDot = .2f;
            D.Moves.Add(Heavy);
            FVortexMonsterMove Slash = Move(TEXT("Slash"), TEXT("Swing0"), K::Strike, .38f, 0.f, 230.f, 240.f, 1.f, .8f);
            Slash.FrontDot = .2f;
            D.Moves.Add(Slash);
            Add(D);
        }
        {
            FVortexSpeciesDef D; D.Species = TEXT("UndeadArcher"); D.Title = TEXT("ARQUEIRO ESQUELÉTICO");
            D.bHumanoid = true; D.Weapon = TEXT("arco_de_betula");
            D.Radius = 30.f; D.Height = 140.f; D.Speed = 330.f; D.Accel = 1600.f; D.Hits = 4.f; D.BaseDamage = 12.f;
            D.RunClip = TEXT("Run"); D.HitSound = TEXT("creature_hit"); D.Sight = 2400.f; D.CloseEnough = 1200.f; D.KeepAway = 600.f;
            D.Splash = FLinearColor(.25f, .3f, .2f);
            D.Moves.Add(Move(TEXT("Shoot"), TEXT("Swing0"), K::Shoot, .4f, 250.f, 2300.f, 2400.f, 1.f, 1.4f));
            FVortexMonsterMove Kick = Move(TEXT("Kick"), TEXT("Swing1"), K::Strike, .32f, 0.f, 200.f, 220.f, .7f, .9f);
            Kick.FrontDot = .2f;
            D.Moves.Add(Kick);
            Add(D);
        }
        // Monstros dos portais ("crie também esqueletos, aranhas de monstros para o portal")
        {   // aranha (modelo próprio, Tools/MakeSpiderBlender.py): rápida, sobe paredes e árvores, anda no teto das
            // dungeons e desce pendurada no fio em cima do herói; mordida e bote
            FVortexSpeciesDef D; D.Species = TEXT("Spider"); D.Title = TEXT("ARANHA DO PORTAL");
            D.bClimber = true;
            D.Radius = 55.f; D.Height = 70.f; D.Speed = 520.f; D.Accel = 2600.f; D.Hits = 3.2f; D.BaseDamage = 11.f;
            D.RunClip = TEXT("Walk"); D.HitSound = TEXT("creature_hit"); D.Sight = 1700.f; D.Splash = FLinearColor(.2f, .5f, .1f);
            FVortexMonsterMove Pounce = Move(TEXT("Pounce"), TEXT("Leap"), K::Lunge, .4f, 300.f, 700.f, 220.f, 1.2f, 1.f);
            Pounce.LungeFrom = .15f; Pounce.LungeDistance = 450.f; Pounce.OwnCooldown = 4.f; Pounce.FrontDot = .2f;
            D.Moves.Add(Pounce);
            FVortexMonsterMove Bite = Move(TEXT("Bite"), TEXT("Attack"), K::Strike, .32f, 0.f, 190.f, 210.f, 1.f, .8f);
            Bite.FrontDot = .2f;
            D.Moves.Add(Bite);
            Add(D);
        }
        {   // esqueletos: o boneco do aventureiro em osso (raça morto-vivo, tudo cor de osso); guerreiro de escudo e arqueiro
            FVortexSpeciesDef D; D.Species = TEXT("Skeleton"); D.Title = TEXT("ESQUELETO GUERREIRO");
            D.bHumanoid = true; D.Weapon = TEXT("espada_escudo"); D.bShield = true;
            D.Radius = 30.f; D.Height = 140.f; D.Speed = 360.f; D.Accel = 1800.f; D.Hits = 4.8f; D.BaseDamage = 13.f;
            D.RunClip = TEXT("Run"); D.HitSound = TEXT("creature_hit"); D.Sight = 1600.f; D.Splash = FLinearColor(.85f, .82f, .7f);
            FVortexMonsterMove Heavy = Move(TEXT("Heavy"), TEXT("Swing2"), K::Strike, .45f, 0.f, 230.f, 250.f, 1.4f, 1.f);
            Heavy.OwnCooldown = 3.f; Heavy.FrontDot = .2f;
            D.Moves.Add(Heavy);
            FVortexMonsterMove Slash = Move(TEXT("Slash"), TEXT("Swing0"), K::Strike, .32f, 0.f, 230.f, 240.f, 1.f, .7f);
            Slash.FrontDot = .2f;
            D.Moves.Add(Slash);
            Add(D);
        }
        {
            FVortexSpeciesDef D; D.Species = TEXT("SkeletonArcher"); D.Title = TEXT("ESQUELETO ARQUEIRO");
            D.bHumanoid = true; D.Weapon = TEXT("arco_de_betula");
            D.Radius = 30.f; D.Height = 140.f; D.Speed = 380.f; D.Accel = 1800.f; D.Hits = 3.6f; D.BaseDamage = 12.f;
            D.RunClip = TEXT("Run"); D.HitSound = TEXT("creature_hit"); D.Sight = 2500.f; D.CloseEnough = 1200.f; D.KeepAway = 650.f;
            D.Splash = FLinearColor(.85f, .82f, .7f);
            D.Moves.Add(Move(TEXT("Shoot"), TEXT("Swing0"), K::Shoot, .36f, 250.f, 2300.f, 2400.f, 1.f, 1.2f));
            FVortexMonsterMove Kick = Move(TEXT("Kick"), TEXT("Swing1"), K::Strike, .3f, 0.f, 200.f, 220.f, .7f, .9f);
            Kick.FrontDot = .2f;
            D.Moves.Add(Kick);
            Add(D);
        }
        {   // animais: pastam em volta do posto e fogem do herói (dá para caçar)
            FVortexSpeciesDef D; D.Species = TEXT("Rabbit"); D.Title = TEXT("COELHO");
            D.Radius = 25.f; D.Height = 77.f; D.Speed = 420.f; D.Accel = 2600.f; D.Hits = 1.2f; D.BaseDamage = 0.f;
            D.bPassive = true; D.Scare = 750.f; D.RunClip = TEXT("Run"); D.GrazeClip = TEXT("Eat"); D.Splash = FLinearColor(.5f, .05f, .05f);
            Add(D);
        }
        {
            FVortexSpeciesDef D; D.Species = TEXT("Deer"); D.Title = TEXT("CERVO");
            D.Radius = 40.f; D.Height = 213.f; D.Speed = 540.f; D.Accel = 1400.f; D.Hits = 2.6f; D.BaseDamage = 0.f;
            D.bPassive = true; D.Scare = 1300.f; D.RunClip = TEXT("Run"); D.GrazeClip = TEXT("Eat"); D.AlertClip = TEXT("Alert");
            Add(D);
        }
        {   // cavalo selvagem: manso, deixa chegar perto; só foge se apanhar
            FVortexSpeciesDef D; D.Species = TEXT("Horse"); D.Title = TEXT("CAVALO SELVAGEM");
            D.Radius = 40.f; D.Height = 165.f; D.Speed = 680.f; D.Accel = 1400.f; D.Hits = 3.f; D.BaseDamage = 0.f;
            D.bPassive = true; D.Scare = 0.f; D.RunClip = TEXT("Run"); D.GrazeClip = TEXT("Eat"); D.AlertClip = TEXT("Rear");
            Add(D);
        }
        {   // Seryth, o Escorpião de Vidro (chefe regional das dunas de Solcânion)
            FVortexSpeciesDef D; D.Species = TEXT("GlassScorpion"); D.Title = TEXT("SERYTH, O ESCORPIÃO DE VIDRO");
            D.Radius = 110.f; D.Height = 242.f; D.Speed = 300.f; D.Accel = 900.f; D.Sight = 3500.f; D.BaseDamage = 26.f;
            D.bBoss = true; D.BossHealth = 1250.f; D.Leash = 3500.f; D.DefaultLevel = 15; D.AwakenClip = TEXT("SandBurst");
            D.HitSound = TEXT("boss_impact"); D.StepSound = TEXT("boss_step"); D.Splash = FLinearColor(.7f, .85f, .95f);
            D.Telegraph = FLinearColor(.95f, .73f, .38f);
            FVortexMonsterMove Pincer = Move(TEXT("Pincer"), TEXT("Attack"), K::Pattern, 0.f, 0.f, 1200.f, 0.f, 1.f, .7f);
            Pincer.Duration = 1.45f; Pincer.Label = TEXT("PINÇAS · afaste-se da frente");
            Pincer.Marks = { Mark(1, 370.f, 330.f, .95f), Mark(1, 520.f, 220.f, 1.18f, .75f) };
            D.Moves.Add(Pincer);
            FVortexMonsterMove Tail = Move(TEXT("TailSlam"), TEXT("TailSlam"), K::Pattern, 0.f, 0.f, 9999.f, 0.f, 1.f, .7f);
            Tail.Duration = 1.9f; Tail.Label = TEXT("FERRÃO · saia do círculo"); Tail.Marks = { Mark(2, 0.f, 340.f, 1.1f, 1.4f) };
            D.Moves.Add(Tail);
            FVortexMonsterMove Sand = Move(TEXT("SandBurst"), TEXT("SandBurst"), K::Pattern, 0.f, 0.f, 9999.f, 0.f, 1.f, .7f);
            Sand.Duration = 2.25f; Sand.Label = TEXT("TEMPESTADE DE AREIA · aproxime-se ou recue");
            Sand.Marks = { Mark(0, 0.f, 700.f, 1.4f, 1.f, true), Mark(0, 0.f, 1100.f, 1.85f, 1.f, true, 2) };
            D.Moves.Add(Sand);
            FVortexMonsterMove Burrow = Move(TEXT("Burrow"), TEXT("Burrow"), K::Pattern, 0.f, 0.f, 9999.f, 0.f, 1.f, .7f);
            Burrow.Duration = 1.7f; Burrow.Rush = 1300.f; Burrow.Label = TEXT("MERGULHO NA AREIA");
            for (int32 i = 1; i <= 4; ++i) Burrow.Marks.Add(Mark(1, i * 320.f, 260.f, .7f + i * .19f));
            D.Moves.Add(Burrow);
            Add(D);
        }
        {   // Thorun, o Titã das Raízes (chefe regional da mata de Verdalume)
            FVortexSpeciesDef D; D.Species = TEXT("RootTitan"); D.Title = TEXT("THORUN, O TITÃ DAS RAÍZES");
            D.Radius = 110.f; D.Height = 346.f; D.Speed = 280.f; D.Accel = 800.f; D.Sight = 5400.f; D.BaseDamage = 32.f;
            D.bBoss = true; D.BossHealth = 1650.f; D.Leash = 5400.f; D.DefaultLevel = 19; D.AwakenClip = TEXT("VineSnare");
            D.HitSound = TEXT("boss_impact"); D.StepSound = TEXT("boss_step"); D.Splash = FLinearColor(.35f, .25f, .12f);
            D.Telegraph = FLinearColor(.41f, .86f, .64f);
            FVortexMonsterMove Cleave = Move(TEXT("RootCleave"), TEXT("RootCleave"), K::Pattern, 0.f, 0.f, 1200.f, 0.f, 1.f, .7f);
            Cleave.Duration = 1.6f; Cleave.Label = TEXT("GOLPE DE RAÍZES"); Cleave.Marks = { Mark(1, 420.f, 360.f, .92f) };
            D.Moves.Add(Cleave);
            FVortexMonsterMove Snare = Move(TEXT("VineSnare"), TEXT("VineSnare"), K::Pattern, 0.f, 0.f, 9999.f, 0.f, 1.f, .7f);
            Snare.Duration = 2.f; Snare.Label = TEXT("VINHAS · saia do selo");
            Snare.Marks = { Mark(2, 0.f, 320.f, 1.35f), Mark(2, 400.f, 280.f, 1.75f, 1.f, false, 2, 86.f) };
            D.Moves.Add(Snare);
            FVortexMonsterMove Spores = Move(TEXT("SporeRing"), TEXT("SporeRing"), K::Pattern, 0.f, 0.f, 9999.f, 0.f, 1.f, .7f);
            Spores.Duration = 2.25f; Spores.Label = TEXT("ESPOROS · saia do anel"); Spores.bMagic = true;
            Spores.Marks = { Mark(0, 0.f, 680.f, 1.4f, 1.f, true), Mark(0, 0.f, 1000.f, 1.9f, 1.f, true, 3) };
            D.Moves.Add(Spores);
            FVortexMonsterMove Rush = Move(TEXT("RootRush"), TEXT("RootRush"), K::Pattern, 0.f, 0.f, 9999.f, 0.f, 1.f, .7f);
            Rush.Duration = 1.65f; Rush.Rush = 1550.f; Rush.Label = TEXT("INVESTIDA DO TITÃ");
            for (int32 i = 1; i <= 5; ++i) Rush.Marks.Add(Mark(1, i * 310.f, 270.f, .72f + i * .16f));
            D.Moves.Add(Rush);
            Add(D);
        }
        {   // Azhûr, o Colosso da Caldeira (o chefe da cratera; o Godot tem 18 golpes, aqui os principais)
            FVortexSpeciesDef D; D.Species = TEXT("RuneColossus"); D.Title = TEXT("AZHÛR, O COLOSSO DA CALDEIRA");
            // o modelo refeito no Vortex (Tools/Blender/BuildAzhur.py): 7 m de pedra e lava
            D.Radius = 175.f; D.Height = 700.f; D.Speed = 190.f; D.Accel = 500.f; D.Sight = 4500.f; D.BaseDamage = 48.f;
            D.bBoss = true; D.BossHealth = 3400.f; D.Leash = 4500.f; D.DefaultLevel = 25; D.AwakenClip = TEXT("Awaken");
            // 02/11: os sons próprios do golem vulcânico (Tools/MakeAzhurSfx.py)
            D.HitSound = TEXT("azhur_hurt"); D.StepSound = TEXT("azhur_step"); D.Splash = FLinearColor(.9f, .3f, .05f);
            D.Telegraph = FLinearColor(1.f, .35f, .05f);
            FVortexMonsterMove Punch = Move(TEXT("Punch"), TEXT("Punch"), K::Pattern, 0.f, 0.f, 1400.f, 0.f, 1.f, .8f);
            Punch.Duration = 1.5f; Punch.Label = TEXT("SOCO DE BASALTO · saia da frente"); Punch.Marks = { Mark(1, 480.f, 420.f, 1.f) };
            D.Moves.Add(Punch);
            FVortexMonsterMove Stomp = Move(TEXT("Stomp"), TEXT("Stomp"), K::Pattern, 0.f, 0.f, 900.f, 0.f, 1.f, .8f);
            Stomp.Duration = 1.6f; Stomp.Label = TEXT("PISÃO · afaste-se dele"); Stomp.Marks = { Mark(0, 0.f, 650.f, 1.15f, .9f) };
            D.Moves.Add(Stomp);
            FVortexMonsterMove Throw = Move(TEXT("Throw"), TEXT("Throw"), K::Pattern, 0.f, 0.f, 9999.f, 0.f, 1.f, .8f);
            Throw.Duration = 1.9f; Throw.Label = TEXT("ROCHA EM BRASA · saia do círculo");
            Throw.Marks = { Mark(2, 0.f, 400.f, 1.6f, 1.1f), Mark(2, 500.f, 320.f, 1.9f, .8f, false, 2, 70.f), Mark(2, 500.f, 320.f, 2.1f, .8f, false, 3, -70.f) };
            D.Moves.Add(Throw);
            FVortexMonsterMove Seismic = Move(TEXT("Seismic"), TEXT("Seismic"), K::Pattern, 0.f, 0.f, 9999.f, 0.f, 1.f, .8f);
            Seismic.Duration = 2.3f; Seismic.Label = TEXT("ONDA SÍSMICA · fique perto dele ou longe");
            Seismic.Marks = { Mark(0, 0.f, 1000.f, 1.5f, 1.f, true), Mark(0, 0.f, 1500.f, 2.f, 1.f, true, 2) };
            D.Moves.Add(Seismic);
            FVortexMonsterMove Charge = Move(TEXT("Charge"), TEXT("Charge"), K::Pattern, 0.f, 0.f, 9999.f, 0.f, 1.f, .8f);
            Charge.Duration = 1.8f; Charge.Rush = 1600.f; Charge.Label = TEXT("INVESTIDA · saia do caminho");
            for (int32 i = 1; i <= 5; ++i) Charge.Marks.Add(Mark(1, i * 320.f, 300.f, .7f + i * .17f));
            D.Moves.Add(Charge);
            // chuva de meteoros: rochas em brasa caem nas marcas (em volta do herói) e cada uma vira lava
            FVortexMonsterMove Meteor = Move(TEXT("Meteor"), TEXT("Throw"), K::Pattern, 0.f, 0.f, 9999.f, 0.f, 1.f, .8f);
            Meteor.Duration = 2.6f; Meteor.bLeavesLava = true; Meteor.bMagic = true;
            Meteor.Label = TEXT("CHUVA DE METEOROS · fuja das marcas (a lava fica)");
            Meteor.Marks = { Mark(2, 0.f, 330.f, 1.6f, 1.f), Mark(2, 520.f, 280.f, 1.85f, .9f, false, 1, 60.f), Mark(2, 520.f, 280.f, 2.05f, .9f, false, 1, -60.f),
                             Mark(2, 780.f, 280.f, 2.25f, .9f, false, 2, 150.f), Mark(2, 780.f, 280.f, 2.4f, .9f, false, 3, -150.f) };
            D.Moves.Add(Meteor);
            FVortexMonsterMove Cata = Move(TEXT("Cataclysm"), TEXT("Cataclysm"), K::Pattern, 0.f, 0.f, 9999.f, 0.f, 1.f, 1.2f);
            Cata.Duration = 2.8f; Cata.Label = TEXT("CATACLISMO · corra para fora");
            Cata.Marks = { Mark(0, 0.f, 800.f, 1.6f, 1.2f), Mark(2, 0.f, 380.f, 2.f, 1.f, false, 3), Mark(0, 0.f, 1600.f, 2.4f, 1.f, true, 3) };
            D.Moves.Add(Cata);
            Add(D);
        }
        return All;
    }

    const TMap<FName, FVortexSpeciesDef>& SpeciesTable()
    {
        static const TMap<FName, FVortexSpeciesDef> Defs = Build();
        return Defs;
    }
}

const FVortexSpeciesDef* FVortexSpeciesDef::Find(FName Species)
{
    // as pelagens do cavalo (Horse_Baio...) usam a ficha do cavalo
    if (Species.ToString().StartsWith(TEXT("Horse_"))) Species = TEXT("Horse");
    return SpeciesTable().Find(Species);
}

const TArray<FName>& FVortexSpeciesDef::All()
{
    static TArray<FName> Names = []()
    {
        TArray<FName> Out;
        SpeciesTable().GetKeys(Out);
        return Out;
    }();
    return Names;
}
