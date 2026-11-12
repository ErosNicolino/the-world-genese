#pragma once

#include "Subsystems/GameInstanceSubsystem.h"
#include "VortexOnline.generated.h"

class FSocketSubsystemEOS;
class IEOSPlatformHandle;

DECLARE_DELEGATE_TwoParams(FVortexOnlineDone, bool /*bOk*/, const FString& /*Result ou erro*/);

/**
 * Multiplayer online pela Epic (EOS), como o eos_bridge.gd do Godot:
 * - entra na Epic sem conta (EOS Connect com o id do aparelho, o "login anônimo" do Godot) com o nome do herói;
 * - o anfitrião cria uma sala pública no balde da versão do jogo com o atributo "codigo" (5 letras,
 *   sem letras que confundem) e a sala some quando ele sai;
 * - o convidado procura a sala pelo código (6 tentativas, 2 s entre elas: uma sala nova demora a
 *   aparecer), entra e conecta no anfitrião por P2P da Epic (NetDriverEOS, com relay: sem abrir porta).
 *
 * As credenciais ficam em Data/Online/eos_credentials.json (fora do git; Tools/CopyEosCredentials.py).
 * Sem sala online, os mundos continuam abrindo para a rede local (LAN) como antes.
 */
UCLASS()
class VORTEX_API UVortexOnline : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /** Anfitrião: entra na Epic, cria a sala e devolve o código de 5 letras. */
    void HostRoom(const FString& DisplayName, FVortexOnlineDone Done);
    /** Convidado: acha a sala do código e devolve o endereço do anfitrião ("EOS:<id>") para viajar. */
    void JoinRoom(const FString& DisplayName, const FString& Code, FVortexOnlineDone Done);
    /** Sai da sala (o anfitrião desfaz a sala). */
    void LeaveRoom();

    bool IsBusy() const { return bBusy; }
    bool HasRoom() const { return !LobbyId.IsEmpty(); }
    bool IsRoomOwner() const { return HasRoom() && bOwner; }
    const FString& GetRoomCode() const { return Code; }
    /** O que está acontecendo agora ("Entrando nos serviços da Epic..."), para o menu. */
    const FString& GetStatus() const { return Status; }
    static bool HasCredentials();

    static constexpr int32 CodeLength = 5;
    static constexpr int32 MaxMembers = 8;

private:
    bool EnsurePlatform(FString& Error);
    void EnsureLogin(const FString& DisplayName, TFunction<void(bool, const FString&)> Then);
    void LoginWithDevice(const FString& DisplayName, TFunction<void(bool, const FString&)> Then);
    void SearchRoom(const FString& Wanted, int32 Attempt, FVortexOnlineDone Done, bool bJoin = true);
    void StartSockets();
    void Fail(FVortexOnlineDone& Done, const FString& Message);
    FString Bucket() const;

    TSharedPtr<IEOSPlatformHandle, ESPMode::ThreadSafe> Platform;
    TSharedPtr<FSocketSubsystemEOS> Sockets;
    void* LocalUser = nullptr;   // EOS_ProductUserId
    FString LobbyId, Code, Status, LoginName;
    bool bP2PDiary = false;
    uint64 AuthNotify = 0;   // o aviso de que o login da Epic vai vencer
    bool bOwner = false, bBusy = false;
};
