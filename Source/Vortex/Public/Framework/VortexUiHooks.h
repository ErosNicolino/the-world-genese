#pragma once

#include "CoreMinimal.h"

class APlayerController;
class AVortexCharacter;
class AVortexNpc;
class SWidget;
class UVortexGameInstance;

/**
 * O que o jogo pede à interface (02/11, a separação em módulos): o módulo VortexUI fica em cima do Vortex. O jogo
 * continua chamando VortexHeroWindow::Open, VortexDialogue::IsOpen... (os cabeçalhos ficaram aqui); essas funções
 * (Framework/VortexUiFacade.cpp) passam para estes handlers, que o VortexUI preenche ao abrir
 * (VortexUI/Private/VortexUiModule.cpp). Sem a interface carregada, não fazem nada.
 * O HUD: o jogo só conhece AVortexHudBase (Framework/VortexHudBase.h); o VortexUI põe o AVortexAimHUD no game mode.
 */
namespace VortexUiHooks
{
    struct FHandlers
    {
        // VortexHeroWindow
        TFunction<void(AVortexCharacter*)> HeroWindowToggle;
        TFunction<void(AVortexCharacter*, int32)> HeroWindowOpen;
        TFunction<void()> HeroWindowClose;
        TFunction<bool()> HeroWindowIsOpen;
        TFunction<void(int32, int32)> HeroWindowSelectForTest;
        TFunction<void(int32)> HeroWindowSpendForTest;
        TFunction<int32()> HeroWindowRebuildsForTest;
        // VortexShopWindow
        TFunction<void(AVortexCharacter*, FName)> ShopWindowOpen;
        TFunction<void()> ShopWindowClose;
        TFunction<bool()> ShopWindowIsOpen;
        // VortexDialogue
        TFunction<void(AVortexCharacter*, AVortexNpc*)> DialogueOpen;
        TFunction<void()> DialogueClose;
        TFunction<bool()> DialogueIsOpen;
        TFunction<AVortexNpc*(const AVortexCharacter*)> DialogueFindTalkable;
        // VortexSocialUi
        TFunction<AVortexCharacter*(const AVortexCharacter*)> SocialUiFindLookedAt;
        TFunction<void(AVortexCharacter*, AVortexCharacter*)> SocialUiOpenMenu;
        TFunction<bool()> SocialUiIsOpen;
        TFunction<void()> SocialUiClose;
        TFunction<void(AVortexCharacter*)> SocialUiTickTrade;
        // VortexDevChat
        TFunction<void(AVortexCharacter*)> DevChatOpen;
        TFunction<void()> DevChatClose;
        TFunction<bool()> DevChatIsTyping;
        TFunction<void(const FString&, bool)> DevChatAddLine;
        TFunction<FString(AVortexCharacter*, const FString&, bool&)> DevChatRunOnServer;
        // VortexIntroComic
        TFunction<void(APlayerController*, TFunction<void()>)> IntroComicOpen;
        TFunction<void()> IntroComicClose;
        TFunction<bool()> IntroComicIsOpen;
        TFunction<void(int32, int32)> IntroComicDebugJump;
        TFunction<FString()> IntroComicDebugState;
        TFunction<bool(float&, float&)> IntroComicDebugSync;
        // VortexAwakening
        TFunction<void(AVortexCharacter*, bool)> AwakeningOpen;
        TFunction<void()> AwakeningClose;
        TFunction<bool()> AwakeningIsOpen;
        TFunction<void(int32)> AwakeningDebugPick;
        TFunction<void()> AwakeningDebugConfirm;
        TFunction<int32()> AwakeningDebugRarest;
        // VortexSpellChoice
        TFunction<void(AVortexCharacter*)> SpellChoiceOpen;
        TFunction<bool()> SpellChoiceIsOpen;
        TFunction<void(AVortexCharacter*)> SpellChoiceOpenHorseName;
        // o menu inicial e o de pausa (UVortexGameInstance)
        TFunction<TSharedPtr<SWidget>(UVortexGameInstance*)> MakeFrontEnd;
        TFunction<void(const TSharedPtr<SWidget>&, const FString&)> FrontEndNotice;
        TFunction<TSharedPtr<SWidget>(UVortexGameInstance*)> MakePauseMenu;
    };

    VORTEX_API FHandlers& Handlers();
}
