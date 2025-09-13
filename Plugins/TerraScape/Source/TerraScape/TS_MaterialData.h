/**
 * @file TS_MaterialData.h
 * @brief Material data structures for TerraScape voxel materials
 * @author Keves
 * @version 1.0
 */

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Materials/MaterialInterface.h"
#include "TS_MaterialData.generated.h"

/**
 * @brief Material data structure for voxel materials
 * Used in data tables to configure materials per voxel type
 */
USTRUCT(BlueprintType)
struct TERRA_SCAPE_API FTS_VoxelMaterialData : public FTableRowBase
{
	GENERATED_BODY()

public:
	/** Material ID for this voxel type */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | Material")
	int32 MaterialID = 0;

	/** Display name for this material */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | Material")
	FString MaterialName = TEXT("Unknown");

	/** Base material to use for this voxel type */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | Material")
	UMaterialInterface* BaseMaterial = nullptr;

	/** Vertex color to apply (used when no material is set) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | Material")
	FColor VertexColor = FColor::White;

	/** Whether this material supports vertex colors */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | Material")
	bool bSupportsVertexColors = true;

	/** Physical properties */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | Material")
	bool bIsSolid = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | Material")
	bool bIsTransparent = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | Material")
	bool bIsDestructible = true;

	/** Default constructor */
	FTS_VoxelMaterialData()
		: MaterialID(0)
		, MaterialName(TEXT("Unknown"))
		, VertexColor(FColor::White)
		, bSupportsVertexColors(true)
		, bIsSolid(true)
		, bIsTransparent(false)
		, bIsDestructible(true)
	{
	}

	/** Constructor with parameters */
	FTS_VoxelMaterialData(int32 InMaterialID, const FString& InName, const FColor& InColor)
		: MaterialID(InMaterialID)
		, MaterialName(InName)
		, VertexColor(InColor)
		, bSupportsVertexColors(true)
		, bIsSolid(true)
		, bIsTransparent(false)
		, bIsDestructible(true)
	{
	}
};

/**
 * @brief Material manager for handling voxel materials
 * Manages material data tables and provides material lookup functionality
 */
UCLASS(BlueprintType, Blueprintable)
class TERRA_SCAPE_API UTS_MaterialManager : public UObject
{
	GENERATED_BODY()

public:
	UTS_MaterialManager();

	/** Initialize the material manager with a data table */
	UFUNCTION(BlueprintCallable, Category = "TerraScape | Material")
	void InitializeMaterialDataTable(UDataTable* InMaterialDataTable);

	/** Get material data for a specific material ID */
	UFUNCTION(BlueprintCallable, Category = "TerraScape | Material")
	FTS_VoxelMaterialData GetMaterialData(int32 MaterialID) const;

	/** Get material interface for a specific material ID */
	UFUNCTION(BlueprintCallable, Category = "TerraScape | Material")
	UMaterialInterface* GetMaterialInterface(int32 MaterialID) const;

	/** Get vertex color for a specific material ID */
	UFUNCTION(BlueprintCallable, Category = "TerraScape | Material")
	FColor GetVertexColor(int32 MaterialID) const;

	/** Check if material supports vertex colors */
	UFUNCTION(BlueprintCallable, Category = "TerraScape | Material")
	bool DoesMaterialSupportVertexColors(int32 MaterialID) const;

	/** Get all available material IDs */
	UFUNCTION(BlueprintCallable, Category = "TerraScape | Material")
	TArray<int32> GetAvailableMaterialIDs() const;

	/** Check if the material manager is initialized with a data table */
	UFUNCTION(BlueprintCallable, Category = "TerraScape | Material")
	bool IsInitialized() const;

protected:
	/** Material data table reference */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | Material")
	UDataTable* MaterialDataTable;

	/** Default material data for unknown materials */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | Material")
	FTS_VoxelMaterialData DefaultMaterialData;

private:
	/** Cache for material data lookups */
	mutable TMap<int32, FTS_VoxelMaterialData> MaterialDataCache;

	/** Cache for material interface lookups */
	mutable TMap<int32, UMaterialInterface*> MaterialInterfaceCache;
};
