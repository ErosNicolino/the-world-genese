#include "Framework/VortexGameInstance.h"
#include "Engine/NetConnection.h"
#include "HAL/PlatformMemory.h"
#include "Misc/App.h"
#include "Misc/ConfigCacheIni.h"
#include "Framework/VortexNetLog.h"
#include "Framework/VortexPlayerSettings.h"
#include "GameFramework/GameUserSettings.h"
#include "Online/VortexOnline.h"
#include "Framework/VortexUiHooks.h"
#include "Framework/VortexGraphics.h"
#include "Combat/VortexHitFx.h"
#include "Framework/VortexHeroWindow.h"

#include "Framework/VortexGameMode.h"
#include "Framework/VortexGameState.h"
#include "Framework/VortexCharacter.h"
#include "Framework/VortexMenuStage.h"
#include "Framework/VortexMenuGameMode.h"
#include "Framework/VortexPlayerState.h"
#include "Rpg/VortexRpg.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CoreDelegates.h"
#include "Misc/Guid.h"
#include "Persistence/VortexWorldSave.h"
#include "Persistence/VortexPlayerSave.h"
#include "ImageUtils.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "TimerManager.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Containers/Ticker.h"
#include "Engine/NetDriver.h"
#include "EngineUtils.h"
#include "UnrealClient.h"

void UVortexGameInstance::Init()
{
    Super::Init();
    // sem as mensagens de desenvolvimento da Unreal por cima da HUD ("LIGHTING NEEDS TO BE REBUILT" e afins;
    // o jogo instalado nem mostra). No editor, "EnableAllScreenMessages" no console traz de volta.
    GAreScreenMessagesEnabled = false;
    // primeira vez: resolução interna de 75% com o TSR reconstruindo a imagem (a GPU era o gargalo;
    // em 1080p dá ~1,7x menos pixels com quase a mesma nitidez). O jogador muda em Opções.
    if (UVortexPlayerSettings* Player = UVortexPlayerSettings::Get(); Player && !Player->bRenderDefaultsApplied && GEngine)
    {
        if (UGameUserSettings* Video = GEngine->GetGameUserSettings())
        {
            Video->SetResolutionScaleValueEx(75.f);
            Video->ApplySettings(false);
            Video->SaveSettings();
        }
        Player->bRenderDefaultsApplied = true;
        Player->SaveConfig();
    }
    // Alpha 1.0: sem configuração salva a Unreal começa tudo no Épico, e as sombras virtuais custavam
    // ~20 FPS (79 contra 92 com as clássicas, 1080p). Padrão: sombras clássicas em qualidade alta.
    if (UVortexPlayerSettings* Player = UVortexPlayerSettings::Get(); Player && Player->RenderDefaultsVersion < 2 && GEngine)
    {
        Player->ShadowMode = 0;
        if (UGameUserSettings* Video = GEngine->GetGameUserSettings())
        {
            Video->SetShadowQuality(FMath::Min(Video->GetShadowQuality(), 2));
            Video->ApplySettings(false);
            Video->SaveSettings();
        }
        Player->RenderDefaultsVersion = 2;
        Player->SaveConfig();
    }
    // Alpha 1.1: na primeira vez o jogo mede o PC (o teste de hardware da Unreal) e escolhe o preset;
    // nunca começa no Épico. Também liga a resolução dinâmica.
    if (UVortexPlayerSettings* Player = UVortexPlayerSettings::Get(); Player && Player->RenderDefaultsVersion < 3 && GEngine && !FCString::Strifind(FCommandLine::Get(), TEXT("VortexPreset=")))
    {
        if (UGameUserSettings* Video = GEngine->GetGameUserSettings())
        {
            Video->RunHardwareBenchmark();
            const int32 Level = FMath::Clamp(Video->ScalabilityQuality.GetMinQualityLevel(), 0, 2);
            VortexGraphics::ApplyPreset(Level);
            UE_LOG(LogTemp, Display, TEXT("VORTEX_GRAPHICS preset escolhido pelo teste de hardware: %d"), Level);
        }
        Player->bDynamicResolution = true;
        Player->RenderDefaultsVersion = 3;
        Player->SaveConfig();
    }

    // 02/11: a qualidade recomendada completa (preset, distâncias, DLSS, sombras) e a neblina ligada, uma vez por PC; o
    // menu pergunta se liga os shaders
    VortexGraphics::FirstRunSetup();

    MapLoadedHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UVortexGameInstance::OnWorldLoaded);
    // shaders, água, anti-aliasing... (as variáveis de render valem desde o primeiro quadro)
    VortexGraphics::Apply(nullptr);
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexLobbySmoke"))) BeginLobbySmoke();
#endif
    // 09/11: o diário completo: início e fim da sessão, erros fatais, avisos do motor e travadas (também no jogo publicado)
    {
        static bool bDiaryStarted = false;
        if (!bDiaryStarted)
        {
            bDiaryStarted = true;
            VortexNetLog::Write(FString::Printf(TEXT("==== SESSAO versao=%s build=%s memoria total=%llu MB"), FApp::GetBuildVersion(), LexToString(FApp::GetBuildConfiguration()),
                FPlatformMemory::GetConstants().TotalPhysical / (1024 * 1024)));
            FCoreDelegates::OnHandleSystemError.AddLambda([]() { VortexNetLog::Write(FString::Printf(TEXT("ERRO FATAL: %s"), GErrorHist)); });
            FCoreDelegates::OnHandleSystemEnsure.AddLambda([]() { VortexNetLog::Write(FString::Printf(TEXT("AVISO DO MOTOR (ensure): %s"), GErrorHist)); });
            FCoreDelegates::OnEnginePreExit.AddLambda([]() { VortexNetLog::Write(TEXT("==== Jogo fechando normalmente")); });
            FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float)
            {
                static double Last = FPlatformTime::Seconds();
                const double Now = FPlatformTime::Seconds(), Gap = Now - Last;
                Last = Now;
                if (Gap > 3.0)
                {
                    const FPlatformMemoryStats Memory = FPlatformMemory::GetStats();
                    VortexNetLog::Write(FString::Printf(TEXT("TRAVADA de %.1f s (memoria usada %llu MB, livre %llu MB)"), Gap, Memory.UsedPhysical / (1024 * 1024), Memory.AvailablePhysical / (1024 * 1024)));
                }
                return true;
            }));
        }
    }
    if (GEngine)
    {
        NetworkFailureHandle = GEngine->OnNetworkFailure().AddUObject(this, &UVortexGameInstance::OnNetworkFailure);
        TravelFailureHandle = GEngine->OnTravelFailure().AddUObject(this, &UVortexGameInstance::OnTravelFailure);
    }
    UE_LOG(LogTemp, Display, TEXT("Vortex framework: game instance initialized"));
}

void UVortexGameInstance::Shutdown()
{
    UE_LOG(LogTemp, Display, TEXT("Vortex framework: game instance shutting down"));
#if !UE_BUILD_SHIPPING
    DeleteSmokeSlots();
#endif
    FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(MapLoadedHandle);
    if (GEngine)
    {
        GEngine->OnNetworkFailure().Remove(NetworkFailureHandle);
        GEngine->OnTravelFailure().Remove(TravelFailureHandle);
    }
    HideFrontEnd();
    HidePauseMenu();
    Super::Shutdown();
}

TArray<FVortexWorldSummary> UVortexGameInstance::GetWorlds() const
{
    UVortexSaveSubsystem* Saves = const_cast<UVortexGameInstance*>(this)->GetSubsystem<UVortexSaveSubsystem>();
    return Saves ? Saves->ListHostedWorlds() : TArray<FVortexWorldSummary>();
}

TArray<FVortexWorldSummary> UVortexGameInstance::GetWorlds(bool bMultiplayer) const
{
    TArray<FVortexWorldSummary> Worlds = GetWorlds();
    Worlds.RemoveAll([bMultiplayer](const FVortexWorldSummary& World) { return World.bMultiplayer != bMultiplayer; });
    return Worlds;
}

int32 UVortexGameInstance::MaxHeroes(FGuid WorldId) const
{
    if (WorldId == TravellerWorld()) return 3;
    for (const FVortexWorldSummary& World : GetWorlds()) if (World.WorldId == WorldId) return World.bMultiplayer ? 3 : 1;
    return 3;
}

TArray<FVortexHeroSummary> UVortexGameInstance::GetHeroes(FGuid WorldId) const
{
    UVortexSaveSubsystem* Saves = const_cast<UVortexGameInstance*>(this)->GetSubsystem<UVortexSaveSubsystem>();
    TArray<FVortexHeroSummary> Out = Saves ? Saves->ListWorldHeroes(WorldId) : TArray<FVortexHeroSummary>();
    // 02/11: num mundo multiplayer o anfitrião também usa os heróis viajantes (os mesmos que levam aos mundos dos amigos):
    // criar um mundo novo não deixa mais o anfitrião sem os heróis dele
    if (Saves && WorldId != TravellerWorld())
        for (const FVortexWorldSummary& World : GetWorlds())
            if (World.WorldId == WorldId && World.bMultiplayer) { Out.Append(Saves->ListWorldHeroes(TravellerWorld())); break; }
    return Out;
}

bool UVortexGameInstance::DeleteHero(FGuid WorldId, FGuid PlayerId, FString& Error)
{
    UVortexSaveSubsystem* Saves = GetSubsystem<UVortexSaveSubsystem>();
    // um herói viajante listado num mundo multiplayer (02/11) é apagado da lista dos viajantes
    if (Saves && (Saves->DeleteWorldHero(WorldId, PlayerId) || Saves->DeleteWorldHero(TravellerWorld(), PlayerId))) return true;
    Error = TEXT("Nao foi possivel excluir este heroi.");
    return false;
}

bool UVortexGameInstance::DeleteWorld(FGuid WorldId, FString& Error)
{
    UVortexSaveSubsystem* Saves = GetSubsystem<UVortexSaveSubsystem>();
    if (Saves && Saves->DeleteHostedWorld(WorldId)) return true;
    Error = TEXT("Não foi possível excluir este mundo.");
    return false;
}

UVortexPlayerSave* UVortexGameInstance::GetSelectedHero() const
{
    UVortexSaveSubsystem* Saves = const_cast<UVortexGameInstance*>(this)->GetSubsystem<UVortexSaveSubsystem>();
    return Saves && SelectedPlayerId.IsValid() ? Saves->LoadLocalPlayer(SelectedPlayerId) : nullptr;
}

FVortexLook UVortexGameInstance::LookFromSave(const UVortexPlayerSave* Hero)
{
    FVortexLook Look;
    if (!Hero) return Look;
    Look.Race = Hero->RaceId;
    Look.Appearance = Hero->AppearanceId;
    Look.Pieces = Hero->Wardrobe;
    for (const TPair<FName, FLinearColor>& Color : Hero->Colors) Look.Colors.Add(Color.Key, Color.Value.ToFColorSRGB());
    return FVortexLookCatalog::Get().Normalized(Look);
}

bool UVortexGameInstance::GetHeroLook(FGuid PlayerId, FVortexLook& OutLook) const
{
    UVortexSaveSubsystem* Saves = GetSubsystem<UVortexSaveSubsystem>();
    const UVortexPlayerSave* Hero = Saves ? Saves->LoadLocalPlayer(PlayerId) : nullptr;
    if (!Hero) return false;
    OutLook = LookFromSave(Hero);
    return true;
}

bool UVortexGameInstance::CreateHero(FGuid WorldId, const FString& Name, FName RaceId, FName AppearanceId,
    const TMap<FName, FName>& Wardrobe, FGuid& OutPlayerId, FString& Error, const TMap<FName, FColor>& Colors)
{
    const FString CleanName = Name.TrimStartAndEnd();
    if (CleanName.IsEmpty() || CleanName.Len() > 24)
    {
        Error = TEXT("Digite um nome de 1 a 24 caracteres para o heroi.");
        return false;
    }
    static const TSet<FName> Races = {TEXT("humano"), TEXT("elfo"), TEXT("diabo"), TEXT("anao"), TEXT("barbaro"), TEXT("morto_vivo")};
    if (!Races.Contains(RaceId)) { Error = TEXT("Raca invalida."); return false; }
    if (AppearanceId != TEXT("male") && AppearanceId != TEXT("female"))
    {
        Error = TEXT("Aparencia invalida."); return false;
    }
    UVortexSaveSubsystem* Saves = GetSubsystem<UVortexSaveSubsystem>();
    // heróis viajantes não pertencem a um mundo deste computador
    const bool bTraveller = WorldId == TravellerWorld();
    if (!Saves || (!bTraveller && !Saves->LoadHostedWorld(WorldId))) { Error = TEXT("Mundo nao encontrado."); return false; }
    // um herói no mundo de um jogador; três no de multiplayer e três viajantes
    const int32 Limit = MaxHeroes(WorldId);
    if (Saves->ListWorldHeroes(WorldId).Num() >= Limit)
    {
        Error = bTraveller ? TEXT("Voce ja tem tres herois viajantes.")
            : Limit == 1 ? TEXT("O mundo de um jogador tem um heroi so.") : TEXT("Este mundo ja tem tres herois.");
        return false;
    }
    UVortexPlayerSave* Hero = Saves->CreateLocalPlayer();
    if (!Hero) { Error = TEXT("Nao foi possivel criar o heroi."); return false; }
    Hero->WorldId = WorldId;
    Hero->HeroName = CleanName;
    Hero->RaceId = RaceId;
    Hero->AppearanceId = AppearanceId;
    Hero->Wardrobe = Wardrobe;
    for (const TPair<FName, FColor>& Color : Colors) Hero->Colors.Add(Color.Key, FLinearColor::FromSRGBColor(Color.Value));
    if (!Saves->SaveLocalPlayer(Hero)) { Error = TEXT("Nao foi possivel salvar o heroi."); return false; }
    OutPlayerId = Hero->Identity.PlayerId;
    return true;
}

bool UVortexGameInstance::OpenWorldWithHero(FGuid WorldId, FGuid PlayerId, bool bHost, FString& Error)
{
    UVortexSaveSubsystem* Saves = GetSubsystem<UVortexSaveSubsystem>();
    const UVortexPlayerSave* Hero = Saves ? Saves->LoadLocalPlayer(PlayerId) : nullptr;
    if (!Hero || (Hero->WorldId != WorldId && Hero->WorldId != TravellerWorld()))
    {
        Error = TEXT("Este heroi nao pertence ao mundo escolhido.");
        return false;
    }
    SelectedPlayerId = PlayerId;
    return OpenHostedWorld(WorldId, bHost, Error);
}

void UVortexGameInstance::OpenArsenal(FGuid PlayerId)
{
    // 08/11: o Arsenal é ferramenta do criador: fora do editor não abre
#if WITH_EDITOR
    UVortexSaveSubsystem* Saves = GetSubsystem<UVortexSaveSubsystem>();
    SelectedPlayerId = PlayerId.IsValid() && Saves && Saves->LoadLocalPlayer(PlayerId) ? PlayerId : FGuid();
    RequestedWorldId.Invalidate();   // mundo provisório (o GameMode não grava)
    bStartedFromFrontEnd = true;
    HideFrontEnd();
    UGameplayStatics::OpenLevel(this, TEXT("/Game/Vortex/Maps/L_Arsenal"));
#endif
}

UTexture2D* UVortexGameInstance::GetMenuBackground()
{
    if (!MenuBackground)
    {
        MenuBackground = LoadObject<UTexture2D>(nullptr, TEXT("/Game/Vortex/UI/Menu/T_MenuBackground.T_MenuBackground"));
    }
    return MenuBackground;
}

bool UVortexGameInstance::CreateAndOpenWorld(const FString& Name, int32 Seed, bool bHost, FString& Error)
{
    FGuid WorldId;
    return CreateWorldOnly(Name, Seed, WorldId, Error) && OpenHostedWorld(WorldId, bHost, Error);
}

bool UVortexGameInstance::CreateWorldOnly(const FString& Name, int32 Seed, FGuid& OutWorldId, FString& Error, bool bMultiplayer)
{
    const FString CleanName = Name.TrimStartAndEnd();
    if (CleanName.IsEmpty() || CleanName.Len() > 40)
    {
        Error = TEXT("Digite um nome de 1 a 40 caracteres.");
        return false;
    }
    UVortexSaveSubsystem* Saves = GetSubsystem<UVortexSaveSubsystem>();
    UVortexWorldSave* Save = Saves ? Saves->CreateNewWorld(Seed, 1) : nullptr;
    if (!Save)
    {
        Error = TEXT("Nao foi possivel criar o mundo neste dispositivo.");
        return false;
    }
    Save->DisplayName = CleanName;
    Save->bMultiplayer = bMultiplayer;
    if (!Saves->SaveHostedWorld(Save))
    {
        Error = TEXT("Nao foi possivel gravar o mundo. Verifique o espaco em disco.");
        return false;
    }
    OutWorldId = Save->WorldId;
    return true;
}

bool UVortexGameInstance::OpenHostedWorld(FGuid WorldId, bool bHost, FString& Error)
{
    UVortexSaveSubsystem* Saves = GetSubsystem<UVortexSaveSubsystem>();
    if (!Saves || !Saves->LoadHostedWorld(WorldId))
    {
        Error = TEXT("O save deste mundo nao foi encontrado ou esta danificado.");
        return false;
    }
    RequestedWorldId = WorldId;
    bStartedFromFrontEnd = true;
    HideFrontEnd();
    // com sala da Epic aberta, o mundo escuta pelo P2P dela; senão, pela rede local (IP) como antes
    const UVortexOnline* Online = GetSubsystem<UVortexOnline>();
    const bool bOnlineRoom = bHost && Online && Online->IsRoomOwner();
    UGameplayStatics::OpenLevel(this, TEXT("/Game/Vortex/Maps/L_TerrainProto"), true,
        bOnlineRoom ? TEXT("listen") : bHost ? TEXT("listen?bIsLanMatch") : TEXT(""));
    return true;
}

bool UVortexGameInstance::JoinOnline(const FString& EosAddress, FString& Error)
{
    APlayerController* Controller = GetFirstLocalPlayerController();
    if (!Controller || !EosAddress.StartsWith(TEXT("EOS:")))
    {
        Error = TEXT("Nenhum jogador local disponivel para conectar.");
        return false;
    }
    bStartedFromFrontEnd = true;
    // o menu fica na tela com "Conectando ao anfitrião..." (some quando o mundo dele carregar); antes ele sumia e o
    // convidado ficava olhando o fundo do menu sem saber se algo acontecia
    RetryHost = EosAddress;
    RetryUntil = FPlatformTime::Seconds() + 120.0;
    RetryCount = 0;
    // entre colchetes, como o OnlineSubsystemEOS faz: sem eles o FURL lê "EOS" como protocolo e o id
    // do anfitrião como nome de mapa, e a viagem falha na hora com "Invalid URL"
    const FString Url = FString::Printf(TEXT("[%s]"), *EosAddress);
    VortexNetLog::Write(FString::Printf(TEXT("Convidado: conectando em %s (heroi %s)"), *Url,
        SelectedPlayerId.IsValid() ? TEXT("escolhido") : TEXT("ainda nao escolhido: cria na sala")));
    Controller->ClientTravel(Url, TRAVEL_Absolute);
    return true;
}

bool UVortexGameInstance::JoinByAddress(const FString& Address, FString& Error)
{
    const FString Clean = Address.TrimStartAndEnd();
    FString Host = Clean, Port;
    const bool bHasPort = Clean.Split(TEXT(":"), &Host, &Port);
    TArray<FString> Octets;
    Host.ParseIntoArray(Octets, TEXT("."), false);
    bool bValid = Octets.Num() == 4;
    for (const FString& Octet : Octets)
    {
        bValid = bValid && !Octet.IsEmpty() && Octet.IsNumeric() &&
            FCString::Atoi(*Octet) >= 0 && FCString::Atoi(*Octet) <= 255;
    }
    if (bHasPort) bValid = bValid && !Port.IsEmpty() && Port.IsNumeric() &&
        FCString::Atoi(*Port) >= 1 && FCString::Atoi(*Port) <= 65535;
    if (!bValid || Clean.Len() > 21)
    {
        Error = TEXT("Digite o IP local do anfitriao, por exemplo 192.168.1.10:7777.");
        return false;
    }
    APlayerController* Controller = GetFirstLocalPlayerController();
    if (!Controller)
    {
        Error = TEXT("Nenhum jogador local disponivel para conectar.");
        return false;
    }
    // o herói viajante escolhido no menu continua selecionado: é ele que entra no mundo do amigo
    RequestedWorldId.Invalidate();
    bStartedFromFrontEnd = true;
    // 02/11: o menu fica na tela ("Conectando...") até o mundo do anfitrião carregar (OnWorldLoaded o esconde). Antes ele
    // sumia na hora e, sem resposta (um código de rede local usado de outra casa), o convidado ficava preso olhando o
    // cenário do menu, sem aviso e sem nada no log
    VortexNetLog::Write(FString::Printf(TEXT("Convidado: conectando pela rede local em %s"), *Clean));
    Controller->ClientTravel(Clean, TRAVEL_Absolute);
    TWeakObjectPtr<UVortexGameInstance> Weak(this);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak, Clean](float)
    {
        UVortexGameInstance* Self = Weak.Get();
        if (!Self) return false;
        const FWorldContext* Context = Self->GetWorldContext();
        if (!Context || !Context->PendingNetGame) return false;   // já chegou (ou já caiu e voltou ao menu)
        VortexNetLog::Write(FString::Printf(TEXT("Convidado: %s nao respondeu em 20 s; desistindo"), *Clean));
        Self->PendingMenuError = TEXT("O anfitrião não respondeu. Um código que começa com L é de rede local: só funciona com os dois no mesmo Wi-Fi. De casas diferentes, o anfitrião precisa abrir a sala online da Epic (código de 5 letras).");
        Self->ReturnToMenu();
        return false;
    }), 20.f);
    return true;
}

bool UVortexGameInstance::OpenLobby(FGuid WorldId, FGuid PlayerId, bool bOnline, FString& Error)
{
    UVortexSaveSubsystem* Saves = GetSubsystem<UVortexSaveSubsystem>();
    const UVortexPlayerSave* Hero = Saves ? Saves->LoadLocalPlayer(PlayerId) : nullptr;
    if (!Saves || !Saves->LoadHostedWorld(WorldId))
    {
        Error = TEXT("O save deste mundo nao foi encontrado ou esta danificado.");
        return false;
    }
    if (!Hero || (Hero->WorldId != WorldId && Hero->WorldId != TravellerWorld()))
    {
        Error = TEXT("Este heroi nao pertence ao mundo escolhido.");
        return false;
    }
    const UVortexOnline* Online = GetSubsystem<UVortexOnline>();
    if (bOnline && !(Online && Online->IsRoomOwner()))
    {
        Error = TEXT("A sala da Epic nao foi aberta.");
        return false;
    }
    ClearHostRetry();
    RequestedWorldId = WorldId;
    SelectedPlayerId = PlayerId;
    bLobbyOnline = bOnline;
    bStartedFromFrontEnd = true;
    HideFrontEnd();
    UE_LOG(LogTemp, Display, TEXT("VORTEX_LOBBY abrindo a sala (%s)"), bOnline ? TEXT("Epic") : TEXT("rede local"));
    UGameplayStatics::OpenLevel(this, TEXT("/Game/Vortex/Maps/L_Menu"), true, bOnline ? TEXT("listen") : TEXT("listen?bIsLanMatch"));
    return true;
}

bool UVortexGameInstance::InLobby() const
{
    const UWorld* World = GetWorld();
    return World && World->GetNetMode() != NM_Standalone && World->GetOutermost()->GetName().EndsWith(TEXT("/L_Menu"));
}

bool UVortexGameInstance::IsLobbyHost() const
{
    const UWorld* World = GetWorld();
    return InLobby() && World->GetNetMode() == NM_ListenServer;
}

FString UVortexGameInstance::GetLobbyAddress() const
{
    ISocketSubsystem* Sockets = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    if (!Sockets) return FString();
    bool bCanBind = false;
    const TSharedRef<FInternetAddr> Local = Sockets->GetLocalHostAddr(*GLog, bCanBind);
    const UWorld* World = GetWorld();
    const int32 Port = World && World->URL.Port > 0 ? World->URL.Port : 7777;
    return FString::Printf(TEXT("%s:%d"), *Local->ToString(false), Port);
}

namespace
{
    // as mesmas letras dos códigos da Epic (sem 0/O, 1/I)
    const TCHAR* CodeAlphabet = TEXT("ABCDEFGHJKLMNPQRSTUVWXYZ23456789");

    FString EncodeBase32(uint64 Value, int32 Digits)
    {
        FString Out;
        for (int32 i = 0; i < Digits; ++i) { Out.InsertAt(0, CodeAlphabet[Value & 31]); Value >>= 5; }
        return Out;
    }

    bool DecodeBase32(const FString& Text, uint64& Out)
    {
        Out = 0;
        for (const TCHAR C : Text)
        {
            const TCHAR* Found = FCString::Strchr(CodeAlphabet, C);
            if (!Found) return false;
            Out = (Out << 5) | uint64(Found - CodeAlphabet);
        }
        return true;
    }
}

// L + 7 letras (IP na porta 7777) ou L + 11 letras (IP e porta)
FString UVortexGameInstance::LanCodeFromAddress(const FString& Address)
{
    FString Host = Address, PortText;
    Address.Split(TEXT(":"), &Host, &PortText);
    TArray<FString> Octets;
    Host.ParseIntoArray(Octets, TEXT("."));
    if (Octets.Num() != 4) return FString();
    uint64 Ip = 0;
    for (const FString& Octet : Octets) Ip = (Ip << 8) | uint64(FMath::Clamp(FCString::Atoi(*Octet), 0, 255));
    const int32 Port = PortText.IsEmpty() ? 7777 : FCString::Atoi(*PortText);
    return Port == 7777 ? TEXT("L") + EncodeBase32(Ip, 7) : TEXT("L") + EncodeBase32((Ip << 16) | uint64(Port & 0xFFFF), 11);
}

bool UVortexGameInstance::AddressFromLanCode(const FString& Code, FString& OutAddress)
{
    FString Clean;
    for (const TCHAR C : Code.ToUpper()) if (FChar::IsAlnum(C)) Clean.AppendChar(C);
    if (!Clean.StartsWith(TEXT("L")) || (Clean.Len() != 8 && Clean.Len() != 12)) return false;
    uint64 Value = 0;
    if (!DecodeBase32(Clean.Mid(1), Value)) return false;
    int32 Port = 7777;
    if (Clean.Len() == 12) { Port = int32(Value & 0xFFFF); Value >>= 16; }
    OutAddress = FString::Printf(TEXT("%d.%d.%d.%d:%d"), int32((Value >> 24) & 255), int32((Value >> 16) & 255), int32((Value >> 8) & 255), int32(Value & 255), Port);
    return true;
}

FString UVortexGameInstance::GetLobbyCode() const
{
    if (bLobbyOnline)
    {
        const UVortexOnline* Online = GetSubsystem<UVortexOnline>();
        return Online ? Online->GetRoomCode() : FString();
    }
    return LanCodeFromAddress(GetLobbyAddress());
}

bool UVortexGameInstance::StartAdventure(FString& Error)
{
    UWorld* World = GetWorld();
    if (!IsLobbyHost() || !RequestedWorldId.IsValid())
    {
        Error = TEXT("So o anfitriao pode comecar a aventura.");
        return false;
    }
    const AGameStateBase* State = World->GetGameState();
    if (State)
    {
        for (const APlayerState* Player : State->PlayerArray)
        {
            const AVortexPlayerState* Lobby = Cast<AVortexPlayerState>(Player);
            if (Lobby && !Lobby->IsLobbyHost() && !Lobby->HasLobbyHero())
            {
                Error = TEXT("Esperando um amigo escolher ou criar o herói dele.");
                return false;
            }
            if (Lobby && !Lobby->IsLobbyHost() && !Lobby->IsLobbyReady())
            {
                Error = FString::Printf(TEXT("Esperando %s ficar pronto."), *Lobby->GetLobbyHero());
                return false;
            }
        }
    }
    VortexNetLog::Write(FString::Printf(TEXT("Anfitriao: comecando a aventura com %d jogadores"), State ? State->PlayerArray.Num() : 1));
    HideFrontEnd();
    // todos viajam juntos; o mundo escuta pelo mesmo caminho da sala (Epic ou rede local)
    World->ServerTravel(bLobbyOnline ? TEXT("/Game/Vortex/Maps/L_TerrainProto?listen") : TEXT("/Game/Vortex/Maps/L_TerrainProto?listen?bIsLanMatch"));
    return true;
}

#if !UE_BUILD_SHIPPING
// Sala de espera, de ponta a ponta (Tools/RunLobbySmoke.ps1):
//   anfitrião  -VortexLobbySmoke -VortexLobbyGuests=N : cria um mundo e um herói de teste, abre a sala
//              na rede local, espera os N convidados prontos, tira a foto sala_espera.png e começa a
//              aventura; no mundo confere que chegaram N+1 heróis com o visual de cada um.
//   convidado  -VortexLobbySmoke -VortexLobbyJoin=IP:porta -VortexLobbyIndex=i : cria um herói
//              viajante, entra na sala, fica pronto e confere que chegou ao mundo com ele.
// Tudo o que o teste cria é apagado no fim.
void UVortexGameInstance::BeginLobbySmoke()
{
    struct FSmoke { int32 Phase = 0; float Time = 0.f; FGuid World, Hero; FString Join; int32 Guests = 0, Index = 0; bool bPass = true; };
    TSharedRef<FSmoke> Smoke = MakeShared<FSmoke>();
    FParse::Value(FCommandLine::Get(), TEXT("VortexLobbyJoin="), Smoke->Join, false);
    FParse::Value(FCommandLine::Get(), TEXT("VortexLobbyGuests="), Smoke->Guests);
    FParse::Value(FCommandLine::Get(), TEXT("VortexLobbyIndex="), Smoke->Index);
    // -VortexLobbyOnline: a sala pela Epic, como os jogadores de verdade. O anfitrião grava o código em
    // Saved/Logs/sala_online.txt; o convidado (-VortexLobbyJoin=online) lê, entra SEM herói e cria o
    // herói já dentro da sala (o fluxo do menu).
    const bool bOnline = FParse::Param(FCommandLine::Get(), TEXT("VortexLobbyOnline"));
    const FString CodeFile = FPaths::ProjectSavedDir() / TEXT("Logs/sala_online.txt");
    const bool bHost = Smoke->Join.IsEmpty();
    TWeakObjectPtr<UVortexGameInstance> Weak(this);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak, Smoke, bHost, bOnline, CodeFile](float Delta) -> bool
    {
        UVortexGameInstance* Self = Weak.Get();
        if (!Self) return false;
        UWorld* World = Self->GetWorld();
        if (!World) return true;
        Smoke->Time += Delta;
        const bool bMenu = World->GetOutermost()->GetName().EndsWith(TEXT("/L_Menu"));
        auto Finish = [&](const TCHAR* Why)
        {
            for (const TPair<const TCHAR*, FGuid>& Slot : { TPair<const TCHAR*, FGuid>(TEXT("Vortex_Player_"), Smoke->Hero), TPair<const TCHAR*, FGuid>(TEXT("Vortex_World_"), Smoke->World) })
            {
                if (!Slot.Value.IsValid()) continue;
                for (const TCHAR* Suffix : { TEXT(""), TEXT("_backup"), TEXT("_staging") })
                    Self->SmokeSlots.Add(FString(Slot.Key) + Slot.Value.ToString(EGuidFormats::Digits) + Suffix);
            }
            Self->DeleteSmokeSlots();
            UE_LOG(LogTemp, Display, TEXT("VORTEX_LOBBY_SMOKE_%s %s %s"), bHost ? TEXT("HOST") : TEXT("GUEST"), Smoke->bPass ? TEXT("PASS") : TEXT("FAIL"), Why);
            FPlatformMisc::RequestExit(false);
            return false;
        };
        static const FName Races[] = { TEXT("elfo"), TEXT("anao"), TEXT("diabo"), TEXT("morto_vivo"), TEXT("barbaro"), TEXT("humano"), TEXT("elfo"), TEXT("anao") };
        switch (Smoke->Phase)
        {
        case 0:
        {
            // menu aberto e o palco pronto
            if (!bMenu || World->GetNetMode() != NM_Standalone || !Self->GetMenuStage() || !Self->GetMenuStage()->IsReady()) return true;
            FString Error;
            const FName Race = Races[FMath::Clamp(Smoke->Index, 0, 7)];
            TMap<FName, FName> Pieces;
            if (bHost)
            {
                Self->CreateWorldOnly(TEXT("Sala de teste"), 4242, Smoke->World, Error, true);
                if (!Smoke->World.IsValid() || !Self->CreateHero(Smoke->World, TEXT("Anfitria"), TEXT("humano"), TEXT("female"), Pieces, Smoke->Hero, Error))
                {
                    Smoke->bPass = false; return Finish(*Error);
                }
                if (bOnline)
                {
                    UVortexOnline* Online = Self->GetSubsystem<UVortexOnline>();
                    if (!Online) { Smoke->bPass = false; return Finish(TEXT("sem o subsistema online")); }
                    IFileManager::Get().Delete(*CodeFile);
                    Smoke->Phase = 10;
                    Online->HostRoom(TEXT("Anfitria"), FVortexOnlineDone::CreateLambda([Weak, Smoke, CodeFile](bool bOk, const FString& Result)
                    {
                        UVortexGameInstance* Inner = Weak.Get();
                        if (!Inner) return;
                        FString OpenError;
                        if (!bOk || !Inner->OpenLobby(Smoke->World, Smoke->Hero, true, OpenError))
                        {
                            UE_LOG(LogTemp, Error, TEXT("VORTEX_LOBBY_SMOKE sala online falhou: %s %s"), *Result, *OpenError);
                            Smoke->bPass = false; Smoke->Phase = 99;
                            return;
                        }
                        FFileHelper::SaveStringToFile(Result, *CodeFile);
                        UE_LOG(LogTemp, Display, TEXT("VORTEX_LOBBY_SMOKE sala online %s"), *Result);
                        Smoke->Phase = 1; Smoke->Time = 0.f;
                    }));
                    return true;
                }
                if (!Self->OpenLobby(Smoke->World, Smoke->Hero, false, Error)) { Smoke->bPass = false; return Finish(*Error); }
            }
            else if (bOnline)
            {
                // convidado pela Epic: espera o código do anfitrião e entra sem herói
                FString Code;
                if (!FFileHelper::LoadFileToString(Code, *CodeFile) || Code.TrimStartAndEnd().Len() != UVortexOnline::CodeLength)
                {
                    if (Smoke->Time > 200.f) { Smoke->bPass = false; return Finish(TEXT("o anfitriao nao abriu a sala online")); }
                    return true;
                }
                UVortexOnline* Online = Self->GetSubsystem<UVortexOnline>();
                if (!Online) { Smoke->bPass = false; return Finish(TEXT("sem o subsistema online")); }
                Self->SetSelectedHero(FGuid());
                Smoke->Phase = 10;
                UE_LOG(LogTemp, Display, TEXT("VORTEX_LOBBY_SMOKE entrando na sala online %s"), *Code);
                Online->JoinRoom(TEXT("Aventureiro"), Code.TrimStartAndEnd(), FVortexOnlineDone::CreateLambda([Weak, Smoke](bool bOk, const FString& Result)
                {
                    UVortexGameInstance* Inner = Weak.Get();
                    if (!Inner) return;
                    FString JoinError;
                    if (!bOk || !Inner->JoinOnline(Result, JoinError))
                    {
                        UE_LOG(LogTemp, Error, TEXT("VORTEX_LOBBY_SMOKE nao entrou: %s %s"), *Result, *JoinError);
                        Smoke->bPass = false; Smoke->Phase = 99;
                        return;
                    }
                    Smoke->Phase = 1; Smoke->Time = 0.f;
                }));
                return true;
            }
            else
            {
                if (!Self->CreateHero(UVortexGameInstance::TravellerWorld(), FString::Printf(TEXT("Viajante %d"), Smoke->Index + 1), Race,
                    Smoke->Index % 2 ? TEXT("female") : TEXT("male"), Pieces, Smoke->Hero, Error))
                {
                    Smoke->bPass = false; return Finish(*Error);
                }
                Self->SetSelectedHero(Smoke->Hero);
                // entra pelo código da sala (o IP vai escondido nele), como o jogador faria
                const FString Code = UVortexGameInstance::LanCodeFromAddress(Smoke->Join);
                FString Address;
                if (!UVortexGameInstance::AddressFromLanCode(Code, Address)) { Smoke->bPass = false; return Finish(TEXT("codigo da rede local invalido")); }
                UE_LOG(LogTemp, Display, TEXT("VORTEX_LOBBY_SMOKE codigo %s -> %s"), *Code, *Address);
                if (!Self->JoinByAddress(Address, Error)) { Smoke->bPass = false; return Finish(*Error); }
            }
            Smoke->Phase = 1; Smoke->Time = 0.f;
            return true;
        }
        case 1:
        {
            if (Smoke->Time > 150.f) { Smoke->bPass = false; return Finish(TEXT("sala nao completou")); }
            if (!Self->InLobby()) return true;
            const AGameStateBase* State = World->GetGameState();
            if (!State) return true;
            if (!bHost)
            {
                // a sala pede o herói de novo (08/11): o convidado do teste escolhe o que já criou
                if (!Self->SelectedPlayerId.IsValid() && Smoke->Hero.IsValid()) Self->SetSelectedHero(Smoke->Hero);
                // entrou sem herói (sala online): cria o viajante agora, dentro da sala, como no menu
                if (!Self->SelectedPlayerId.IsValid())
                {
                    FString Error;
                    TMap<FName, FName> Pieces;
                    FGuid Created;
                    if (!Self->CreateHero(UVortexGameInstance::TravellerWorld(), FString::Printf(TEXT("Viajante %d"), Smoke->Index + 1),
                        Races[FMath::Clamp(Smoke->Index, 0, 7)], TEXT("male"), Pieces, Created, Error))
                    {
                        Smoke->bPass = false; return Finish(*Error);
                    }
                    Smoke->Hero = Created;
                    Self->SetSelectedHero(Created);
                    UE_LOG(LogTemp, Display, TEXT("VORTEX_LOBBY_SMOKE heroi criado dentro da sala"));
                }
                const APlayerController* Controller = Self->GetFirstLocalPlayerController();
                AVortexPlayerState* Mine = Controller ? Controller->GetPlayerState<AVortexPlayerState>() : nullptr;
                if (!Mine || !Mine->HasLobbyHero() || Smoke->Time < 3.f + Smoke->Index) return true;
                Mine->ServerSetLobbyReady(true);
                Smoke->Phase = 2; Smoke->Time = 0.f;
                return true;
            }
            int32 Ready = 0;
            for (const APlayerState* Player : State->PlayerArray)
            {
                const AVortexPlayerState* Lobby = Cast<AVortexPlayerState>(Player);
                Ready += Lobby && !Lobby->IsLobbyHost() && Lobby->HasLobbyHero() && Lobby->IsLobbyReady();
            }
            if (Ready < Smoke->Guests) { Smoke->Time = FMath::Min(Smoke->Time, 149.f); return true; }
            UE_LOG(LogTemp, Display, TEXT("VORTEX_LOBBY_SMOKE sala completa: %d jogadores, %d convidados prontos"), State->PlayerArray.Num(), Ready);
            Smoke->Phase = 2; Smoke->Time = 0.f;
            return true;
        }
        case 2:
            if (!bHost) { Smoke->Phase = 3; Smoke->Time = 0.f; return true; }
            if (Smoke->Time < 3.f) return true;
            if (Smoke->Time - Delta < 3.f)
            {
                FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/sala_espera.png"), true, false);
                return true;
            }
            if (Smoke->Time < 4.5f) return true;
            {
                FString Error;
                if (!Self->StartAdventure(Error)) { Smoke->bPass = false; return Finish(*Error); }
            }
            Smoke->Phase = 3; Smoke->Time = 0.f;
            return true;
        case 3:
        {
            if (bMenu)
            {
                // convidado: o anfitrião saiu (fim do teste) ou a viagem falhou
                // (enquanto ele tenta reconectar no anfitrião que ainda carrega o mundo, o menu aparece por uns segundos)
                if (!bHost && Smoke->Time > 2.f && World->GetNetMode() == NM_Standalone && !Self->IsRetryingHost()) { Smoke->bPass = false; return Finish(TEXT("voltou ao menu sem chegar ao mundo")); }
                if (Smoke->Time > 90.f) { Smoke->bPass = false; return Finish(TEXT("nao saiu da sala")); }
                return true;
            }
            if (Smoke->Time < 14.f) return true;
            {
                const APlayerController* Pc = Self->GetFirstLocalPlayerController();
                const AVortexCharacter* Own = Pc ? Cast<AVortexCharacter>(Pc->GetPawn()) : nullptr;
                const UVortexPlayerSave* Picked = Self->GetSelectedHero();
                if (Smoke->Time < 45.f && !(Own && Picked && Own->GetLook().Race == Picked->RaceId)) return true;
                if (!bHost ? Smoke->Time < 18.f : Smoke->Time < 30.f) return true;
            }
            int32 Heroes = 0;
            FString Who;
            for (TActorIterator<AVortexCharacter> It(World); It; ++It)
            {
                if (!It->GetPlayerState()) continue;
                ++Heroes;
                Who += FString::Printf(TEXT(" %s(%s)"), *It->GetPlayerState()->GetPlayerName(), *It->GetLook().Race.ToString());
            }
            const AGameStateBase* TravelState = World->GetGameState();
            const int32 ExpectedPlayers = Smoke->Guests + 1;
            const bool bAllPlayersPresent = TravelState
                && TravelState->PlayerArray.Num() >= ExpectedPlayers
                && Heroes >= ExpectedPlayers;
            if (!bAllPlayersPresent && Smoke->Time < 60.f) return true;
            if (!bHost && TravelState)
            {
                const APawn* LocalPawn = Self->GetFirstLocalPlayerController()
                    ? Self->GetFirstLocalPlayerController()->GetPawn() : nullptr;
                UE_LOG(LogTemp, Display, TEXT("VORTEX_LOBBY_SMOKE client pawns=%d actors=%d local=%s"),
                    TravelState->PlayerArray.Num(), Heroes, *GetNameSafe(LocalPawn));
                for (const APlayerState* Player : TravelState->PlayerArray)
                {
                    const APawn* Pawn = Player ? Player->GetPawn() : nullptr;
                    UE_LOG(LogTemp, Display, TEXT("VORTEX_LOBBY_SMOKE player=%s pawn=%s role=%d remote=%d hidden=%d pos=%s"),
                        *GetNameSafe(Player), *GetNameSafe(Pawn), Pawn ? int32(Pawn->GetLocalRole()) : -1,
                        Pawn ? int32(Pawn->GetRemoteRole()) : -1, Pawn && Pawn->IsHidden(),
                        Pawn ? *Pawn->GetActorLocation().ToCompactString() : TEXT("-"));
                }
                if (Heroes < TravelState->PlayerArray.Num() && Smoke->Time < 60.f) return true;
            }
            const APlayerController* Controller = Self->GetFirstLocalPlayerController();
            const AVortexCharacter* Mine = Controller ? Cast<AVortexCharacter>(Controller->GetPawn()) : nullptr;
            const UVortexPlayerSave* Selected = Self->GetSelectedHero();
            const bool bMineOk = Mine && Selected && Mine->GetLook().Race == Selected->RaceId;
            UE_LOG(LogTemp, Display, TEXT("VORTEX_LOBBY_SMOKE no mundo: %d herois%s, o meu %s"), Heroes, *Who, bMineOk ? TEXT("com o visual certo") : TEXT("ERRADO"));
            Smoke->bPass &= bMineOk && TravelState
                && TravelState->PlayerArray.Num() == ExpectedPlayers
                && Heroes == ExpectedPlayers;
            if (bHost)
            {
                // os convidados ainda conferem o próprio herói antes do anfitrião fechar
                if (Smoke->Time < 60.f) { Smoke->Phase = 4; return true; }
            }
            if (!bHost) { Smoke->Phase = 5; Smoke->Time = 0.f; return true; }
            return Finish(TEXT("chegaram juntos"));
        }
        case 4:
            if (Smoke->Time < 60.f) return true;
            return Finish(TEXT("chegaram juntos"));
        case 10:
            // esperando a Epic (criar a sala / achar e entrar nela)
            if (Smoke->Time > 120.f) { Smoke->bPass = false; return Finish(TEXT("a Epic nao respondeu")); }
            return true;
        case 99:
            return Finish(TEXT("a sala online falhou (ver VortexRede.log)"));
        default:
            // convidado: espera o anfitrião fechar (volta ao menu) ou sai sozinho
            if ((bMenu && World->GetNetMode() == NM_Standalone) || Smoke->Time > 30.f) return Finish(TEXT("chegou com o herói viajante"));
            return true;
        }
    }));
}
#endif

#if !UE_BUILD_SHIPPING
void UVortexGameInstance::DeleteSmokeSlots()
{
    for (const FString& Slot : SmokeSlots) UGameplayStatics::DeleteGameInSlot(Slot, 0);
}
#endif

UVortexWorldSave* UVortexGameInstance::ConsumeRequestedWorld()
{
    const FGuid Id = RequestedWorldId;
    RequestedWorldId.Invalidate();
    UVortexSaveSubsystem* Saves = GetSubsystem<UVortexSaveSubsystem>();
    return Id.IsValid() && Saves ? Saves->LoadHostedWorld(Id) : nullptr;
}

void UVortexGameInstance::ClearHostRetry()
{
    RetryHost.Reset();
    RetryUntil = 0.0;
    bRetryToWorld = false;
}

void UVortexGameInstance::ReturnToMenu()
{
    if (UWorld* World = GetWorld())
    {
        if (AVortexGameMode* Mode = World->GetAuthGameMode<AVortexGameMode>())
        {
            Mode->SaveActiveWorld();
        }
    }
    RequestedWorldId.Invalidate();
    ClearHostRetry();
    HidePauseMenu();
    VortexHeroWindow::Close();
    // a sala da Epic acaba junto (o anfitrião desfaz; o convidado sai)
    if (UVortexOnline* Online = GetSubsystem<UVortexOnline>()) Online->LeaveRoom();
    bStartedFromFrontEnd = false;
    bLobbyOnline = false;
    UGameplayStatics::OpenLevel(this, TEXT("/Game/Vortex/Maps/L_Menu"));
}

void UVortexGameInstance::HidePauseMenu()
{
    if (PauseWidget.IsValid() && GEngine && GEngine->GameViewport)
    {
        GEngine->GameViewport->RemoveViewportWidgetContent(PauseWidget.ToSharedRef());
    }
    PauseWidget.Reset();
    if (APlayerController* Controller = GetFirstLocalPlayerController())
    {
        Controller->bShowMouseCursor = false;
        Controller->SetInputMode(FInputModeGameOnly());
    }
}

void UVortexGameInstance::TogglePauseMenu()
{
    UWorld* World = GetWorld();
    if (!World || World->GetOutermost()->GetName().EndsWith(TEXT("/L_Menu")) ||
        !GEngine || !GEngine->GameViewport) return;
    if (PauseWidget.IsValid())
    {
        HidePauseMenu();
        return;
    }
    PauseStatus.Empty();
    // menu de pausa no estilo do Vortex, com as opções (Framework/VortexPauseMenu)
    if (!VortexUiHooks::Handlers().MakePauseMenu) return;
    PauseWidget = VortexUiHooks::Handlers().MakePauseMenu(this);
    GEngine->GameViewport->AddViewportWidgetContent(PauseWidget.ToSharedRef(), 110);
    if (APlayerController* Controller = GetFirstLocalPlayerController())
    {
        Controller->bShowMouseCursor = true;
        FInputModeGameAndUI Input;
        Input.SetWidgetToFocus(PauseWidget);
        Controller->SetInputMode(Input);
    }

}

void UVortexGameInstance::OnNetworkFailure(UWorld* World, UNetDriver* NetDriver,
    ENetworkFailure::Type FailureType, const FString& ErrorString)
{
    // 09/11: tudo o que a Unreal avisa, antes de qualquer filtro
    VortexNetLog::Write(FString::Printf(TEXT("FALHA DE REDE tipo=%s mapa=%s modo=%d driver=%s servidor=%d convidados=%d uptime=%.0fs erro: %s"), ENetworkFailure::ToString(FailureType),
        World ? *World->GetMapName() : TEXT("(sem mundo)"), World ? int32(World->GetNetMode()) : -1, NetDriver ? *NetDriver->NetDriverName.ToString() : TEXT("(sem driver)"),
        NetDriver && NetDriver->IsServer() ? 1 : 0, NetDriver ? NetDriver->ClientConnections.Num() : 0, FPlatformTime::Seconds() - GStartTime, *ErrorString));
    // conexão ainda pendente: a Unreal pode mandar o mundo nulo ou o do menu (a viagem da sala para o mundo também cai aqui)
    if (!World) World = GetWorld();
    const bool bPending = NetDriver && NetDriver->NetDriverName == NAME_PendingNetDriver;
    if ((!bStartedFromFrontEnd && !(bPending && ErrorString.Contains(TEXT("RemoteAddr: EOS:")))) || !World || World->GetGameInstance() != this) return;
#if !UE_BUILD_SHIPPING
    FString JoinSmokeAddress;
    if (FParse::Value(FCommandLine::Get(), TEXT("VortexMenuJoinSmoke="), JoinSmokeAddress))
    {
        UE_LOG(LogTemp, Error, TEXT("VORTEX_MENU_JOIN_SMOKE_FAIL network: %s"), *ErrorString);
        FPlatformMisc::RequestExit(false);
        return;
    }
#endif
    VortexNetLog::Write(FString::Printf(TEXT("QUEDA DE REDE (%s) no mapa %s, modo %d: %s"), ENetworkFailure::ToString(FailureType),
        *World->GetMapName(), int32(World->GetNetMode()), *ErrorString));
    // 08/11: no anfitrião, a falha é da conexão de UM convidado (o jogo dele fechou): ele sai sozinho e a partida continua.
    // Antes o anfitrião voltava ao menu e derrubava todo mundo
    if (World->GetNetMode() == NM_ListenServer && NetDriver && !NetDriver->ServerConnection) return;
    // ainda conectando num anfitrião da Epic (a entrada pelo código ou a viagem da sala para o mundo): ele pode estar
    // carregando o mapa, que trava o jogo dele uns 15 s e recusa quem chega; tenta de novo a cada 3 s por até 2 minutos
    if (NetDriver && NetDriver->NetDriverName == NAME_PendingNetDriver)
    {
        FString Host;
        const int32 At = ErrorString.Find(TEXT("RemoteAddr: EOS:"));
        if (At != INDEX_NONE)
        {
            Host = ErrorString.Mid(At + 12);
            int32 End = 0;
            while (End < Host.Len() && (FChar::IsAlnum(Host[End]) || Host[End] == TEXT(':'))) ++End;
            Host = Host.Left(End);
            if (Host.EndsWith(TEXT(":"))) Host.LeftChopInline(1);
        }
        if (!Host.IsEmpty())
        {
            // 08/11: só insiste quem está entrando pelo código ou indo da sala para o mundo (ExpectHostTravel). Antes qualquer falha religava
            // os 2 minutos: o amigo que caiu ficava tentando e entrava sozinho na próxima sala aberta pelo anfitrião
            // a viagem da sala para o mundo: o anfitrião recusa enquanto carrega o mapa (~15 s), então aqui o convidado pode insistir
            if (RetryUntil <= 0.0 && World->GetNetMode() == NM_Client && World->GetOutermost()->GetName().EndsWith(TEXT("/L_Menu")))
            {
                RetryUntil = FPlatformTime::Seconds() + 60.0;
                bRetryToWorld = true;
            }
            RetryHost = Host;
            if (FPlatformTime::Seconds() < RetryUntil)
            {
                ++RetryCount;
                VortexNetLog::Write(FString::Printf(TEXT("Convidado: o anfitriao ainda nao aceitou (tentativa %d), tentando de novo em 3 s"), RetryCount));
                PendingMenuError = FString::Printf(TEXT("Esperando o anfitrião carregar o mundo... (tentativa %d)"), RetryCount);
                TWeakObjectPtr<UVortexGameInstance> Weak(this);
                FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak](float)
                {
                    UVortexGameInstance* Self = Weak.Get();
                    if (!Self || Self->RetryHost.IsEmpty()) return false;
                    APlayerController* Controller = Self->GetFirstLocalPlayerController();
                    if (!Controller) return true;   // o menu ainda está abrindo: espera
                    Self->bStartedFromFrontEnd = true;
                    Controller->ClientTravel(FString::Printf(TEXT("[%s]"), *Self->RetryHost), TRAVEL_Absolute);
                    return false;
                }), 3.f);
                return;
            }
            RetryHost.Reset();
            RetryUntil = 0.0;
        }
    }
    FString Why;
    switch (FailureType)
    {
    case ENetworkFailure::OutdatedClient:
    case ENetworkFailure::OutdatedServer: Why = TEXT("vocês estão com versões diferentes do jogo. Atualizem os dois e tentem de novo."); break;
    case ENetworkFailure::ConnectionTimeout: Why = TEXT("o anfitrião parou de responder (internet ou o jogo dele travou)."); break;
    case ENetworkFailure::PendingConnectionFailure: Why = TEXT("não deu para conectar no anfitrião. Peça para ele abrir a sala de novo."); break;
    default: Why = ErrorString.Left(120); break;
    }
    PendingMenuError = TEXT("Conexão encerrada: ") + Why;
    ReturnToMenu();
}

void UVortexGameInstance::OnTravelFailure(UWorld* World, ETravelFailure::Type FailureType,
    const FString& ErrorString)
{
    VortexNetLog::Write(FString::Printf(TEXT("FALHA DE VIAGEM tipo=%s mapa=%s erro: %s"), ETravelFailure::ToString(FailureType), World ? *World->GetMapName() : TEXT("(sem mundo)"), *ErrorString));
    if (!bStartedFromFrontEnd || !World || World->GetGameInstance() != this) return;
    VortexNetLog::Write(FString::Printf(TEXT("FALHA NA VIAGEM (%s) no mapa %s: %s"), ETravelFailure::ToString(FailureType), *World->GetMapName(), *ErrorString));
    PendingMenuError = FString::Printf(TEXT("Não foi possível entrar: %s"), *ErrorString.Left(120));
    ReturnToMenu();
}

void UVortexGameInstance::HideFrontEnd()
{
    if (FrontEndWidget.IsValid() && GEngine && GEngine->GameViewport)
    {
        GEngine->GameViewport->RemoveViewportWidgetContent(FrontEndWidget.ToSharedRef());
    }
    FrontEndWidget.Reset();
}

void UVortexGameInstance::OnWorldLoaded(UWorld* LoadedWorld)
{
    if (!LoadedWorld || LoadedWorld != GetWorld() || !LoadedWorld->IsGameWorld()) return;
    // chegou no anfitrião: as tentativas de reconectar param
    const bool bClientInLobby = LoadedWorld->GetNetMode() == NM_Client && LoadedWorld->GetOutermost()->GetName().EndsWith(TEXT("/L_Menu"));
    // insistia para chegar ao mundo e caiu numa sala nova: o anfitrião saiu e abriu outra. Não entra sem ter sido chamado
    if (bClientInLobby && bRetryToWorld)
    {
        ClearHostRetry();
        PendingMenuError = TEXT("O anfitrião encerrou a partida.");
        TWeakObjectPtr<UVortexGameInstance> Weak(this);
        FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak](float) { if (Weak.IsValid()) Weak->ReturnToMenu(); return false; }));
        return;
    }
    if (LoadedWorld->GetNetMode() == NM_Client) { ClearHostRetry(); RetryCount = 0; }
    // 08/11: numa sala nova o amigo escolhe ou cria o herói (o último usado só vale para voltar a um mundo em andamento)
    if (LoadedWorld->GetNetMode() == NM_Client && LoadedWorld->GetOutermost()->GetName().EndsWith(TEXT("/L_Menu"))) SelectedPlayerId.Invalidate();
    // 02/11: o herói viajante que entrou no mundo de um amigo fica lembrado: saindo e voltando pelo código, ele volta com
    // o mesmo herói (antes voltava sem nenhum, e o jogo dava um herói novo de nível 1 no meio do mapa)
    if (LoadedWorld->GetNetMode() == NM_Client && SelectedPlayerId.IsValid() && !LoadedWorld->GetOutermost()->GetName().EndsWith(TEXT("/L_Menu")))
    {
        GConfig->SetString(TEXT("Vortex.Multiplayer"), TEXT("LastTravellerHero"), *SelectedPlayerId.ToString(), GGameUserSettingsIni);
        GConfig->Flush(false, GGameUserSettingsIni);
        VortexNetLog::Write(FString::Printf(TEXT("Convidado: chegou ao mundo com o heroi %s"), *SelectedPlayerId.ToString()));
    }
    HideFrontEnd();
    HidePauseMenu();
    VortexGraphics::Apply(LoadedWorld);
    // efeitos dos golpes e poderes carregados agora, e não no primeiro uso (era uma travada grande)
    if (!LoadedWorld->GetOutermost()->GetName().EndsWith(TEXT("/L_Menu"))) VortexHitFx::Warmup(LoadedWorld);
    const bool bMenuMap = LoadedWorld->GetOutermost()->GetName().EndsWith(TEXT("/L_Menu"));
#if !UE_BUILD_SHIPPING
    const bool bFlowSmoke = FParse::Param(FCommandLine::Get(), TEXT("VortexMenuFlowSmoke"));
    FString JoinSmokeAddress;
    const bool bJoinSmoke = FParse::Value(FCommandLine::Get(), TEXT("VortexMenuJoinSmoke="), JoinSmokeAddress);
    if (bJoinSmoke && !bMenuMap && bMenuJoinSmokeStarted)
    {
        FTimerHandle JoinCheckTimer;
        LoadedWorld->GetTimerManager().SetTimer(JoinCheckTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
        {
            const UWorld* Current = GetWorld();
            const AVortexGameState* State = Current ? Current->GetGameState<AVortexGameState>() : nullptr;
            const bool bPassed = Current && Current->GetNetMode() == NM_Client && State &&
                State->GetActiveWorldId().IsValid() && State->GetConnectedPlayers() >= 1;
            UE_LOG(LogTemp, Display, TEXT("VORTEX_MENU_JOIN_SMOKE_%s"), bPassed ? TEXT("PASS") : TEXT("FAIL"));
            FPlatformMisc::RequestExit(false);
        }), 4.f, false);
    }
    if (bFlowSmoke && !bMenuMap && MenuFlowSmokePhase == 1)
    {
        const AVortexGameState* State = LoadedWorld->GetGameState<AVortexGameState>();
        const APlayerController* Host = LoadedWorld->GetFirstPlayerController();
        const APawn* Hero = Host ? Host->GetPawn() : nullptr;
        if (Hero) MenuFlowSmokeLocation = Hero->GetActorLocation();
        bMenuFlowSmokePassed = bMenuFlowSmokePassed && State &&
            State->GetActiveWorldId() == MenuFlowSmokeWorldId && State->GetWorldSeed() == 271828 && Hero;
        MenuFlowSmokePhase = 2;
        LoadedWorld->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
        {
            ReturnToMenu();
        }));
    }
    else if (bFlowSmoke && !bMenuMap && MenuFlowSmokePhase == 3)
    {
        MenuFlowSmokePhase = 4;
        FTimerHandle ResumeCheck;
        LoadedWorld->GetTimerManager().SetTimer(ResumeCheck, FTimerDelegate::CreateWeakLambda(this, [this]()
        {
            const UWorld* Current = GetWorld();
            const APlayerController* Host = Current ? Current->GetFirstPlayerController() : nullptr;
            const AVortexCharacter* Hero = Host ? Cast<AVortexCharacter>(Host->GetPawn()) : nullptr;
            bMenuFlowSmokePassed = bMenuFlowSmokePassed && Hero &&
                FVector::Dist2D(Hero->GetActorLocation(), MenuFlowSmokeLocation) < 150.f &&
                Hero->GetCharacterMovement()->MovementMode == MOVE_Walking;
            ReturnToMenu();
        }), 5.f, false);
    }
#endif
    if (!bMenuMap)
    {
        if (SelectedPlayerId.IsValid())
        {
            // o convidado só recebe o herói depois de conectar: tenta de novo até ele existir
            ApplyLookTries = 0;
            LoadedWorld->GetTimerManager().SetTimer(ApplyLookTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
            {
                APlayerController* Controller = GetFirstLocalPlayerController();
                AVortexCharacter* Character = Controller ? Cast<AVortexCharacter>(Controller->GetPawn()) : nullptr;
                UVortexPlayerSave* Hero = GetSelectedHero();
                const bool bGiveUp = ++ApplyLookTries > 240 || !Hero;
                if (Character && Hero)
                {
                    // raça, peças, cores e tamanho escolhidos na criação
                    Character->ApplyLook(LookFromSave(Hero));
                    UE_LOG(LogTemp, Display, TEXT("VORTEX_HERO_LOADED name=%s race=%s"), *Hero->HeroName, *Hero->RaceId.ToString());
                }
                if ((Character && Hero) || bGiveUp) if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(ApplyLookTimer);
            }), .25f, true, 0.f);
        }
        return;
    }
    if (!GEngine || !GEngine->GameViewport || !FSlateApplication::IsInitialized()) return;

    // palco 3D atrás do menu: cenário, herói e música
    FActorSpawnParameters StageParams;
    StageParams.ObjectFlags |= RF_Transient;
    MenuStage = LoadedWorld->SpawnActor<AVortexMenuStage>(FVector(0.f, 0.f, 100000.f), FRotator::ZeroRotator, StageParams);
    if (InLobby())
    {
        UE_LOG(LogTemp, Display, TEXT("VORTEX_LOBBY no mapa da sala (%s)"), IsLobbyHost() ? TEXT("anfitriao") : TEXT("convidado"));
        // 02/11: no VortexRede.log, que tipo de sala abriu e o código (a rede local só serve na mesma casa)
        if (IsLobbyHost()) VortexNetLog::Write(FString::Printf(TEXT("Sala aberta (%s), codigo %s"),
            bLobbyOnline ? TEXT("online pela Epic") : TEXT("rede local: so amigos no mesmo Wi-Fi"), *GetLobbyCode()));
        else VortexNetLog::Write(TEXT("Convidado: chegou na sala do anfitriao"));
        // cada um conta à sala quem é o próprio herói (nome, nível e visual), assim que o estado
        // do jogador chega pela rede; repete enquanto o servidor não confirmar
        LoadedWorld->GetTimerManager().SetTimer(LobbyHeroTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
        {
            if (!InLobby()) { if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(LobbyHeroTimer); return; }
            const APlayerController* Controller = GetFirstLocalPlayerController();
            AVortexPlayerState* Mine = Controller ? Controller->GetPlayerState<AVortexPlayerState>() : nullptr;
            const UVortexPlayerSave* Hero = GetSelectedHero();
            if (!Mine || !Hero || Mine->GetLobbyHero() == Hero->HeroName) return;
            Mine->ServerSetLobbyHero(Hero->HeroName, FVortexRpgCatalog::LevelFromXp(Hero->Experience),
                AVortexCharacter::LookToNet(LookFromSave(Hero)));
        }), 1.f, true, .3f);
    }
    FrontEndWidget = VortexUiHooks::Handlers().MakeFrontEnd ? VortexUiHooks::Handlers().MakeFrontEnd(this) : nullptr;
    if (!PendingMenuError.IsEmpty())
    {
        if (VortexUiHooks::Handlers().FrontEndNotice) VortexUiHooks::Handlers().FrontEndNotice(FrontEndWidget, PendingMenuError);
        PendingMenuError.Empty();
    }
    if (FrontEndWidget.IsValid()) GEngine->GameViewport->AddViewportWidgetContent(FrontEndWidget.ToSharedRef(), 100);
    if (APlayerController* Controller = GetFirstLocalPlayerController())
    {
        Controller->bShowMouseCursor = true;
        FInputModeUIOnly Input;
        Input.SetWidgetToFocus(FrontEndWidget);
        Controller->SetInputMode(Input);
    }

#if !UE_BUILD_SHIPPING
    if (bMenuMap && FParse::Param(FCommandLine::Get(), TEXT("VortexHeroSmoke")))
    {
        UVortexSaveSubsystem* Saves = GetSubsystem<UVortexSaveSubsystem>();
        // mundo de multiplayer: até 3 heróis (o de um jogador, abaixo, aceita um só)
        UVortexWorldSave* WorldSave = Saves ? Saves->CreateNewWorld(13579, 1) : nullptr;
        if (WorldSave) WorldSave->bMultiplayer = true;
        UVortexWorldSave* SoloSave = Saves ? Saves->CreateNewWorld(24680, 1) : nullptr;
        FGuid PlayerIds[4], SoloIds[2];
        FString Error;
        bool bPassed = WorldSave != nullptr && SoloSave != nullptr;
        if (bPassed) bPassed = Saves->SaveHostedWorld(WorldSave) && Saves->SaveHostedWorld(SoloSave);
        for (int32 Index = 0; Index < 3 && bPassed; ++Index)
        {
            TMap<FName, FName> Pieces;
            Pieces.Add(TEXT("Hair"), Index == 0 ? TEXT("hair_female_2") : TEXT("hair_male_1"));
            bPassed = CreateHero(WorldSave->WorldId, FString::Printf(TEXT("Hero %d"), Index),
                Index == 0 ? TEXT("elfo") : TEXT("humano"), Index == 0 ? TEXT("female") : TEXT("male"), Pieces, PlayerIds[Index], Error);
        }
        if (bPassed)
        {
            const TArray<FVortexHeroSummary> Heroes = GetHeroes(WorldSave->WorldId);
            const UVortexPlayerSave* Reloaded = Saves->LoadLocalPlayerFromDisk(PlayerIds[0]);
            bPassed = Heroes.Num() == 3 && Reloaded && Reloaded->HeroName == TEXT("Hero 0") &&
                Reloaded->RaceId == TEXT("elfo") && Reloaded->AppearanceId == TEXT("female") &&
                Reloaded->Wardrobe.FindRef(TEXT("Hair")) == TEXT("hair_female_2") &&
                !CreateHero(WorldSave->WorldId, TEXT("Fourth"), TEXT("humano"), TEXT("male"), {}, PlayerIds[3], Error);
            FString DeleteError;
            bPassed = bPassed && !DeleteHero(FGuid::NewGuid(), PlayerIds[0], DeleteError) &&
                DeleteHero(WorldSave->WorldId, PlayerIds[0], DeleteError) &&
                GetHeroes(WorldSave->WorldId).Num() == 2;
            FString SoloError;
            bPassed = bPassed && CreateHero(SoloSave->WorldId, TEXT("Solo"), TEXT("humano"), TEXT("male"), {}, SoloIds[0], SoloError) &&
                !CreateHero(SoloSave->WorldId, TEXT("Solo 2"), TEXT("humano"), TEXT("male"), {}, SoloIds[1], SoloError) &&
                GetWorlds(true).ContainsByPredicate([&](const FVortexWorldSummary& W) { return W.WorldId == WorldSave->WorldId; }) &&
                GetWorlds(false).ContainsByPredicate([&](const FVortexWorldSummary& W) { return W.WorldId == SoloSave->WorldId; });
            if (!bPassed && Error.IsEmpty()) Error = TEXT("mundo de um jogador ou listas por modo");
        }
        for (const FGuid& Id : SoloIds)
        {
            if (!Id.IsValid()) continue;
            const FString Slot = TEXT("Vortex_Player_") + Id.ToString(EGuidFormats::Digits);
            for (const TCHAR* Suffix : { TEXT(""), TEXT("_backup"), TEXT("_staging") })
                UGameplayStatics::DeleteGameInSlot(Slot + Suffix, 0);
        }
        if (SoloSave)
        {
            const FString Slot = TEXT("Vortex_World_") + SoloSave->WorldId.ToString(EGuidFormats::Digits);
            for (const TCHAR* Suffix : { TEXT(""), TEXT("_backup"), TEXT("_staging") })
                UGameplayStatics::DeleteGameInSlot(Slot + Suffix, 0);
        }
        for (const FGuid& Id : PlayerIds)
        {
            if (!Id.IsValid()) continue;
            const FString Slot = TEXT("Vortex_Player_") + Id.ToString(EGuidFormats::Digits);
            for (const TCHAR* Suffix : { TEXT(""), TEXT("_backup"), TEXT("_staging") })
                UGameplayStatics::DeleteGameInSlot(Slot + Suffix, 0);
        }
        if (WorldSave)
        {
            const FString Slot = TEXT("Vortex_World_") + WorldSave->WorldId.ToString(EGuidFormats::Digits);
            for (const TCHAR* Suffix : { TEXT(""), TEXT("_backup"), TEXT("_staging") })
                UGameplayStatics::DeleteGameInSlot(Slot + Suffix, 0);
        }
        UE_LOG(LogTemp, Display, TEXT("VORTEX_HERO_SMOKE_%s %s"), bPassed ? TEXT("PASS") : TEXT("FAIL"), *Error);
        FPlatformMisc::RequestExit(false);
        return;
    }
    if (bJoinSmoke && !bMenuJoinSmokeStarted)
    {
        bMenuJoinSmokeStarted = true;
        LoadedWorld->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this, JoinSmokeAddress]()
        {
            FString Error;
            if (!JoinByAddress(JoinSmokeAddress, Error))
            {
                UE_LOG(LogTemp, Error, TEXT("VORTEX_MENU_JOIN_SMOKE_FAIL start: %s"), *Error);
                FPlatformMisc::RequestExit(false);
            }
        }));
        return;
    }
    if (bFlowSmoke)
    {
        if (MenuFlowSmokePhase == 0)
        {
            UVortexSaveSubsystem* Saves = GetSubsystem<UVortexSaveSubsystem>();
            UVortexWorldSave* Temp = Saves ? Saves->CreateNewWorld(271828, 1) : nullptr;
            bMenuFlowSmokePassed = bMenuFlowSmokePassed && Temp != nullptr;
            if (Temp)
            {
                Temp->DisplayName = TEXT("Menu flow temporary world");
                MenuFlowSmokeWorldId = Temp->WorldId;
                bMenuFlowSmokePassed = bMenuFlowSmokePassed && Saves->SaveHostedWorld(Temp);
            }
            MenuFlowSmokePhase = 1;
            LoadedWorld->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
            {
                FString Error;
                if (!MenuFlowSmokeWorldId.IsValid() || !OpenHostedWorld(MenuFlowSmokeWorldId, false, Error))
                {
                    UE_LOG(LogTemp, Error, TEXT("VORTEX_MENU_FLOW_SMOKE_FAIL open: %s"), *Error);
                    FPlatformMisc::RequestExit(false);
                }
            }));
        }
        else if (MenuFlowSmokePhase == 2)
        {
            UVortexSaveSubsystem* Saves = GetSubsystem<UVortexSaveSubsystem>();
            const UVortexWorldSave* Loaded = Saves ? Saves->LoadHostedWorld(MenuFlowSmokeWorldId) : nullptr;
            bMenuFlowSmokePassed = bMenuFlowSmokePassed && Loaded && Loaded->Seed == 271828 &&
                Loaded->DisplayName == TEXT("Menu flow temporary world") && Loaded->bHasHostLocation &&
                FVector::Dist2D(Loaded->HostLocation, MenuFlowSmokeLocation) < 10.f &&
                Loaded->NpcStates.Num() > 0;
            bMenuFlowSmokePassed = bMenuFlowSmokePassed && Saves && Saves->ListHostedWorlds().ContainsByPredicate(
                [this](const FVortexWorldSummary& Item) { return Item.WorldId == MenuFlowSmokeWorldId; });
            MenuFlowSmokePhase = 3;
            LoadedWorld->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
            {
                FString Error;
                if (!OpenHostedWorld(MenuFlowSmokeWorldId, false, Error))
                {
                    UE_LOG(LogTemp, Error, TEXT("VORTEX_MENU_FLOW_SMOKE_FAIL reopen: %s"), *Error);
                    FPlatformMisc::RequestExit(false);
                }
            }));
        }
        else if (MenuFlowSmokePhase == 4)
        {
            const FString Slot = TEXT("Vortex_World_") + MenuFlowSmokeWorldId.ToString(EGuidFormats::Digits);
            for (const TCHAR* Suffix : { TEXT(""), TEXT("_backup"), TEXT("_staging") })
            {
                UGameplayStatics::DeleteGameInSlot(Slot + Suffix, 0);
            }
            UE_LOG(LogTemp, Display, TEXT("VORTEX_MENU_FLOW_SMOKE_%s"), bMenuFlowSmokePassed ? TEXT("PASS") : TEXT("FAIL"));
            FPlatformMisc::RequestExit(false);
        }
        return;
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexMenuSmoke")))
    {
        UVortexSaveSubsystem* Saves = GetSubsystem<UVortexSaveSubsystem>();
        UVortexWorldSave* Temp = Saves ? Saves->CreateNewWorld(314159, 1) : nullptr;
        bool bPassed = Temp != nullptr;
        if (Temp)
        {
            Temp->DisplayName = TEXT("Menu smoke temporary world");
            bPassed = bPassed && Saves->SaveHostedWorld(Temp);
            const UVortexWorldSave* Loaded = Saves->LoadHostedWorld(Temp->WorldId);
            bPassed = bPassed && Loaded && Loaded->Seed == 314159 && Loaded->DisplayName == Temp->DisplayName;
            bPassed = bPassed && Saves->ListHostedWorlds().ContainsByPredicate([Temp](const FVortexWorldSummary& Item)
            {
                return Item.WorldId == Temp->WorldId && Item.Seed == 314159;
            });
            const FString Slot = TEXT("Vortex_World_") + Temp->WorldId.ToString(EGuidFormats::Digits);
            for (const TCHAR* Suffix : { TEXT(""), TEXT("_backup"), TEXT("_staging") })
            {
                UGameplayStatics::DeleteGameInSlot(Slot + Suffix, 0);
            }
        }
        UE_LOG(LogTemp, Display, TEXT("VORTEX_MENU_SMOKE_%s"), bPassed ? TEXT("PASS") : TEXT("FAIL"));
        FPlatformMisc::RequestExit(false);
    }
    else if (FParse::Param(FCommandLine::Get(), TEXT("VortexMenuShot")))
    {
        FTimerHandle ShotTimer;
        LoadedWorld->GetTimerManager().SetTimer(ShotTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
        {
            if (!FrontEndWidget.IsValid()) return;
            TArray<FColor> Pixels;
            FIntVector Size;
            const bool bCaptured = FSlateApplication::Get().TakeScreenshot(FrontEndWidget.ToSharedRef(), Pixels, Size);
            TArray<uint8> Png;
            if (bCaptured)
            {
                TArray64<uint8> Compressed;
                FImageUtils::PNGCompressImageArray(Size.X, Size.Y,
                    TArrayView64<const FColor>(Pixels.GetData(), Pixels.Num()), Compressed);
                Png.Append(Compressed.GetData(), static_cast<int32>(Compressed.Num()));
            }
            const FString Path = FPaths::ProjectSavedDir() / TEXT("Screenshots/menu_frontend.png");
            IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), true);
            const bool bSaved = Png.Num() > 0 && FFileHelper::SaveArrayToFile(Png, *Path);
            UE_LOG(LogTemp, Display, TEXT("VORTEX_MENU_SHOT_%s %s"), bSaved ? TEXT("PASS") : TEXT("FAIL"), *Path);
            FPlatformMisc::RequestExit(false);
        }), 2.f, false);
    }
#endif
}
