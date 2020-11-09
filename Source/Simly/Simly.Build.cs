// Some copyright should be here...

using UnrealBuildTool;
using System.IO;

public class Simly : ModuleRules
{
	public Simly(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        bEnableExceptions = true;

        PrivateIncludePaths.AddRange(new string[] { Path.Combine(ModuleDirectory, "Private") });
        PublicIncludePaths.AddRange(new string[] { Path.Combine(ModuleDirectory, "Public") });

        PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
		);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
		);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
                "Core",
                "CoreUObject",
                "Sockets",
                "Networking",
                "Engine",
                "InputCore",
                "RHI",
                "RenderCore",
                "Slate",
                "SlateCore",
                "RuntimeMeshComponent",
                "LidarPointCloudRuntime",
                "RealSense",
                "MediaCompositing"
				// ... add other public dependencies that you statically link with here ...
			}
		);

        if (Target.bBuildEditor)
        {
            PublicDependencyModuleNames.AddRange(
            new string[] {
                "UnrealEd",
                "EditorStyle",
                "DesktopWidgets",
                "DesktopPlatform"
            });
        }


        PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
                "MediaCompositing"
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
		);

        if (Target.Platform == UnrealTargetPlatform.Linux)
        {
            string RealSenseDirectory = "/usr/local/";
            PublicIncludePaths.Add(Path.Combine(RealSenseDirectory, "include"));
            PublicAdditionalLibraries.Add(Path.Combine(RealSenseDirectory, "lib64/librealsense2.so"));
        }
        else if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            string RealSenseDirectory = "C:/Program Files (x86)/Intel RealSense SDK 2.0";
            PublicIncludePaths.Add(Path.Combine(RealSenseDirectory, "include"));
            PublicLibraryPaths.Add(Path.Combine(RealSenseDirectory, "lib", "x64"));
            PublicLibraryPaths.Add(Path.Combine(RealSenseDirectory, "bin", "x64"));
            PublicAdditionalLibraries.Add("realsense2.lib");
        }
    }
}
