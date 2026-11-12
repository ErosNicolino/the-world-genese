using UnrealBuildTool;

// O mundo (02/11: o primeiro pedaço tirado do módulo Vortex, para compilar menos): o terreno e o streamer, a névoa, o
// dia e a noite, a vida do mato, a cidade, a água, os portais, os marcos e os pontos de interesse, mais as opções do
// jogador (UVortexPlayerSettings). Não depende do jogo (herói, combate, interface): o que ele precisa de lá chega por
// VortexWorldHooks, preenchido pelo módulo Vortex ao abrir (Framework/VortexWorldHookup.cpp).
public class VortexWorld : ModuleRules
{
    public VortexWorld(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "ProceduralMeshComponent",
            "MeshDescription",
            "StaticMeshDescription",
            "RenderCore",
            "RHI",
            "Json"
        });

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "Niagara",
            "NetCore"
        });
    }
}
