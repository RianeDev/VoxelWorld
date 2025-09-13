#include "TS_WorldGenerator.h"
#include "TS_ProceduralNoise.h"
#include "TS_BiomeManager.h"
#include "Math/UnrealMathUtility.h"

UTS_WorldGenerator::UTS_WorldGenerator()
{
	// Initialize components
	NoiseGenerator = nullptr;
	BiomeManager = nullptr;
	
	// Initialize with default parameters
	Initialize();
}

void UTS_WorldGenerator::Initialize()
{
	// Create noise generator
	if (!NoiseGenerator)
	{
		NoiseGenerator = NewObject<UTS_ProceduralNoise>(this);
	}

	// Create biome manager
	if (!BiomeManager)
	{
		BiomeManager = NewObject<UTS_BiomeManager>(this);
		BiomeManager->InitializeDefaultBiomes();
	}

	// Initialize noise parameters
	InitializeNoiseParameters();
}

void UTS_WorldGenerator::GenerateChunkVoxels(int32 ChunkX, int32 ChunkY, int32 ChunkZ, int32 ChunkSize, float VoxelSize, TArray<int32>& OutVoxelData)
{
	// Clear output array
	OutVoxelData.Empty();
	OutVoxelData.SetNum(ChunkSize * ChunkSize * ChunkSize);

	// Calculate chunk world position
	float ChunkWorldX = ChunkX * ChunkSize * VoxelSize;
	float ChunkWorldY = ChunkY * ChunkSize * VoxelSize;
	float ChunkWorldZ = ChunkZ * ChunkSize * VoxelSize;

	// Generate voxels for this chunk
	for (int32 X = 0; X < ChunkSize; X++)
	{
		for (int32 Y = 0; Y < ChunkSize; Y++)
		{
			for (int32 Z = 0; Z < ChunkSize; Z++)
			{
				// Calculate world coordinates for this voxel
				float WorldX = ChunkWorldX + (X * VoxelSize);
				float WorldY = ChunkWorldY + (Y * VoxelSize);
				float WorldZ = ChunkWorldZ + (Z * VoxelSize);

				// Generate voxel at this location
				FTS_VoxelGenResult Result = GenerateVoxelAtLocation(WorldX, WorldY, WorldZ);

				// Calculate array index
				int32 Index = X + (Y * ChunkSize) + (Z * ChunkSize * ChunkSize);

				// Store voxel data (0 = air, >0 = material ID)
				OutVoxelData[Index] = Result.bIsSolid ? Result.MaterialID : 0;
			}
		}
	}
}

FTS_VoxelGenResult UTS_WorldGenerator::GenerateVoxelAtLocation(float WorldX, float WorldY, float WorldZ)
{
	FTS_VoxelGenResult Result;

	// Calculate terrain height at this location
	float TerrainHeight = CalculateTerrainHeight(WorldX, WorldY);
	Result.Height = TerrainHeight;

	// Determine if voxel should be solid
	Result.bIsSolid = ShouldVoxelBeSolid(WorldX, WorldY, WorldZ, TerrainHeight);

	// Get material ID if solid
	if (Result.bIsSolid)
	{
		Result.MaterialID = GetVoxelMaterialID(WorldX, WorldY, WorldZ, TerrainHeight);

		// Get biome name for this location
		if (BiomeManager && WorldGenParameters.bEnableBiomes)
		{
			FTS_Biome Biome = BiomeManager->GetBiomeAtLocation(WorldX, WorldY, WorldZ);
			Result.BiomeName = Biome.BiomeName;
		}
		else
		{
			Result.BiomeName = TEXT("Default");
		}
	}
	else
	{
		Result.MaterialID = 0;
		Result.BiomeName = TEXT("Air");
	}

	return Result;
}

void UTS_WorldGenerator::SetWorldGenParameters(const FTS_WorldGenParameters& Parameters)
{
	WorldGenParameters = Parameters;
	
	// Update noise parameters with new seed
	InitializeNoiseParameters();
}

FTS_WorldGenParameters UTS_WorldGenerator::GetWorldGenParameters() const
{
	return WorldGenParameters;
}

UTS_BiomeManager* UTS_WorldGenerator::GetBiomeManager() const
{
	return BiomeManager;
}

UTS_ProceduralNoise* UTS_WorldGenerator::GetNoiseGenerator() const
{
	return NoiseGenerator;
}

void UTS_WorldGenerator::RegenerateWorld(int32 NewSeed)
{
	WorldGenParameters.WorldSeed = NewSeed;
	InitializeNoiseParameters();
}

bool UTS_WorldGenerator::ShouldVoxelBeSolid(float WorldX, float WorldY, float WorldZ, float TerrainHeight) const
{
	// Check if voxel is below terrain height
	if (WorldZ < TerrainHeight)
	{
		// Check for caves if enabled
		if (WorldGenParameters.bEnableCaves)
		{
			float CaveDensity = CalculateCaveDensity(WorldX, WorldY, WorldZ);
			
			// If cave density is above threshold, make voxel empty
			if (CaveDensity > WorldGenParameters.CaveThreshold)
			{
				return false;
			}
		}

		// Check height bounds
		if (WorldZ >= WorldGenParameters.MinHeight && WorldZ <= WorldGenParameters.MaxHeight)
		{
			return true;
		}
	}

	return false;
}

int32 UTS_WorldGenerator::GetVoxelMaterialID(float WorldX, float WorldY, float WorldZ, float TerrainHeight) const
{
	// Default material
	int32 MaterialID = 1;

	// Use biome system if enabled
	if (BiomeManager && WorldGenParameters.bEnableBiomes)
	{
		MaterialID = BiomeManager->GetMaterialIDAtLocation(WorldX, WorldY, WorldZ, TerrainHeight);
	}
	else
	{
		// Simple material assignment based on height
		if (WorldZ < TerrainHeight * 0.3f)
		{
			MaterialID = 3; // Stone (deep underground)
		}
		else if (WorldZ < TerrainHeight * 0.8f)
		{
			MaterialID = 2; // Dirt (middle layers)
		}
		else
		{
			MaterialID = 1; // Grass (surface)
		}
	}

	return MaterialID;
}

void UTS_WorldGenerator::InitializeNoiseParameters()
{
	// Terrain noise parameters
	TerrainNoiseParams.Seed = WorldGenParameters.WorldSeed;
	TerrainNoiseParams.Frequency = 0.005f;
	TerrainNoiseParams.Amplitude = WorldGenParameters.MaxHeight - WorldGenParameters.MinHeight;
	TerrainNoiseParams.Octaves = 4;
	TerrainNoiseParams.Persistence = 0.5f;
	TerrainNoiseParams.Lacunarity = 2.0f;
	TerrainNoiseParams.Offset = FVector::ZeroVector;

	// Cave noise parameters
	CaveNoiseParams.Seed = WorldGenParameters.WorldSeed + 1000; // Different seed for caves
	CaveNoiseParams.Frequency = 0.01f;
	CaveNoiseParams.Amplitude = 1.0f;
	CaveNoiseParams.Octaves = 3;
	CaveNoiseParams.Persistence = 0.6f;
	CaveNoiseParams.Lacunarity = 2.0f;
	CaveNoiseParams.Offset = FVector(500.0f, 500.0f, 500.0f);
}

float UTS_WorldGenerator::CalculateTerrainHeight(float X, float Y) const
{
	if (!NoiseGenerator)
	{
		return WorldGenParameters.BaseHeight;
	}

	// Use terrain noise parameters
	FTS_NoiseParameters HeightParams = TerrainNoiseParams;
	HeightParams.Offset = FVector(0.0f, 0.0f, 0.0f);

	float Height = UTS_ProceduralNoise::FractalNoise(X, Y, 0.0f, HeightParams);
	
	// Scale to world height range
	Height = FMath::GetMappedRangeValueClamped(
		FVector2D(-1.0f, 1.0f),
		FVector2D(WorldGenParameters.MinHeight, WorldGenParameters.MaxHeight),
		Height
	);

	// Add base height
	Height += WorldGenParameters.BaseHeight;

	return Height;
}

float UTS_WorldGenerator::CalculateCaveDensity(float X, float Y, float Z) const
{
	if (!NoiseGenerator || !WorldGenParameters.bEnableCaves)
	{
		return 0.0f; // No caves
	}

	// Use cave noise parameters
	return UTS_ProceduralNoise::GetCaveDensity(X, Y, Z, CaveNoiseParams);
}
