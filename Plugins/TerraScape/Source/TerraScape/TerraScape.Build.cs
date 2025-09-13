/**
 * @file TerraScape.Build.cs
 * @brief Build configuration for TerraScape plugin MVP
 * @author Keves
 * @version 1.0
 */

using UnrealBuildTool;

public class TerraScape : ModuleRules
{
	public TerraScape(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"ProceduralMeshComponent",
				"RenderCore",
				"RHI"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				// Keep minimal for MVP
			}
		);

		// Define API macro
		PublicDefinitions.Add("TERRA_SCAPE_API=DLLEXPORT");
	}
}