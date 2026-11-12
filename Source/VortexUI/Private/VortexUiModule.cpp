#include "Framework/VortexAimHUD.h"
#include "Framework/VortexFrontEnd.h"
#include "Framework/VortexGameMode.h"
#include "Framework/VortexPauseMenu.h"
#include "Framework/VortexUiHooks.h"
#include "Modules/ModuleManager.h"
#include "VortexUiImpl.h"

// A interface (02/11): ao abrir, preenche os ganchos que o jogo chama (VortexUiHooks) e põe o HUD no game mode.
// (Gerado por Tools/GenUiHooks.py: mudar lá.)
class FVortexUiModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        VortexUiHooks::FHandlers& H = VortexUiHooks::Handlers();
        H.HeroWindowToggle = &VortexHeroWindowImpl::Toggle;
        H.HeroWindowOpen = &VortexHeroWindowImpl::Open;
        H.HeroWindowClose = &VortexHeroWindowImpl::Close;
        H.HeroWindowIsOpen = &VortexHeroWindowImpl::IsOpen;
        H.HeroWindowSelectForTest = &VortexHeroWindowImpl::SelectForTest;
        H.HeroWindowSpendForTest = &VortexHeroWindowImpl::SpendForTest;
        H.HeroWindowRebuildsForTest = &VortexHeroWindowImpl::RebuildsForTest;
        H.ShopWindowOpen = &VortexShopWindowImpl::Open;
        H.ShopWindowClose = &VortexShopWindowImpl::Close;
        H.ShopWindowIsOpen = &VortexShopWindowImpl::IsOpen;
        H.DialogueOpen = &VortexDialogueImpl::Open;
        H.DialogueClose = &VortexDialogueImpl::Close;
        H.DialogueIsOpen = &VortexDialogueImpl::IsOpen;
        H.DialogueFindTalkable = &VortexDialogueImpl::FindTalkable;
        H.SocialUiFindLookedAt = &VortexSocialUiImpl::FindLookedAt;
        H.SocialUiOpenMenu = &VortexSocialUiImpl::OpenMenu;
        H.SocialUiIsOpen = &VortexSocialUiImpl::IsOpen;
        H.SocialUiClose = &VortexSocialUiImpl::Close;
        H.SocialUiTickTrade = &VortexSocialUiImpl::TickTrade;
        H.DevChatOpen = &VortexDevChatImpl::Open;
        H.DevChatClose = &VortexDevChatImpl::Close;
        H.DevChatIsTyping = &VortexDevChatImpl::IsTyping;
        H.DevChatAddLine = &VortexDevChatImpl::AddLine;
        H.DevChatRunOnServer = &VortexDevChatImpl::RunOnServer;
        H.IntroComicOpen = &VortexIntroComicImpl::Open;
        H.IntroComicClose = &VortexIntroComicImpl::Close;
        H.IntroComicIsOpen = &VortexIntroComicImpl::IsOpen;
        H.IntroComicDebugJump = &VortexIntroComicImpl::DebugJump;
        H.IntroComicDebugState = &VortexIntroComicImpl::DebugState;
        H.IntroComicDebugSync = &VortexIntroComicImpl::DebugSync;
        H.AwakeningOpen = &VortexAwakeningImpl::Open;
        H.AwakeningClose = &VortexAwakeningImpl::Close;
        H.AwakeningIsOpen = &VortexAwakeningImpl::IsOpen;
        H.AwakeningDebugPick = &VortexAwakeningImpl::DebugPick;
        H.AwakeningDebugConfirm = &VortexAwakeningImpl::DebugConfirm;
        H.AwakeningDebugRarest = &VortexAwakeningImpl::DebugRarest;
        H.SpellChoiceOpen = &VortexSpellChoiceImpl::Open;
        H.SpellChoiceIsOpen = &VortexSpellChoiceImpl::IsOpen;
        H.SpellChoiceOpenHorseName = &VortexSpellChoiceImpl::OpenHorseName;
        H.MakeFrontEnd = [](UVortexGameInstance* Owner) -> TSharedPtr<SWidget> { return SNew(SVortexFrontEnd).Owner(Owner); };
        H.FrontEndNotice = [](const TSharedPtr<SWidget>& Widget, const FString& Message)
        {
            if (Widget.IsValid()) StaticCastSharedPtr<SVortexFrontEnd>(Widget)->SetNotice(Message);
        };
        H.MakePauseMenu = [](UVortexGameInstance* Owner) -> TSharedPtr<SWidget> { return MakeVortexPauseMenu(Owner); };
        // o HUD: posto quando o game mode começa (InitGame), antes de qualquer jogador entrar
        GameModeReady = FGameModeEvents::OnGameModeInitializedEvent().AddLambda([](AGameModeBase* Mode)
        {
            if (Mode && Mode->IsA<AVortexGameMode>()) Mode->HUDClass = AVortexAimHUD::StaticClass();
        });
    }

    virtual void ShutdownModule() override
    {
        FGameModeEvents::OnGameModeInitializedEvent().Remove(GameModeReady);
        VortexUiHooks::Handlers() = VortexUiHooks::FHandlers();
    }

private:
    FDelegateHandle GameModeReady;
};

IMPLEMENT_MODULE(FVortexUiModule, VortexUI);
