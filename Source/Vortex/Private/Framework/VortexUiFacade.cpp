#include "Framework/VortexUiHooks.h"

#include "Framework/VortexAwakening.h"
#include "Framework/VortexDevChat.h"
#include "Framework/VortexDialogue.h"
#include "Framework/VortexHeroWindow.h"
#include "Framework/VortexIntroComic.h"
#include "Framework/VortexShopWindow.h"
#include "Framework/VortexSocialUi.h"
#include "Framework/VortexSpellChoice.h"

// As janelas que o jogo chama, passando para a interface (VortexUiHooks.h). Gerado por Tools/GenUiHooks.py.
VortexUiHooks::FHandlers& VortexUiHooks::Handlers()
{
    static FHandlers Instance;
    return Instance;
}

void VortexHeroWindow::Toggle(AVortexCharacter* Hero) { if (VortexUiHooks::Handlers().HeroWindowToggle) VortexUiHooks::Handlers().HeroWindowToggle(Hero); }
void VortexHeroWindow::Open(AVortexCharacter* Hero, int32 Page) { if (VortexUiHooks::Handlers().HeroWindowOpen) VortexUiHooks::Handlers().HeroWindowOpen(Hero, Page); }
void VortexHeroWindow::Close() { if (VortexUiHooks::Handlers().HeroWindowClose) VortexUiHooks::Handlers().HeroWindowClose(); }
bool VortexHeroWindow::IsOpen() { return VortexUiHooks::Handlers().HeroWindowIsOpen ? VortexUiHooks::Handlers().HeroWindowIsOpen() : false; }
void VortexHeroWindow::SelectForTest(int32 BagIndex, int32 EquipSlot) { if (VortexUiHooks::Handlers().HeroWindowSelectForTest) VortexUiHooks::Handlers().HeroWindowSelectForTest(BagIndex, EquipSlot); }
void VortexHeroWindow::SpendForTest(int32 Attribute) { if (VortexUiHooks::Handlers().HeroWindowSpendForTest) VortexUiHooks::Handlers().HeroWindowSpendForTest(Attribute); }
int32 VortexHeroWindow::RebuildsForTest() { return VortexUiHooks::Handlers().HeroWindowRebuildsForTest ? VortexUiHooks::Handlers().HeroWindowRebuildsForTest() : -1; }
void VortexShopWindow::Open(AVortexCharacter* Hero, FName Vendor) { if (VortexUiHooks::Handlers().ShopWindowOpen) VortexUiHooks::Handlers().ShopWindowOpen(Hero, Vendor); }
void VortexShopWindow::Close() { if (VortexUiHooks::Handlers().ShopWindowClose) VortexUiHooks::Handlers().ShopWindowClose(); }
bool VortexShopWindow::IsOpen() { return VortexUiHooks::Handlers().ShopWindowIsOpen ? VortexUiHooks::Handlers().ShopWindowIsOpen() : false; }
void VortexDialogue::Open(AVortexCharacter* Hero, AVortexNpc* Npc) { if (VortexUiHooks::Handlers().DialogueOpen) VortexUiHooks::Handlers().DialogueOpen(Hero, Npc); }
void VortexDialogue::Close() { if (VortexUiHooks::Handlers().DialogueClose) VortexUiHooks::Handlers().DialogueClose(); }
bool VortexDialogue::IsOpen() { return VortexUiHooks::Handlers().DialogueIsOpen ? VortexUiHooks::Handlers().DialogueIsOpen() : false; }
AVortexNpc* VortexDialogue::FindTalkable(const AVortexCharacter* Hero) { return VortexUiHooks::Handlers().DialogueFindTalkable ? VortexUiHooks::Handlers().DialogueFindTalkable(Hero) : nullptr; }
AVortexCharacter* VortexSocialUi::FindLookedAt(const AVortexCharacter* Hero) { return VortexUiHooks::Handlers().SocialUiFindLookedAt ? VortexUiHooks::Handlers().SocialUiFindLookedAt(Hero) : nullptr; }
void VortexSocialUi::OpenMenu(AVortexCharacter* Hero, AVortexCharacter* Other) { if (VortexUiHooks::Handlers().SocialUiOpenMenu) VortexUiHooks::Handlers().SocialUiOpenMenu(Hero, Other); }
bool VortexSocialUi::IsOpen() { return VortexUiHooks::Handlers().SocialUiIsOpen ? VortexUiHooks::Handlers().SocialUiIsOpen() : false; }
void VortexSocialUi::Close() { if (VortexUiHooks::Handlers().SocialUiClose) VortexUiHooks::Handlers().SocialUiClose(); }
void VortexSocialUi::TickTrade(AVortexCharacter* Hero) { if (VortexUiHooks::Handlers().SocialUiTickTrade) VortexUiHooks::Handlers().SocialUiTickTrade(Hero); }
void VortexDevChat::Open(AVortexCharacter* Hero) { if (VortexUiHooks::Handlers().DevChatOpen) VortexUiHooks::Handlers().DevChatOpen(Hero); }
void VortexDevChat::Close() { if (VortexUiHooks::Handlers().DevChatClose) VortexUiHooks::Handlers().DevChatClose(); }
bool VortexDevChat::IsTyping() { return VortexUiHooks::Handlers().DevChatIsTyping ? VortexUiHooks::Handlers().DevChatIsTyping() : false; }
void VortexDevChat::AddLine(const FString& Text, bool bError) { if (VortexUiHooks::Handlers().DevChatAddLine) VortexUiHooks::Handlers().DevChatAddLine(Text, bError); }
FString VortexDevChat::RunOnServer(AVortexCharacter* Hero, const FString& Line, bool& bOutError) { return VortexUiHooks::Handlers().DevChatRunOnServer ? VortexUiHooks::Handlers().DevChatRunOnServer(Hero, Line, bOutError) : (bOutError = true, FString(TEXT("sem interface"))); }
void VortexIntroComic::Open(APlayerController* Controller, TFunction<void()> OnClosed) { if (VortexUiHooks::Handlers().IntroComicOpen) VortexUiHooks::Handlers().IntroComicOpen(Controller, MoveTemp(OnClosed)); }
void VortexIntroComic::Close() { if (VortexUiHooks::Handlers().IntroComicClose) VortexUiHooks::Handlers().IntroComicClose(); }
bool VortexIntroComic::IsOpen() { return VortexUiHooks::Handlers().IntroComicIsOpen ? VortexUiHooks::Handlers().IntroComicIsOpen() : false; }
void VortexIntroComic::DebugJump(int32 Page, int32 Panel) { if (VortexUiHooks::Handlers().IntroComicDebugJump) VortexUiHooks::Handlers().IntroComicDebugJump(Page, Panel); }
FString VortexIntroComic::DebugState() { return VortexUiHooks::Handlers().IntroComicDebugState ? VortexUiHooks::Handlers().IntroComicDebugState() : FString(TEXT("fechada")); }
bool VortexIntroComic::DebugSync(float& Image, float& Song) { return VortexUiHooks::Handlers().IntroComicDebugSync ? VortexUiHooks::Handlers().IntroComicDebugSync(Image, Song) : (Image = Song = -1.f, false); }
void VortexAwakening::Open(AVortexCharacter* Hero, bool bAlreadyRevealed) { if (VortexUiHooks::Handlers().AwakeningOpen) VortexUiHooks::Handlers().AwakeningOpen(Hero, bAlreadyRevealed); }
void VortexAwakening::Close() { if (VortexUiHooks::Handlers().AwakeningClose) VortexUiHooks::Handlers().AwakeningClose(); }
bool VortexAwakening::IsOpen() { return VortexUiHooks::Handlers().AwakeningIsOpen ? VortexUiHooks::Handlers().AwakeningIsOpen() : false; }
void VortexAwakening::DebugPick(int32 Index) { if (VortexUiHooks::Handlers().AwakeningDebugPick) VortexUiHooks::Handlers().AwakeningDebugPick(Index); }
void VortexAwakening::DebugConfirm() { if (VortexUiHooks::Handlers().AwakeningDebugConfirm) VortexUiHooks::Handlers().AwakeningDebugConfirm(); }
int32 VortexAwakening::DebugRarest() { return VortexUiHooks::Handlers().AwakeningDebugRarest ? VortexUiHooks::Handlers().AwakeningDebugRarest() : INDEX_NONE; }
void VortexSpellChoice::Open(AVortexCharacter* Hero) { if (VortexUiHooks::Handlers().SpellChoiceOpen) VortexUiHooks::Handlers().SpellChoiceOpen(Hero); }
bool VortexSpellChoice::IsOpen() { return VortexUiHooks::Handlers().SpellChoiceIsOpen ? VortexUiHooks::Handlers().SpellChoiceIsOpen() : false; }
void VortexSpellChoice::OpenHorseName(AVortexCharacter* Hero) { if (VortexUiHooks::Handlers().SpellChoiceOpenHorseName) VortexUiHooks::Handlers().SpellChoiceOpenHorseName(Hero); }
