#include "Framework/VortexGraphics.h"
#include "Framework/VortexHudBase.h"
#if VORTEX_WITH_DLSS
#include "DLSSLibrary.h"
#endif

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/VortexAudio.h"
#include "Framework/VortexCharacter.h"
#include "Framework/VortexPlayerSettings.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/CommandLine.h"
#include "Containers/Ticker.h"
#include "Misc/Parse.h"
#include "RHIGlobals.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "World/VortexTerrainStreamer.h"

namespace VortexGraphics
{
    namespace
    {
        // 11/11 (o usuário: "organize melhor, separe"): Tela, Qualidade, Luz e efeitos, Desempenho, Áudio, Controles, Interface
        const TCHAR* TabScreen = TEXT("Tela");
        const TCHAR* TabGraphics = TEXT("Qualidade");
        const TCHAR* TabShaders = TEXT("Luz e efeitos");
        const TCHAR* TabPerformance = TEXT("Desempenho");
        const TCHAR* TabControls = TEXT("Controles");
        const TCHAR* TabInterface = TEXT("Interface");
        const TCHAR* TabAudio = TEXT("Áudio");

        const TCHAR* Levels[] = { TEXT("Baixa"), TEXT("Média"), TEXT("Alta"), TEXT("Épica"), TEXT("Cinemática") };
        const float FrameLimits[] = { 0.f, 30.f, 60.f, 90.f, 120.f, 144.f, 165.f, 240.f };
        const TCHAR* OnOff(bool b) { return b ? TEXT("Ligado") : TEXT("Desligado"); }

        // 11/11 (o usuário: "às vezes consigo só mudar e sair e já está aplicado, e trava"): o mundo lê as opções a toda
        // hora (grama, distância, água), então o painel edita uma cópia (Draft) e só o Aplicar passa para as de verdade
        bool bEditing = false, bPending = false, bCommitQueued = false, bApplying = false;
        TStrongObjectPtr<UVortexPlayerSettings> Draft;
        float PendingFirstFov = -1.f;     // o campo de visão da 1ª pessoa fica no GameUserSettings.ini, fora do objeto
        double ApplyStarted = 0.0;
        uint64 AppliedFrame = 0;

        UVortexPlayerSettings* Real() { return UVortexPlayerSettings::Get(); }
        /** O que as opções mostram e mudam: a cópia enquanto o painel está aberto. */
        UVortexPlayerSettings* Settings() { return (bEditing || bCommitQueued) && Draft ? Draft.Get() : Real(); }

        void CopySettings(UVortexPlayerSettings* To, const UVortexPlayerSettings* From)
        {
            for (TFieldIterator<FProperty> It(UVortexPlayerSettings::StaticClass()); It; ++It) It->CopyCompleteValue_InContainer(To, From);
        }

        void SyncDraft()
        {
            if (!Draft) Draft.Reset(NewObject<UVortexPlayerSettings>(GetTransientPackage()));
            CopySettings(Draft.Get(), Real());
            PendingFirstFov = -1.f;
        }
        UGameUserSettings* Video() { return GEngine ? GEngine->GetGameUserSettings() : nullptr; }
        int32 Wrap(int32 Value, int32 Count) { return (Value % Count + Count) % Count; }

        void SetCVar(const TCHAR* Name, float Value)
        {
            if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(Name)) Var->Set(*FString::SanitizeFloat(Value), ECVF_SetByCode);
        }

        UWorld* GameWorld()
        {
            if (!GEngine) return nullptr;
            for (const FWorldContext& Context : GEngine->GetWorldContexts())
            {
                if (Context.World() && (Context.WorldType == EWorldType::Game || Context.WorldType == EWorldType::PIE)) return Context.World();
            }
            return nullptr;
        }

        void Commit(bool bVideo = false)
        {
            // com o painel de opções aberto: só marca como pendente (o Aplicar grava e aplica)
            if (bEditing)
            {
                bPending = true;
                return;
            }
            Real()->SaveConfig();
            if (bVideo && Video())
            {
                Video()->ApplySettings(false);
                Video()->SaveSettings();
            }
            Apply();
            if (GEngine && GEngine->GameViewport) VortexAudio::ApplyVolumes(GEngine->GameViewport->GetWorld());
        }

        // ---- qualidade geral: um pacote que não mexe na distância -----------------------------
        using FGetter = int32 (UGameUserSettings::*)() const;
        using FSetter = void (UGameUserSettings::*)(int32);
        struct FGroup { FGetter Get; FSetter Set; };
        const FGroup PresetGroups[] = {
            { &UGameUserSettings::GetShadowQuality, &UGameUserSettings::SetShadowQuality },
            { &UGameUserSettings::GetGlobalIlluminationQuality, &UGameUserSettings::SetGlobalIlluminationQuality },
            { &UGameUserSettings::GetReflectionQuality, &UGameUserSettings::SetReflectionQuality },
            { &UGameUserSettings::GetTextureQuality, &UGameUserSettings::SetTextureQuality },
            { &UGameUserSettings::GetVisualEffectQuality, &UGameUserSettings::SetVisualEffectQuality },
            { &UGameUserSettings::GetPostProcessingQuality, &UGameUserSettings::SetPostProcessingQuality },
            { &UGameUserSettings::GetShadingQuality, &UGameUserSettings::SetShadingQuality },
            { &UGameUserSettings::GetFoliageQuality, &UGameUserSettings::SetFoliageQuality },
            { &UGameUserSettings::GetAntiAliasingQuality, &UGameUserSettings::SetAntiAliasingQuality },
        };

        // vegetação de cada nível da qualidade geral (alta e épica: toda)
        const int32 DensityFor[] = { 0, 1, 3, 3 };

        int32 PresetLevel()
        {
            const UGameUserSettings* V = Video();
            const UVortexPlayerSettings* S = Settings();
            if (!V) return -1;
            const int32 L = (V->*PresetGroups[0].Get)();
            for (const FGroup& G : PresetGroups) if ((V->*G.Get)() != L) return -1;
            if (L < 0 || L > 3) return -1;
            if (S->GrassDensity != DensityFor[L] || S->WaterQuality != L || S->bAmbientOcclusion != (L >= 1)) return -1;
            if (S->bShaderGI != (L >= 2) || S->bShaderReflections != (L >= 2)) return -1;
            return L;
        }

        void SetPreset(int32 L)
        {
            UGameUserSettings* V = Video();
            UVortexPlayerSettings* S = Settings();
            if (V) for (const FGroup& G : PresetGroups) (V->*G.Set)(L);
            S->GrassDensity = DensityFor[L];
            S->WaterQuality = L;
            S->bAmbientOcclusion = L >= 1;
            // 11/11 (auditoria de FPS): a luz global e os reflexos do Lumen são o que mais pesa (5 ms de placa e 4,7 ms da
            // linha de desenho na praça); sem eles a Baixa dava o mesmo FPS que a Épica
            S->bShaderGI = S->bShaderReflections = L >= 2;
            // suavização das bordas pelo preset: FXAA (Baixa), TAA (Média), TSR (Alta e Épica)
            S->AntiAliasingMethod = L <= 0 ? 2 : L == 1 ? 1 : 0;
            Commit(true);
        }

        // ---- montagem das linhas -----------------------------------------------------------------
        FOption Make(const FString& Tab, const FString& Section, const FString& Name, const FString& Description,
            const FString& Value, int32 Index, int32 Count, TFunction<void(int32)> Step)
        {
            FOption O;
            O.Tab = Tab; O.Section = Section; O.Name = Name; O.Description = Description;
            O.Value = Value; O.Index = Index; O.Count = Count; O.Step = MoveTemp(Step);
            return O;
        }

        FOption Group(const FString& Tab, const FString& Section, const FString& Name, const FString& Description, FGetter Get, FSetter Set)
        {
            const UGameUserSettings* V = Video();
            const int32 Current = V ? FMath::Clamp((V->*Get)(), 0, 4) : 2;
            return Make(Tab, Section, Name, Description, Levels[Current], Current, 5, [Get, Set](int32 D)
            {
                if (UGameUserSettings* Vid = Video()) { (Vid->*Set)(FMath::Clamp(FMath::Clamp((Vid->*Get)(), 0, 4) + D, 0, 4)); Commit(true); }
            });
        }

        FOption Toggle(const FString& Tab, const FString& Section, const FString& Name, const FString& Description, bool UVortexPlayerSettings::*Field)
        {
            const bool b = Settings()->*Field;
            return Make(Tab, Section, Name, Description, OnOff(b), b ? 1 : 0, 2, [Field](int32) { Settings()->*Field = !(Settings()->*Field); Commit(); });
        }

        FOption Pick(const FString& Tab, const FString& Section, const FString& Name, const FString& Description,
            int32 UVortexPlayerSettings::*Field, const TArray<FString>& Names)
        {
            const int32 Count = Names.Num();
            const int32 Current = FMath::Clamp(Settings()->*Field, 0, Count - 1);
            return Make(Tab, Section, Name, Description, Names[Current], Current, Count,
                [Field, Count](int32 D) { Settings()->*Field = FMath::Clamp(Settings()->*Field + D, 0, Count - 1); Commit(); });
        }

        // -VortexShaders=0/1, -VortexWaterQuality=0..3 e -VortexGrassDensity=0..3 (testes): só nesta execução, sem gravar
        void CommandLineOverride()
        {
            static bool bDone = false;
            if (bDone) return;
            bDone = true;
            auto Settings = []() { return Real(); };   // a linha de comando muda as de verdade
            int32 Forced = -1;
            if (FParse::Value(FCommandLine::Get(), TEXT("VortexShaders="), Forced) && Forced >= 0)
            {
                UVortexPlayerSettings* S = Settings();
                S->bShaderGI = S->bShaderReflections = S->bShaderLightShafts = S->bShaderMotionBlur = S->bShaderBloom = S->bShaderVolumetricFog = Forced != 0;
            }
            int32 Water = -1;
            if (FParse::Value(FCommandLine::Get(), TEXT("VortexWaterQuality="), Water) && Water >= 0) Settings()->WaterQuality = FMath::Clamp(Water, 0, 3);
            int32 Density = -1;
            if (FParse::Value(FCommandLine::Get(), TEXT("VortexGrassDensity="), Density) && Density >= 0) Settings()->GrassDensity = FMath::Clamp(Density, 0, 3);
            // -VortexPreset=0..3 (Baixa..Épica), -VortexShadowMode=0/1 e -VortexMist=0/1: o teste de desempenho mede cada preset
            int32 Preset = -1;
            if (FParse::Value(FCommandLine::Get(), TEXT("VortexPreset="), Preset) && Preset >= 0)
            {
                const int32 L = FMath::Clamp(Preset, 0, 3);
                if (UGameUserSettings* V = Video())
                {
                    for (const FGroup& G : PresetGroups) (V->*G.Set)(L);
                    V->ApplySettings(false);
                }
                Settings()->GrassDensity = DensityFor[L];
                Settings()->WaterQuality = L;
                Settings()->bAmbientOcclusion = L >= 1;
            }
            int32 Mode = -1;
            if (FParse::Value(FCommandLine::Get(), TEXT("VortexShadowMode="), Mode) && Mode >= 0) Settings()->ShadowMode = FMath::Clamp(Mode, 0, 1);
            int32 Up = -1;
            if (FParse::Value(FCommandLine::Get(), TEXT("VortexUpscaler="), Up) && Up >= 0) { Settings()->Upscaler = FMath::Clamp(Up, 0, 4); Settings()->RenderDefaultsVersion = FMath::Max(Settings()->RenderDefaultsVersion, 4); }
            int32 Mist = -1;
            if (FParse::Value(FCommandLine::Get(), TEXT("VortexMist="), Mist) && Mist >= 0) Settings()->bValleyMist = Mist != 0;
        }
    }

    const TArray<FString>& Tabs()
    {
        static const TArray<FString> All = { TabScreen, TabGraphics, TabShaders, TabPerformance, TabAudio, TabControls, TabInterface };
        return All;
    }

    int32 ShadersState()
    {
        const UVortexPlayerSettings* S = Settings();
        const bool Flags[] = { S->bShaderGI, S->bShaderReflections, S->bShaderLightShafts, S->bShaderMotionBlur, S->bShaderBloom, S->bShaderVolumetricFog };
        int32 On = 0;
        for (bool b : Flags) On += b;
        return On == 0 ? 0 : On == UE_ARRAY_COUNT(Flags) ? 1 : -1;
    }

    void SetShaders(bool bOn)
    {
        UVortexPlayerSettings* S = Settings();
        S->bShaderGI = S->bShaderReflections = S->bShaderLightShafts = S->bShaderMotionBlur = S->bShaderBloom = S->bShaderVolumetricFog = bOn;
        Commit();
    }

    // GrassDistanceScale, ViewDistanceScale e GrassDensity: no módulo VortexWorld (Framework/VortexViewScales.cpp)
    float FieldOfView() { return FMath::Clamp(Real()->FieldOfView, 60.f, 110.f); }
    /** O que o painel mostra (a cópia enquanto edita). */
    float ShownFieldOfView() { return FMath::Clamp(Settings()->FieldOfView, 60.f, 110.f); }

    // 01/11: o campo de visão da primeira pessoa à parte (antes era sempre o da terceira + 10°). Fica no
    // GameUserSettings.ini ([Vortex.Camera]) e não no UVortexPlayerSettings: mudar o cabeçalho dele recompilava o jogo
    // quase inteiro. AVortexCharacter declara a função no próprio .cpp
    float FirstPersonFieldOfView()
    {
        float Value = FieldOfView() + 10.f;
        if (GConfig) GConfig->GetFloat(TEXT("Vortex.Camera"), TEXT("FirstPersonFieldOfView"), Value, GGameUserSettingsIni);
        return FMath::Clamp(Value, 60.f, 120.f);
    }

    float ShownFirstPersonFieldOfView() { return PendingFirstFov > 0.f ? PendingFirstFov : FirstPersonFieldOfView(); }

    void ApplyPreset(int32 Level) { SetPreset(FMath::Clamp(Level, 0, 3)); }

    FString LastRecommendation;

    void Recommend();

    void FirstRunSetup()
    {
        // uma vez por PC: a qualidade recomendada e a pergunta dos shaders (a versão sobe se a regra mudar)
        constexpr int32 Version = 1;
        int32 Done = 0;
        if (!GConfig) return;
        GConfig->GetInt(TEXT("Vortex.Graphics"), TEXT("AutoSetup"), Done, GGameUserSettingsIni);
        if (Done >= Version || FCString::Strifind(FCommandLine::Get(), TEXT("VortexPreset="))) return;
        Recommend();
        Real()->bValleyMist = true;
        Real()->SaveConfig();
        GConfig->SetInt(TEXT("Vortex.Graphics"), TEXT("AutoSetup"), Version, GGameUserSettingsIni);
        GConfig->SetBool(TEXT("Vortex.Graphics"), TEXT("AskShaders"), true, GGameUserSettingsIni);
        GConfig->Flush(false, GGameUserSettingsIni);
    }

    bool NeedsShaderChoice()
    {
        bool bAsk = false;
        if (GConfig) GConfig->GetBool(TEXT("Vortex.Graphics"), TEXT("AskShaders"), bAsk, GGameUserSettingsIni);
        return bAsk;
    }

    void ChooseShaders(bool bOn)
    {
        SetShaders(bOn);
        if (!GConfig) return;
        GConfig->SetBool(TEXT("Vortex.Graphics"), TEXT("AskShaders"), false, GGameUserSettingsIni);
        GConfig->Flush(false, GGameUserSettingsIni);
    }

    void Recommend()
    {
        UGameUserSettings* V = Video();
        UVortexPlayerSettings* S = Settings();
        if (!V) return;
        // o teste de hardware da Unreal (placa e processador; nota ~100 = um PC de jogo comum de 2014)
        V->RunHardwareBenchmark(10, 1.f, 1.f);
        const float Gpu = V->GetLastGPUBenchmarkResult(), Cpu = V->GetLastCPUBenchmarkResult();
        int32 L = Gpu >= 350.f ? 3 : Gpu >= 150.f ? 2 : Gpu >= 70.f ? 1 : 0;
        if (Cpu > 0.f && Cpu < 60.f) L = FMath::Min(L, 1);   // processador fraco: o mundo e as criaturas pesam
        SetPreset(L);
        S->ShadowMode = 0;                                  // sombras leves (as detalhadas custam ~20 FPS)
        S->ViewDistance = L >= 2 ? 3 : L + 1;
        S->GrassDistance = FMath::Clamp(L, 1, 3);
        S->bValleyMist = true;                              // 02/11: a neblina sempre ligada (pedido do usuário)
        S->bDynamicResolution = true;
        S->bShaderGI = S->bShaderReflections = S->bShaderVolumetricFog = false;   // Lumen: só se o jogador quiser
        const bool bDlss = DlssSupported();
        S->Upscaler = bDlss ? (L >= 2 ? 1 : 2) : 0;
        V->SetResolutionScaleValueEx(100.f);                // a resolução dinâmica ou o DLSS cuidam do resto
        LastRecommendation = FString::Printf(TEXT("%s%s"), Levels[L], bDlss ? (L >= 2 ? TEXT(" • DLSS Qualidade") : TEXT(" • DLSS Equilibrado")) : TEXT(" • TSR"));
        UE_LOG(LogTemp, Display, TEXT("VORTEX_GRAPHICS recomendação: placa %.0f, processador %.0f -> %s"), Gpu, Cpu, *LastRecommendation);
        Commit(true);
    }

    void BeginEditing()
    {
        if (bCommitQueued) { bEditing = true; return; }
        bEditing = true;
        bPending = false;
        SyncDraft();
        // o que a tela e a qualidade mudaram sem aplicar volta ao que está gravado
        if (UGameUserSettings* V = Video()) V->LoadSettings(true);
    }

    void DiscardPending()
    {
        if (bCommitQueued) return;
        if (UGameUserSettings* V = Video()) V->LoadSettings(true);
        SyncDraft();
        bPending = false;
    }

    void EndEditing()
    {
        bEditing = false;
        // fechou com o Aplicar a caminho: ele termina sozinho; senão, o que não foi aplicado é descartado
        if (bCommitQueued) return;
        if (bPending) DiscardPending();
        Draft.Reset();
    }

    bool HasPending() { return bPending && !bCommitQueued; }

    namespace
    {
        void CommitDraft()
        {
            bCommitQueued = false;
            if (!Draft) return;
            CopySettings(Real(), Draft.Get());
            if (PendingFirstFov > 0.f && GConfig)
            {
                GConfig->SetFloat(TEXT("Vortex.Camera"), TEXT("FirstPersonFieldOfView"), PendingFirstFov, GGameUserSettingsIni);
                GConfig->Flush(false, GGameUserSettingsIni);
            }
            const bool bWasEditing = bEditing;
            bEditing = false;
            bPending = false;
            Commit(true);
            AppliedFrame = GFrameCounter;
            bEditing = bWasEditing;
            if (bEditing) SyncDraft();
            else Draft.Reset();
        }
    }

    void ApplyPending()
    {
        if (!bPending || bCommitQueued) return;
        bCommitQueued = bApplying = true;
        ApplyStarted = FPlatformTime::Seconds();
        AppliedFrame = TNumericLimits<uint64>::Max() - 8;
        // no mundo: a tela de carregamento cobre a troca (o chão e a grama remontam por trás)
        if (UWorld* World = GameWorld())
            for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
                if (AVortexHudBase* Hud = It->Get() && It->Get()->IsLocalController() ? Cast<AVortexHudBase>(It->Get()->GetHUD()) : nullptr)
                    Hud->BeginApplyLoading();
        // aplica só depois de 2 quadros: o "Aplicando" já está na tela quando o motor para para trocar tudo
        FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Frames = 2](float) mutable
        {
            if (--Frames > 0) return true;
            CommitDraft();
            return false;
        }));
    }

    bool IsApplying()
    {
        if (!bApplying) return false;
        if (bCommitQueued || GFrameCounter < AppliedFrame + 3) return true;
        // o mundo terminou de remontar (o chão e a grama em volta) e a fila de carregamento esvaziou; no máximo 20 s
        int32 Pending = 0;
        if (UWorld* World = GameWorld())
            for (TActorIterator<AVortexTerrainStreamer> It(World); It; ++It) Pending += It->PendingChunks();
        if ((Pending == 0 && GetNumAsyncPackages() == 0) || FPlatformTime::Seconds() - ApplyStarted > 20.0) bApplying = false;
        return bApplying;
    }

    bool DlssSupported()
    {
#if VORTEX_WITH_DLSS
        // o módulo do DLSS só sobe depois que o motor termina de iniciar
        return FModuleManager::Get().IsModuleLoaded(TEXT("DLSS")) && UDLSSLibrary::IsDLSSSupported();
#else
        return false;
#endif
    }

    void Apply(UWorld* World)
    {
        CommandLineOverride();
        // 11/11 (o usuário: "meus amigos falaram que nos PCs deles o jogo iniciou muito alto"): uma vez por PC, o volume
        // geral vai a no máximo 50% e a música a 60% (os sons são normalizados perto do máximo; 100% assustava)
        if (GConfig)
        {
            int32 AudioDefaults = 0;
            GConfig->GetInt(TEXT("Vortex.Audio"), TEXT("Defaults"), AudioDefaults, GGameUserSettingsIni);
            if (AudioDefaults < 1)
            {
                Real()->VolumeMaster = FMath::Min(Real()->VolumeMaster, .5f);
                Real()->VolumeMusic = FMath::Min(Real()->VolumeMusic, .6f);
                Real()->SaveConfig();
                GConfig->SetInt(TEXT("Vortex.Audio"), TEXT("Defaults"), 1, GGameUserSettingsIni);
                GConfig->Flush(false, GGameUserSettingsIni);
            }
        }
        const UVortexPlayerSettings* S = Real();
        if (World) VortexAudio::ApplyVolumes(World);
        // efeitos dos shaders (acima da qualidade da Unreal: "SetByCode" vence o grupo sg.*)
        SetCVar(TEXT("r.Lumen.DiffuseIndirect.Allow"), S->bShaderGI);
        // 11/11 (auditoria de FPS): fora da Épica, a luz da cena do Lumen atualiza na metade do ritmo (0,7 ms de placa a
        // menos na praça; a troca da luz do dia demora um pouco mais para aparecer no rebatido)
        {
            const bool bEpic = Video() && Video()->GetGlobalIlluminationQuality() >= 3;
            SetCVar(TEXT("r.LumenScene.DirectLighting.UpdateFactor"), bEpic ? 32.f : 64.f);
            SetCVar(TEXT("r.LumenScene.Radiosity.UpdateFactor"), bEpic ? 64.f : 128.f);
        }
        SetCVar(TEXT("r.Shadow.Virtual.Enable"), S->ShadowMode == 1);
        // sombras clássicas: 2 camadas nas qualidades baixas, 3 nas altas, e um alcance um pouco menor
        // (cada camada redesenha a cena para a luz do sol)
        if (const UGameUserSettings* V = Video())
        {
            SetCVar(TEXT("r.Shadow.CSM.MaxCascades"), V->GetShadowQuality() >= 2 ? 3.f : 2.f);
            SetCVar(TEXT("r.Shadow.DistanceScale"), V->GetShadowQuality() >= 3 ? 1.f : .75f);
        }
        // DLSS / DLAA (placas RTX): a IA da NVIDIA reconstrói a imagem no lugar do TSR; com ele a resolução
        // dinâmica fica desligada (o modo do DLSS já fixa a resolução interna)
        // a neblina dos vales (manchas do FogArea) ficou parecendo nuvens soltas: desligada para todos uma vez
        if (S->RenderDefaultsVersion < 5 && S->RenderDefaultsVersion >= 4)
        {
            Real()->bValleyMist = false;
            Real()->RenderDefaultsVersion = 5;
            Real()->SaveConfig();
        }
        // placas RTX, na primeira vez com um mundo carregado (o módulo do DLSS já subiu): DLSS Qualidade
        if (S->RenderDefaultsVersion < 4 && (World || GameWorld()))
        {
            UVortexPlayerSettings* Mutable = Real();
            if (DlssSupported()) Mutable->Upscaler = 1;
            Mutable->RenderDefaultsVersion = 5;
            Mutable->bValleyMist = false;
            Mutable->SaveConfig();
        }
        const bool bDlss = S->Upscaler > 0 && DlssSupported();
#if VORTEX_WITH_DLSS
        {
            static const UDLSSMode Modes[] = { UDLSSMode::Off, UDLSSMode::Quality, UDLSSMode::Balanced, UDLSSMode::Performance, UDLSSMode::DLAA };
            static int32 Applied = -1;
            const int32 Wanted = bDlss ? FMath::Clamp(S->Upscaler, 0, 4) : 0;
            static int32 AppliedModel = -1;
            if ((Wanted != Applied || S->DlssModel != AppliedModel) && FModuleManager::Get().IsModuleLoaded(TEXT("DLSS")))
            {
                if (Wanted > 0) SetCVar(TEXT("r.AntiAliasingMethod"), 4.f);   // o DLSS entra no lugar do TSR
                // 11/11: o modelo do DLSS (5 = preset E, o leve; 0 = o padrão do plugin)
                const bool bOldCard = GRHIAdapterName.Contains(TEXT("RTX 20")) || GRHIAdapterName.Contains(TEXT("RTX 30"));
                SetCVar(TEXT("r.NGX.DLSS.Preset"), S->DlssModel == 1 || (S->DlssModel == 0 && bOldCard) ? 5.f : 0.f);
                UDLSSLibrary::SetDLSSMode(World ? World : GameWorld(), Modes[Wanted]);
                // voltando para o TSR: a resolução interna volta a ser a das opções
                if (Wanted == 0 && Applied > 0)
                    if (UGameUserSettings* V = Video()) V->ApplySettings(false);
                Applied = Wanted;
                AppliedModel = S->DlssModel;
            }
        }
#endif
        // resolução dinâmica (com o TSR/TAA reconstruindo a imagem): mira o alvo de FPS, entre 50% e 100%
        SetCVar(TEXT("r.DynamicRes.OperationMode"), S->bDynamicResolution && !bDlss ? 2.f : 0.f);
        // a meta da resolução dinâmica é o próprio limite de FPS (um limitador só); sem limite, 60
        {
            const float Limit = Video() ? Video()->GetFrameRateLimit() : 0.f;
            SetCVar(TEXT("r.DynamicRes.FrameTimeBudget"), 1000.f / FMath::Clamp(Limit > 0.f ? Limit : 60.f, 30.f, 240.f));
        }
        SetCVar(TEXT("r.DynamicRes.MinScreenPercentage"), 50.f);
        SetCVar(TEXT("r.DynamicRes.MaxScreenPercentage"), 100.f);
        SetCVar(TEXT("r.Lumen.Reflections.Allow"), S->bShaderReflections);
        SetCVar(TEXT("r.LightShaftQuality"), S->bShaderLightShafts);
        SetCVar(TEXT("r.MotionBlurQuality"), S->bShaderMotionBlur ? 4.f : 0.f);
        SetCVar(TEXT("r.BloomQuality"), S->bShaderBloom ? 3.f : 0.f);   // 3: o mesmo halo, mais barato que o 5
        // a neblina do FogArea é material de volume: precisa da névoa volumétrica (sem o efeito dos shaders, numa grade mais leve)
        SetCVar(TEXT("r.VolumetricFog"), S->bShaderVolumetricFog || S->bValleyMist);
        // grade da névoa volumétrica: 12 px e 96 fatias com o efeito ligado (antes 8 e 128, o dobro do custo)
        SetCVar(TEXT("r.VolumetricFog.GridPixelSize"), S->bShaderVolumetricFog ? 12.f : 16.f);
        SetCVar(TEXT("r.VolumetricFog.GridSizeZ"), S->bShaderVolumetricFog ? 96.f : 64.f);
        SetCVar(TEXT("r.AmbientOcclusionLevels"), S->bAmbientOcclusion ? -1.f : 0.f);
        SetCVar(TEXT("r.DistanceFieldAO"), S->bAmbientOcclusion);
        SetCVar(TEXT("r.Lumen.TranslucencyReflections.FrontLayer.Enable"), S->WaterQuality >= 2 && S->bShaderReflections);
        static const float Methods[] = { 4.f, 2.f, 1.f, 0.f };
        SetCVar(TEXT("r.AntiAliasingMethod"), S->Upscaler > 0 && DlssSupported() ? 4.f : Methods[FMath::Clamp(S->AntiAliasingMethod, 0, 3)]);
        static const float Sharpness[] = { 0.f, .4f, .8f, 1.3f };
        SetCVar(TEXT("r.Tonemapper.Sharpen"), Sharpness[FMath::Clamp(S->Sharpen, 0, 3)]);
        static const float Anisotropy[] = { 1.f, 4.f, 8.f, 16.f };
        SetCVar(TEXT("r.MaxAnisotropy"), Anisotropy[FMath::Clamp(S->Anisotropy, 0, 3)]);
        static const float History[] = { 100.f, 150.f, 200.f };
        SetCVar(TEXT("r.TSR.History.ScreenPercentage"), History[FMath::Clamp(S->EdgeQuality, 0, 2)]);
        // distância de visualização: também o detalhe de longe da Unreal (criaturas, objetos)
        static const float ViewDistanceQuality[] = { 0.f, 1.f, 2.f, 3.f };
        SetCVar(TEXT("sg.ViewDistanceQuality"), ViewDistanceQuality[FMath::Clamp(S->ViewDistance, 0, 3)]);

        if (!World) World = GameWorld();
        if (!World) return;
        for (TActorIterator<AVortexTerrainStreamer> It(World); It; ++It) It->ApplyGraphicsSettings();
        for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
        {
            const APlayerController* Controller = It->Get();
            if (AVortexCharacter* Hero = Controller && Controller->IsLocalController() ? Cast<AVortexCharacter>(Controller->GetPawn()) : nullptr)
            {
                Hero->RefreshCameraSettings();
            }
        }
    }

    void ResetTab(const FString& Tab)
    {
        UVortexPlayerSettings* S = Settings();
        UGameUserSettings* V = Video();
        if (Tab == TabScreen && V)
        {
            V->SetFullscreenMode(EWindowMode::WindowedFullscreen);
            V->SetScreenResolution(V->GetDesktopResolution());
            S->FieldOfView = 75.f;
            S->Brightness = 3;
            PendingFirstFov = 85.f;
            Commit(true);
        }
        else if (Tab == TabPerformance && V)
        {
            V->SetResolutionScaleValueEx(75.f);
            V->SetFrameRateLimit(0.f);
            V->SetVSyncEnabled(false);
            S->Upscaler = DlssSupported() ? 1 : 0;
            S->DlssModel = 0;
            S->bDynamicResolution = true;
            Commit(true);
        }
        else if (Tab == TabGraphics)
        {
            S->ViewDistance = 3;
            S->GrassDistance = 2;
            S->AntiAliasingMethod = 0;
            S->Sharpen = 0;
            S->Anisotropy = 3;
            S->EdgeQuality = 1;
            SetPreset(2);
        }
        else if (Tab == TabShaders)
        {
            S->ShadowMode = 0;
            S->bAmbientOcclusion = true;
            SetShaders(true);
        }
        else if (Tab == TabAudio)
        {
            S->VolumeEffects = S->VolumeAmbience = S->VolumeInterface = 1.f;
            S->VolumeMaster = .5f;
            S->VolumeMusic = .6f;
            Commit();
        }
        else if (Tab == TabControls)
        {
            S->MouseSensitivity = .6f;
            S->AimSensitivityScale = .7f;
            S->bInvertMouseY = false;
            Commit();
        }
        else if (Tab == TabInterface)
        {
            S->bShowHud = true;
            S->bShowFps = false;
            S->MinimapRadius = 100.f;
            S->bMinimapRotates = true;
            Commit();
        }
    }

    TArray<FOption> Options(const FString& Tab)
    {
        UVortexPlayerSettings* S = Settings();
        UGameUserSettings* V = Video();
        TArray<FOption> Out;
        if (Tab == TabScreen)
        {
            if (V)
            {
                TArray<FIntPoint> Sizes;
                UKismetSystemLibrary::GetSupportedFullscreenResolutions(Sizes);
                const FIntPoint Size = V->GetScreenResolution();
                const int32 SizeIndex = Sizes.IndexOfByKey(Size);
                Out.Add(Make(Tab, TEXT("Janela"), TEXT("Resolução"), TEXT("O tamanho da imagem na tela. A do seu monitor fica mais nítida. Para ganhar FPS, prefira baixar a resolução interna (aba Desempenho)."),
                    FString::Printf(TEXT("%d × %d"), Size.X, Size.Y), FMath::Max(SizeIndex, 0), 0, [Sizes](int32 D)
                    {
                        UGameUserSettings* Vid = Video();
                        if (!Vid || Sizes.IsEmpty()) return;
                        const int32 Index = Sizes.IndexOfByKey(Vid->GetScreenResolution());
                        Vid->SetScreenResolution(Sizes[Index == INDEX_NONE ? Sizes.Num() - 1 : FMath::Clamp(Index + D, 0, Sizes.Num() - 1)]);
                        Commit(true);
                    }));
                static const EWindowMode::Type Modes[] = { EWindowMode::Windowed, EWindowMode::WindowedFullscreen, EWindowMode::Fullscreen };
                static const TCHAR* ModeNames[] = { TEXT("Janela"), TEXT("Tela cheia sem borda"), TEXT("Tela cheia exclusiva") };
                int32 ModeIndex = 0;
                for (int32 i = 0; i < 3; ++i) if (Modes[i] == V->GetFullscreenMode()) ModeIndex = i;
                Out.Add(Make(Tab, TEXT("Janela"), TEXT("Modo de exibição"), TEXT("Sem borda: troca de programa (Alt+Tab) sem piscar. Exclusiva: pode dar alguns FPS a mais e menos atraso."),
                    ModeNames[ModeIndex], ModeIndex, 3, [](int32 D)
                    {
                        UGameUserSettings* Vid = Video();
                        if (!Vid) return;
                        int32 Index = 0;
                        for (int32 i = 0; i < 3; ++i) if (Modes[i] == Vid->GetFullscreenMode()) Index = i;
                        Vid->SetFullscreenMode(Modes[FMath::Clamp(Index + D, 0, 2)]);
                        Commit(true);
                    }));
            }
            Out.Add(Pick(Tab, TEXT("Imagem"), TEXT("Brilho"), TEXT("Clareia ou escurece a imagem toda (dia, noite e cavernas). Não pesa nada."),
                &UVortexPlayerSettings::Brightness, { TEXT("Mais escuro"), TEXT("Original"), TEXT("Um pouco mais claro"), TEXT("Claro"), TEXT("Bem claro"), TEXT("Máximo") }));
            const int32 Fov = FMath::RoundToInt(ShownFieldOfView());
            Out.Add(Make(Tab, TEXT("Câmera"), TEXT("Campo de visão (3ª pessoa)"), TEXT("Quanto do mundo cabe na tela atrás do herói, de 60° a 110°. Ao mirar, fecha 7°."),
                FString::Printf(TEXT("%d°"), Fov), (Fov - 60) / 5, 11,
                [](int32 D) { Settings()->FieldOfView = FMath::Clamp(FMath::RoundToFloat(ShownFieldOfView() / 5.f) * 5.f + 5.f * D, 60.f, 110.f); Commit(); }));
            const int32 FirstFov = FMath::RoundToInt(ShownFirstPersonFieldOfView());
            Out.Add(Make(Tab, TEXT("Câmera"), TEXT("Campo de visão (1ª pessoa)"), TEXT("O mesmo na visão pelos olhos do herói, de 60° a 120°. Ao mirar, fecha 7°."),
                FString::Printf(TEXT("%d°"), FirstFov), (FirstFov - 60) / 5, 13,
                [](int32 D)
                {
                    PendingFirstFov = FMath::Clamp(FMath::RoundToFloat(ShownFirstPersonFieldOfView() / 5.f) * 5.f + 5.f * D, 60.f, 120.f);
                    Commit();
                }));
        }
        else if (Tab == TabGraphics)
        {
            const int32 Preset = PresetLevel();
            Out.Add(Make(Tab, TEXT("Geral"), TEXT("Recomendação automática"),
                TEXT("Mede a sua placa de vídeo e o processador e escolhe tudo para o jogo rodar liso. Depois é só Aplicar."),
                LastRecommendation.IsEmpty() ? FString(TEXT("Calibrar")) : LastRecommendation, 0, 0, [](int32) { Recommend(); }));
            Out.Add(Make(Tab, TEXT("Geral"), TEXT("Qualidade geral"),
                TEXT("Muda tudo de uma vez: sombras, luz, efeitos, texturas, vegetação e água. Alta e Épica ligam a luz global e os reflexos do Lumen (os efeitos que mais pesam); Baixa e Média desligam. Não mexe nas distâncias."),
                Preset < 0 ? FString(TEXT("Personalizada")) : FString(Levels[Preset]), FMath::Max(Preset, 0), 4,
                [Preset](int32 D) { SetPreset(FMath::Clamp((Preset < 0 ? 2 : Preset) + D, 0, 3)); }));
            Out.Add(Pick(Tab, TEXT("Distância"), TEXT("Distância de visão"), TEXT("Até onde aparecem árvores, pedras, construções e criaturas. Pesa no processador."),
                &UVortexPlayerSettings::ViewDistance, { TEXT("Curta"), TEXT("Média"), TEXT("Longa"), TEXT("Máxima") }));
            Out.Add(Pick(Tab, TEXT("Distância"), TEXT("Distância da grama"), TEXT("Até onde a grama aparece em volta de você."),
                &UVortexPlayerSettings::GrassDistance, { TEXT("Curta"), TEXT("Média"), TEXT("Longa"), TEXT("Máxima") }));
            Out.Add(Pick(Tab, TEXT("Mundo"), TEXT("Densidade da vegetação"), TEXT("Quantos tufos de grama e flores por metro. Baixa pesa bem menos."),
                &UVortexPlayerSettings::GrassDensity, { TEXT("Baixa"), TEXT("Média"), TEXT("Alta"), TEXT("Máxima") }));
            Out.Add(Pick(Tab, TEXT("Mundo"), TEXT("Água"), TEXT("Baixa: opaca. Média: transparente e parada. Alta: com marolinhas. Épica: refração e ondas."),
                &UVortexPlayerSettings::WaterQuality, { TEXT("Baixa"), TEXT("Média"), TEXT("Alta"), TEXT("Épica") }));
            Out.Add(Toggle(Tab, TEXT("Mundo"), TEXT("Neblina nos vales"), TEXT("Manchas de névoa baixa nos vales, na beira da água e nas matas. Pesa pouco."), &UVortexPlayerSettings::bValleyMist));
            if (V)
            {
                const FString Detail = TEXT("Detalhe");
                Out.Add(Group(Tab, Detail, TEXT("Texturas"), TEXT("Resolução das texturas. Pesa na memória da placa, quase nada no FPS."), &UGameUserSettings::GetTextureQuality, &UGameUserSettings::SetTextureQuality));
                Out.Add(Group(Tab, Detail, TEXT("Partículas"), TEXT("Fumaça, faíscas, poeira e os brilhos dos golpes e magias."), &UGameUserSettings::GetVisualEffectQuality, &UGameUserSettings::SetVisualEffectQuality));
                Out.Add(Group(Tab, Detail, TEXT("Folhagem"), TEXT("Detalhe das folhas e plantas."), &UGameUserSettings::GetFoliageQuality, &UGameUserSettings::SetFoliageQuality));
                Out.Add(Group(Tab, Detail, TEXT("Materiais"), TEXT("O acabamento da luz em cada superfície (pedra, metal, tecido)."), &UGameUserSettings::GetShadingQuality, &UGameUserSettings::SetShadingQuality));
                Out.Add(Group(Tab, Detail, TEXT("Pós-processamento"), TEXT("Cor, exposição, profundidade e acabamento final da imagem."), &UGameUserSettings::GetPostProcessingQuality, &UGameUserSettings::SetPostProcessingQuality));
            }
            Out.Add(Pick(Tab, TEXT("Bordas"), TEXT("Suavização das bordas"), TEXT("Tira o serrilhado. TSR é o mais bonito; FXAA o mais leve. Com o DLSS ligado (aba Desempenho), quem suaviza é ele."),
                &UVortexPlayerSettings::AntiAliasingMethod, { TEXT("TSR"), TEXT("TAA"), TEXT("FXAA"), TEXT("Desligada") }));
            if (V) Out.Add(Group(Tab, TEXT("Bordas"), TEXT("Qualidade da suavização"), TEXT("Quanto a suavização trabalha em cada quadro."), &UGameUserSettings::GetAntiAliasingQuality, &UGameUserSettings::SetAntiAliasingQuality));
            Out.Add(Pick(Tab, TEXT("Bordas"), TEXT("Quinas lisas (TSR)"), TEXT("O TSR guarda a imagem anterior em resolução maior: as quinas dos blocos e as folhas ficam sem escadinha. Pesa um pouco."),
                &UVortexPlayerSettings::EdgeQuality, { TEXT("Normal"), TEXT("Alta"), TEXT("Máxima") }));
            Out.Add(Pick(Tab, TEXT("Bordas"), TEXT("Nitidez"), TEXT("Realça os contornos depois da suavização."),
                &UVortexPlayerSettings::Sharpen, { TEXT("Desligada"), TEXT("Leve"), TEXT("Média"), TEXT("Forte") }));
            Out.Add(Pick(Tab, TEXT("Bordas"), TEXT("Filtro anisotrópico"), TEXT("Deixa nítido o chão e as paredes vistos de lado (sem ele borram ao longe). Quase não pesa."),
                &UVortexPlayerSettings::Anisotropy, { TEXT("Desligado"), TEXT("4x"), TEXT("8x"), TEXT("16x") }));
        }
        else if (Tab == TabShaders)
        {
            const int32 State = ShadersState();
            Out.Add(Make(Tab, TEXT("Geral"), TEXT("Todos os efeitos"), TEXT("Liga ou desliga de uma vez os efeitos de luz abaixo. Ligados ficam bem mais bonitos, mas custam cerca de 25% do FPS."),
                State == 1 ? TEXT("Ligados") : State == 0 ? TEXT("Desligados") : TEXT("Personalizados"), State == 1 ? 1 : 0, 2,
                [](int32) { SetShaders(ShadersState() != 1); }));
            const FString Light = TEXT("Luz");
            Out.Add(Toggle(Tab, Light, TEXT("Luz global (Lumen)"), TEXT("A luz do sol rebate no chão e nas paredes e as sombras ganham cor. O efeito que mais pesa."), &UVortexPlayerSettings::bShaderGI));
            if (V) Out.Add(Group(Tab, Light, TEXT("Qualidade da luz global"), TEXT("O detalhe da luz rebatida (só com a luz global ligada)."), &UGameUserSettings::GetGlobalIlluminationQuality, &UGameUserSettings::SetGlobalIlluminationQuality));
            Out.Add(Toggle(Tab, Light, TEXT("Reflexos (Lumen)"), TEXT("Reflexos de verdade na água, no metal e nas superfícies lisas."), &UVortexPlayerSettings::bShaderReflections));
            if (V) Out.Add(Group(Tab, Light, TEXT("Qualidade dos reflexos"), TEXT("O detalhe dos reflexos (só com os reflexos ligados)."), &UGameUserSettings::GetReflectionQuality, &UGameUserSettings::SetReflectionQuality));
            Out.Add(Toggle(Tab, Light, TEXT("Oclusão de ambiente"), TEXT("Sombra suave nos cantos, embaixo das coisas e entre as pedras."), &UVortexPlayerSettings::bAmbientOcclusion));
            const FString Shadow = TEXT("Sombras");
            Out.Add(Pick(Tab, Shadow, TEXT("Tipo de sombra"), TEXT("Clássicas: bem mais rápidas. Virtuais: as da Unreal 5, nítidas de perto e de longe, mas custam uns 20 FPS."),
                &UVortexPlayerSettings::ShadowMode, { TEXT("Clássicas"), TEXT("Virtuais") }));
            if (V) Out.Add(Group(Tab, Shadow, TEXT("Qualidade das sombras"), TEXT("Nitidez e alcance das sombras do sol. Das opções que mais pesam."), &UGameUserSettings::GetShadowQuality, &UGameUserSettings::SetShadowQuality));
            const FString Each = TEXT("Efeitos de câmera");
            Out.Add(Toggle(Tab, Each, TEXT("Raios de sol"), TEXT("Feixes de luz entre as árvores quando você olha para o sol."), &UVortexPlayerSettings::bShaderLightShafts));
            Out.Add(Toggle(Tab, Each, TEXT("Brilho das luzes"), TEXT("O halo em volta do sol, do fogo e das magias."), &UVortexPlayerSettings::bShaderBloom));
            Out.Add(Toggle(Tab, Each, TEXT("Névoa volumétrica"), TEXT("Névoa que a luz atravessa, com fachos de luz."), &UVortexPlayerSettings::bShaderVolumetricFog));
            Out.Add(Toggle(Tab, Each, TEXT("Desfoque de movimento"), TEXT("Borra a imagem quando a câmera gira rápido."), &UVortexPlayerSettings::bShaderMotionBlur));
        }
        else if (Tab == TabPerformance)
        {
            if (DlssSupported())
            {
                Out.Add(Pick(Tab, TEXT("Super resolução"), TEXT("DLSS"), TEXT("A IA da NVIDIA desenha em resolução menor e reconstrói a imagem. Qualidade fica quase igual à nativa, com bem mais FPS. DLAA: resolução cheia, só a suavização da IA."),
                    &UVortexPlayerSettings::Upscaler, { TEXT("Desligado (TSR)"), TEXT("Qualidade"), TEXT("Equilibrado"), TEXT("Desempenho"), TEXT("DLAA") }));
                Out.Add(Pick(Tab, TEXT("Super resolução"), TEXT("Modelo do DLSS"), TEXT("Leve: quase 3 ms a menos por quadro nas placas RTX 20 e 30, um pouco menos nítido em movimento. Nítido: o modelo novo da NVIDIA. Automático: o leve nas RTX 20 e 30."),
                    &UVortexPlayerSettings::DlssModel, { TEXT("Automático"), TEXT("Leve"), TEXT("Nítido") }));
            }
            if (V)
            {
                float Normalized = 1.f, Current = 100.f, Min = 0.f, Max = 0.f;
                V->GetResolutionScaleInformationEx(Normalized, Current, Min, Max);
                if (Current <= 1.f) Current = 100.f;
                static const float Scales[] = { 50.f, 60.f, 67.f, 75.f, 85.f, 100.f };
                int32 ScaleIndex = 0;
                for (int32 i = 0; i < UE_ARRAY_COUNT(Scales); ++i) if (FMath::Abs(Scales[i] - Current) < FMath::Abs(Scales[ScaleIndex] - Current)) ScaleIndex = i;
                Out.Add(Make(Tab, TEXT("Resolução interna"), TEXT("Resolução interna"), TEXT("O jogo desenha menos pixels e o TSR reconstrói a imagem (com o DLSS ligado, quem manda é o modo do DLSS). O que mais dá FPS em telas grandes."),
                    FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Current)), ScaleIndex, UE_ARRAY_COUNT(Scales), [ScaleIndex](int32 D)
                    {
                        if (UGameUserSettings* Vid = Video()) { Vid->SetResolutionScaleValueEx(Scales[FMath::Clamp(ScaleIndex + D, 0, int32(UE_ARRAY_COUNT(Scales)) - 1)]); Commit(true); }
                    }));
            }
            Out.Add(Toggle(Tab, TEXT("Resolução interna"), TEXT("Resolução dinâmica"), TEXT("Quando o FPS cai abaixo do limite (sem limite, 60), a resolução interna baixa sozinha até 50% e volta quando dá. Não vale com o DLSS."), &UVortexPlayerSettings::bDynamicResolution));
            if (V)
            {
                const float Limit = V->GetFrameRateLimit();
                int32 LimitIndex = 0;
                for (int32 i = 0; i < UE_ARRAY_COUNT(FrameLimits); ++i) if (FMath::IsNearlyEqual(FrameLimits[i], Limit, 1.f)) LimitIndex = i;
                Out.Add(Make(Tab, TEXT("Quadros"), TEXT("Limite de FPS"), TEXT("Segura os quadros por segundo: menos calor e barulho da placa e quadros mais regulares."),
                    Limit <= 0.f ? FString(TEXT("Sem limite")) : FString::Printf(TEXT("%d"), FMath::RoundToInt(Limit)), LimitIndex, 0, [LimitIndex](int32 D)
                    {
                        if (UGameUserSettings* Vid = Video()) { Vid->SetFrameRateLimit(FrameLimits[Wrap(LimitIndex + D, UE_ARRAY_COUNT(FrameLimits))]); Commit(true); }
                    }));
                Out.Add(Make(Tab, TEXT("Quadros"), TEXT("Sincronia vertical"), TEXT("Tira o \"rasgo\" da imagem, mas prende o FPS ao monitor e soma um pouco de atraso."),
                    V->IsVSyncEnabled() ? TEXT("Ligada") : TEXT("Desligada"), V->IsVSyncEnabled() ? 1 : 0, 2,
                    [](int32) { if (UGameUserSettings* Vid = Video()) { Vid->SetVSyncEnabled(!Vid->IsVSyncEnabled()); Commit(true); } }));
            }
        }
        else if (Tab == TabControls)
        {
            const int32 Sens = FMath::RoundToInt(S->MouseSensitivity * 10.f);
            Out.Add(Make(Tab, TEXT("Mouse"), TEXT("Sensibilidade do mouse"), TEXT("A velocidade da câmera. No jogo, [ e ] também mudam."),
                FString::Printf(TEXT("%.1f"), S->MouseSensitivity).Replace(TEXT("."), TEXT(",")), Sens - 1, 0,
                [](int32 D)
                {
                    UVortexPlayerSettings* Set = Settings();
                    Set->MouseSensitivity = FMath::Clamp(FMath::RoundToFloat(Set->MouseSensitivity * 10.f + D) / 10.f, UVortexPlayerSettings::MinSensitivity, UVortexPlayerSettings::MaxSensitivity);
                    Commit();
                }));
            const int32 Aim = FMath::RoundToInt(S->AimSensitivityScale * 10.f);
            Out.Add(Make(Tab, TEXT("Mouse"), TEXT("Sensibilidade mirando"), TEXT("Multiplica a sensibilidade enquanto você segura o botão direito."),
                FString::Printf(TEXT("%.1f×"), S->AimSensitivityScale).Replace(TEXT("."), TEXT(",")), Aim - 1, 0,
                [](int32 D) { Settings()->AimSensitivityScale = FMath::Clamp(FMath::RoundToFloat(Settings()->AimSensitivityScale * 10.f + D) / 10.f, .1f, 2.f); Commit(); }));
            Out.Add(Make(Tab, TEXT("Mouse"), TEXT("Inverter o eixo vertical"), TEXT("Mouse para cima olha para baixo, como em simuladores de voo."),
                S->bInvertMouseY ? TEXT("Sim") : TEXT("Não"), S->bInvertMouseY ? 1 : 0, 2, [](int32) { Settings()->bInvertMouseY = !Settings()->bInvertMouseY; Commit(); }));
        }
        else if (Tab == TabAudio)
        {
            // de 10 em 10%, de mudo a 100%
            auto Volume = [&Out, Tab](const TCHAR* Name, const TCHAR* Description, float UVortexPlayerSettings::* Field)
            {
                const int32 Steps = FMath::RoundToInt(Settings()->*Field * 10.f);
                Out.Add(Make(Tab, TEXT("Volume"), Name, Description, Steps == 0 ? FString(TEXT("Mudo")) : FString::Printf(TEXT("%d%%"), Steps * 10), Steps, 11,
                    [Field](int32 D)
                    {
                        UVortexPlayerSettings* Set = Settings();
                        Set->*Field = FMath::Clamp(FMath::RoundToInt(Set->*Field * 10.f) + D, 0, 10) / 10.f;
                        Commit();
                    }));
            };
            Volume(TEXT("Volume geral"), TEXT("Tudo o que o jogo toca, de uma vez."), &UVortexPlayerSettings::VolumeMaster);
            Volume(TEXT("Música"), TEXT("Os temas do vale, da vila, da noite e das lutas."), &UVortexPlayerSettings::VolumeMusic);
            Volume(TEXT("Efeitos"), TEXT("Golpes, magias, passos, criaturas e chefes."), &UVortexPlayerSettings::VolumeEffects);
            Volume(TEXT("Ambiente"), TEXT("Vento, pássaros, insetos, a cidade e a água."), &UVortexPlayerSettings::VolumeAmbience);
            Volume(TEXT("Interface"), TEXT("Cliques, janelas, avisos e diálogos."), &UVortexPlayerSettings::VolumeInterface);
        }
        else if (Tab == TabInterface)
        {
            Out.Add(Make(Tab, TEXT("Na tela"), TEXT("Mostrar a HUD"), TEXT("Vida, mana, barra de ações e minimapa. Bom desligar para tirar fotos."),
                S->bShowHud ? TEXT("Sim") : TEXT("Não"), S->bShowHud ? 1 : 0, 2, [](int32) { Settings()->bShowHud = !Settings()->bShowHud; Commit(); }));
            Out.Add(Make(Tab, TEXT("Na tela"), TEXT("Mostrar o FPS"), TEXT("O contador de quadros por segundo no canto da tela."),
                S->bShowFps ? TEXT("Sim") : TEXT("Não"), S->bShowFps ? 1 : 0, 2, [](int32) { Settings()->bShowFps = !Settings()->bShowFps; Commit(); }));
            const int32 Radius = FMath::RoundToInt(S->MinimapRadius);
            const int32 RadiusIndex = Radius < 80 ? 0 : Radius < 130 ? 1 : 2;
            static const float Radii[] = { 60.f, 100.f, 160.f };
            Out.Add(Make(Tab, TEXT("Minimapa"), TEXT("Alcance do minimapa"), TEXT("Quanto do mundo o minimapa mostra em volta de você. No jogo, M troca."),
                FString::Printf(TEXT("%d m"), Radius), RadiusIndex, 3, [RadiusIndex](int32 D)
                {
                    Settings()->MinimapRadius = Radii[FMath::Clamp(RadiusIndex + D, 0, 2)];
                    Commit();
                }));
            Out.Add(Make(Tab, TEXT("Minimapa"), TEXT("Minimapa gira com a câmera"), TEXT("Sim: o que está à sua frente fica para cima e o N anda na borda. Não: o norte fica sempre para cima."),
                S->bMinimapRotates ? TEXT("Sim") : TEXT("Não"), S->bMinimapRotates ? 1 : 0, 2, [](int32) { Settings()->bMinimapRotates = !Settings()->bMinimapRotates; Commit(); }));
        }
        return Out;
    }
}
