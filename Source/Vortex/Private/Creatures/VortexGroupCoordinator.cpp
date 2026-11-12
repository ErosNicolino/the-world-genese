#include "Creatures/VortexGroupCoordinator.h"

#include "Creatures/VortexCreature.h"
#include "Framework/VortexCharacter.h"

namespace
{
    bool Alive(const TWeakObjectPtr<AActor>& Actor)
    {
        const IVortexCreature* Creature = Cast<IVortexCreature>(Actor.Get());
        return Actor.IsValid() && (!Creature || !Creature->IsCreatureDead());
    }
}

void UVortexGroupCoordinator::RegisterMember(int32 GroupId, AActor* Member, EVortexGroupRole Role)
{
    if (GroupId < 0 || !Member || !GetWorld() || !GetWorld()->GetAuthGameMode()) return;
    FGroup& Group = Groups.FindOrAdd(GroupId);
    Group.Members.RemoveAll([](const FMember& Entry) { return !Entry.Actor.IsValid(); });
    if (!Group.Members.ContainsByPredicate([Member](const FMember& Entry) { return Entry.Actor.Get() == Member; }))
        Group.Members.Add({Member, Role});
    if (Role == EVortexGroupRole::Leader) Group.Leader = Member;
    Group.Peak = FMath::Max(Group.Peak, Group.Members.Num());
}

void UVortexGroupCoordinator::UnregisterMember(int32 GroupId, AActor* Member)
{
    FGroup* Group = Groups.Find(GroupId);
    if (!Group) return;
    Group->Members.RemoveAll([Member](const FMember& Entry) { return !Entry.Actor.IsValid() || Entry.Actor.Get() == Member; });
    Group->TokenOwners.RemoveAll([Member](const TWeakObjectPtr<AActor>& Owner) { return !Owner.IsValid() || Owner.Get() == Member; });
    if (Group->Leader.Get() == Member) Group->bLeaderFell = true;
    if (Group->Members.IsEmpty()) Groups.Remove(GroupId);
}

void UVortexGroupCoordinator::ReportTarget(int32 GroupId, AVortexCharacter* Target)
{
    if (FGroup* Group = Groups.Find(GroupId); Group && Target) Group->Target = Target;
}

AVortexCharacter* UVortexGroupCoordinator::SharedTarget(int32 GroupId) const
{
    const FGroup* Group = Groups.Find(GroupId);
    return Group ? Group->Target.Get() : nullptr;
}

bool UVortexGroupCoordinator::AcquireAttack(int32 GroupId, AActor* Member, float Now)
{
    if (GroupId < 0) return true;
    FGroup* Group = Groups.Find(GroupId);
    if (!Group || !Member || Now < Group->NextTokenTime ||
        !Group->Members.ContainsByPredicate([Member](const FMember& Entry) { return Entry.Actor.Get() == Member; })) return false;
    Group->TokenOwners.RemoveAll([](const TWeakObjectPtr<AActor>& Owner) { return !Alive(Owner); });
    if (Group->TokenOwners.Contains(Member)) return true;
    if (Group->TokenOwners.Num() >= FMath::Max(1, Group->MaxAttackers)) return false;
    Group->TokenOwners.Add(Member);
    // o segundo atacante entra um pouco depois do primeiro (ataque em pinça, não em fila)
    Group->NextTokenTime = Now + .25f;
    return true;
}

void UVortexGroupCoordinator::ReleaseAttack(int32 GroupId, AActor* Member, float Now)
{
    FGroup* Group = Groups.Find(GroupId);
    if (!Group || !Group->TokenOwners.Contains(Member)) return;
    Group->TokenOwners.Remove(Member);
    Group->NextTokenTime = FMath::Max(Group->NextTokenTime, Now + .35f);
}

int32 UVortexGroupCoordinator::MemberCount(int32 GroupId) const
{
    const FGroup* Group = Groups.Find(GroupId);
    return Group ? Group->Members.Num() : 0;
}

int32 UVortexGroupCoordinator::ActiveTokens(int32 GroupId) const
{
    const FGroup* Group = Groups.Find(GroupId);
    if (!Group) return 0;
    int32 Count = 0;
    for (const TWeakObjectPtr<AActor>& Owner : Group->TokenOwners) Count += Alive(Owner) ? 1 : 0;
    return Count;
}

int32 UVortexGroupCoordinator::MemberIndex(int32 GroupId, const AActor* Member) const
{
    const FGroup* Group = Groups.Find(GroupId);
    if (!Group) return 0;
    int32 Index = 0;
    for (const FMember& Entry : Group->Members)
    {
        if (Entry.Actor.Get() == Member) return Index;
        if (Alive(Entry.Actor)) ++Index;
    }
    return Index;
}

int32 UVortexGroupCoordinator::AliveCount(int32 GroupId) const
{
    const FGroup* Group = Groups.Find(GroupId);
    if (!Group) return 0;
    int32 Count = 0;
    for (const FMember& Entry : Group->Members) Count += Alive(Entry.Actor) ? 1 : 0;
    return Count;
}

int32 UVortexGroupCoordinator::PeakCount(int32 GroupId) const
{
    const FGroup* Group = Groups.Find(GroupId);
    return Group ? Group->Peak : 0;
}

bool UVortexGroupCoordinator::IsBroken(int32 GroupId) const
{
    const FGroup* Group = Groups.Find(GroupId);
    if (!Group || Group->Peak < 3) return false;
    const bool bLeaderDown = Group->bLeaderFell || !Alive(Group->Leader);
    return bLeaderDown && AliveCount(GroupId) < FMath::CeilToInt(Group->Peak * .4f);
}

void UVortexGroupCoordinator::SetMaxAttackers(int32 GroupId, int32 Max)
{
    if (FGroup* Group = Groups.Find(GroupId)) Group->MaxAttackers = FMath::Clamp(Max, 1, 3);
}
