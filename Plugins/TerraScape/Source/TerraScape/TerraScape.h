/**
 * @file TerraScape.h
 * @brief Main TerraScape plugin module header
 * @author Keves
 * @version 1.0
 */

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

namespace TerraScape
{
	/**
	 * @brief Main TerraScape plugin module class
	 * 
	 * This module serves as the entry point for the TerraScape voxel system.
	 * It initializes all sub-modules and provides the main plugin interface.
	 */
	class FTerraScapeModule : public IModuleInterface
	{
	public:
		/** IModuleInterface implementation */
		virtual void StartupModule() override;
		virtual void ShutdownModule() override;
		
		/**
		 * @brief Get the singleton instance of the TerraScape module
		 * @return Reference to the TerraScape module instance
		 */
		static inline FTerraScapeModule& Get()
		{
			return FModuleManager::LoadModuleChecked<FTerraScapeModule>("TerraScape");
		}
		
		/**
		 * @brief Check if the TerraScape module is available
		 * @return True if the module is loaded and available
		 */
		static inline bool IsAvailable()
		{
			return FModuleManager::Get().IsModuleLoaded("TerraScape");
		}

	private:
		/** Initialize all TerraScape sub-modules */
		void InitializeSubModules();
		
		/** Shutdown all TerraScape sub-modules */
		void ShutdownSubModules();
	};
}
