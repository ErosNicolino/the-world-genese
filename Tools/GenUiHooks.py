import io, os, re
import os
R = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "Source").replace("\\", "/")
# Gera os ganchos da interface (Docs/01_Base/MODULOS.md, VortexUI): VortexUiHooks.h, VortexUiFacade.cpp, VortexUiImpl.h e VortexUiModule.cpp.
# (namespace, ret, name, params with defaults, args, default return)
SPEC = [
 ("VortexHeroWindow","void","Toggle","AVortexCharacter* Hero","Hero",None),
 ("VortexHeroWindow","void","Open","AVortexCharacter* Hero, int32 Page = 0","Hero, Page",None),
 ("VortexHeroWindow","void","Close","","",None),
 ("VortexHeroWindow","bool","IsOpen","","","false"),
 ("VortexHeroWindow","void","SelectForTest","int32 BagIndex, int32 EquipSlot","BagIndex, EquipSlot",None),
 ("VortexHeroWindow","void","SpendForTest","int32 Attribute","Attribute",None),
 ("VortexHeroWindow","int32","RebuildsForTest","","","-1"),
 ("VortexShopWindow","void","Open","AVortexCharacter* Hero, FName Vendor","Hero, Vendor",None),
 ("VortexShopWindow","void","Close","","",None),
 ("VortexShopWindow","bool","IsOpen","","","false"),
 ("VortexDialogue","void","Open","AVortexCharacter* Hero, AVortexNpc* Npc","Hero, Npc",None),
 ("VortexDialogue","void","Close","","",None),
 ("VortexDialogue","bool","IsOpen","","","false"),
 ("VortexDialogue","AVortexNpc*","FindTalkable","const AVortexCharacter* Hero","Hero","nullptr"),
 ("VortexSocialUi","AVortexCharacter*","FindLookedAt","const AVortexCharacter* Hero","Hero","nullptr"),
 ("VortexSocialUi","void","OpenMenu","AVortexCharacter* Hero, AVortexCharacter* Other","Hero, Other",None),
 ("VortexSocialUi","bool","IsOpen","","","false"),
 ("VortexSocialUi","void","Close","","",None),
 ("VortexSocialUi","void","TickTrade","AVortexCharacter* Hero","Hero",None),
 ("VortexDevChat","void","Open","AVortexCharacter* Hero","Hero",None),
 ("VortexDevChat","void","Close","","",None),
 ("VortexDevChat","bool","IsTyping","","","false"),
 ("VortexDevChat","void","AddLine","const FString& Text, bool bError = false","Text, bError",None),
 ("VortexDevChat","FString","RunOnServer","AVortexCharacter* Hero, const FString& Line, bool& bOutError","Hero, Line, bOutError","(bOutError = true, FString(TEXT(\"sem interface\")))"),
 ("VortexIntroComic","void","Open","APlayerController* Controller, TFunction<void()> OnClosed = nullptr","Controller, MoveTemp(OnClosed)",None),
 ("VortexIntroComic","void","Close","","",None),
 ("VortexIntroComic","bool","IsOpen","","","false"),
 ("VortexIntroComic","void","DebugJump","int32 Page, int32 Panel","Page, Panel",None),
 ("VortexIntroComic","FString","DebugState","","","FString(TEXT(\"fechada\"))"),
 ("VortexIntroComic","bool","DebugSync","float& Image, float& Song","Image, Song","(Image = Song = -1.f, false)"),
 ("VortexAwakening","void","Open","AVortexCharacter* Hero, bool bAlreadyRevealed = false","Hero, bAlreadyRevealed",None),
 ("VortexAwakening","void","Close","","",None),
 ("VortexAwakening","bool","IsOpen","","","false"),
 ("VortexAwakening","void","DebugPick","int32 Index","Index",None),
 ("VortexAwakening","void","DebugConfirm","","",None),
 ("VortexAwakening","int32","DebugRarest","","","INDEX_NONE"),
 ("VortexSpellChoice","void","Open","AVortexCharacter* Hero","Hero",None),
 ("VortexSpellChoice","bool","IsOpen","","","false"),
 ("VortexSpellChoice","void","OpenHorseName","AVortexCharacter* Hero","Hero",None),
]

def nodef(p): return re.sub(r'\s*=\s*[^,]+', '', p)
def ptypes(p):
    if not p: return ""
    return ", ".join(re.sub(r'\s+\w+$', '', x.strip()) for x in nodef(p).split(","))
def field(ns, name): return ns.replace("Vortex", "") + name
def write(path, lines):
    io.open(R + path, "w", encoding="utf-8", newline="\r\n").write("\n".join(lines))

h = ['''#pragma once

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
    {''']
cur = None
for ns, ret, name, par, args, dr in SPEC:
    if ns != cur:
        h.append(f'        // {ns}')
        cur = ns
    h.append(f'        TFunction<{ret}({ptypes(par)})> {field(ns, name)};')
h.append('''        // o menu inicial e o de pausa (UVortexGameInstance)
        TFunction<TSharedPtr<SWidget>(UVortexGameInstance*)> MakeFrontEnd;
        TFunction<void(const TSharedPtr<SWidget>&, const FString&)> FrontEndNotice;
        TFunction<TSharedPtr<SWidget>(UVortexGameInstance*)> MakePauseMenu;
    };

    VORTEX_API FHandlers& Handlers();
}
''')
write("/Vortex/Public/Framework/VortexUiHooks.h", h)

c = ['''#include "Framework/VortexUiHooks.h"

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
''']
for ns, ret, name, par, args, dr in SPEC:
    f = field(ns, name)
    call = f'VortexUiHooks::Handlers().{f}({args})'
    if ret == "void":
        body = f'if (VortexUiHooks::Handlers().{f}) {call};'
    else:
        body = f'return VortexUiHooks::Handlers().{f} ? {call} : {dr};'
    c.append(f'{ret} {ns}::{name}({nodef(par)}) {{ {body} }}')
c.append('')
write("/Vortex/Private/Framework/VortexUiFacade.cpp", c)

i = ['''#pragma once

#include "CoreMinimal.h"

class APlayerController;
class AVortexCharacter;
class AVortexNpc;

// As janelas de verdade: as funções do jogo (Framework/Vortex*.h no módulo Vortex) chegam aqui pelos VortexUiHooks.''']
cur = None
for ns, ret, name, par, args, dr in SPEC:
    if ns != cur:
        if cur: i.append('}')
        i.append(f'namespace {ns}Impl\n{{')
        cur = ns
    i.append(f'    {ret} {name}({par});')
i.append('}\n')
write("/VortexUI/Private/VortexUiImpl.h", i)

m = ['''#include "Framework/VortexAimHUD.h"
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
        VortexUiHooks::FHandlers& H = VortexUiHooks::Handlers();''']
for ns, ret, name, par, args, dr in SPEC:
    m.append(f'        H.{field(ns, name)} = &{ns}Impl::{name};')
m.append('''        H.MakeFrontEnd = [](UVortexGameInstance* Owner) -> TSharedPtr<SWidget> { return SNew(SVortexFrontEnd).Owner(Owner); };
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
''')
write("/VortexUI/Private/VortexUiModule.cpp", m)
print("ok")
