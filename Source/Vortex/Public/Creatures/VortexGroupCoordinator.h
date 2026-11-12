#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "VortexGroupCoordinator.generated.h"

class AVortexCharacter;

UENUM(BlueprintType)
enum class EVortexGroupRole : uint8 { Leader, Flanker, Anchor };

/** Server-side group target and exclusive attack token. Members remain ordinary actors. */
UCLASS()
class VORTEX_API UVortexGroupCoordinator : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    void RegisterMember(int32 GroupId, AActor* Member, EVortexGroupRole Role);
    void UnregisterMember(int32 GroupId, AActor* Member);
    void ReportTarget(int32 GroupId, AVortexCharacter* Target);
    AVortexCharacter* SharedTarget(int32 GroupId) const;
    bool AcquireAttack(int32 GroupId, AActor* Member, float Now);
    void ReleaseAttack(int32 GroupId, AActor* Member, float Now);
    int32 MemberCount(int32 GroupId) const;
    int32 ActiveTokens(int32 GroupId) const;
    /** Posição do membro entre os vivos (0 = o primeiro a entrar, normalmente o alfa). */
    int32 MemberIndex(int32 GroupId, const AActor* Member) const;
    /** Quantos estão vivos agora e quantos o grupo já teve. */
    int32 AliveCount(int32 GroupId) const;
    int32 PeakCount(int32 GroupId) const;
    /** Matilha quebrada: o alfa caiu e restam menos de 40% (os que sobram fogem). */
    bool IsBroken(int32 GroupId) const;
    /** Quantos podem atacar ao mesmo tempo (2 em grupos de 4 ou mais). */
    void SetMaxAttackers(int32 GroupId, int32 Max);

private:
    struct FMember { TWeakObjectPtr<AActor> Actor; EVortexGroupRole Role; };
    struct FGroup
    {
        TArray<FMember> Members;
        TWeakObjectPtr<AVortexCharacter> Target;
        TArray<TWeakObjectPtr<AActor>> TokenOwners;
        TWeakObjectPtr<AActor> Leader;
        int32 Peak = 0;
        int32 MaxAttackers = 1;
        bool bLeaderFell = false;
        float NextTokenTime = 0.f;
    };
    TMap<int32, FGroup> Groups;
};
