#pragma once

#include "CoreMinimal.h"

class APlayerController;
class AVortexCharacter;
class AVortexNpc;

// As janelas de verdade: as funções do jogo (Framework/Vortex*.h no módulo Vortex) chegam aqui pelos VortexUiHooks.
namespace VortexHeroWindowImpl
{
    void Toggle(AVortexCharacter* Hero);
    void Open(AVortexCharacter* Hero, int32 Page = 0);
    void Close();
    bool IsOpen();
    void SelectForTest(int32 BagIndex, int32 EquipSlot);
    void SpendForTest(int32 Attribute);
    int32 RebuildsForTest();
}
namespace VortexShopWindowImpl
{
    void Open(AVortexCharacter* Hero, FName Vendor);
    void Close();
    bool IsOpen();
}
namespace VortexDialogueImpl
{
    void Open(AVortexCharacter* Hero, AVortexNpc* Npc);
    void Close();
    bool IsOpen();
    AVortexNpc* FindTalkable(const AVortexCharacter* Hero);
}
namespace VortexSocialUiImpl
{
    AVortexCharacter* FindLookedAt(const AVortexCharacter* Hero);
    void OpenMenu(AVortexCharacter* Hero, AVortexCharacter* Other);
    bool IsOpen();
    void Close();
    void TickTrade(AVortexCharacter* Hero);
}
namespace VortexDevChatImpl
{
    void Open(AVortexCharacter* Hero);
    void Close();
    bool IsTyping();
    void AddLine(const FString& Text, bool bError = false);
    FString RunOnServer(AVortexCharacter* Hero, const FString& Line, bool& bOutError);
}
namespace VortexIntroComicImpl
{
    void Open(APlayerController* Controller, TFunction<void()> OnClosed = nullptr);
    void Close();
    bool IsOpen();
    void DebugJump(int32 Page, int32 Panel);
    FString DebugState();
    bool DebugSync(float& Image, float& Song);
}
namespace VortexAwakeningImpl
{
    void Open(AVortexCharacter* Hero, bool bAlreadyRevealed = false);
    void Close();
    bool IsOpen();
    void DebugPick(int32 Index);
    void DebugConfirm();
    int32 DebugRarest();
}
namespace VortexSpellChoiceImpl
{
    void Open(AVortexCharacter* Hero);
    bool IsOpen();
    void OpenHorseName(AVortexCharacter* Hero);
}
