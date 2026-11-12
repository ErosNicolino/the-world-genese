#include "Framework/VortexAudio.h"
#include "Misc/PackageName.h"
#include "World/VortexCaldera.h"
#include "Framework/Application/SlateApplication.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/AudioComponent.h"
#include "Creatures/VortexWolf.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/VortexMenuStage.h"
#include "Framework/VortexCharacter.h"
#include "Combat/VortexWeaponComponent.h"
#include "Combat/VortexWeaponDefinition.h"
#include "Combat/VortexWeaponKit.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"
#include "Creatures/VortexCreature.h"
#include "Framework/VortexPlayerSettings.h"
#include "World/VortexDayNightSubsystem.h"
#include "World/VortexTerrainStreamer.h"

namespace
{
    bool bCutscene = false;   // VortexAudio::SetCutscene: o mundo quieto enquanto uma cena toca

    TMap<FName, TWeakObjectPtr<USoundBase>>& SoundCache()
    {
        static TMap<FName, TWeakObjectPtr<USoundBase>> Cache;
        return Cache;
    }

    /** 08/11: os sons pedidos na pré-carga e que ainda não chegaram (tocar um deles agora esperaria a fila inteira). */
    TSet<FName>& PendingSounds()
    {
        static TSet<FName> Pending;
        return Pending;
    }

    USoundBase* Load(FName Name)
    {
        TMap<FName, TWeakObjectPtr<USoundBase>>& Cache = SoundCache();
        if (TWeakObjectPtr<USoundBase>* Found = Cache.Find(Name); Found && Found->IsValid()) return Found->Get();
        // ainda chegando da pré-carga: desta vez não toca (pedir agora travava o quadro esperando a fila)
        if (PendingSounds().Contains(Name)) return nullptr;
        // som que não existe (ex.: as variações wfx_<família>_hit_N que caem no som comum): procurar no disco
        // a cada golpe custava uma busca e um aviso no log
        static TSet<FName> Missing;
        if (Missing.Contains(Name)) return nullptr;
        const FString Text = Name.ToString();
        // <nome>_v1.._v4: as gravações com variações (Sfx/Recorded)
        const bool bTake = Text.Len() > 3 && Text[Text.Len() - 3] == TEXT('_') && Text[Text.Len() - 2] == TEXT('v') && FChar::IsDigit(Text[Text.Len() - 1]);
        const FString Folder = bTake ? TEXT("Sfx/Recorded") : Text.StartsWith(TEXT("MUS_")) ? TEXT("Music")
            : Text.StartsWith(TEXT("ui_")) || Text == TEXT("equip") ? TEXT("UI") : TEXT("Sfx");
        // 07/11: antes de carregar, confere se o pacote existe: o LoadObject de um som que não existe (as tomadas _v1 que
        // não há) esvaziava a fila de carregamento inteira (FlushAsyncLoading com 200+ pacotes: travadas de 400 ms)
        const FString Package = FString::Printf(TEXT("/Game/Vortex/Audio/%s/%s"), *Folder, *Text);
        if (!FPackageName::DoesPackageExist(Package)) { Missing.Add(Name); Cache.Add(Name, nullptr); return nullptr; }
        USoundBase* Sound = LoadObject<USoundBase>(nullptr, *FString::Printf(TEXT("%s.%s"), *Package, *Text));
        if (Sound) Sound->AddToRoot();   // sons curtos e poucos: ficam carregados
        else Missing.Add(Name);
        Cache.Add(Name, Sound);
        return Sound;
    }

    USoundAttenuation* MakeFalloff(float Inner, float Distance)
    {
        USoundAttenuation* Settings = NewObject<USoundAttenuation>(GetTransientPackage());
        Settings->AddToRoot();
        Settings->Attenuation.bAttenuate = true;
        Settings->Attenuation.bSpatialize = true;
        Settings->Attenuation.AttenuationShape = EAttenuationShape::Sphere;
        Settings->Attenuation.AttenuationShapeExtents = FVector(Inner);
        Settings->Attenuation.FalloffDistance = Distance;
        // de longe soa mais abafado (o ar come os agudos), como na vida
        Settings->Attenuation.bAttenuateWithLPF = true;
        Settings->Attenuation.LPFRadiusMin = Inner + Distance * .25f;
        Settings->Attenuation.LPFRadiusMax = Inner + Distance;
        Settings->Attenuation.LPFFrequencyAtMax = 2500.f;
        return Settings;
    }

    /** A distância pelo tipo: passos e bichinhos perto; a maioria ~30 m; chefes, trovões e explosões longe. */
    USoundAttenuation* Falloff(FName Sound)
    {
        static USoundAttenuation* Near = nullptr;
        static USoundAttenuation* Normal = nullptr;
        static USoundAttenuation* Far = nullptr;
        if (!Normal) { Near = MakeFalloff(150.f, 1450.f); Normal = MakeFalloff(250.f, 2750.f); Far = MakeFalloff(600.f, 6500.f); }
        const FString Text = Sound.ToString();
        if (Text.StartsWith(TEXT("step_")) || Text.StartsWith(TEXT("bird_chirp")) || Text == TEXT("fish_plop") || Text == TEXT("swim_stroke")) return Near;
        if (Text.StartsWith(TEXT("boss_")) || Text == TEXT("thunder") || Text == TEXT("meteor") || Text == TEXT("lightning") || Text == TEXT("wolf_howl")
            || Text == TEXT("town_bell") || Text.StartsWith(TEXT("portal_"))) return Far;
        return Normal;
    }

    /** As tomadas de um som (Sfx/Recorded/<nome>_v1..): quantas há (0: só o som simples). */
    int32 Takes(FName Name)
    {
        static TMap<FName, int32> Counts;
        if (const int32* Found = Counts.Find(Name)) return *Found;
        int32 Count = 0;
        while (Count < 8 && Load(FName(*FString::Printf(TEXT("%s_v%d"), *Name.ToString(), Count + 1)))) ++Count;
        Counts.Add(Name, Count);
        return Count;
    }

    /** O som a tocar: uma das tomadas gravadas (nunca a mesma duas vezes seguidas) ou o som simples. */
    USoundBase* Pick(FName Name)
    {
        const int32 Count = Takes(Name);
        if (Count == 0) return Load(Name);
        static TMap<FName, int32> Last;
        int32& Previous = Last.FindOrAdd(Name, -1);
        int32 Take = FMath::RandRange(0, Count - 1);
        if (Count > 1 && Take == Previous) Take = (Take + 1 + FMath::RandRange(0, Count - 2)) % Count;
        Previous = Take;
        return Load(FName(*FString::Printf(TEXT("%s_v%d"), *Name.ToString(), Take + 1)));
    }

    /** O mesmo som no mesmo instante: no máximo 4 em 0,12 s (o resto só embolaria e estouraria). */
    bool Crowded(FName Name)
    {
        static TMap<FName, TArray<double>> Recent;
        const double Now = FPlatformTime::Seconds();
        TArray<double>& Times = Recent.FindOrAdd(Name);
        Times.RemoveAll([Now](double T) { return Now - T > .12; });
        if (Times.Num() >= 4) return true;
        Times.Add(Now);
        return false;
    }

    bool CanHear(const UObject* Context)
    {
        const UWorld* World = Context ? Context->GetWorld() : nullptr;
        return World && World->GetNetMode() != NM_DedicatedServer;
    }
}

void VortexAudio::Play(const UObject* Context, FName Sound, const FVector& At, float Volume, float PitchJitter)
{
    if (!CanHear(Context) || Crowded(Sound)) return;
    if (USoundBase* Wave = Pick(Sound))
    {
        UGameplayStatics::PlaySoundAtLocation(Context, Wave, At, Volume, 1.f + FMath::FRandRange(-PitchJitter, PitchJitter), 0.f, Falloff(Sound));
    }
}

bool VortexAudio::Has(FName Sound)
{
    return Load(Sound) != nullptr || Takes(Sound) > 0;
}

FName VortexAudio::CreatureVoice(FName Species, const TCHAR* Event)
{
    // as vozes gravadas (Godot recorded): lobo, gota, golem, cogumelo, morcego, cervo, coelho, bandido e cavalo; 03/11
    // (Tools/MakeCombatSfx.py): goblin, esqueleto, morto-vivo, aranha e sapo
    static const TCHAR* Voices[] = { TEXT("wolf"), TEXT("slime"), TEXT("golem"), TEXT("mushroom"), TEXT("bat"), TEXT("deer"), TEXT("rabbit"), TEXT("bandit"), TEXT("horse"),
        TEXT("goblin"), TEXT("skeleton"), TEXT("undead"), TEXT("spider"), TEXT("frog") };
    const FString Lower = Species.ToString().ToLower();
    for (const TCHAR* Voice : Voices)
    {
        if (!Lower.StartsWith(Voice)) continue;
        const FName Name(*FString::Printf(TEXT("%s_%s"), Voice, Event));
        return Has(Name) ? Name : NAME_None;
    }
    return NAME_None;
}

FName VortexAudio::WeaponSound(const AActor* Hero, const TCHAR* Kind, int32 Variant, FName Fallback)
{
    const AVortexCharacter* Character = Cast<AVortexCharacter>(Hero);
    const UVortexWeaponComponent* Weapon = Character ? Character->GetWeapon() : nullptr;
    const UVortexWeaponDefinition* Definition = Weapon ? Weapon->GetDefinition() : nullptr;
    const VortexWeaponKit::FKit* Kit = Definition ? VortexWeaponKit::Find(Definition->WeaponId) : nullptr;
    if (!Kit) return Fallback;
    const FString Family = Kit->Family.ToString().ToLower();
    if (Variant >= 0)
    {
        const FName Numbered(*FString::Printf(TEXT("wfx_%s_%s_%d"), *Family, Kind, 1 + Variant % 3));
        if (Has(Numbered)) return Numbered;
    }
    const FName Plain(*FString::Printf(TEXT("wfx_%s_%s"), *Family, Kind));
    return Has(Plain) ? Plain : Fallback;
}

uint64& VortexAudio::UiSoundCount()
{
    static uint64 Count = 0;
    return Count;
}

double& VortexAudio::LastUiSoundAt()
{
    static double At = -1.0;
    return At;
}

void VortexAudio::Play2D(const UObject* Context, FName Sound, float Volume)
{
    if (!CanHear(Context)) return;
    // os sons antigos da interface viram os cartoon (Tools/MakeUiSoundsCartoon.py) em todas as telas de uma vez;
    // o clique baixinho que as telas tocam no "passar o mouse" vira o tique do mouse
    static const FName Click(TEXT("ui_click")), Open(TEXT("ui_open")), Close(TEXT("ui_close")), Error(TEXT("ui_error"));
    FName Cartoon;
    if (Sound == Click) Cartoon = Volume < .3f ? FName(TEXT("ui_c_hover")) : FName(TEXT("ui_c_pop"));
    else if (Sound == Open) Cartoon = Volume < .3f ? FName(TEXT("ui_c_hover")) : FName(TEXT("ui_c_open"));
    else if (Sound == Close) Cartoon = TEXT("ui_c_close");
    else if (Sound == Error) Cartoon = TEXT("ui_c_error");
    if (!Cartoon.IsNone() && Has(Cartoon)) Sound = Cartoon;
    // um som de interface tocado agora: o botão de borracha não toca o "pop" por cima
    if (Sound.ToString().StartsWith(TEXT("ui_"))) { LastUiSoundAt() = FSlateApplication::IsInitialized() ? FSlateApplication::Get().GetCurrentTime() : FPlatformTime::Seconds(); ++UiSoundCount(); }
    if (USoundBase* Wave = Pick(Sound)) UGameplayStatics::PlaySound2D(Context, Wave, Volume);
}

void VortexAudio::ApplyVolumes(const UObject* Context)
{
    if (!CanHear(Context)) return;
    static USoundMix* Mix = nullptr;
    static USoundClass* Classes[5] = {};
    if (!Mix)
    {
        Mix = LoadObject<USoundMix>(nullptr, TEXT("/Game/Vortex/Audio/SM_VortexVolumes.SM_VortexVolumes"));
        Classes[0] = LoadObject<USoundClass>(nullptr, TEXT("/Engine/EngineSounds/Master.Master"));
        const TCHAR* Names[] = { TEXT("Music"), TEXT("Effects"), TEXT("Ambience"), TEXT("Interface") };
        for (int32 i = 0; i < 4; ++i)
        {
            Classes[i + 1] = LoadObject<USoundClass>(nullptr, *FString::Printf(TEXT("/Game/Vortex/Audio/SC_%s.SC_%s"), Names[i], Names[i]));
        }
        if (Mix) Mix->AddToRoot();
        for (USoundClass* Class : Classes) if (Class) Class->AddToRoot();
    }
    if (!Mix) return;
    const UVortexPlayerSettings* S = UVortexPlayerSettings::Get();
    const float Master = FMath::Clamp(S->VolumeMaster, 0.f, 1.f);
    // numa cena (a HQ de abertura) o mundo fica quieto: sem ambiente (pássaros, vento) e efeitos quase mudos
    const float World = bCutscene ? .04f : 1.f;
    const float Values[5] = { Master, S->VolumeMusic * Master, S->VolumeEffects * Master * World, S->VolumeAmbience * Master * (bCutscene ? 0.f : 1.f), S->VolumeInterface * Master };
    for (int32 i = 0; i < 5; ++i)
    {
        // o geral vale também para o que não tem categoria (sons da engine e de pacotes)
        if (Classes[i]) UGameplayStatics::SetSoundMixClassOverride(Context, Mix, Classes[i], FMath::Clamp(Values[i], 0.f, 1.f), 1.f, bCutscene ? 1.2f : .1f, i == 0 ? false : true);
    }
    UGameplayStatics::PushSoundMixModifier(Context, Mix);
}

void VortexAudio::SetCutscene(const UObject* Context, bool bOn)
{
    bCutscene = bOn;
    ApplyVolumes(Context);
}

void VortexAudio::Preload()
{
    // os efeitos e os sons da interface em segundo plano: carregar cada um na primeira vez que toca
    // segurava o quadro (e esperava tudo o que estivesse carregando junto)
    static bool bStarted = false;
    if (bStarted) return;
    bStarted = true;
    IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
    for (const TCHAR* Path : { TEXT("/Game/Vortex/Audio/Sfx"), TEXT("/Game/Vortex/Audio/UI") })
    {
        TArray<FAssetData> Assets;
        Registry.GetAssetsByPath(FName(Path), Assets, true);
        for (const FAssetData& Asset : Assets)
        {
            const FName Name = Asset.AssetName;
            if (SoundCache().Contains(Name)) continue;
            PendingSounds().Add(Name);
            LoadPackageAsync(Asset.PackageName.ToString(), FLoadPackageAsyncDelegate::CreateLambda([Name](const FName&, UPackage* Loaded, EAsyncLoadingResult::Type)
            {
                PendingSounds().Remove(Name);
                if (!Loaded) return;
                if (USoundBase* Sound = FindObject<USoundBase>(Loaded, *Name.ToString()))
                {
                    Sound->AddToRoot();
                    SoundCache().Add(Name, Sound);
                }
            }));
        }
    }
}

FName VortexAudio::StepSound(const FVector& Feet, int32 Variant)
{
    FString Set = TEXT("grass");
    if (const TSharedPtr<FVortexTerrainData> Data = AVortexTerrainStreamer::SharedData())
    {
        const int32 X = FMath::FloorToInt(Feet.X / Data->CellSize), Y = FMath::FloorToInt(Feet.Y / Data->CellSize);
        if (Data->HasCell(X, Y))
        {
            // a cor da célula diz o chão: verde = grama, amarelada clara = areia, cinza = pedra, marrom escuro = lama
            const FLinearColor C = FLinearColor::FromSRGBColor(Data->CellColor(X, Y));
            const float Lum = C.GetLuminance();
            float Surface = 0.f;
            // água rasa por cima de tudo (chapinhar); neve: branco quase sem cor
            if (Data->IsWaterAt(Feet.X, Feet.Y, Surface) && Surface > Feet.Z - 5.f) Set = TEXT("water");
            else if (Lum > .6f && FMath::Max3(C.R, C.G, C.B) - FMath::Min3(C.R, C.G, C.B) < .12f) Set = TEXT("snow");
            else if (C.G > C.R * 1.05f && C.G > C.B * 1.15f) Set = TEXT("grass");
            else if (C.R > C.B * 1.3f && Lum > .25f) Set = TEXT("sand");
            else if (C.R > C.B * 1.2f && Lum <= .25f) Set = TEXT("mud");
            else Set = TEXT("stone");
        }
    }
    return FName(*FString::Printf(TEXT("step_%s_%d"), *Set, 1 + FMath::Abs(Variant) % 3));
}

// ---- música ---------------------------------------------------------------------------------

bool UVortexMusicSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    const UWorld* World = Cast<UWorld>(Outer);
    return World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE) && World->GetNetMode() != NM_DedicatedServer &&
        !FParse::Param(FCommandLine::Get(), TEXT("VortexNoMusic"));
}

TStatId UVortexMusicSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UVortexMusicSubsystem, STATGROUP_Tickables);
}

void UVortexMusicSubsystem::SetDucked(bool bOn)
{
    if (Playing) Playing->AdjustVolume(1.5f, bOn ? .05f : 1.f);
}

void UVortexMusicSubsystem::Switch(FName NewTrack)
{
    if (NewTrack == Track) return;
    Track = NewTrack;
    if (Playing) Playing->FadeOut(2.5f, 0.f);
    Playing = nullptr;
    if (NewTrack.IsNone()) return;
    if (USoundBase* Wave = Load(NewTrack))
    {
        Playing = UGameplayStatics::CreateSound2D(GetWorld(), Wave, MusicVolume, 1.f, 0.f, nullptr, false, false);
        if (Playing) Playing->FadeIn(2.5f, NewTrack == TEXT("MUS_Combat") ? MusicVolume * 1.2f : MusicVolume);
    }
}

void UVortexMusicSubsystem::Tick(float DeltaTime)
{
    CheckTime -= DeltaTime;
    CombatHold = FMath::Max(0.f, CombatHold - DeltaTime);
    if (CheckTime > 0.f) return;
    CheckTime = .5f;
    UWorld* World = GetWorld();
    // os sons em segundo plano, só no mundo de verdade (os mapas de teste não precisam)
    if (!bPreloaded && World)
        if (TActorIterator<AVortexTerrainStreamer> It(World); It) { bPreloaded = true; VortexAudio::Preload(); }
    // o menu tem a própria música; só toca onde há terreno (o mundo do jogo)
    if (TActorIterator<AVortexMenuStage>(World) || !TActorIterator<AVortexTerrainStreamer>(World)) { Switch(NAME_None); return; }
    const APlayerController* Controller = World->GetFirstPlayerController();
    const APawn* Hero = Controller ? Controller->GetPawn() : nullptr;
    if (!Hero) return;
    const FVector Here = Hero->GetActorLocation();
    // qualquer criatura brigando perto (lobo, gota, golem, chefe...) chama a música de luta
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        const IVortexCreature* Creature = Cast<IVortexCreature>(*It);
        if (!Creature || Creature->IsCreatureDead() || Creature->IsPassive() || !Creature->IsEngaged()) continue;
        if (FVector::DistSquared(It->GetActorLocation(), Here) < FMath::Square(Creature->IsBoss() ? 6000.f : 2500.f)) CombatHold = 6.f;
    }
    const UVortexDayNightSubsystem* DayNight = World->GetSubsystem<UVortexDayNightSubsystem>();
    const bool bNight = DayNight && DayNight->IsActive() && DayNight->GetDaylight() < .3f;
    const bool bVillage = FVector::Dist2D(Here, FVector(0.f, 1500.f, 0.f)) < 4500.f;
    // 03/11: dentro da cratera do Azhûr, o tema dele (Docs/musicas futuras -> Data/Audio/MUS_Azhur.wav), na luta ou não
    const bool bCrater = FVector::Dist2D(Here, FVector(UVortexCalderaSubsystem::CenterX, UVortexCalderaSubsystem::CenterY, 0.f)) < 5200.f;
    Switch(bCrater ? TEXT("MUS_Azhur") : CombatHold > 0.f ? TEXT("MUS_Combat") : bVillage ? TEXT("MUS_Village") : bNight ? TEXT("MUS_Night") : TEXT("MUS_Theme"));
}
