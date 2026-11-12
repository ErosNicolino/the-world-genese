#include "Creatures/VortexNpcRoutine.h"

#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "Framework/VortexGameState.h"
#include "Algo/Reverse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
    struct FBlockDef { float From = 0.f, To = 24.f; FString Where, Anim; };
    struct FNpcDef { FName Role, Work; FString Voice; };
    struct FBench { FVector2D Seats[2], Stand; float FaceYaw = 0.f; };

    struct FData
    {
        // a grade (metros do Godot; cada casa Cell m)
        FVector2D Origin = FVector2D::ZeroVector;
        float Cell = .5f;
        int32 W = 0, H = 0;
        TArray<uint8> Walk;
        // os lugares (cm)
        TArray<FVector2D> HouseDoors, ShopDoors, Plaza;
        FVector2D TavernInside = FVector2D::ZeroVector, ChapelInside = FVector2D::ZeroVector, Board = FVector2D::ZeroVector, PlazaCenter = FVector2D::ZeroVector;
        bool bTavern = false, bChapel = false, bBoard = false;
        TArray<FBench> Benches;
        // a rotina
        TMap<FName, TArray<FBlockDef>> Roles;
        TMap<FName, FNpcDef> Npcs;
        TMap<FString, TMap<FString, TArray<FString>>> Falas;
        TMap<FString, TArray<FName>> Gestos;
        TMap<FString, TArray<TArray<FString>>> Conversas;
        TArray<VortexNpcRoutine::FExtra> Extras;
        TMap<int32, FName> Seats;
        bool bLoaded = false;

        bool Walkable(int32 I, int32 J) const { return I >= 0 && J >= 0 && I < H && J < W && Walk[I * W + J]; }
        FIntPoint CellOf(const FVector2D& Cm) const
        {
            return FIntPoint(FMath::FloorToInt((Cm.X / 100.f - Origin.X) / Cell), FMath::FloorToInt((Cm.Y / 100.f - Origin.Y) / Cell));   // (J, I)
        }
        FVector2D CenterOf(int32 I, int32 J) const { return FVector2D((Origin.X + (J + .5f) * Cell) * 100.f, (Origin.Y + (I + .5f) * Cell) * 100.f); }
        /** A casa andável mais perto (até 10 casas de distância). */
        bool Snap(FIntPoint& C) const
        {
            if (Walkable(C.Y, C.X)) return true;
            for (int32 R = 1; R <= 10; ++R)
                for (int32 Dy = -R; Dy <= R; ++Dy)
                    for (int32 Dx = -R; Dx <= R; ++Dx)
                        if ((FMath::Abs(Dx) == R || FMath::Abs(Dy) == R) && Walkable(C.Y + Dy, C.X + Dx)) { C += FIntPoint(Dx, Dy); return true; }
            return false;
        }
        FVector2D SnapCm(const FVector2D& Cm) const
        {
            FIntPoint C = CellOf(Cm);
            return Snap(C) && C != CellOf(Cm) ? CenterOf(C.Y, C.X) : Cm;
        }
    };

    TSharedPtr<FJsonObject> ReadJson(const TCHAR* Relative)
    {
        FString Text;
        TSharedPtr<FJsonObject> Json;
        if (!FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir() / Relative)) ||
            !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json) || !Json.IsValid()) return nullptr;
        return Json;
    }

    FVector2D Point(const TSharedPtr<FJsonValue>& V)
    {
        const TArray<TSharedPtr<FJsonValue>>* A = nullptr;
        return V.IsValid() && V->TryGetArray(A) && A->Num() >= 2 ? FVector2D((*A)[0]->AsNumber(), (*A)[1]->AsNumber()) * 100.f : FVector2D::ZeroVector;
    }

    void LoadNav(FData& D)
    {
        const TSharedPtr<FJsonObject> Nav = ReadJson(TEXT("Data/Population/vila_nav.json"));
        if (!Nav) return;
        const TArray<TSharedPtr<FJsonValue>>& O = Nav->GetArrayField(TEXT("origin"));
        D.Origin = FVector2D(O[0]->AsNumber(), O[1]->AsNumber());
        D.Cell = Nav->GetNumberField(TEXT("cell"));
        D.W = Nav->GetIntegerField(TEXT("w"));
        D.H = Nav->GetIntegerField(TEXT("h"));
        const FString Walk = Nav->GetStringField(TEXT("walk"));
        if (Walk.Len() != D.W * D.H) { D.W = D.H = 0; return; }
        D.Walk.SetNumUninitialized(Walk.Len());
        for (int32 K = 0; K < Walk.Len(); ++K) D.Walk[K] = Walk[K] == TEXT('1');
        const TSharedPtr<FJsonObject> P = Nav->GetObjectField(TEXT("places"));
        for (const TSharedPtr<FJsonValue>& V : P->GetArrayField(TEXT("houses"))) D.HouseDoors.Add(Point(V->AsObject()->TryGetField(TEXT("door"))));
        for (const auto& B : P->GetObjectField(TEXT("buildings"))->Values)
        {
            const TSharedPtr<FJsonObject> E = B.Value->AsObject();
            D.ShopDoors.Add(Point(E->TryGetField(TEXT("door"))));
            if (FString(*B.Key).StartsWith(TEXT("TAVERNA"))) { D.TavernInside = Point(E->TryGetField(E->HasField(TEXT("center")) ? TEXT("center") : TEXT("inside"))); D.bTavern = true; }   // 08/11: o meio do salão (a porta amontoava)
            if (B.Key == TEXT("CAPELA")) { D.ChapelInside = Point(E->TryGetField(TEXT("inside"))); D.bChapel = true; }
        }
        for (const TSharedPtr<FJsonValue>& V : P->GetArrayField(TEXT("benches")))
        {
            const TSharedPtr<FJsonObject> E = V->AsObject();
            const float T = FMath::DegreesToRadians(-E->GetNumberField(TEXT("yaw")));
            const FVector2D Along(FMath::Cos(T), FMath::Sin(T)), Front(FMath::Sin(T), -FMath::Cos(T));
            const FVector2D Seat = Point(E->TryGetField(TEXT("seat")));
            FBench& B = D.Benches.AddDefaulted_GetRef();
            B.Seats[0] = Seat - Along * 45.f;
            B.Seats[1] = Seat + Along * 45.f;
            B.Stand = Point(E->TryGetField(TEXT("stand")));
            B.FaceYaw = FMath::RadiansToDegrees(FMath::Atan2(Front.Y, Front.X));
        }
        for (const TSharedPtr<FJsonValue>& V : P->GetArrayField(TEXT("plaza"))) D.Plaza.Add(Point(V));
        for (const FVector2D& Q : D.Plaza) D.PlazaCenter += Q / FMath::Max(1, D.Plaza.Num());
        const TSharedPtr<FJsonValue> Board = P->TryGetField(TEXT("board"));
        if (Board.IsValid() && !Board->IsNull()) { D.Board = Point(Board); D.bBoard = true; }
    }

    void LoadSchedule(FData& D)
    {
        const TSharedPtr<FJsonObject> S = ReadJson(TEXT("Data/Population/npc_schedule.json"));
        if (!S) return;
        for (const auto& R : S->GetObjectField(TEXT("papeis"))->Values)
        {
            TArray<FBlockDef>& Blocks = D.Roles.Add(FName(*R.Key));
            for (const TSharedPtr<FJsonValue>& V : R.Value->AsArray())
            {
                const TSharedPtr<FJsonObject> B = V->AsObject();
                FBlockDef& Def = Blocks.AddDefaulted_GetRef();
                Def.From = B->GetNumberField(TEXT("de"));
                Def.To = B->GetNumberField(TEXT("ate"));
                Def.Where = B->GetStringField(TEXT("onde"));
                B->TryGetStringField(TEXT("anim"), Def.Anim);
            }
        }
        for (const auto& N : S->GetObjectField(TEXT("npcs"))->Values)
        {
            const TSharedPtr<FJsonObject> O = N.Value->AsObject();
            FNpcDef& Def = D.Npcs.Add(FName(*N.Key));
            Def.Role = FName(*O->GetStringField(TEXT("papel")));
            Def.Work = FName(*O->GetStringField(TEXT("trabalho")));
            O->TryGetStringField(TEXT("voz"), Def.Voice);
        }
        for (const auto& Kind : S->GetObjectField(TEXT("falas"))->Values)
            for (const auto& Period : Kind.Value->AsObject()->Values)
            {
                TArray<FString>& Lines = D.Falas.FindOrAdd(FString(*Kind.Key)).FindOrAdd(FString(*Period.Key));
                for (const TSharedPtr<FJsonValue>& L : Period.Value->AsArray()) Lines.Add(L->AsString());
            }
        const TSharedPtr<FJsonObject>* Gestures = nullptr;
        if (S->TryGetObjectField(TEXT("gestos"), Gestures))
            for (const auto& G : (*Gestures)->Values)
                for (const TSharedPtr<FJsonValue>& V : G.Value->AsArray()) D.Gestos.FindOrAdd(FString(*G.Key)).Add(V->AsString().IsEmpty() ? FName() : FName(*V->AsString()));
        const TSharedPtr<FJsonObject>* Talks = nullptr;
        if (S->TryGetObjectField(TEXT("conversas"), Talks))
            for (const auto& P : (*Talks)->Values)
                for (const TSharedPtr<FJsonValue>& V : P.Value->AsArray())
                {
                    TArray<FString>& Lines = D.Conversas.FindOrAdd(FString(*P.Key)).AddDefaulted_GetRef();
                    for (const TSharedPtr<FJsonValue>& L : V->AsArray()) Lines.Add(L->AsString());
                }
        // os passantes: sorteados com a semente (o mesmo sorteio em toda máquina e todo jogo)
        const TSharedPtr<FJsonObject>* Crowd = nullptr;
        if (!S->TryGetObjectField(TEXT("passantes"), Crowd) || D.HouseDoors.IsEmpty()) return;
        FRandomStream Rng((*Crowd)->GetIntegerField(TEXT("semente")));
        auto List = [&](const TCHAR* Field) { TArray<FString> Out; for (const TSharedPtr<FJsonValue>& V : (*Crowd)->GetArrayField(Field)) Out.Add(V->AsString()); return Out; };
        const TArray<FString> Male = List(TEXT("nomes_m")), Female = List(TEXT("nomes_f")), HairColors = List(TEXT("cores_cabelo"));
        const int32 Adults = (*Crowd)->GetIntegerField(TEXT("adultos")), Kids = (*Crowd)->GetIntegerField(TEXT("criancas"));
        for (int32 K = 0; K < Adults + Kids; ++K)
        {
            VortexNpcRoutine::FExtra& E = D.Extras.AddDefaulted_GetRef();
            E.bChild = K >= Adults;
            const bool bFemale = Rng.FRand() < .5f;
            E.Id = FName(*FString::Printf(TEXT("%s_%d"), E.bChild ? TEXT("crianca") : TEXT("passante"), E.bChild ? K - Adults : K));
            const TArray<FString>& Names = bFemale ? Female : Male;
            E.Name = Names.Num() ? Names[Rng.RandHelper(Names.Num())] : TEXT("Morador");
            E.Title = E.bChild ? TEXT("Criança da vila") : bFemale ? TEXT("Moradora") : TEXT("Morador");
            E.Look.Race = TEXT("humano");
            E.Look.Appearance = bFemale ? TEXT("female") : TEXT("male");
            TArray<FName> Hairs, Outfits;
            for (const FVortexBlockHair& Hair : FVortexLookCatalog::BlockHairs()) if (!Hair.Mesh.IsEmpty() && Hair.Fits(E.Look.Appearance)) Hairs.Add(Hair.Id);
            for (const FVortexBlockOutfit& Outfit : FVortexLookCatalog::BlockOutfits()) if (!Outfit.Mesh.IsEmpty() && Outfit.Fits(E.Look.Appearance)) Outfits.Add(Outfit.Id);
            if (Hairs.Num()) E.Look.Pieces.Add(TEXT("block_hair"), Hairs[Rng.RandHelper(Hairs.Num())]);
            if (Outfits.Num()) E.Look.Pieces.Add(TEXT("block_outfit"), Outfits[Rng.RandHelper(Outfits.Num())]);
            if (HairColors.Num()) E.Look.Colors.Add(TEXT("hair_color"), FColor::FromHex(HairColors[Rng.RandHelper(HairColors.Num())]));
            E.Home = D.HouseDoors[(K * 7 + 3) % D.HouseDoors.Num()];
        }
    }

    FData& Data()
    {
        static FData D;
        if (!D.bLoaded)
        {
            D.bLoaded = true;
            LoadNav(D);
            LoadSchedule(D);
        }
        return D;
    }

    const VortexNpcRoutine::FExtra* FindExtra(FName Id)
    {
        return Data().Extras.FindByPredicate([Id](const VortexNpcRoutine::FExtra& E) { return E.Id == Id; });
    }

    FName RoleOf(FName Id)
    {
        if (const FNpcDef* Def = Data().Npcs.Find(Id)) return Def->Role;
        if (const VortexNpcRoutine::FExtra* E = FindExtra(Id)) return E->bChild ? FName(TEXT("crianca")) : FName(TEXT("passante"));
        return NAME_None;
    }

    uint32 Hash(FName Id, int32 Salt) { return HashCombineFast(GetTypeHash(Id), ::GetTypeHash(Salt)); }

    FName ResolveAnim(FName Id, const FString& Anim)
    {
        if (Anim.IsEmpty()) return NAME_None;
        if (Anim == TEXT("trabalho")) return VortexNpcRoutine::WorkClip(Id);
        if (Anim == TEXT("conversa")) return TEXT("Npc_Conversar");
        if (Anim == TEXT("beber")) return TEXT("Beber");
        if (Anim == TEXT("rezar")) return TEXT("Npc_Rezar");
        if (Anim == TEXT("vigiar")) return TEXT("Npc_Vigiar");
        if (Anim == TEXT("sentar")) return TEXT("SentadoBanco");
        return FName(*Anim);
    }
}

namespace VortexNpcRoutine
{
    bool HasRoutine(FName NpcId) { return !RoleOf(NpcId).IsNone() && Data().W > 0; }
    bool IsChild(FName NpcId) { const FExtra* E = FindExtra(NpcId); return E && E->bChild; }
    bool IsGuard(FName NpcId) { const FName Role = RoleOf(NpcId); return Role == TEXT("guarda") || Role == TEXT("capita"); }
    FName WorkClip(FName NpcId) { const FNpcDef* Def = Data().Npcs.Find(NpcId); return Def ? Def->Work : FName(TEXT("Npc_Conversar")); }
    const TArray<FExtra>& Extras() { return Data().Extras; }

    float HourOf(const UWorld* World)
    {
        const AVortexGameState* State = World ? World->GetGameState<AVortexGameState>() : nullptr;
        return State ? State->GetHour() : 10.f;
    }

    bool Plan(FName NpcId, const FVector2D& Post, float PostYaw, float Hour, int32 Wander, FGoal& Out)
    {
        FData& D = Data();
        const TArray<FBlockDef>* Blocks = D.Roles.Find(RoleOf(NpcId));
        if (!Blocks || D.W == 0) return false;
        Out = FGoal();
        for (int32 K = 0; K < Blocks->Num(); ++K)
        {
            const FBlockDef& B = (*Blocks)[K];
            const bool bIn = B.To > B.From ? Hour >= B.From && Hour < B.To : Hour >= B.From || Hour < B.To;
            if (bIn) { Out.Block = K; break; }
        }
        if (Out.Block < 0) return false;
        const FBlockDef& B = (*Blocks)[Out.Block];
        Out.Anim = ResolveAnim(NpcId, B.Anim);
        const uint32 H = Hash(NpcId, Out.Block);
        auto Near = [&](const FVector2D& Base, float Radius)
        {
            // um lugar perto do ponto (cada morador num canto), puxado para onde dá para pisar
            const float A = (H % 360) * PI / 180.f, R = Radius * (.4f + (H / 360 % 100) / 166.f);
            return D.SnapCm(Base + FVector2D(FMath::Cos(A), FMath::Sin(A)) * R);
        };
        auto Face = [&](const FVector2D& From, const FVector2D& To) { Out.Yaw = FMath::RadiansToDegrees(FMath::Atan2(To.Y - From.Y, To.X - From.X)); Out.bFaceYaw = true; };
        if (B.Where == TEXT("posto")) { Out.At = Post; Out.Yaw = PostYaw; Out.bFaceYaw = true; }
        else if (B.Where == TEXT("casa"))
        {
            FVector2D Home = Post;
            if (const FExtra* E = FindExtra(NpcId)) Home = E->Home;
            else
            {
                float Best = 1e12f;
                for (const FVector2D& Door : D.HouseDoors) if (FVector2D::DistSquared(Door, Post) < Best) { Best = FVector2D::DistSquared(Door, Post); Home = Door; }
            }
            Out.At = Home;
            Out.bSleep = true;
        }
        else if (B.Where == TEXT("taverna") && D.bTavern) { Out.At = Near(D.TavernInside, 520.f); Face(Out.At, D.TavernInside); }
        else if (B.Where == TEXT("capela") && D.bChapel) { Out.At = Near(D.ChapelInside, 150.f); }
        else if (B.Where == TEXT("banco") && D.Benches.Num())
        {
            Out.Seat = int32(H % uint32(D.Benches.Num() * 2));
            const FBench& Bench = D.Benches[Out.Seat / 2];
            Out.At = Bench.Seats[Out.Seat % 2];
            Out.Stand = Bench.Stand;
            Out.Yaw = Bench.FaceYaw;
            Out.bFaceYaw = Out.bSit = true;
            Out.Anim = TEXT("SentadoBanco");
        }
        else if (B.Where == TEXT("quadro") && D.bBoard) { Out.At = Near(D.Board, 120.f); Face(Out.At, D.Board); }
        else if (B.Where == TEXT("patrulha")) { Out.At = Post; Out.bPatrol = true; }
        else if (B.Where == TEXT("passeio"))
        {
            // 08/11: a praça é só um dos lugares do passeio (com os 8 pontos dela, a multidão ficava toda em volta da fonte)
            TArray<FVector2D> Spots;
            for (int32 P = 0; P < D.Plaza.Num(); P += 4) Spots.Add(D.Plaza[P]);
            for (const FBench& Bench : D.Benches) Spots.Add(Bench.Stand);
            Spots.Append(D.ShopDoors);
            if (D.bBoard) Spots.Add(D.Board);
            const uint32 Pick = Hash(NpcId, Out.Block * 131 + Wander * 7919);
            Out.At = Spots.Num() ? D.SnapCm(Spots[Pick % uint32(Spots.Num())] + FVector2D(int32(Pick >> 8 & 255) - 128, int32(Pick >> 16 & 255) - 128) * 1.2f) : Post;
            Out.bWander = true;
        }
        else
        {
            // a praça (e o que não tiver lugar na vila: taverna, capela, quadro, banco sem dados)
            if (D.Plaza.Num()) { Out.At = Near(D.Plaza[H % uint32(D.Plaza.Num())], 120.f); Face(Out.At, D.PlazaCenter); }
            else Out.At = Post;
        }
        return true;
    }

    bool ClaimSeat(int32 Seat, FName NpcId)
    {
        FName& Who = Data().Seats.FindOrAdd(Seat);
        if (!Who.IsNone() && Who != NpcId) return false;
        Who = NpcId;
        return true;
    }

    void ReleaseSeat(int32 Seat, FName NpcId)
    {
        if (const FName* Who = Data().Seats.Find(Seat); Who && *Who == NpcId) Data().Seats.Remove(Seat);
    }

    bool Path(const FVector2D& From, const FVector2D& To, TArray<FVector2D>& Out)
    {
        Out.Reset();
        const FData& D = Data();
        if (D.W == 0) return false;
        FIntPoint A = D.CellOf(From), B = D.CellOf(To);
        if (!D.Snap(A) || !D.Snap(B)) return false;
        const int32 W = D.W, N = D.W * D.H;
        const int32 Start = A.Y * W + A.X, Goal = B.Y * W + B.X;
        TArray<float> G;
        TArray<int32> Parent;
        G.Init(TNumericLimits<float>::Max(), N);
        Parent.Init(-1, N);
        struct FOpen { float F; int32 Index; bool operator<(const FOpen& O) const { return F < O.F; } };
        TArray<FOpen> Open;
        auto Heuristic = [&](int32 Index)
        {
            const int32 Dx = FMath::Abs(Index % W - B.X), Dy = FMath::Abs(Index / W - B.Y);
            return float(FMath::Max(Dx, Dy)) + .414f * float(FMath::Min(Dx, Dy));
        };
        G[Start] = 0.f;
        Open.HeapPush({ Heuristic(Start), Start });
        static const int32 Dx8[] = { 1, -1, 0, 0, 1, 1, -1, -1 }, Dy8[] = { 0, 0, 1, -1, 1, -1, 1, -1 };
        int32 Expanded = 0;
        bool bFound = Start == Goal;
        while (Open.Num() && !bFound && Expanded < 60000)
        {
            FOpen Top;
            Open.HeapPop(Top);
            const int32 Cx = Top.Index % W, Cy = Top.Index / W;
            if (Top.F - Heuristic(Top.Index) > G[Top.Index] + .01f) continue;   // já saiu por um caminho melhor
            ++Expanded;
            for (int32 K = 0; K < 8; ++K)
            {
                const int32 Nx = Cx + Dx8[K], Ny = Cy + Dy8[K];
                if (!D.Walkable(Ny, Nx)) continue;
                if (K >= 4 && (!D.Walkable(Cy, Nx) || !D.Walkable(Ny, Cx))) continue;   // sem cortar quina
                const int32 Next = Ny * W + Nx;
                const float Cost = G[Top.Index] + (K < 4 ? 1.f : 1.414f);
                if (Cost >= G[Next]) continue;
                G[Next] = Cost;
                Parent[Next] = Top.Index;
                if (Next == Goal) { bFound = true; break; }
                Open.HeapPush({ Cost + Heuristic(Next), Next });
            }
        }
        if (!bFound) return false;
        TArray<int32> Cells;
        for (int32 C = Goal; C != -1; C = Parent[C]) Cells.Add(C);
        Algo::Reverse(Cells);
        // estica o caminho: de cada ponto pula para o mais longe que se enxerga em linha reta pela grade
        auto Clear = [&](int32 P, int32 Q)
        {
            const FVector2D U(P % W, P / W), V(Q % W, Q / W);
            const int32 Steps = FMath::CeilToInt(FVector2D::Distance(U, V) * 2.f);
            for (int32 S = 1; S < Steps; ++S)
            {
                const FVector2D M = FMath::Lerp(U, V, float(S) / Steps);
                for (const FVector2D& Off : { FVector2D(.3f, .3f), FVector2D(-.3f, .3f), FVector2D(.3f, -.3f), FVector2D(-.3f, -.3f) })
                    if (!D.Walkable(FMath::FloorToInt(M.Y + .5f + Off.Y), FMath::FloorToInt(M.X + .5f + Off.X))) return false;
            }
            return true;
        };
        int32 At = 0;
        while (At < Cells.Num() - 1)
        {
            int32 Far = At + 1;
            for (int32 K = Cells.Num() - 1; K > At + 1; --K) if (Clear(Cells[At], Cells[K])) { Far = K; break; }
            Out.Add(D.CenterOf(Cells[Far] / W, Cells[Far] % W));
            At = Far;
        }
        // o último ponto é o destino de verdade quando ele mesmo é andável
        const FIntPoint Real = D.CellOf(To);
        if (Out.Num() && D.Walkable(Real.Y, Real.X)) Out.Last() = To;
        if (Out.IsEmpty()) Out.Add(To);
        return true;
    }

    TArray<FString> Barks(FName NpcId, float Hour)
    {
        const FString Period = Hour >= 5.f && Hour < 12.f ? TEXT("manha") : Hour >= 12.f && Hour < 18.f ? TEXT("tarde") : TEXT("noite");
        TArray<FString> Out;
        const FData& D = Data();
        auto Add = [&](const TCHAR* Kind) { if (const TMap<FString, TArray<FString>>* K = D.Falas.Find(Kind)) if (const TArray<FString>* L = K->Find(Period)) Out.Append(*L); };
        if (!HasRoutine(NpcId)) return Out;
        Add(TEXT("todos"));
        if (IsGuard(NpcId)) Add(TEXT("guarda"));
        else if (IsChild(NpcId)) Add(TEXT("crianca"));
        else if (FindExtra(NpcId)) Add(TEXT("passante"));
        return Out;
    }

    FName GreetClip(FName NpcId, int32 Count)
    {
        const TArray<FName>* Pool = Data().Gestos.Find(IsGuard(NpcId) ? TEXT("guarda") : IsChild(NpcId) ? TEXT("crianca") : TEXT("adulto"));
        return Pool && Pool->Num() ? (*Pool)[Hash(NpcId, Count * 37 + 5) % uint32(Pool->Num())] : FName();
    }

    TArray<FString> Conversation(float Hour, int32 Pick)
    {
        const FString Period = Hour >= 5.f && Hour < 12.f ? TEXT("manha") : Hour >= 12.f && Hour < 18.f ? TEXT("tarde") : TEXT("noite");
        const TArray<TArray<FString>>* List = Data().Conversas.Find(Period);
        return List && List->Num() ? (*List)[FMath::Abs(Pick) % List->Num()] : TArray<FString>();
    }

    FName Voice(FName NpcId, int32 Pick)
    {
        FString Kind = TEXT("medio");
        if (const FNpcDef* Def = Data().Npcs.Find(NpcId); Def && !Def->Voice.IsEmpty()) Kind = Def->Voice;
        else if (const FExtra* E = FindExtra(NpcId)) Kind = E->bChild ? TEXT("crianca") : E->Look.Appearance == TEXT("female") ? TEXT("agudo") : (GetTypeHash(NpcId) % 2 ? TEXT("grave") : TEXT("medio"));
        return FName(*FString::Printf(TEXT("npc_voz_%s_%d"), *Kind, 1 + FMath::Abs(Pick) % 4));
    }
}
