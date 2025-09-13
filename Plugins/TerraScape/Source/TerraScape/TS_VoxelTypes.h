/**
 * @file TS_VoxelTypes.h
 * @brief Basic voxel data types for TerraScape MVP
 * @author Keves
 * @version 1.0
 */

#pragma once

#include "CoreMinimal.h"
#include "TS_VoxelTypes.generated.h"

/**
 * @brief Simple voxel structure for MVP
 * Contains only material ID - keep it minimal for now
 */
USTRUCT(BlueprintType)
struct FTS_Voxel
{
	GENERATED_BODY()

public:
	/** Material ID (0 = air/empty, >0 = solid material) */
	UPROPERTY(BlueprintReadWrite, Category = "TerraScape | Voxel")
	int32 MaterialID = 0;

	FTS_Voxel()
		: MaterialID(0)
	{
	}

	FTS_Voxel(int32 InMaterialID)
		: MaterialID(InMaterialID)
	{
	}

	/** Check if this voxel is solid */
	bool IsSolid() const
	{
		return MaterialID > 0;
	}

	/** Check if this voxel is empty */
	bool IsEmpty() const
	{
		return MaterialID == 0;
	}
};

/**
 * @brief Basic chunk data structure for MVP
 */
USTRUCT(BlueprintType)
struct FTS_Chunk
{
	GENERATED_BODY()

public:
	/** Chunk ID in world space */
	UPROPERTY(BlueprintReadOnly, Category = "TerraScape | Voxel")
	FIntVector ChunkID;

	/** World position of this chunk */
	UPROPERTY(BlueprintReadOnly, Category = "TerraScape | Voxel")
	FVector WorldPosition;

	/** Whether this chunk is loaded */
	UPROPERTY(BlueprintReadOnly, Category = "TerraScape | Voxel")
	bool bIsLoaded = false;

	FTS_Chunk()
		: ChunkID(FIntVector::ZeroValue)
		, WorldPosition(FVector::ZeroVector)
		, bIsLoaded(false)
	{
	}
};

