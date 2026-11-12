using UnrealBuildTool;

// A interface (02/11, o terceiro pedaço tirado do módulo Vortex): o HUD, o menu inicial, o de pausa, as opções e as
// janelas (herói, loja, conversa, social, chat de testes, a HQ de abertura, o Despertar). Fica em cima do jogo: o
// jogo chama as janelas pelos VortexUiHooks e só conhece o HUD pela base (AVortexHudBase).
public class VortexUI : ModuleRules
{
    public VortexUI(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "Slate",
            "SlateCore",
            "Vortex",
            "VortexWorld",
            "VortexData"
        });

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "ApplicationCore",
            "GameplayAbilities",
            "ImageCore",
            "Json",
            "RenderCore",
            "RHI"
        });
    }
}
