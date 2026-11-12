using UnrealBuildTool;

public class VortexEditorTarget : TargetRules
{
    public VortexEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.Add("Vortex");
        // a interface fica em cima do jogo e ninguém depende dela: entra por aqui (02/11)
        ExtraModuleNames.Add("VortexUI");
    }
}
