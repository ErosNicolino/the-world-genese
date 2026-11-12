using UnrealBuildTool;

// Os catálogos lidos dos JSON (02/11, o segundo pedaço tirado do módulo Vortex): a aparência (VortexLook, os estilos de
// rosto), o RPG (itens, classes, perícias, receitas) e os tipos do save. Só dados: não depende do mundo nem do jogo.
public class VortexData : ModuleRules
{
    public VortexData(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "Json"
        });

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "ImageCore"
        });
    }
}
