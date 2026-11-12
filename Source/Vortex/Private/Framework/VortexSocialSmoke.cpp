// Teste de jogador com jogador (-VortexSocialSmoke, Tools/RunSocialSmoke.ps1): anfitrião (que desenha a
// tela) e um cliente. No servidor: convite e grupo, duelo até 15% (a vida volta), troca de poções e
// moedas, amizade. Fotos da HUD do anfitrião (Saved/Screenshots/social_*.png): o cartão do grupo, o nome
// sobre o companheiro, a contagem e a faixa do duelo.
#if !UE_BUILD_SHIPPING
#include "Framework/VortexCreatureSmoke.h"

#include "AbilitySystemComponent.h"
#include "Combat/VortexAttributes.h"
#include "Combat/VortexCombatComponent.h"
#include "Containers/Ticker.h"
#include "EngineUtils.h"
#include "Framework/VortexCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "Misc/Paths.h"
#include "Rpg/VortexHeroSheet.h"
#include "Rpg/VortexSocial.h"
#include "UnrealClient.h"
#include "World/VortexTerrainStreamer.h"

namespace VortexSocialSmoke
{
    void Begin(AGameModeBase* Mode, APlayerController* Player)
    {
        static bool bStarted = false;
        if (bStarted) return;
        bStarted = true;
        struct FState { int32 Step = 0; float Time = 0.f, Before = 0.f; int32 Coins = 0; bool bPass = true; TArray<FString> Lines; };
        TSharedRef<FState> S = MakeShared<FState>();
        TWeakObjectPtr<AGameModeBase> WeakMode(Mode);
        TWeakObjectPtr<APlayerController> Host(Player);
        FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakMode, Host, S](float Delta) -> bool
        {
            AGameModeBase* Game = WeakMode.Get();
            if (!Game) return false;
            UWorld* World = Game->GetWorld();
            AVortexCharacter* A = Host.IsValid() ? Cast<AVortexCharacter>(Host->GetPawn()) : nullptr;
            AVortexCharacter* B = nullptr;
            for (TActorIterator<AVortexCharacter> It(World); It; ++It) if (*It != A && It->IsPlayerControlled()) B = *It;
            AVortexTerrainStreamer* Streamer = nullptr;
            for (TActorIterator<AVortexTerrainStreamer> It(World); It; ++It) Streamer = *It;
            S->Time += Delta;
            const float T = S->Time;
            auto At = [&](float When) { return T >= When && T - Delta < When; };
            auto Shot = [&](const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / FString::Printf(TEXT("Screenshots/social_%s.png"), Name), true, false); };
            auto Check = [&](bool bOk, const FString& What) { S->Lines.Add(FString::Printf(TEXT("%s %s"), bOk ? TEXT("ok") : TEXT("FALHOU"), *What)); S->bPass &= bOk; };
            auto Next = [&](int32 Step) { S->Step = Step; S->Time = 0.f; };
            auto Finish = [&]()
            {
                for (const FString& Line : S->Lines) UE_LOG(LogTemp, Display, TEXT("VORTEX_SOCIAL_SMOKE %s"), *Line);
                UE_LOG(LogTemp, Display, TEXT("VORTEX_SOCIAL_SMOKE_%s"), S->bPass ? TEXT("PASS") : TEXT("FAIL"));
                FPlatformMisc::RequestExit(false);
            };
            if (S->Step < 1 && T > 120.f) { Check(false, TEXT("dois heróis na sessão")); Finish(); return false; }
            if (!A || !B || !A->GetSheet() || !B->GetSheet() || !A->GetSheet()->IsLoaded() || !B->GetSheet()->IsLoaded() || !Streamer || !Streamer->IsReady()) return true;
            UVortexSocial* SA = A->GetSocial();
            UVortexSocial* SB = B->GetSocial();
            auto Health = [](AVortexCharacter* C) { return C->GetAbilitySystemComponent()->GetNumericAttribute(UVortexAttributes::GetHealthAttribute()); };
            const FVector2D Spot(-30000.f, -8000.f);
            switch (S->Step)
            {
            case 0:   // os dois lado a lado num campo aberto, a câmera do anfitrião olhando o outro
            {
                float Ground = 0.f;
                A->SetActorLocation(FVector(Spot, Streamer->GroundAt(Spot.X, Spot.Y) + 150.f), false, nullptr, ETeleportType::TeleportPhysics);
                B->SetActorLocation(FVector(Spot + FVector2D(250.f, 0.f), Streamer->GroundAt(Spot.X + 250.f, Spot.Y) + 150.f), false, nullptr, ETeleportType::TeleportPhysics);
                if (!Streamer->GroundReadyAt(Spot.X, Spot.Y, Ground) || T < 4.f) return true;
                Host->SetControlRotation(FRotator(-12.f, 0.f, 0.f));
                Check(true, FString::Printf(TEXT("dois heróis: %s e %s"), *SA->GetHeroName(), *SB->GetHeroName()));
                Next(1);
                return true;
            }
            case 1:   // grupo
            {
                if (At(.1f)) SA->ServerAct_Implementation(B, TEXT("party_invite"));
                if (At(.5f))
                {
                    Check(SB->HasPending(A, TEXT("party")), TEXT("o outro recebeu o convite de grupo"));
                    SB->ServerAct_Implementation(A, TEXT("party_accept"));
                }
                if (At(1.f)) Check(SA->SameParty(B) && SA->GetPartyId() != 0, FString::Printf(TEXT("mesmo grupo (id %d), %d companheiro"), SA->GetPartyId(), SA->PartyMates().Num()));
                if (At(2.5f)) Shot(TEXT("grupo"));
                if (At(3.f)) Next(2);
                return true;
            }
            case 2:   // duelo: contagem, luta, golpe que levaria à morte para em 15% e a vida volta
            {
                if (At(.1f))
                {
                    S->Before = Health(B);
                    SA->ServerAct_Implementation(B, TEXT("duel_invite"));
                    SB->ServerAct_Implementation(A, TEXT("duel_accept"));
                    Check(SA->GetDuelPhase() == 1 && SB->GetDuelOpponent() == A, TEXT("duelo aceito, contagem"));
                }
                if (At(1.2f)) Shot(TEXT("duelo_contagem"));
                if (At(4.2f))
                {
                    Check(SA->IsDuelFighting(B), TEXT("a luta começou depois do 3, 2, 1"));
                    Check(A->GetCombat()->CanHarm(B), TEXT("no duelo os dois se ferem"));
                }
                if (At(4.6f))
                {
                    B->GetAbilitySystemComponent()->SetNumericAttributeBase(UVortexAttributes::GetHealthAttribute(), B->GetVitals()->GetMaxHealth() * .6f);
                    Shot(TEXT("duelo_luta"));
                }
                if (At(5.f)) A->GetCombat()->ApplyHit(B, 5000.f, EVortexDamageKind::Melee, 0.f);
                if (At(5.4f))
                {
                    Check(SA->GetDuelPhase() == 0 && SB->GetDuelPhase() == 0, TEXT("o duelo acabou em 15%"));
                    Check(!B->IsDead() && FMath::IsNearlyEqual(Health(B), S->Before, 1.f), FString::Printf(TEXT("ninguém morreu e a vida voltou (%.0f -> %.0f)"), S->Before, Health(B)));
                    Check(!A->GetCombat()->CanHarm(B), TEXT("fora do duelo, heróis não se ferem"));
                    Next(3);
                }
                return true;
            }
            case 3:   // troca: 2 poções e 20 moedas de A por nada; pronto dos dois
            {
                if (At(.1f))
                {
                    A->GetSheet()->GiveItem(TEXT("potion_red"), 3, false);
                    A->GetSheet()->AddCoins(50);
                    S->Coins = B->GetSheet()->GetCoins();
                    S->Before = B->GetSheet()->CountOf(TEXT("potion_red"));
                    SA->ServerAct_Implementation(B, TEXT("trade_invite"));
                    SB->ServerAct_Implementation(A, TEXT("trade_accept"));
                    Check(SA->GetTradePartner() == B, TEXT("troca aberta nos dois"));
                    FVortexItemStack Potion; Potion.Item = TEXT("potion_red"); Potion.Count = 2;
                    SA->ServerTradeOffer_Implementation({ Potion }, 20);
                    SB->ServerTradeReady_Implementation(true);
                    SA->ServerTradeReady_Implementation(true);
                }
                if (At(.6f))
                {
                    Check(B->GetSheet()->CountOf(TEXT("potion_red")) == int32(S->Before) + 2 && B->GetSheet()->GetCoins() == S->Coins + 20,
                        FString::Printf(TEXT("troca concluída: o outro tem +2 poções (%d) e +20 moedas (%d)"), B->GetSheet()->CountOf(TEXT("potion_red")), B->GetSheet()->GetCoins()));
                    Check(SA->GetTradePartner() == nullptr, TEXT("a mesa fechou"));
                    Next(4);
                }
                return true;
            }
            case 4:   // amizade e sair do grupo
            {
                if (At(.1f))
                {
                    SA->ServerAct_Implementation(B, TEXT("friend_add"));
                    Check(SB->HasPending(A, TEXT("friend")), TEXT("pedido de amizade chegou"));
                    SB->ServerAct_Implementation(A, TEXT("friend_accept"));
                    Check(!SB->HasPending(A, TEXT("friend")), TEXT("amizade aceita"));
                    SB->ServerAct_Implementation(A, TEXT("party_leave"));
                }
                if (At(.5f)) { Check(SA->GetPartyId() == 0 && SB->GetPartyId() == 0, TEXT("saiu do grupo e o grupo de dois acabou")); Finish(); return false; }
                return true;
            }
            }
            return true;
        }));
    }
}
#endif
