#pragma once

#include "CoreMinimal.h"
#include "Framework/VortexLook.h"

/**
 * A rotina dos moradores da vila (08/11): horários (Data/Population/npc_schedule.json), a malha de caminhos
 * (Data/Population/vila_nav.json, Tools/Town/BuildNpcNav.py) e os passantes sorteados. AVortexNpc pergunta para onde ir
 * a cada hora; UVortexPopulationSubsystem cria os passantes e coloca cada um onde a rotina diz quando ele aparece.
 * Posições em cm (Unreal X, Y).
 */
namespace VortexNpcRoutine
{
    /** Para onde ir agora e o que fazer lá. */
    struct FGoal
    {
        FVector2D At = FVector2D::ZeroVector;
        float Yaw = 0.f;
        bool bFaceYaw = false;   // chegando, vira para Yaw (o posto, o banco); senão fica de frente como chegou
        FName Anim;              // o clipe em laço parado lá (NAME_None: parado)
        bool bSleep = false;     // a porta de casa: entra e some
        bool bSit = false;       // o banco: senta (At é o assento)
        FVector2D Stand = FVector2D::ZeroVector;   // banco: onde parar antes de sentar
        bool bPatrol = false;    // a volta própria do npc_extra.json
        bool bWander = false;    // passeio: troca de lugar a cada 40-90 s
        int32 Block = -1;        // qual horário (muda: novo destino)
        int32 Seat = -1;         // o assento do banco (para não sentar dois no mesmo)
    };

    /** Um passante sorteado (o posto é a porta da casa dele). */
    struct FExtra
    {
        FName Id;
        FString Name, Title;
        FVortexLook Look;
        bool bChild = false;
        FVector2D Home = FVector2D::ZeroVector;
    };

    VORTEX_API bool HasRoutine(FName NpcId);
    /** O destino da hora (Wander: o sorteio do passeio, troca a cada chamada nova com outro número). */
    VORTEX_API bool Plan(FName NpcId, const FVector2D& Post, float PostYaw, float Hour, int32 Wander, FGoal& Out);
    /** O caminho de From até To pela grade da vila (pontos em cm, sem o primeiro). false: fora da grade ou sem caminho. */
    VORTEX_API bool Path(const FVector2D& From, const FVector2D& To, TArray<FVector2D>& Out);
    /** Reservar ou soltar um assento de banco (dois moradores não sentam no mesmo). */
    VORTEX_API bool ClaimSeat(int32 Seat, FName NpcId);
    VORTEX_API void ReleaseSeat(int32 Seat, FName NpcId);
    /** Frases a mais da hora (todos + o tipo do morador). */
    VORTEX_API TArray<FString> Barks(FName NpcId, float Hour);
    /** O som de voz do morador (npc_voz_<tipo>_<n>, Tools/MakeNpcVoices.py) e se é criança. */
    VORTEX_API FName Voice(FName NpcId, int32 Pick);
    /** O gesto de cumprimento da vez (NAME_None: só olha). */
    VORTEX_API FName GreetClip(FName NpcId, int32 Count);
    /** Uma conversa da hora (as falas se alternam entre os dois moradores). */
    VORTEX_API TArray<FString> Conversation(float Hour, int32 Pick);
    VORTEX_API bool IsChild(FName NpcId);
    VORTEX_API bool IsGuard(FName NpcId);
    /** O clipe do ofício (para o teste e as fotos). */
    VORTEX_API FName WorkClip(FName NpcId);
    /** Os passantes (sorteados uma vez com a semente do arquivo). */
    VORTEX_API const TArray<FExtra>& Extras();
    /** A hora do jogo agora (0..24; sem relógio: 10). */
    VORTEX_API float HourOf(const UWorld* World);
}
