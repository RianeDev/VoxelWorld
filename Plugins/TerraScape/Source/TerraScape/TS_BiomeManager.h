#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "TS_BiomeManager.generated.h"

/**
 * Biome definition for procedural generation
 * UE 5.6 UHT Compliance: Defined in global scope without namespace
 */
USTRUCT(BlueprintType)
struct TERRA_SCAPE_API FTS_Biome
{
	GENERATED_BODY()

	/** Name of the biome */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | Voxel | Procedural")
	FString BiomeName = TEXT("Default");

	/** Height range for this biome (min, max) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | Voxel | Procedural")
	FVector2D HeightRange = FVector2D(-1000.0f, 1000.0f);

	/** Moisture range for this biome (min, max) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | Voxel | Procedural")
	FVector2D MoistureRange = FVector2D(0.0f, 1.0f);

	/** Temperature range for this biome (min, max) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | Voxel | Procedural")
	FVector2D TemperatureRange = FVector2D(0.0f, 1.0f);

	/** Primary voxel material ID for this biome */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | Voxel | Procedural")
	int32 PrimaryMaterialID = 1;

	/** Secondary voxel material ID for this biome */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | Voxel | Procedural")
	int32 SecondaryMaterialID = 2;

	/** Weight for biome selection (higher = more likely) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | Voxel | Procedural")
	float Weight = 1.0f;

	/** Whether this biome is enabled */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | Voxel | Procedural")
	bool bEnabled = true;

	FTS_Biome()
	{
		BiomeName = TEXT("Default");
		HeightRange = FVector2D(-1000.0f, 1000.0f);
		MoistureRange = FVector2D(0.0f, 1.0f);
		TemperatureRange = FVector2D(0.0f, 1.0f);
		PrimaryMaterialID = 1;
		SecondaryMaterialID = 2;
		Weight = 1.0f;
		bEnabled = true;
	}
};

/**
 * Biome blend result for smooth transitions
 * UE 5.6 UHT Compliance: Defined in global scope without namespace
 */
USTRUCT(BlueprintType)
struct TERRA_SCAPE_API FTS_BiomeBlend
{
	GENERATED_BODY()

	/** Primary biome */
	UPROPERTY(BlueprintReadOnly, Category = "TerraScape | Voxel | Procedural")
	FTS_Biome PrimaryBiome;

	/** Secondary biome for blending */
	UPROPERTY(BlueprintReadOnly, Category = "TerraScape | Voxel | Procedural")
	FTS_Biome SecondaryBiome;

	/** Blend factor (0.0 = primary, 1.0 = secondary) */
	UPROPERTY(BlueprintReadOnly, Category = "TerraScape | Voxel | Procedural")
	float BlendFactor = 0.0f;

	FTS_BiomeBlend()
	{
		BlendFactor = 0.0f;
	}
};

/**
 * Biome manager for procedural generation
 * Handles biome selection and material assignment based on environmental factors
 */
UCLASS(BlueprintType, Blueprintable)
class TERRA_SCAPE_API UTS_BiomeManager : public UObject
{
	GENERATED_BODY()

public:
	UTS_BiomeManager();

	/**
	 * Initialize default biomes for MVP
	 */
	UFUNCTION(BlueprintCallable, Category = "TerraScape | Voxel | Procedural")
	void InitializeDefaultBiomes();

	/**
	 * Get biome at given world coordinates
	 * @param X, Y, Z - World coordinates
	 * @return Selected biome
	 */
	UFUNCTION(BlueprintCallable, Category = "TerraScape | Voxel | Procedural")
	FTS_Biome GetBiomeAtLocation(float X, float Y, float Z) const;

	/**
	 * Get biome blend at given world coordinates (for smooth transitions)
	 * @param X, Y, Z - World coordinates
	 * @return Biome blend result
	 */
	UFUNCTION(BlueprintCallable, Category = "TerraScape | Voxel | Procedural")
	FTS_BiomeBlend GetBiomeBlendAtLocation(float X, float Y, float Z) const;

	/**
	 * Get material ID for voxel at given coordinates
	 * @param X, Y, Z - World coordinates
	 * @param Height - Height at this location
	 * @return Material ID
	 */
	UFUNCTION(BlueprintCallable, Category = "TerraScape | Voxel | Procedural")
	int32 GetMaterialIDAtLocation(float X, float Y, float Z, float Height) const;

	/**
	 * Add a new biome to the manager
	 * @param Biome - Biome to add
	 */
	UFUNCTION(BlueprintCallable, Category = "TerraScape | Voxel | Procedural")
	void AddBiome(const FTS_Biome& Biome);

	/**
	 * Remove a biome by name
	 * @param BiomeName - Name of biome to remove
	 */
	UFUNCTION(BlueprintCallable, Category = "TerraScape | Voxel | Procedural")
	void RemoveBiome(const FString& BiomeName);

	/**
	 * Get all available biomes
	 * @return Array of all biomes
	 */
	UFUNCTION(BlueprintCallable, Category = "TerraScape | Voxel | Procedural")
	TArray<FTS_Biome> GetAllBiomes() const;

	/**
	 * Set biome parameters
	 * @param Biomes - Array of biomes to set
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | Voxel | Procedural")
	TArray<FTS_Biome> Biomes;

private:
	/**
	 * Calculate environmental factors at given location
	 * @param X, Y, Z - World coordinates
	 * @param OutHeight - Output height
	 * @param OutMoisture - Output moisture
	 * @param OutTemperature - Output temperature
	 */
	void CalculateEnvironmentalFactors(float X, float Y, float Z, float& OutHeight, float& OutMoisture, float& OutTemperature) const;

	/**
	 * Find best matching biome for given environmental factors
	 * @param Height - Height value
	 * @param Moisture - Moisture value
	 * @param Temperature - Temperature value
	 * @return Best matching biome
	 */
	FTS_Biome FindBestBiome(float Height, float Moisture, float Temperature) const;

	/**
	 * Calculate biome transition blend
	 * @param X, Y, Z - World coordinates
	 * @param PrimaryBiome - Primary biome
	 * @param OutSecondaryBiome - Output secondary biome
	 * @param OutBlendFactor - Output blend factor
	 */
	void CalculateBiomeTransition(float X, float Y, float Z, const FTS_Biome& PrimaryBiome, FTS_Biome& OutSecondaryBiome, float& OutBlendFactor) const;
};
