// Teste do "Aplicar" das opções (-VortexApplySmoke, Tools/RunApplySmoke.ps1): com o painel em edição, mudar a
// densidade da grama fica pendente (o chão continua montado); Aplicar mostra a tela de carregamento, o chão
// remonta sozinho com o herói parado e a tela fecha; depois liga o DLSS Qualidade (se a placa tiver).
#if !UE_BUILD_SHIPPING
#include "Framework/VortexCreatureSmoke.h"

#include "Containers/Ticker.h"
#include "EngineUtils.h"
#include "Framework/VortexHudBase.h"
#include "Framework/VortexCharacter.h"
#include "Framework/VortexGraphics.h"
#include "Framework/VortexPlayerSettings.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Rpg/VortexHeroSheet.h"
#include "World/VortexTerrainStreamer.h"

namespace VortexApplySmoke
{
    void Begin(AGameModeBase* Mode, APlayerController* Player)
    {
        struct FState { int32 Step = 0; float Time = 0.f, StepAt = 0.f; int32 Before = 0; bool bPass = true, bSawLoading = false; };
        TSharedRef<FState> S = MakeShared<FState>();
        TWeakObjectPtr<AGameModeBase> WeakMode(Mode);
        TWeakObjectPtr<APlayerController> Controller(Player);
        FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakMode, Controller, S](float Delta) -> bool
        {
            AGameModeBase* Game = WeakMode.Get();
            AVortexCharacter* Hero = Controller.IsValid() ? Cast<AVortexCharacter>(Controller->GetPawn()) : nullptr;
            if (!Game) return false;
            AVortexTerrainStreamer* Streamer = nullptr;
            for (TActorIterator<AVortexTerrainStreamer> It(Game->GetWorld()); It; ++It) Streamer = *It;
            if (!Hero || !Streamer || !Streamer->IsReady() || !Streamer->IsInitialFillDone() || !Hero->GetSheet() || !Hero->GetSheet()->IsLoaded()) return true;
            AVortexHudBase* Hud = Cast<AVortexHudBase>(Controller->GetHUD());
            S->Time += Delta;
            auto Check = [&](bool bOk, const TCHAR* What)
            {
                UE_LOG(LogTemp, Display, TEXT("VORTEX_APPLY_SMOKE %s %s"), bOk ? TEXT("ok") : TEXT("FALHOU"), What);
                S->bPass &= bOk;
            };
            Hero->GetCharacterMovement()->StopMovementImmediately();
            UVortexPlayerSettings* Options = UVortexPlayerSettings::Get();
            switch (S->Step)
            {
            case 0:   // espera o chão em volta e muda a densidade da grama com o painel em edição
                if (S->Time < 4.f || Streamer->PendingChunks() > 0) return true;
                S->Before = Streamer->VisualChunkCount();
                VortexGraphics::BeginEditing();
                for (const VortexGraphics::FOption& Item : VortexGraphics::Options(TEXT("Qualidade")))
                    if (Item.Name == TEXT("Densidade da vegetação") && Item.Step) Item.Step(Options->GrassDensity >= 2 ? -1 : 1);
                Check(VortexGraphics::HasPending(), TEXT("mudança pendente (sem aplicar)"));
                S->Step = 1; S->StepAt = S->Time;
                return true;
            case 1:   // pendente: o chão não pode sumir
                if (S->Time - S->StepAt < 1.f) return true;
                Check(Streamer->VisualChunkCount() >= S->Before * .9f, TEXT("o chão continua montado antes de aplicar"));
                VortexGraphics::ApplyPending();
                Check(!VortexGraphics::HasPending(), TEXT("aplicado"));
                S->Step = 2; S->StepAt = S->Time;
                return true;
            case 2:   // a tela de carregamento cobre a remontagem e fecha sozinha, com o herói parado
                S->bSawLoading |= Hud && Hud->IsLoadingScreen();
                if (S->Time - S->StepAt < 1.5f || (Hud && Hud->IsLoadingScreen()) || Streamer->PendingChunks() > 0)
                {
                    if (S->Time - S->StepAt > 30.f) { Check(false, TEXT("remontou em até 30 s")); S->Step = 3; }
                    return true;
                }
                Check(S->bSawLoading, TEXT("tela de carregamento ao aplicar"));
                Check(Streamer->VisualChunkCount() >= S->Before * .9f, TEXT("o chão remontou sem andar"));
                S->Step = 3; S->StepAt = S->Time;
                return true;
            case 3:   // a recomendação gráfica: calibra e deixa tudo pendente para o Aplicar
                for (const VortexGraphics::FOption& Item : VortexGraphics::Options(TEXT("Qualidade")))
                    if (Item.Name == TEXT("Recomendação automática") && Item.Step) Item.Step(1);
                Check(VortexGraphics::HasPending(), TEXT("recomendação gráfica calculada (pendente)"));
                for (const VortexGraphics::FOption& Item : VortexGraphics::Options(TEXT("Qualidade")))
                    if (Item.Name == TEXT("Recomendação automática")) UE_LOG(LogTemp, Display, TEXT("VORTEX_APPLY_SMOKE recomendação: %s"), *Item.Value);
                VortexGraphics::ApplyPending();
                S->Step = 30;
                return true;
            case 30:
                if (VortexGraphics::DlssSupported())
                {
                    // 11/11: o painel edita uma cópia; o passo vai até a opção mostrar "Qualidade" (o índice 1)
                    for (int32 Guard = 0; Guard < 8; ++Guard)
                        for (const VortexGraphics::FOption& Item : VortexGraphics::Options(TEXT("Desempenho")))
                            if (Item.Name == TEXT("DLSS") && Item.Step && Item.Index != 1) Item.Step(Item.Index > 1 ? -1 : 1);
                    VortexGraphics::ApplyPending();
                    S->Step = 31; S->StepAt = S->Time;
                    return true;
                }
                UE_LOG(LogTemp, Display, TEXT("VORTEX_APPLY_SMOKE sem DLSS nesta placa (pulado)"));
                VortexGraphics::EndEditing();
                S->Step = 4; S->StepAt = S->Time;
                return true;
            case 31:   // o Aplicar espera 2 quadros (o "Aplicando" aparece antes) e o mundo remontar
                if (VortexGraphics::IsApplying() && S->Time - S->StepAt < 25.f) return true;
                {
                    const IConsoleVariable* Dlss = IConsoleManager::Get().FindConsoleVariable(TEXT("r.NGX.DLSS.Enable"));
                    Check(Dlss && Dlss->GetInt() == 1, TEXT("DLSS Qualidade ligado"));
                    Check(UVortexPlayerSettings::Get()->Upscaler == 1, TEXT("o Aplicar passou a cópia para as opções de verdade"));
                }
                VortexGraphics::EndEditing();
                S->Step = 4; S->StepAt = S->Time;
                return true;
            default:
                if (S->Time - S->StepAt < 2.f) return true;
                UE_LOG(LogTemp, Display, TEXT("VORTEX_APPLY_SMOKE_%s"), S->bPass ? TEXT("PASS") : TEXT("FAIL"));
                FPlatformMisc::RequestExit(false);
                return false;
            }
        }));
    }
}
#endif
