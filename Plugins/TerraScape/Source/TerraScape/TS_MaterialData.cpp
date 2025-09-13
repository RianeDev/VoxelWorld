/**
 * @file TS_MaterialData.cpp
 * @brief Material data implementation for TerraScape voxel materials
 * @author Keves
 * @version 1.0
 */

#include "TS_MaterialData.h"
#include "Engine/DataTable.h"
#include "Materials/MaterialInterface.h"

UTS_MaterialManager::UTS_MaterialManager()
{
	// Set up default material data
	DefaultMaterialData = FTS_VoxelMaterialData(0, TEXT("Empty"), FColor::Transparent);
	DefaultMaterialData.bIsSolid = false;
	DefaultMaterialData.bIsTransparent = true;
	DefaultMaterialData.bIsDestructible = false;
}

void UTS_MaterialManager::InitializeMaterialDataTable(UDataTable* InMaterialDataTable)
{
	MaterialDataTable = InMaterialDataTable;
	
	// Clear caches when data table changes
	MaterialDataCache.Empty();
	MaterialInterfaceCache.Empty();
	
	if (MaterialDataTable)
	{
		UE_LOG(LogTemp, Log, TEXT("TerraScape Material Manager: Initialized with data table containing %d rows"), 
			MaterialDataTable->GetRowMap().Num());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("TerraScape Material Manager: No data table provided, using defaults only"));
	}
}

FTS_VoxelMaterialData UTS_MaterialManager::GetMaterialData(int32 MaterialID) const
{
	// Check cache first
	if (MaterialDataCache.Contains(MaterialID))
	{
		return MaterialDataCache[MaterialID];
	}

	// Look up in data table
	if (MaterialDataTable)
	{
		FString RowName = FString::Printf(TEXT("Material_%d"), MaterialID);
		FTS_VoxelMaterialData* FoundData = MaterialDataTable->FindRow<FTS_VoxelMaterialData>(*RowName, TEXT("GetMaterialData"));
		
		if (FoundData)
		{
			// Cache the result
			MaterialDataCache.Add(MaterialID, *FoundData);
			return *FoundData;
		}
	}

	// Return default material data
	return DefaultMaterialData;
}

UMaterialInterface* UTS_MaterialManager::GetMaterialInterface(int32 MaterialID) const
{
	// Check cache first
	if (MaterialInterfaceCache.Contains(MaterialID))
	{
		return MaterialInterfaceCache[MaterialID];
	}

	// Get material data
	FTS_VoxelMaterialData MaterialData = GetMaterialData(MaterialID);
	
	// Get the material directly (no loading needed for direct pointer)
	UMaterialInterface* MaterialInterface = MaterialData.BaseMaterial;

	// Cache the result (even if null)
	MaterialInterfaceCache.Add(MaterialID, MaterialInterface);
	
	return MaterialInterface;
}

FColor UTS_MaterialManager::GetVertexColor(int32 MaterialID) const
{
	FTS_VoxelMaterialData MaterialData = GetMaterialData(MaterialID);
	return MaterialData.VertexColor;
}

bool UTS_MaterialManager::DoesMaterialSupportVertexColors(int32 MaterialID) const
{
	FTS_VoxelMaterialData MaterialData = GetMaterialData(MaterialID);
	return MaterialData.bSupportsVertexColors;
}

TArray<int32> UTS_MaterialManager::GetAvailableMaterialIDs() const
{
	TArray<int32> MaterialIDs;
	
	if (MaterialDataTable)
	{
		// Get all row names from the data table
		TArray<FName> RowNames = MaterialDataTable->GetRowNames();
		
		for (const FName& RowName : RowNames)
		{
			FString RowNameString = RowName.ToString();
			if (RowNameString.StartsWith(TEXT("Material_")))
			{
				FString MaterialIDString = RowNameString.Mid(9); // Remove "Material_" prefix
				int32 MaterialID = FCString::Atoi(*MaterialIDString);
				MaterialIDs.Add(MaterialID);
			}
		}
	}
	
	// Sort the array
	MaterialIDs.Sort();
	
	return MaterialIDs;
}

bool UTS_MaterialManager::IsInitialized() const
{
	return MaterialDataTable != nullptr;
}
