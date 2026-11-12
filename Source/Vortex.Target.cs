using UnrealBuildTool;

public class VortexTarget : TargetRules
{
    public VortexTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.Add("Vortex");
        // a interface fica em cima do jogo e ninguém depende dela: entra por aqui (02/11)
        ExtraModuleNames.Add("VortexUI");
    }
}
