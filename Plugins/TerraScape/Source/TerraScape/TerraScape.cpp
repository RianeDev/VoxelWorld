/**
 * @file TerraScape.cpp
 * @brief Main TerraScape plugin module implementation
 * @author Keves
 * @version 1.0
 */

#include "TerraScape.h"

#define LOCTEXT_NAMESPACE "FTerraScapeModule"

namespace TerraScape
{
	void FTerraScapeModule::StartupModule()
	{
		// Initialize TerraScape sub-modules
		InitializeSubModules();
		
		UE_LOG(LogTemp, Log, TEXT("TerraScape Module: Started successfully"));
	}

	void FTerraScapeModule::ShutdownModule()
	{
		// Shutdown TerraScape sub-modules
		ShutdownSubModules();
		
		UE_LOG(LogTemp, Log, TEXT("TerraScape Module: Shutdown complete"));
	}

	void FTerraScapeModule::InitializeSubModules()
	{
		// Initialize core runtime modules
		// Note: TS_ChunkManager is now a component, not a separate module
		// Other modules have been removed for MVP
		
		UE_LOG(LogTemp, Log, TEXT("TerraScape: Sub-modules initialized (MVP mode)"));
	}

	void FTerraScapeModule::ShutdownSubModules()
	{
		// Shutdown sub-modules (MVP mode - no additional modules to unload)
		UE_LOG(LogTemp, Log, TEXT("TerraScape: Sub-modules shutdown (MVP mode)"));
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(TerraScape::FTerraScapeModule, TerraScape)
