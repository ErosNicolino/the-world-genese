#include "Creatures/VortexPathGrid.h"

#include "Components/CapsuleComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Framework/VortexCharacter.h"
#include "GameFramework/Character.h"
#include "World/VortexTerrainData.h"
#include "World/VortexTerrainStreamer.h"

namespace VortexPathGrid
{
    namespace
    {
        constexpr int32 N = 40;            // células por lado
        constexpr float Cell = 100.f;      // cm
        constexpr float Climb = 75.f;      // desnível que sobe de uma célula para a vizinha (cm)
        constexpr int32 MaxExpand = 700;   // limite de células abertas por busca

        struct FGrid
        {
            const ACharacter* Self = nullptr;
            TSharedPtr<FVortexTerrainData> Terrain;
            FVector2D Origin = FVector2D::ZeroVector;   // canto da célula (0, 0)
            float Radius = 35.f, Half = 50.f;
            TArray<int8> State;       // 0 não testada, 1 livre, 2 bloqueada
            TArray<float> Ground;
            FIntPoint Start, Goal;

            int32 Index(FIntPoint P) const { return P.Y * N + P.X; }
            bool Inside(FIntPoint P) const { return P.X >= 0 && P.Y >= 0 && P.X < N && P.Y < N; }
            FVector2D Center(FIntPoint P) const { return Origin + FVector2D((P.X + .5f) * Cell, (P.Y + .5f) * Cell); }
            float GroundOf(FIntPoint P)
            {
                const int32 I = Index(P);
                if (State[I] == 0) Test(P);
                return Ground[I];
            }
            bool Free(FIntPoint P)
            {
                if (!Inside(P)) return false;
                const int32 I = Index(P);
                if (State[I] == 0) Test(P);
                return State[I] == 1;
            }
            void Test(FIntPoint P)
            {
                const int32 I = Index(P);
                const FVector2D C = Center(P);
                const float Z = Terrain->GroundAt(C.X, C.Y);
                Ground[I] = Z;
                if (Z <= -299.f) { State[I] = 2; return; }
                float Surface = 0.f;
                if (Terrain->IsWaterAt(C.X, C.Y, Surface) && Surface - Z > 90.f) { State[I] = 2; return; }   // água funda
                // a cápsula, um pouco acima do chão (degrau e rampa não contam), bate em algo que não é bicho?
                TArray<FOverlapResult> Hits;
                FCollisionQueryParams Query(SCENE_QUERY_STAT(VortexPathCell), false, Self);
                Self->GetWorld()->OverlapMultiByChannel(Hits, FVector(C.X, C.Y, Z + 45.f + Half), FQuat::Identity, ECC_Pawn,
                    FCollisionShape::MakeCapsule(Radius, Half), Query);
                bool bBlocked = false;
                for (const FOverlapResult& Hit : Hits)
                    if (Hit.GetActor() && !Hit.GetActor()->IsA<APawn>() && Hit.bBlockingHit) { bBlocked = true; break; }
                State[I] = (P == Start || P == Goal || !bBlocked) ? 1 : 2;
            }
            bool Step(FIntPoint A, FIntPoint B)
            {
                if (!Free(B)) return false;
                if (FMath::Abs(GroundOf(B) - GroundOf(A)) > Climb * (A.X != B.X && A.Y != B.Y ? 1.41f : 1.f)) return false;
                // diagonal: não corta a quina (as duas vizinhas retas também livres)
                if (A.X != B.X && A.Y != B.Y && (!Free(FIntPoint(B.X, A.Y)) || !Free(FIntPoint(A.X, B.Y)))) return false;
                return true;
            }
            /** Dá para ir de A a B em linha reta na grade (todas as células do meio livres e sem barranco)? */
            bool Straight(FIntPoint A, FIntPoint B)
            {
                const int32 Steps = FMath::Max(FMath::Abs(B.X - A.X), FMath::Abs(B.Y - A.Y)) * 2;
                FIntPoint Prev = A;
                for (int32 k = 1; k <= Steps; ++k)
                {
                    const float T = float(k) / Steps;
                    const FIntPoint P(FMath::RoundToInt(FMath::Lerp(float(A.X), float(B.X), T)), FMath::RoundToInt(FMath::Lerp(float(A.Y), float(B.Y), T)));
                    if (P == Prev) continue;
                    if (!Step(Prev, P)) return false;
                    Prev = P;
                }
                return true;
            }
        };
    }

    bool LineBlocked(const ACharacter* Self, const FVector& To)
    {
        if (!Self || !Self->GetWorld()) return false;
        const UCapsuleComponent* Capsule = Self->GetCapsuleComponent();
        const float Radius = Capsule->GetScaledCapsuleRadius() * .9f;
        const float Half = Capsule->GetScaledCapsuleHalfHeight();
        const FVector Lift(0.f, 0.f, FMath::Min(45.f, Half * .5f));
        const FVector From = Self->GetActorLocation() + Lift;
        FVector End = To;
        End.Z = Self->GetActorLocation().Z;
        End += Lift;
        FCollisionQueryParams Query(SCENE_QUERY_STAT(VortexPathLine), false, Self);
        TArray<FHitResult> Hits;
        Self->GetWorld()->SweepMultiByChannel(Hits, From, End, FQuat::Identity, ECC_Pawn,
            FCollisionShape::MakeCapsule(Radius, FMath::Max(Radius, Half * .6f)), Query);
        for (const FHitResult& Hit : Hits)
        {
            if (!Hit.bBlockingHit || (Hit.GetActor() && Hit.GetActor()->IsA<APawn>())) continue;
            if (Hit.ImpactNormal.Z > .7f) continue;   // chão em rampa
            return true;
        }
        return false;
    }

    bool Find(const ACharacter* Self, const FVector& GoalWorld, TArray<FVector>& Out)
    {
        Out.Reset();
        const TSharedPtr<FVortexTerrainData> Terrain = AVortexTerrainStreamer::SharedData();
        if (!Self || !Self->GetWorld() || !Terrain) return false;
        FGrid G;
        G.Self = Self;
        G.Terrain = Terrain;
        const UCapsuleComponent* Capsule = Self->GetCapsuleComponent();
        G.Radius = FMath::Clamp(Capsule->GetScaledCapsuleRadius() * .8f, 25.f, 90.f);
        G.Half = FMath::Max(G.Radius, Capsule->GetScaledCapsuleHalfHeight() * .6f);
        const FVector2D Here(Self->GetActorLocation()), There(GoalWorld);
        // a grade fica no meio dos dois; destino longe demais: vai até a borda, na direção dele
        const float Reach = (N / 2 - 2) * Cell;
        FVector2D Goal2 = There;
        if (FVector2D::Distance(Here, There) > Reach * 1.8f) Goal2 = Here + (There - Here).GetSafeNormal() * Reach * 1.8f;
        const FVector2D Mid = (Here + Goal2) * .5f;
        G.Origin = Mid - FVector2D(N * Cell * .5f);
        auto ToCell = [&](const FVector2D& P) { return FIntPoint(FMath::Clamp(FMath::FloorToInt((P.X - G.Origin.X) / Cell), 0, N - 1), FMath::Clamp(FMath::FloorToInt((P.Y - G.Origin.Y) / Cell), 0, N - 1)); };
        G.Start = ToCell(Here);
        G.Goal = ToCell(Goal2);
        if (G.Start == G.Goal) return false;
        G.State.Init(0, N * N);
        G.Ground.Init(0.f, N * N);
        // A* com 8 vizinhos (custo octil)
        TArray<float> Cost;
        Cost.Init(TNumericLimits<float>::Max(), N * N);
        TArray<int32> From;
        From.Init(INDEX_NONE, N * N);
        TArray<bool> Closed;
        Closed.Init(false, N * N);
        struct FOpen { float F; int32 I; bool operator<(const FOpen& O) const { return F < O.F; } };
        TArray<FOpen> Open;
        auto Heuristic = [&](FIntPoint P) { const int32 DX = FMath::Abs(P.X - G.Goal.X), DY = FMath::Abs(P.Y - G.Goal.Y); return float(FMath::Max(DX, DY)) + .41f * FMath::Min(DX, DY); };
        Cost[G.Index(G.Start)] = 0.f;
        Open.HeapPush({ Heuristic(G.Start), G.Index(G.Start) });
        int32 Expanded = 0;
        bool bFound = false;
        while (!Open.IsEmpty() && Expanded < MaxExpand)
        {
            FOpen Top;
            Open.HeapPop(Top);
            if (Closed[Top.I]) continue;
            Closed[Top.I] = true;
            ++Expanded;
            const FIntPoint P(Top.I % N, Top.I / N);
            if (P == G.Goal) { bFound = true; break; }
            for (int32 DY = -1; DY <= 1; ++DY)
                for (int32 DX = -1; DX <= 1; ++DX)
                {
                    if (!DX && !DY) continue;
                    const FIntPoint Q(P.X + DX, P.Y + DY);
                    if (!G.Inside(Q) || Closed[G.Index(Q)] || !G.Step(P, Q)) continue;
                    const float C = Cost[Top.I] + (DX && DY ? 1.41f : 1.f);
                    if (C < Cost[G.Index(Q)])
                    {
                        Cost[G.Index(Q)] = C;
                        From[G.Index(Q)] = Top.I;
                        Open.HeapPush({ C + Heuristic(Q), G.Index(Q) });
                    }
                }
        }
        if (!bFound) return false;
        TArray<FIntPoint> Cells;
        for (int32 I = G.Index(G.Goal); I != INDEX_NONE; I = From[I]) Cells.Insert(FIntPoint(I % N, I / N), 0);
        // encurta: de cada ponto, o mais longe que dá para ir em linha reta
        FIntPoint At = Cells[0];
        int32 k = 0;
        while (k < Cells.Num() - 1)
        {
            int32 Far = k + 1;
            for (int32 j = Cells.Num() - 1; j > k + 1; --j)
                if (G.Straight(At, Cells[j])) { Far = j; break; }
            At = Cells[Far];
            k = Far;
            const FVector2D C = Far == Cells.Num() - 1 && Goal2 == There ? There : G.Center(At);
            Out.Add(FVector(C.X, C.Y, G.GroundOf(At)));
        }
        return !Out.IsEmpty();
    }
}
