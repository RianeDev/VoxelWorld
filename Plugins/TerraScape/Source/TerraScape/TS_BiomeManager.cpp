#include "TS_BiomeManager.h"
#include "TS_ProceduralNoise.h"
#include "Math/UnrealMathUtility.h"

UTS_BiomeManager::UTS_BiomeManager()
{
	// Initialize default biomes
	InitializeDefaultBiomes();
}

void UTS_BiomeManager::InitializeDefaultBiomes()
{
	Biomes.Empty();

	// Plains Biome
	FTS_Biome PlainsBiome;
	PlainsBiome.BiomeName = TEXT("Plains");
	PlainsBiome.HeightRange = FVector2D(50.0f, 200.0f);
	PlainsBiome.MoistureRange = FVector2D(0.3f, 0.7f);
	PlainsBiome.TemperatureRange = FVector2D(0.4f, 0.8f);
	PlainsBiome.PrimaryMaterialID = 1; // Grass
	PlainsBiome.SecondaryMaterialID = 2; // Dirt
	PlainsBiome.Weight = 1.0f;
	PlainsBiome.bEnabled = true;
	Biomes.Add(PlainsBiome);

	// Mountains Biome
	FTS_Biome MountainsBiome;
	MountainsBiome.BiomeName = TEXT("Mountains");
	MountainsBiome.HeightRange = FVector2D(200.0f, 500.0f);
	MountainsBiome.MoistureRange = FVector2D(0.0f, 0.5f);
	MountainsBiome.TemperatureRange = FVector2D(0.0f, 0.6f);
	MountainsBiome.PrimaryMaterialID = 3; // Stone
	MountainsBiome.SecondaryMaterialID = 4; // Rock
	MountainsBiome.Weight = 0.8f;
	MountainsBiome.bEnabled = true;
	Biomes.Add(MountainsBiome);

	// Desert Biome
	FTS_Biome DesertBiome;
	DesertBiome.BiomeName = TEXT("Desert");
	DesertBiome.HeightRange = FVector2D(0.0f, 150.0f);
	DesertBiome.MoistureRange = FVector2D(0.0f, 0.2f);
	DesertBiome.TemperatureRange = FVector2D(0.7f, 1.0f);
	DesertBiome.PrimaryMaterialID = 5; // Sand
	DesertBiome.SecondaryMaterialID = 6; // Sandstone
	DesertBiome.Weight = 0.6f;
	DesertBiome.bEnabled = true;
	Biomes.Add(DesertBiome);

	// Forest Biome
	FTS_Biome ForestBiome;
	ForestBiome.BiomeName = TEXT("Forest");
	ForestBiome.HeightRange = FVector2D(100.0f, 250.0f);
	ForestBiome.MoistureRange = FVector2D(0.6f, 1.0f);
	ForestBiome.TemperatureRange = FVector2D(0.3f, 0.7f);
	ForestBiome.PrimaryMaterialID = 7; // Wood
	ForestBiome.SecondaryMaterialID = 8; // Leaves
	ForestBiome.Weight = 0.9f;
	ForestBiome.bEnabled = true;
	Biomes.Add(ForestBiome);

	// Ocean Biome
	FTS_Biome OceanBiome;
	OceanBiome.BiomeName = TEXT("Ocean");
	OceanBiome.HeightRange = FVector2D(-500.0f, 0.0f);
	OceanBiome.MoistureRange = FVector2D(0.8f, 1.0f);
	OceanBiome.TemperatureRange = FVector2D(0.2f, 0.8f);
	OceanBiome.PrimaryMaterialID = 9; // Water
	OceanBiome.SecondaryMaterialID = 10; // Sand
	OceanBiome.Weight = 0.7f;
	OceanBiome.bEnabled = true;
	Biomes.Add(OceanBiome);
}

FTS_Biome UTS_BiomeManager::GetBiomeAtLocation(float X, float Y, float Z) const
{
	float Height, Moisture, Temperature;
	CalculateEnvironmentalFactors(X, Y, Z, Height, Moisture, Temperature);
	
	return FindBestBiome(Height, Moisture, Temperature);
}

FTS_BiomeBlend UTS_BiomeManager::GetBiomeBlendAtLocation(float X, float Y, float Z) const
{
	FTS_BiomeBlend Result;
	
	// Get primary biome
	Result.PrimaryBiome = GetBiomeAtLocation(X, Y, Z);
	
	// Calculate transition blend
	CalculateBiomeTransition(X, Y, Z, Result.PrimaryBiome, Result.SecondaryBiome, Result.BlendFactor);
	
	return Result;
}

int32 UTS_BiomeManager::GetMaterialIDAtLocation(float X, float Y, float Z, float Height) const
{
	FTS_Biome Biome = GetBiomeAtLocation(X, Y, Z);
	
	// Simple material selection based on height within biome
	float HeightNormalized = FMath::GetMappedRangeValueClamped(
		Biome.HeightRange,
		FVector2D(0.0f, 1.0f),
		Height
	);
	
	// Use primary material for lower heights, secondary for higher
	return HeightNormalized < 0.5f ? Biome.PrimaryMaterialID : Biome.SecondaryMaterialID;
}

void UTS_BiomeManager::AddBiome(const FTS_Biome& Biome)
{
	Biomes.Add(Biome);
}

void UTS_BiomeManager::RemoveBiome(const FString& BiomeName)
{
	for (int32 i = Biomes.Num() - 1; i >= 0; i--)
	{
		if (Biomes[i].BiomeName == BiomeName)
		{
			Biomes.RemoveAt(i);
			break;
		}
	}
}

TArray<FTS_Biome> UTS_BiomeManager::GetAllBiomes() const
{
	return Biomes;
}

void UTS_BiomeManager::CalculateEnvironmentalFactors(float X, float Y, float Z, float& OutHeight, float& OutMoisture, float& OutTemperature) const
{
	// Height calculation using terrain noise
	FTS_NoiseParameters HeightParams;
	HeightParams.Frequency = 0.005f;
	HeightParams.Amplitude = 200.0f;
	HeightParams.Octaves = 4;
	HeightParams.Persistence = 0.5f;
	HeightParams.Lacunarity = 2.0f;
	HeightParams.Seed = 12345;
	
	OutHeight = UTS_ProceduralNoise::GetTerrainHeight(X, Y, HeightParams);
	
	// Moisture calculation using different noise
	FTS_NoiseParameters MoistureParams;
	MoistureParams.Frequency = 0.003f;
	MoistureParams.Amplitude = 1.0f;
	MoistureParams.Octaves = 3;
	MoistureParams.Persistence = 0.6f;
	MoistureParams.Lacunarity = 2.0f;
	MoistureParams.Seed = 54321;
	MoistureParams.Offset = FVector(1000.0f, 1000.0f, 0.0f);
	
	OutMoisture = (UTS_ProceduralNoise::FractalNoise(X, Y, Z, MoistureParams) + 1.0f) * 0.5f;
	OutMoisture = FMath::Clamp(OutMoisture, 0.0f, 1.0f);
	
	// Temperature calculation using another noise layer
	FTS_NoiseParameters TemperatureParams;
	TemperatureParams.Frequency = 0.002f;
	TemperatureParams.Amplitude = 1.0f;
	TemperatureParams.Octaves = 3;
	TemperatureParams.Persistence = 0.7f;
	TemperatureParams.Lacunarity = 2.0f;
	TemperatureParams.Seed = 98765;
	TemperatureParams.Offset = FVector(-500.0f, -500.0f, 0.0f);
	
	OutTemperature = (UTS_ProceduralNoise::FractalNoise(X, Y, Z, TemperatureParams) + 1.0f) * 0.5f;
	OutTemperature = FMath::Clamp(OutTemperature, 0.0f, 1.0f);
}

FTS_Biome UTS_BiomeManager::FindBestBiome(float Height, float Moisture, float Temperature) const
{
	FTS_Biome BestBiome;
	float BestScore = -1.0f;
	
	for (const FTS_Biome& Biome : Biomes)
	{
		if (!Biome.bEnabled)
		{
			continue;
		}
		
		// Calculate match score for this biome
		float HeightScore = 0.0f;
		if (Height >= Biome.HeightRange.X && Height <= Biome.HeightRange.Y)
		{
			HeightScore = 1.0f; // Perfect match
		}
		else
		{
			// Calculate distance from range
			float Distance = FMath::Min(
				FMath::Abs(Height - Biome.HeightRange.X),
				FMath::Abs(Height - Biome.HeightRange.Y)
			);
			HeightScore = FMath::Max(0.0f, 1.0f - (Distance / 100.0f)); // Falloff over 100 units
		}
		
		float MoistureScore = 0.0f;
		if (Moisture >= Biome.MoistureRange.X && Moisture <= Biome.MoistureRange.Y)
		{
			MoistureScore = 1.0f;
		}
		else
		{
			float Distance = FMath::Min(
				FMath::Abs(Moisture - Biome.MoistureRange.X),
				FMath::Abs(Moisture - Biome.MoistureRange.Y)
			);
			MoistureScore = FMath::Max(0.0f, 1.0f - (Distance / 0.2f)); // Falloff over 0.2
		}
		
		float TemperatureScore = 0.0f;
		if (Temperature >= Biome.TemperatureRange.X && Temperature <= Biome.TemperatureRange.Y)
		{
			TemperatureScore = 1.0f;
		}
		else
		{
			float Distance = FMath::Min(
				FMath::Abs(Temperature - Biome.TemperatureRange.X),
				FMath::Abs(Temperature - Biome.TemperatureRange.Y)
			);
			TemperatureScore = FMath::Max(0.0f, 1.0f - (Distance / 0.2f)); // Falloff over 0.2
		}
		
		// Combined score with weights
		float TotalScore = (HeightScore * 0.5f + MoistureScore * 0.25f + TemperatureScore * 0.25f) * Biome.Weight;
		
		if (TotalScore > BestScore)
		{
			BestScore = TotalScore;
			BestBiome = Biome;
		}
	}
	
	return BestBiome;
}

void UTS_BiomeManager::CalculateBiomeTransition(float X, float Y, float Z, const FTS_Biome& PrimaryBiome, FTS_Biome& OutSecondaryBiome, float& OutBlendFactor) const
{
	// Simple transition calculation based on distance from biome centers
	// In a more advanced system, this would use Voronoi diagrams or similar
	
	OutSecondaryBiome = PrimaryBiome; // Default to same biome
	OutBlendFactor = 0.0f;
	
	// Find second best biome
	float BestScore = -1.0f;
	for (const FTS_Biome& Biome : Biomes)
	{
		if (!Biome.bEnabled || Biome.BiomeName == PrimaryBiome.BiomeName)
		{
			continue;
		}
		
		float Height, Moisture, Temperature;
		CalculateEnvironmentalFactors(X, Y, Z, Height, Moisture, Temperature);
		
		// Calculate score for this biome
		float HeightScore = 0.0f;
		if (Height >= Biome.HeightRange.X && Height <= Biome.HeightRange.Y)
		{
			HeightScore = 1.0f;
		}
		else
		{
			float Distance = FMath::Min(
				FMath::Abs(Height - Biome.HeightRange.X),
				FMath::Abs(Height - Biome.HeightRange.Y)
			);
			HeightScore = FMath::Max(0.0f, 1.0f - (Distance / 100.0f));
		}
		
		float MoistureScore = 0.0f;
		if (Moisture >= Biome.MoistureRange.X && Moisture <= Biome.MoistureRange.Y)
		{
			MoistureScore = 1.0f;
		}
		else
		{
			float Distance = FMath::Min(
				FMath::Abs(Moisture - Biome.MoistureRange.X),
				FMath::Abs(Moisture - Biome.MoistureRange.Y)
			);
			MoistureScore = FMath::Max(0.0f, 1.0f - (Distance / 0.2f));
		}
		
		float TemperatureScore = 0.0f;
		if (Temperature >= Biome.TemperatureRange.X && Temperature <= Biome.TemperatureRange.Y)
		{
			TemperatureScore = 1.0f;
		}
		else
		{
			float Distance = FMath::Min(
				FMath::Abs(Temperature - Biome.TemperatureRange.X),
				FMath::Abs(Temperature - Biome.TemperatureRange.Y)
			);
			TemperatureScore = FMath::Max(0.0f, 1.0f - (Distance / 0.2f));
		}
		
		float TotalScore = (HeightScore * 0.5f + MoistureScore * 0.25f + TemperatureScore * 0.25f) * Biome.Weight;
		
		if (TotalScore > BestScore)
		{
			BestScore = TotalScore;
			OutSecondaryBiome = Biome;
		}
	}
	
	// Calculate blend factor based on how close the scores are
	// If scores are very close, blend more
	OutBlendFactor = FMath::Clamp(BestScore * 0.3f, 0.0f, 0.5f);
}
