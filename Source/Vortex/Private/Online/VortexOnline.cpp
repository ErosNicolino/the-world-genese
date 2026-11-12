#include "Online/VortexOnline.h"
#include "Framework/VortexNetLog.h"

#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "IEOSSDKManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "SocketSubsystemEOS.h"
#include "SocketSubsystemEOSUtils.h"
#include "TimerManager.h"
#include "Containers/Ticker.h"
#include "Misc/CommandLine.h"

#include "eos_connect.h"
#include "eos_lobby.h"
#include "eos_p2p.h"
#include "eos_sdk.h"

namespace
{
    const TCHAR* CredentialsFile() { return TEXT("Data/Online/eos_credentials.json"); }
    // a mesma regra do Godot: 5 letras, sem I, O, 0 e 1 (que se confundem ao ditar)
    const TCHAR CodeChars[] = TEXT("ABCDEFGHJKLMNPQRSTUVWXYZ23456789");
    constexpr int32 SearchAttempts = 6;
    constexpr float SearchRetrySeconds = 2.f;
    // muda quando a rede do jogo muda: versões diferentes não se encontram
    constexpr int32 ProtocolVersion = 2;
    const char* CodeKey = "codigo";

    /** Chamada assíncrona da EOS: guarda a função até o resultado chegar (no passo do jogo). */
    template<typename TInfo>
    struct TEosCall
    {
        TFunction<void(const TInfo*)> Fn;
        static void EOS_CALL Run(const TInfo* Data)
        {
            if (!EOS_EResult_IsOperationComplete(Data->ResultCode)) return;   // a EOS ainda vai tentar de novo
            TEosCall* Self = static_cast<TEosCall*>(Data->ClientData);
            Self->Fn(Data);
            delete Self;
        }
    };
    template<typename TInfo>
    void* Callback(TFunction<void(const TInfo*)> Fn) { return new TEosCall<TInfo>{ MoveTemp(Fn) }; }

    FString UserString(EOS_ProductUserId User)
    {
        char Buffer[EOS_PRODUCTUSERID_MAX_LENGTH + 1] = {};
        int32_t Length = sizeof(Buffer);
        return User && EOS_ProductUserId_ToString(User, Buffer, &Length) == EOS_EResult::EOS_Success ? FString(UTF8_TO_TCHAR(Buffer)) : FString();
    }

    FString SocketText(const EOS_P2P_SocketId* Socket) { return Socket ? FString(UTF8_TO_TCHAR(Socket->SocketName)) : FString(TEXT("-")); }

    const TCHAR* ClosedReason(EOS_EConnectionClosedReason Reason)
    {
        switch (Reason)
        {
        case EOS_EConnectionClosedReason::EOS_CCR_ClosedByLocalUser: return TEXT("fechada por este jogo");
        case EOS_EConnectionClosedReason::EOS_CCR_ClosedByPeer: return TEXT("fechada pelo outro jogador");
        case EOS_EConnectionClosedReason::EOS_CCR_TimedOut: return TEXT("tempo esgotado (sem resposta)");
        case EOS_EConnectionClosedReason::EOS_CCR_TooManyConnections: return TEXT("conexoes demais");
        case EOS_EConnectionClosedReason::EOS_CCR_InvalidMessage: return TEXT("mensagem invalida");
        case EOS_EConnectionClosedReason::EOS_CCR_InvalidData: return TEXT("dados invalidos");
        case EOS_EConnectionClosedReason::EOS_CCR_ConnectionFailed: return TEXT("a conexao falhou");
        case EOS_EConnectionClosedReason::EOS_CCR_ConnectionClosed: return TEXT("conexao ja encerrada");
        case EOS_EConnectionClosedReason::EOS_CCR_NegotiationFailed: return TEXT("negociacao falhou");
        default: return TEXT("desconhecido");
        }
    }

    FString ResultText(EOS_EResult Result) { return UTF8_TO_TCHAR(EOS_EResult_ToString(Result)); }

    /** O que o P2P da Epic (NetDriverEOS) precisa saber: quem somos e em que mundo. */
    class FVortexEosUtils final : public ISocketSubsystemEOSUtils
    {
    public:
        EOS_ProductUserId User = nullptr;
        FString Session;
        virtual EOS_ProductUserId GetLocalUserId() override { return User; }
        virtual FString GetSessionId() override { return Session; }
        virtual bool IsLoggedIn() override { return User != nullptr; }
        virtual FName GetSubsystemInstanceName() override
        {
            // no editor (PIE) o mundo do jogo é achado pelo nome do contexto; fora dele, o mundo do jogo
            if (GEngine)
            {
                for (const FWorldContext& Context : GEngine->GetWorldContexts())
                {
                    if (Context.WorldType == EWorldType::PIE && Context.World()) return Context.ContextHandle;
                }
            }
            return NAME_None;
        }
    };
}

bool UVortexOnline::HasCredentials()
{
    return FPaths::FileExists(FPaths::ProjectDir() / CredentialsFile());
}

FString UVortexOnline::Bucket() const
{
    return FString::Printf(TEXT("vortex-%d"), ProtocolVersion);
}

void UVortexOnline::Fail(FVortexOnlineDone& Done, const FString& Message)
{
    bBusy = false;
    Status = Message;
    VortexNetLog::Write(TEXT("Epic falhou: ") + Message);
    Done.ExecuteIfBound(false, Message);
}

bool UVortexOnline::EnsurePlatform(FString& Error)
{
    if (Platform.IsValid()) return true;
    FString Text;
    if (!FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir() / CredentialsFile())))
    {
        Error = TEXT("Faltam as credenciais da Epic no jogo (Data/Online/eos_credentials.json; rode Tools/CopyEosCredentials.py).");
        return false;
    }
    TSharedPtr<FJsonObject> Json;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json) || !Json)
    {
        Error = TEXT("As credenciais da Epic estão com defeito.");
        return false;
    }
    IEOSSDKManager* Manager = IEOSSDKManager::Get();
    if (!Manager || !Manager->IsInitialized())
    {
        Error = TEXT("Os serviços da Epic não iniciaram neste computador.");
        return false;
    }
    // os textos precisam viver até a criação (a EOS copia na hora)
    const FTCHARToUTF8 ProductId(*Json->GetStringField(TEXT("product_id")));
    const FTCHARToUTF8 SandboxId(*Json->GetStringField(TEXT("sandbox_id")));
    const FTCHARToUTF8 DeploymentId(*Json->GetStringField(TEXT("deployment_id")));
    const FTCHARToUTF8 ClientId(*Json->GetStringField(TEXT("client_id")));
    const FTCHARToUTF8 ClientSecret(*Json->GetStringField(TEXT("client_secret")));
    FString CacheName = TEXT("EOSCache");
#if !UE_BUILD_SHIPPING
    // teste com dois jogos no mesmo PC (Tools/RunOnlineLobbySmoke.ps1): cada um com o seu cache da Epic
    FParse::Value(FCommandLine::Get(), TEXT("VortexEosCache="), CacheName);
#endif
    const FTCHARToUTF8 Cache(*FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / CacheName));
    EOS_Platform_Options Options = {};
    Options.ApiVersion = EOS_PLATFORM_OPTIONS_API_LATEST;
    Options.ProductId = ProductId.Get();
    Options.SandboxId = SandboxId.Get();
    Options.DeploymentId = DeploymentId.Get();
    Options.ClientCredentials.ClientId = ClientId.Get();
    Options.ClientCredentials.ClientSecret = ClientSecret.Get();
    Options.bIsServer = EOS_FALSE;
    Options.Flags = EOS_PF_DISABLE_OVERLAY;   // sem a sobreposição da Epic (igual ao Godot)
    Options.CacheDirectory = Cache.Get();
    Platform = Manager->CreatePlatform(Options);
    if (!Platform.IsValid())
    {
        Error = TEXT("Não foi possível iniciar os serviços da Epic.");
        return false;
    }
    UE_LOG(LogTemp, Display, TEXT("VORTEX_ONLINE plataforma da Epic pronta"));
    return true;
}

void UVortexOnline::EnsureLogin(const FString& DisplayName, TFunction<void(bool, const FString&)> Then)
{
    if (LocalUser)
    {
        Then(true, FString());
        return;
    }
    Status = TEXT("Entrando nos serviços da Epic...");
    // o "login anônimo" do Godot: um id deste aparelho, criado na primeira vez
    EOS_HConnect Connect = EOS_Platform_GetConnectInterface(*Platform);
    EOS_Connect_CreateDeviceIdOptions Device = {};
    Device.ApiVersion = EOS_CONNECT_CREATEDEVICEID_API_LATEST;
    Device.DeviceModel = "PC";
    TWeakObjectPtr<UVortexOnline> Weak(this);
    auto Create = [Weak, DisplayName, Then, Device]() mutable
    {
        if (!Weak.IsValid()) return;
        EOS_Connect_CreateDeviceId(EOS_Platform_GetConnectInterface(*Weak->Platform), &Device, Callback<EOS_Connect_CreateDeviceIdCallbackInfo>(
            [Weak, DisplayName, Then](const EOS_Connect_CreateDeviceIdCallbackInfo* Data)
            {
                if (!Weak.IsValid()) return;
                if (Data->ResultCode != EOS_EResult::EOS_Success && Data->ResultCode != EOS_EResult::EOS_DuplicateNotAllowed)
                {
                    Then(false, FString::Printf(TEXT("Não foi possível criar o id deste aparelho na Epic (%s)."), *ResultText(Data->ResultCode)));
                    return;
                }
                Weak->LoginWithDevice(DisplayName, Then);
            }), &TEosCall<EOS_Connect_CreateDeviceIdCallbackInfo>::Run);
    };
#if !UE_BUILD_SHIPPING
    // teste com dois jogos no mesmo PC (Tools/RunOnlineLobbySmoke.ps1): o id anônimo fica no perfil do Windows, então o
    // convidado apaga o guardado e cria outro, para ser outro jogador na Epic (como um amigo em outro computador)
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexEosFreshDevice")))
    {
        EOS_Connect_DeleteDeviceIdOptions Delete = {};
        Delete.ApiVersion = EOS_CONNECT_DELETEDEVICEID_API_LATEST;
        EOS_Connect_DeleteDeviceId(Connect, &Delete, Callback<EOS_Connect_DeleteDeviceIdCallbackInfo>(
            [Create](const EOS_Connect_DeleteDeviceIdCallbackInfo*) mutable { Create(); }), &TEosCall<EOS_Connect_DeleteDeviceIdCallbackInfo>::Run);
        return;
    }
#endif
    Create();
}

void UVortexOnline::LoginWithDevice(const FString& DisplayName, TFunction<void(bool, const FString&)> Then)
{
    EOS_HConnect Connect = EOS_Platform_GetConnectInterface(*Platform);
    LoginName = DisplayName;
    const FString Name = DisplayName.TrimStartAndEnd().Left(30);
    const FTCHARToUTF8 NameUtf8(Name.IsEmpty() ? TEXT("Aventureiro") : *Name);
    EOS_Connect_Credentials Credentials = {};
    Credentials.ApiVersion = EOS_CONNECT_CREDENTIALS_API_LATEST;
    Credentials.Type = EOS_EExternalCredentialType::EOS_ECT_DEVICEID_ACCESS_TOKEN;
    Credentials.Token = nullptr;
    EOS_Connect_UserLoginInfo Info = {};
    Info.ApiVersion = EOS_CONNECT_USERLOGININFO_API_LATEST;
    Info.DisplayName = NameUtf8.Get();
    EOS_Connect_LoginOptions Login = {};
    Login.ApiVersion = EOS_CONNECT_LOGIN_API_LATEST;
    Login.Credentials = &Credentials;
    Login.UserLoginInfo = &Info;
    TWeakObjectPtr<UVortexOnline> Weak(this);
    EOS_Connect_Login(Connect, &Login, Callback<EOS_Connect_LoginCallbackInfo>([Weak, Then](const EOS_Connect_LoginCallbackInfo* Data)
    {
        UVortexOnline* Self = Weak.Get();
        if (!Self) return;
        if (Data->ResultCode == EOS_EResult::EOS_Success)
        {
            Self->LocalUser = Data->LocalUserId;
            Self->StartSockets();
            VortexNetLog::Write(FString::Printf(TEXT("Entrou na Epic como %s (P2P %s)"), *UserString(Data->LocalUserId), Self->Sockets.IsValid() ? TEXT("pronto") : TEXT("FALHOU")));
            Then(true, FString());
            return;
        }
        if (Data->ResultCode != EOS_EResult::EOS_InvalidUser || !Data->ContinuanceToken)
        {
            Then(false, FString::Printf(TEXT("Não foi possível entrar nos serviços da Epic (%s). Confira a internet."), *ResultText(Data->ResultCode)));
            return;
        }
        // primeira vez deste aparelho: cria o usuário do jogo
        EOS_Connect_CreateUserOptions Create = {};
        Create.ApiVersion = EOS_CONNECT_CREATEUSER_API_LATEST;
        Create.ContinuanceToken = Data->ContinuanceToken;
        EOS_Connect_CreateUser(EOS_Platform_GetConnectInterface(*Self->Platform), &Create,
            Callback<EOS_Connect_CreateUserCallbackInfo>([Weak, Then](const EOS_Connect_CreateUserCallbackInfo* Created)
            {
                UVortexOnline* Inner = Weak.Get();
                if (!Inner) return;
                if (Created->ResultCode != EOS_EResult::EOS_Success)
                {
                    Then(false, FString::Printf(TEXT("A Epic não criou o jogador (%s)."), *ResultText(Created->ResultCode)));
                    return;
                }
                Inner->LocalUser = Created->LocalUserId;
                Inner->StartSockets();
                VortexNetLog::Write(FString::Printf(TEXT("Jogador novo criado na Epic: %s (P2P %s)"), *UserString(Created->LocalUserId), Inner->Sockets.IsValid() ? TEXT("pronto") : TEXT("FALHOU")));
                Then(true, FString());
            }), &TEosCall<EOS_Connect_CreateUserCallbackInfo>::Run);
    }), &TEosCall<EOS_Connect_LoginCallbackInfo>::Run);
}

void UVortexOnline::StartSockets()
{
    // 09/11: o motivo de cada conexão entre jogadores fechar (a Unreal só diz depois que "o envio falhou")
    if (!bP2PDiary)
    {
        bP2PDiary = true;
        EOS_HP2P P2P = EOS_Platform_GetP2PInterface(*Platform);
        EOS_P2P_AddNotifyPeerConnectionClosedOptions Closed = {};
        Closed.ApiVersion = EOS_P2P_ADDNOTIFYPEERCONNECTIONCLOSED_API_LATEST;
        Closed.LocalUserId = static_cast<EOS_ProductUserId>(LocalUser);
        EOS_P2P_AddNotifyPeerConnectionClosed(P2P, &Closed, nullptr, [](const EOS_P2P_OnRemoteConnectionClosedInfo* Data)
        {
            VortexNetLog::Write(FString::Printf(TEXT("EPIC P2P FECHOU com %s (canal %s): %s"), *UserString(Data->RemoteUserId), *SocketText(Data->SocketId), ClosedReason(Data->Reason)));
        });
        EOS_P2P_AddNotifyPeerConnectionEstablishedOptions Open = {};
        Open.ApiVersion = EOS_P2P_ADDNOTIFYPEERCONNECTIONESTABLISHED_API_LATEST;
        Open.LocalUserId = static_cast<EOS_ProductUserId>(LocalUser);
        EOS_P2P_AddNotifyPeerConnectionEstablished(P2P, &Open, nullptr, [](const EOS_P2P_OnPeerConnectionEstablishedInfo* Data)
        {
            VortexNetLog::Write(FString::Printf(TEXT("EPIC P2P conectou com %s (canal %s, %s, %s)"), *UserString(Data->RemoteUserId), *SocketText(Data->SocketId),
                Data->ConnectionType == EOS_EConnectionEstablishedType::EOS_CET_Reconnection ? TEXT("reconexao") : TEXT("nova"),
                Data->NetworkType == EOS_ENetworkConnectionType::EOS_NCT_RelayedConnection ? TEXT("pelo relay da Epic") : TEXT("direta")));
        });
        EOS_P2P_AddNotifyPeerConnectionInterruptedOptions Interrupted = {};
        Interrupted.ApiVersion = EOS_P2P_ADDNOTIFYPEERCONNECTIONINTERRUPTED_API_LATEST;
        Interrupted.LocalUserId = static_cast<EOS_ProductUserId>(LocalUser);
        EOS_P2P_AddNotifyPeerConnectionInterrupted(P2P, &Interrupted, nullptr, [](const EOS_P2P_OnPeerConnectionInterruptedInfo* Data)
        {
            VortexNetLog::Write(FString::Printf(TEXT("EPIC P2P INTERROMPIDA com %s (canal %s): a Epic vai tentar reconectar"), *UserString(Data->RemoteUserId), *SocketText(Data->SocketId)));
        });
    }
    // 09/11: o login da Epic vence depois de um tempo: sem renovar, quem ficou com a sala aberta não cria nem recebe mais ninguém
    if (!AuthNotify)
    {
        EOS_Connect_AddNotifyAuthExpirationOptions Expiration = {};
        Expiration.ApiVersion = EOS_CONNECT_ADDNOTIFYAUTHEXPIRATION_API_LATEST;
        AuthNotify = EOS_Connect_AddNotifyAuthExpiration(EOS_Platform_GetConnectInterface(*Platform), &Expiration, this, [](const EOS_Connect_AuthExpirationCallbackInfo* Data)
        {
            UVortexOnline* Self = static_cast<UVortexOnline*>(Data->ClientData);
            Self->LoginWithDevice(Self->LoginName, [](bool bOk, const FString& Error) { VortexNetLog::Write(bOk ? FString(TEXT("Login da Epic renovado")) : TEXT("Login da Epic não renovou: ") + Error); });
        });
    }
    if (Sockets.IsValid()) return;
    // o P2P da Epic (relay incluído) para o NetDriverEOS; registrado como o subsistema "EOS"
    TSharedPtr<FVortexEosUtils, ESPMode::ThreadSafe> Utils = MakeShared<FVortexEosUtils, ESPMode::ThreadSafe>();
    Utils->User = static_cast<EOS_ProductUserId>(LocalUser);
    Sockets = MakeShared<FSocketSubsystemEOS>(Platform, Utils);
    FString Error;
    if (!Sockets->Init(Error))
    {
        VortexNetLog::Write(TEXT("P2P da Epic não iniciou: ") + Error);
        Sockets.Reset();
    }
}

void UVortexOnline::HostRoom(const FString& DisplayName, FVortexOnlineDone Done)
{
    if (bBusy) return;
    LeaveRoom();   // 09/11: uma sala anterior não pode ficar aberta na Epic
    bBusy = true;
    FString Error;
    Status = TEXT("Conectando aos serviços da Epic...");
    if (!EnsurePlatform(Error)) return Fail(Done, Error);
    TWeakObjectPtr<UVortexOnline> Weak(this);
    EnsureLogin(DisplayName, [Weak, Done](bool bOk, const FString& LoginError) mutable
    {
        UVortexOnline* Self = Weak.Get();
        if (!Self) return;
        if (!bOk) return Self->Fail(Done, LoginError);
        Self->Status = TEXT("Criando a sala...");
        const FTCHARToUTF8 BucketUtf8(*Self->Bucket());
        EOS_Lobby_CreateLobbyOptions Create = {};
        Create.ApiVersion = EOS_LOBBY_CREATELOBBY_API_LATEST;
        Create.LocalUserId = static_cast<EOS_ProductUserId>(Self->LocalUser);
        Create.MaxLobbyMembers = MaxMembers;
        Create.PermissionLevel = EOS_ELobbyPermissionLevel::EOS_LPL_PUBLICADVERTISED;
        Create.bPresenceEnabled = EOS_FALSE;
        Create.bAllowInvites = EOS_FALSE;
        Create.BucketId = BucketUtf8.Get();
        Create.bDisableHostMigration = EOS_TRUE;   // o mundo mora no anfitrião: se ele sai, a sala acaba
        Create.bEnableRTCRoom = EOS_FALSE;         // sem chat de voz
        EOS_Lobby_CreateLobby(EOS_Platform_GetLobbyInterface(*Self->Platform), &Create,
            Callback<EOS_Lobby_CreateLobbyCallbackInfo>([Weak, Done](const EOS_Lobby_CreateLobbyCallbackInfo* Data) mutable
            {
                UVortexOnline* Owner = Weak.Get();
                if (!Owner) return;
                if (Data->ResultCode != EOS_EResult::EOS_Success)
                {
                    return Owner->Fail(Done, FString::Printf(TEXT("Não foi possível criar a sala na Epic (%s). Confira a internet e tente de novo."), *ResultText(Data->ResultCode)));
                }
                Owner->LobbyId = UTF8_TO_TCHAR(Data->LobbyId);
                Owner->bOwner = true;
                Owner->Code.Reset();
                for (int32 i = 0; i < CodeLength; ++i) Owner->Code.AppendChar(CodeChars[FMath::RandHelper(UE_ARRAY_COUNT(CodeChars) - 1)]);
                // o código vai como atributo público da sala (é por ele que o amigo procura)
                EOS_HLobby Lobby = EOS_Platform_GetLobbyInterface(*Owner->Platform);
                const FTCHARToUTF8 LobbyUtf8(*Owner->LobbyId);
                const FTCHARToUTF8 CodeUtf8(*Owner->Code);
                EOS_Lobby_UpdateLobbyModificationOptions ModOptions = {};
                ModOptions.ApiVersion = EOS_LOBBY_UPDATELOBBYMODIFICATION_API_LATEST;
                ModOptions.LobbyId = LobbyUtf8.Get();
                ModOptions.LocalUserId = static_cast<EOS_ProductUserId>(Owner->LocalUser);
                EOS_HLobbyModification Modification = nullptr;
                if (EOS_Lobby_UpdateLobbyModification(Lobby, &ModOptions, &Modification) != EOS_EResult::EOS_Success)
                {
                    Owner->LeaveRoom();
                    return Owner->Fail(Done, TEXT("A sala foi criada, mas não deu para registrar o código dela."));
                }
                EOS_Lobby_AttributeData Attribute = {};
                Attribute.ApiVersion = EOS_LOBBY_ATTRIBUTEDATA_API_LATEST;
                Attribute.Key = CodeKey;
                Attribute.Value.AsUtf8 = CodeUtf8.Get();
                Attribute.ValueType = EOS_ELobbyAttributeType::EOS_AT_STRING;
                EOS_LobbyModification_AddAttributeOptions Add = {};
                Add.ApiVersion = EOS_LOBBYMODIFICATION_ADDATTRIBUTE_API_LATEST;
                Add.Attribute = &Attribute;
                Add.Visibility = EOS_ELobbyAttributeVisibility::EOS_LAT_PUBLIC;
                EOS_LobbyModification_AddAttribute(Modification, &Add);
                EOS_Lobby_UpdateLobbyOptions Update = {};
                Update.ApiVersion = EOS_LOBBY_UPDATELOBBY_API_LATEST;
                Update.LobbyModificationHandle = Modification;
                EOS_Lobby_UpdateLobby(Lobby, &Update, Callback<EOS_Lobby_UpdateLobbyCallbackInfo>([Weak, Done](const EOS_Lobby_UpdateLobbyCallbackInfo* Updated) mutable
                {
                    UVortexOnline* Inner = Weak.Get();
                    if (!Inner) return;
                    if (Updated->ResultCode != EOS_EResult::EOS_Success)
                    {
                        Inner->LeaveRoom();
                        return Inner->Fail(Done, TEXT("A sala foi criada, mas não deu para registrar o código dela."));
                    }
                    Inner->bBusy = false;
                    Inner->Status = FString::Printf(TEXT("Sala online aberta: %s"), *Inner->Code);
                    VortexNetLog::Write(FString::Printf(TEXT("Sala criada: codigo %s, anfitriao %s"), *Inner->Code, *UserString(static_cast<EOS_ProductUserId>(Inner->LocalUser))));
                    Done.ExecuteIfBound(true, Inner->Code);
                }), &TEosCall<EOS_Lobby_UpdateLobbyCallbackInfo>::Run);
                EOS_LobbyModification_Release(Modification);
            }), &TEosCall<EOS_Lobby_CreateLobbyCallbackInfo>::Run);
    });
}

void UVortexOnline::JoinRoom(const FString& DisplayName, const FString& RoomCode, FVortexOnlineDone Done)
{
    if (bBusy) return;
    const FString Wanted = RoomCode.TrimStartAndEnd().ToUpper();
    LeaveRoom();
    bBusy = true;
    if (Wanted.Len() != CodeLength) return Fail(Done, FString::Printf(TEXT("O código da sala tem %d letras."), CodeLength));
    FString Error;
    Status = TEXT("Conectando aos serviços da Epic...");
    if (!EnsurePlatform(Error)) return Fail(Done, Error);
    TWeakObjectPtr<UVortexOnline> Weak(this);
    EnsureLogin(DisplayName, [Weak, Wanted, Done](bool bOk, const FString& LoginError) mutable
    {
        UVortexOnline* Self = Weak.Get();
        if (!Self) return;
        if (!bOk) return Self->Fail(Done, LoginError);
        Self->SearchRoom(Wanted, 0, Done);
    });
}

void UVortexOnline::SearchRoom(const FString& Wanted, int32 Attempt, FVortexOnlineDone Done, bool bJoin)
{
    Status = FString::Printf(TEXT("Procurando a sala %s..."), *Wanted);
    EOS_HLobby Lobby = EOS_Platform_GetLobbyInterface(*Platform);
    EOS_Lobby_CreateLobbySearchOptions SearchOptions = {};
    SearchOptions.ApiVersion = EOS_LOBBY_CREATELOBBYSEARCH_API_LATEST;
    SearchOptions.MaxResults = 4;
    EOS_HLobbySearch Search = nullptr;
    if (EOS_Lobby_CreateLobbySearch(Lobby, &SearchOptions, &Search) != EOS_EResult::EOS_Success) return Fail(Done, TEXT("A busca de salas da Epic falhou."));
    // mesma versão do jogo (balde) e o código
    const FTCHARToUTF8 BucketUtf8(*Bucket());
    const FTCHARToUTF8 CodeUtf8(*Wanted);
    auto Parameter = [Search](const char* Key, const char* Value)
    {
        EOS_Lobby_AttributeData Data = {};
        Data.ApiVersion = EOS_LOBBY_ATTRIBUTEDATA_API_LATEST;
        Data.Key = Key;
        Data.Value.AsUtf8 = Value;
        Data.ValueType = EOS_ELobbyAttributeType::EOS_AT_STRING;
        EOS_LobbySearch_SetParameterOptions Options = {};
        Options.ApiVersion = EOS_LOBBYSEARCH_SETPARAMETER_API_LATEST;
        Options.Parameter = &Data;
        Options.ComparisonOp = EOS_EComparisonOp::EOS_CO_EQUAL;
        EOS_LobbySearch_SetParameter(Search, &Options);
    };
    Parameter(EOS_LOBBY_SEARCH_BUCKET_ID, BucketUtf8.Get());
    Parameter(CodeKey, CodeUtf8.Get());
    EOS_LobbySearch_FindOptions Find = {};
    Find.ApiVersion = EOS_LOBBYSEARCH_FIND_API_LATEST;
    Find.LocalUserId = static_cast<EOS_ProductUserId>(LocalUser);
    TWeakObjectPtr<UVortexOnline> Weak(this);
    EOS_LobbySearch_Find(Search, &Find, Callback<EOS_LobbySearch_FindCallbackInfo>([Weak, Search, Wanted, Attempt, Done, bJoin](const EOS_LobbySearch_FindCallbackInfo* Data) mutable
    {
        UVortexOnline* Self = Weak.Get();
        if (!Self)
        {
            EOS_LobbySearch_Release(Search);
            return;
        }
        EOS_LobbySearch_GetSearchResultCountOptions CountOptions = {};
        CountOptions.ApiVersion = EOS_LOBBYSEARCH_GETSEARCHRESULTCOUNT_API_LATEST;
        const uint32_t Count = Data->ResultCode == EOS_EResult::EOS_Success ? EOS_LobbySearch_GetSearchResultCount(Search, &CountOptions) : 0;
        if (Count == 0)
        {
            EOS_LobbySearch_Release(Search);
            if (Attempt + 1 >= SearchAttempts)
            {
                return Self->Fail(Done, FString::Printf(TEXT("Nenhuma sala com o código %s existe agora. Confira o código, se a sala ainda está aberta e se vocês dois têm a mesma versão do jogo."), *Wanted));
            }
            // uma sala recém-criada leva alguns segundos para aparecer na busca da Epic
            // no relógio do motor: o do mundo acaba numa troca de mapa e a busca ficava "ocupada" para sempre
            FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(Self, [Self, Wanted, Attempt, Done, bJoin](float)
            {
                Self->SearchRoom(Wanted, Attempt + 1, Done, bJoin);
                return false;
            }), SearchRetrySeconds);
            return;
        }
        EOS_LobbySearch_CopySearchResultByIndexOptions CopyOptions = {};
        CopyOptions.ApiVersion = EOS_LOBBYSEARCH_COPYSEARCHRESULTBYINDEX_API_LATEST;
        CopyOptions.LobbyIndex = 0;
        EOS_HLobbyDetails Details = nullptr;
        const EOS_EResult Copied = EOS_LobbySearch_CopySearchResultByIndex(Search, &CopyOptions, &Details);
        EOS_LobbySearch_Release(Search);
        if (Copied != EOS_EResult::EOS_Success) return Self->Fail(Done, TEXT("A sala foi achada, mas não deu para ler os dados dela."));
        EOS_LobbyDetails_GetLobbyOwnerOptions OwnerOptions = {};
        OwnerOptions.ApiVersion = EOS_LOBBYDETAILS_GETLOBBYOWNER_API_LATEST;
        const FString Host = UserString(EOS_LobbyDetails_GetLobbyOwner(Details, &OwnerOptions));
        if (!bJoin)
        {
            // só procurar (o teste deste PC: achar a própria sala pelo código)
            EOS_LobbyDetails_Release(Details);
            Self->bBusy = false;
            Done.ExecuteIfBound(!Host.IsEmpty(), TEXT("EOS:") + Host);
            return;
        }
        Self->Status = FString::Printf(TEXT("Entrando na sala %s..."), *Wanted);
        EOS_Lobby_JoinLobbyOptions Join = {};
        Join.ApiVersion = EOS_LOBBY_JOINLOBBY_API_LATEST;
        Join.LobbyDetailsHandle = Details;
        Join.LocalUserId = static_cast<EOS_ProductUserId>(Self->LocalUser);
        Join.bPresenceEnabled = EOS_FALSE;
        EOS_Lobby_JoinLobby(EOS_Platform_GetLobbyInterface(*Self->Platform), &Join,
            Callback<EOS_Lobby_JoinLobbyCallbackInfo>([Weak, Wanted, Host, Done](const EOS_Lobby_JoinLobbyCallbackInfo* Joined) mutable
            {
                UVortexOnline* Inner = Weak.Get();
                if (!Inner) return;
                if (Joined->ResultCode != EOS_EResult::EOS_Success || Host.IsEmpty())
                {
                    return Inner->Fail(Done, FString::Printf(TEXT("Não foi possível entrar na sala %s (ela pode estar cheia)."), *Wanted));
                }
                Inner->LobbyId = UTF8_TO_TCHAR(Joined->LobbyId);
                Inner->Code = Wanted;
                Inner->bOwner = false;
                Inner->bBusy = false;
                Inner->Status = TEXT("Conectando ao anfitrião...");
                VortexNetLog::Write(FString::Printf(TEXT("Entrou na sala %s: viajando ate o anfitriao EOS:%s"), *Wanted, *Host));
                Done.ExecuteIfBound(true, TEXT("EOS:") + Host);
            }), &TEosCall<EOS_Lobby_JoinLobbyCallbackInfo>::Run);
        EOS_LobbyDetails_Release(Details);
    }), &TEosCall<EOS_LobbySearch_FindCallbackInfo>::Run);
}

void UVortexOnline::LeaveRoom()
{
    if (!Platform.IsValid() || LobbyId.IsEmpty() || !LocalUser) return;
    const FTCHARToUTF8 LobbyUtf8(*LobbyId);
    EOS_HLobby Lobby = EOS_Platform_GetLobbyInterface(*Platform);
    if (bOwner)
    {
        EOS_Lobby_DestroyLobbyOptions Destroy = {};
        Destroy.ApiVersion = EOS_LOBBY_DESTROYLOBBY_API_LATEST;
        Destroy.LocalUserId = static_cast<EOS_ProductUserId>(LocalUser);
        Destroy.LobbyId = LobbyUtf8.Get();
        EOS_Lobby_DestroyLobby(Lobby, &Destroy, Callback<EOS_Lobby_DestroyLobbyCallbackInfo>([](const EOS_Lobby_DestroyLobbyCallbackInfo*) {}),
            &TEosCall<EOS_Lobby_DestroyLobbyCallbackInfo>::Run);
    }
    else
    {
        EOS_Lobby_LeaveLobbyOptions Leave = {};
        Leave.ApiVersion = EOS_LOBBY_LEAVELOBBY_API_LATEST;
        Leave.LocalUserId = static_cast<EOS_ProductUserId>(LocalUser);
        Leave.LobbyId = LobbyUtf8.Get();
        EOS_Lobby_LeaveLobby(Lobby, &Leave, Callback<EOS_Lobby_LeaveLobbyCallbackInfo>([](const EOS_Lobby_LeaveLobbyCallbackInfo*) {}),
            &TEosCall<EOS_Lobby_LeaveLobbyCallbackInfo>::Run);
    }
    UE_LOG(LogTemp, Display, TEXT("VORTEX_ONLINE saiu da sala %s"), *Code);
    LobbyId.Reset();
    Code.Reset();
    bOwner = false;
    Status.Reset();
}

void UVortexOnline::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
#if !UE_BUILD_SHIPPING
    // -VortexOnlineProbe: entra na Epic, cria a sala e acha ela pelo código (o que dá para testar num PC só)
    if (FParse::Param(FCommandLine::Get(), TEXT("VortexOnlineProbe")))
    {
        FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float)
        {
            HostRoom(TEXT("Teste"), FVortexOnlineDone::CreateWeakLambda(this, [this](bool bOk, const FString& Result)
            {
                if (!bOk)
                {
                    UE_LOG(LogTemp, Display, TEXT("VORTEX_ONLINE_PROBE FAIL %s"), *Result);
                    return;
                }
                bBusy = true;
                SearchRoom(Result, 0, FVortexOnlineDone::CreateWeakLambda(this, [this](bool bFound, const FString& Host)
                {
                    UE_LOG(LogTemp, Display, TEXT("VORTEX_ONLINE_PROBE %s sala=%s anfitriao=%s"), bFound ? TEXT("PASS") : TEXT("FAIL"),
                        *Code, Host.StartsWith(TEXT("EOS:")) && Host.Len() > 10 ? TEXT("ok") : TEXT("-"));
                    LeaveRoom();
                    // o mesmo login que a renovação faz quando o da Epic vence
                    LoginWithDevice(TEXT("Teste"), [](bool bRenewed, const FString& Error) { UE_LOG(LogTemp, Display, TEXT("VORTEX_ONLINE_RENEW %s %s"), bRenewed ? TEXT("PASS") : TEXT("FAIL"), *Error); });
                }), false);
            }));
            return false;
        }), 2.f);
    }
#endif
}

void UVortexOnline::Deinitialize()
{
    LeaveRoom();
    if (AuthNotify && Platform.IsValid())
    {
        EOS_Connect_RemoveNotifyAuthExpiration(EOS_Platform_GetConnectInterface(*Platform), AuthNotify);
        AuthNotify = 0;
    }
    if (IsEngineExitRequested())
    {
        // fechando o jogo: o NetDriverEOS do mundo é destruído DEPOIS (na coleta de lixo da saída) e ainda usa o P2P.
        // Desligar aqui fazia o convidado travar ao sair ("Pure virtual function being called"). Fica vivo até o fim.
        // 09/11: nunca destruídos (new, de propósito): como "static" eles morriam no fim do processo, depois dos módulos da Epic, e chamavam função pura
        new TSharedPtr<FSocketSubsystemEOS>(Sockets);
        new TSharedPtr<IEOSPlatformHandle, ESPMode::ThreadSafe>(Platform);
        Super::Deinitialize();
        return;
    }
    if (Sockets.IsValid())
    {
        Sockets->Shutdown();
        Sockets.Reset();
    }
    Platform.Reset();
    Super::Deinitialize();
}
