using System.IO;
using UnrealBuildTool;

public class Vortex : ModuleRules
{
    public Vortex(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            // o mundo (terreno, névoa, dia e noite, cidade...): módulo próprio embaixo deste (02/11)
            "VortexWorld",
            // os catálogos dos JSON (aparência, RPG, tipos do save): módulo próprio embaixo deste (02/11)
            "VortexData",
            "Engine",
            "InputCore",
            "GameplayAbilities",
            "GameplayTags",
            "GameplayTasks",
            "MeshDescription",
            "StaticMeshDescription",
            "ProceduralMeshComponent",
            "RenderCore",
            "RHI",
            "Json",
            "ImageCore",
            "Slate",
            "SlateCore"
        });

        // multiplayer online pela Epic (Source/Vortex/*/Online): SDK da EOS e o P2P (NetDriverEOS)
        PrivateDependencyModuleNames.AddRange(new[]
        {
            "AssetRegistry",
            "EOSShared",
            "EOSSDK",
            "SocketSubsystemEOS",
            "Sockets",
            "NetCore",
            "ClothingSystemRuntimeInterface",
            "OnlineSubsystemUtils",
            "Niagara",
            "ApplicationCore"
        });

        // 09/11: o comando VortexMakeCloth (cria o Chaos Cloth das saias; só no editor)
        if (Target.bBuildEditor)
            PrivateDependencyModuleNames.AddRange(new[] { "UnrealEd", "ClothingSystemEditor", "ClothingSystemEditorInterface", "ClothingSystemRuntimeCommon", "ClothingSystemRuntimeInterface", "PhysicsCore" });

        // DLSS (plugin da NVIDIA em Plugins/DLSS, fora do git): o jogo só usa se ele estiver na pasta
        bool bDlss = Directory.Exists(Path.Combine(PluginDirectoryOrProject(), "Plugins", "DLSS"));
        if (bDlss) PrivateDependencyModuleNames.Add("DLSSBlueprint");
        PublicDefinitions.Add("VORTEX_WITH_DLSS=" + (bDlss ? "1" : "0"));
    }

    string PluginDirectoryOrProject()
    {
        // Source/Vortex -> raiz do projeto
        return Path.GetFullPath(Path.Combine(ModuleDirectory, "..", ".."));
    }
}
