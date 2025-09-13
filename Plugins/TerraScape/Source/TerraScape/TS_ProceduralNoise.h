#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "TS_ProceduralNoise.generated.h"

/**
 * Noise parameters for procedural generation
 * UE 5.6 UHT Compliance: Defined in global scope without namespace
 */
USTRUCT(BlueprintType)
struct TERRA_SCAPE_API FTS_NoiseParameters
{
	GENERATED_BODY()

	/** Seed for noise generation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | Voxel | Procedural")
	int32 Seed = 0;

	/** Frequency of noise (higher = more detail) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | Voxel | Procedural")
	float Frequency = 0.01f;

	/** Amplitude of noise (height variation) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | Voxel | Procedural")
	float Amplitude = 100.0f;

	/** Octaves for fractal noise (more = more detail) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | Voxel | Procedural")
	int32 Octaves = 4;

	/** Persistence for octave falloff */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | Voxel | Procedural")
	float Persistence = 0.5f;

	/** Lacunarity for octave frequency increase */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | Voxel | Procedural")
	float Lacunarity = 2.0f;

	/** Offset for noise sampling */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | Voxel | Procedural")
	FVector Offset = FVector::ZeroVector;

	FTS_NoiseParameters()
	{
		Seed = 0;
		Frequency = 0.01f;
		Amplitude = 100.0f;
		Octaves = 4;
		Persistence = 0.5f;
		Lacunarity = 2.0f;
		Offset = FVector::ZeroVector;
	}
};

/**
 * Noise layer for combining multiple noise functions
 * UE 5.6 UHT Compliance: Defined in global scope without namespace
 */
USTRUCT(BlueprintType)
struct TERRA_SCAPE_API FTS_NoiseLayer
{
	GENERATED_BODY()

	/** Noise parameters for this layer */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | Voxel | Procedural")
	FTS_NoiseParameters Parameters;

	/** Weight of this layer in final result */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | Voxel | Procedural")
	float Weight = 1.0f;

	/** Whether to use this layer */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | Voxel | Procedural")
	bool bEnabled = true;

	FTS_NoiseLayer()
	{
		Weight = 1.0f;
		bEnabled = true;
	}
};

/**
 * Procedural noise generation system
 * Provides Perlin and Simplex noise functions for terrain generation
 */
UCLASS(BlueprintType, Blueprintable)
class TERRA_SCAPE_API UTS_ProceduralNoise : public UObject
{
	GENERATED_BODY()

public:
	UTS_ProceduralNoise();

	/**
	 * Generate Perlin noise value at given coordinates
	 * @param X, Y, Z - World coordinates
	 * @param Parameters - Noise parameters
	 * @return Noise value between -1.0 and 1.0
	 */
	UFUNCTION(BlueprintCallable, Category = "TerraScape | Voxel | Procedural")
	static float PerlinNoise(float X, float Y, float Z, const FTS_NoiseParameters& Parameters);

	/**
	 * Generate Simplex noise value at given coordinates
	 * @param X, Y, Z - World coordinates
	 * @param Parameters - Noise parameters
	 * @return Noise value between -1.0 and 1.0
	 */
	UFUNCTION(BlueprintCallable, Category = "TerraScape | Voxel | Procedural")
	static float SimplexNoise(float X, float Y, float Z, const FTS_NoiseParameters& Parameters);

	/**
	 * Generate fractal noise (multiple octaves)
	 * @param X, Y, Z - World coordinates
	 * @param Parameters - Noise parameters
	 * @return Combined noise value
	 */
	UFUNCTION(BlueprintCallable, Category = "TerraScape | Voxel | Procedural")
	static float FractalNoise(float X, float Y, float Z, const FTS_NoiseParameters& Parameters);

	/**
	 * Combine multiple noise layers
	 * @param X, Y, Z - World coordinates
	 * @param Layers - Array of noise layers
	 * @return Combined noise value
	 */
	UFUNCTION(BlueprintCallable, Category = "TerraScape | Voxel | Procedural")
	static float CombineNoiseLayers(float X, float Y, float Z, const TArray<FTS_NoiseLayer>& Layers);

	/**
	 * Generate height value for terrain at given coordinates
	 * @param X, Y - World coordinates (2D)
	 * @param Parameters - Noise parameters
	 * @return Height value in world units
	 */
	UFUNCTION(BlueprintCallable, Category = "TerraScape | Voxel | Procedural")
	static float GetTerrainHeight(float X, float Y, const FTS_NoiseParameters& Parameters);

	/**
	 * Generate cave noise for underground structures
	 * @param X, Y, Z - World coordinates
	 * @param Parameters - Noise parameters
	 * @return Cave density (0.0 = solid, 1.0 = empty)
	 */
	UFUNCTION(BlueprintCallable, Category = "TerraScape | Voxel | Procedural")
	static float GetCaveDensity(float X, float Y, float Z, const FTS_NoiseParameters& Parameters);

private:
	/**
	 * Simple hash function for pseudo-random number generation
	 */
	static uint32 Hash(uint32 Input);

	/**
	 * Linear interpolation
	 */
	static float Lerp(float A, float B, float T);

	/**
	 * Smooth interpolation (smoothstep)
	 */
	static float SmoothStep(float T);

	/**
	 * Fade function for Perlin noise
	 */
	static float Fade(float T);
};
