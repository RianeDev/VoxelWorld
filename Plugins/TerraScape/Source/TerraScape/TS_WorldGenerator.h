#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "TS_WorldGenerator.generated.h"

class UTS_ProceduralNoise;
class UTS_BiomeManager;

/**
 * World generation parameters
 * UE 5.6 UHT Compliance: Defined in global scope without namespace
 */
USTRUCT(BlueprintType)
struct TERRA_SCAPE_API FTS_WorldGenParameters
{
	GENERATED_BODY()

	/** Global seed for world generation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | Voxel | Procedural")
	int32 WorldSeed = 12345;

	/** Base terrain height */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | Voxel | Procedural")
	float BaseHeight = 100.0f;

	/** Maximum terrain height */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | Voxel | Procedural")
	float MaxHeight = 500.0f;

	/** Minimum terrain height */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | Voxel | Procedural")
	float MinHeight = -100.0f;

	/** Enable cave generation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | Voxel | Procedural")
	bool bEnableCaves = true;

	/** Cave threshold (0.0 = no caves, 1.0 = many caves) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | Voxel | Procedural")
	float CaveThreshold = 0.3f;

	/** Enable biome generation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | Voxel | Procedural")
	bool bEnableBiomes = true;

	/** Enable ore generation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | Voxel | Procedural")
	bool bEnableOres = false;

	FTS_WorldGenParameters()
	{
		WorldSeed = 12345;
		BaseHeight = 100.0f;
		MaxHeight = 500.0f;
		MinHeight = -100.0f;
		bEnableCaves = true;
		CaveThreshold = 0.3f;
		bEnableBiomes = true;
		bEnableOres = false;
	}
};

/**
 * Voxel generation result
 * UE 5.6 UHT Compliance: Defined in global scope without namespace
 */
USTRUCT(BlueprintType)
struct TERRA_SCAPE_API FTS_VoxelGenResult
{
	GENERATED_BODY()

	/** Whether this voxel should be solid */
	UPROPERTY(BlueprintReadOnly, Category = "TerraScape | Voxel | Procedural")
	bool bIsSolid = false;

	/** Material ID for this voxel */
	UPROPERTY(BlueprintReadOnly, Category = "TerraScape | Voxel | Procedural")
	int32 MaterialID = 0;

	/** Biome name at this location */
	UPROPERTY(BlueprintReadOnly, Category = "TerraScape | Voxel | Procedural")
	FString BiomeName = TEXT("Unknown");

	/** Height at this location */
	UPROPERTY(BlueprintReadOnly, Category = "TerraScape | Voxel | Procedural")
	float Height = 0.0f;

	FTS_VoxelGenResult()
	{
		bIsSolid = false;
		MaterialID = 0;
		BiomeName = TEXT("Unknown");
		Height = 0.0f;
	}
};

/**
 * World generator for procedural terrain generation
 * Integrates noise functions, biomes, and material assignment
 */
UCLASS(BlueprintType, Blueprintable)
class TERRA_SCAPE_API UTS_WorldGenerator : public UObject
{
	GENERATED_BODY()

public:
	UTS_WorldGenerator();

	/**
	 * Initialize the world generator with default settings
	 */
	UFUNCTION(BlueprintCallable, Category = "TerraScape | Voxel | Procedural")
	void Initialize();

	/**
	 * Generate voxel data for a chunk at given coordinates
	 * @param ChunkX, ChunkY, ChunkZ - Chunk coordinates
	 * @param ChunkSize - Size of the chunk
	 * @param VoxelSize - Size of each voxel
	 * @param OutVoxelData - Output voxel data array
	 */
	UFUNCTION(BlueprintCallable, Category = "TerraScape | Voxel | Procedural")
	void GenerateChunkVoxels(int32 ChunkX, int32 ChunkY, int32 ChunkZ, int32 ChunkSize, float VoxelSize, TArray<int32>& OutVoxelData);

	/**
	 * Generate voxel data for a single voxel at world coordinates
	 * @param WorldX, WorldY, WorldZ - World coordinates
	 * @return Voxel generation result
	 */
	UFUNCTION(BlueprintCallable, Category = "TerraScape | Voxel | Procedural")
	FTS_VoxelGenResult GenerateVoxelAtLocation(float WorldX, float WorldY, float WorldZ);

	/**
	 * Set world generation parameters
	 * @param Parameters - New parameters
	 */
	UFUNCTION(BlueprintCallable, Category = "TerraScape | Voxel | Procedural")
	void SetWorldGenParameters(const FTS_WorldGenParameters& Parameters);

	/**
	 * Get current world generation parameters
	 * @return Current parameters
	 */
	UFUNCTION(BlueprintCallable, Category = "TerraScape | Voxel | Procedural")
	FTS_WorldGenParameters GetWorldGenParameters() const;

	/**
	 * Get the biome manager
	 * @return Biome manager instance
	 */
	UFUNCTION(BlueprintCallable, Category = "TerraScape | Voxel | Procedural")
	UTS_BiomeManager* GetBiomeManager() const;

	/**
	 * Get the noise generator
	 * @return Noise generator instance
	 */
	UFUNCTION(BlueprintCallable, Category = "TerraScape | Voxel | Procedural")
	UTS_ProceduralNoise* GetNoiseGenerator() const;

	/**
	 * Regenerate world with new seed
	 * @param NewSeed - New world seed
	 */
	UFUNCTION(BlueprintCallable, Category = "TerraScape | Voxel | Procedural")
	void RegenerateWorld(int32 NewSeed);

	/**
	 * Check if a voxel should be solid based on height and cave generation
	 * @param WorldX, WorldY, WorldZ - World coordinates
	 * @param TerrainHeight - Height of terrain at this location
	 * @return True if voxel should be solid
	 */
	UFUNCTION(BlueprintCallable, Category = "TerraScape | Voxel | Procedural")
	bool ShouldVoxelBeSolid(float WorldX, float WorldY, float WorldZ, float TerrainHeight) const;

	/**
	 * Get material ID for a voxel based on biome and height
	 * @param WorldX, WorldY, WorldZ - World coordinates
	 * @param TerrainHeight - Height of terrain at this location
	 * @return Material ID
	 */
	UFUNCTION(BlueprintCallable, Category = "TerraScape | Voxel | Procedural")
	int32 GetVoxelMaterialID(float WorldX, float WorldY, float WorldZ, float TerrainHeight) const;

	/**
	 * World generation parameters
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | Voxel | Procedural")
	FTS_WorldGenParameters WorldGenParameters;

private:
	/**
	 * Noise generator instance
	 */
	UPROPERTY()
	UTS_ProceduralNoise* NoiseGenerator;

	/**
	 * Biome manager instance
	 */
	UPROPERTY()
	UTS_BiomeManager* BiomeManager;

	/**
	 * Terrain noise parameters
	 */
	FTS_NoiseParameters TerrainNoiseParams;

	/**
	 * Cave noise parameters
	 */
	FTS_NoiseParameters CaveNoiseParams;

	/**
	 * Initialize noise parameters
	 */
	void InitializeNoiseParameters();

	/**
	 * Calculate terrain height at given coordinates
	 * @param X, Y - World coordinates
	 * @return Terrain height
	 */
	float CalculateTerrainHeight(float X, float Y) const;

	/**
	 * Calculate cave density at given coordinates
	 * @param X, Y, Z - World coordinates
	 * @return Cave density (0.0 = solid, 1.0 = empty)
	 */
	float CalculateCaveDensity(float X, float Y, float Z) const;
};
