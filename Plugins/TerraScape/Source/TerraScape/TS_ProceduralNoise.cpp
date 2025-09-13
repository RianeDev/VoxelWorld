#include "TS_ProceduralNoise.h"
#include "Math/UnrealMathUtility.h"
#include "Math/Vector.h"

UTS_ProceduralNoise::UTS_ProceduralNoise()
{
	// Constructor
}

float UTS_ProceduralNoise::PerlinNoise(float X, float Y, float Z, const FTS_NoiseParameters& Parameters)
{
	// Apply frequency and offset
	float SampleX = (X + Parameters.Offset.X) * Parameters.Frequency;
	float SampleY = (Y + Parameters.Offset.Y) * Parameters.Frequency;
	float SampleZ = (Z + Parameters.Offset.Z) * Parameters.Frequency;

	// Get integer coordinates
	int32 X0 = FMath::FloorToInt(SampleX);
	int32 Y0 = FMath::FloorToInt(SampleY);
	int32 Z0 = FMath::FloorToInt(SampleZ);
	int32 X1 = X0 + 1;
	int32 Y1 = Y0 + 1;
	int32 Z1 = Z0 + 1;

	// Get fractional parts
	float XFrac = SampleX - X0;
	float YFrac = SampleY - Y0;
	float ZFrac = SampleZ - Z0;

	// Apply fade function for smooth interpolation
	float U = Fade(XFrac);
	float V = Fade(YFrac);
	float W = Fade(ZFrac);

	// Generate gradient vectors for each corner
	FVector Grad000 = FVector(
		FMath::FRandRange(-1.0f, 1.0f),
		FMath::FRandRange(-1.0f, 1.0f),
		FMath::FRandRange(-1.0f, 1.0f)
	).GetSafeNormal();

	FVector Grad001 = FVector(
		FMath::FRandRange(-1.0f, 1.0f),
		FMath::FRandRange(-1.0f, 1.0f),
		FMath::FRandRange(-1.0f, 1.0f)
	).GetSafeNormal();

	FVector Grad010 = FVector(
		FMath::FRandRange(-1.0f, 1.0f),
		FMath::FRandRange(-1.0f, 1.0f),
		FMath::FRandRange(-1.0f, 1.0f)
	).GetSafeNormal();

	FVector Grad011 = FVector(
		FMath::FRandRange(-1.0f, 1.0f),
		FMath::FRandRange(-1.0f, 1.0f),
		FMath::FRandRange(-1.0f, 1.0f)
	).GetSafeNormal();

	FVector Grad100 = FVector(
		FMath::FRandRange(-1.0f, 1.0f),
		FMath::FRandRange(-1.0f, 1.0f),
		FMath::FRandRange(-1.0f, 1.0f)
	).GetSafeNormal();

	FVector Grad101 = FVector(
		FMath::FRandRange(-1.0f, 1.0f),
		FMath::FRandRange(-1.0f, 1.0f),
		FMath::FRandRange(-1.0f, 1.0f)
	).GetSafeNormal();

	FVector Grad110 = FVector(
		FMath::FRandRange(-1.0f, 1.0f),
		FMath::FRandRange(-1.0f, 1.0f),
		FMath::FRandRange(-1.0f, 1.0f)
	).GetSafeNormal();

	FVector Grad111 = FVector(
		FMath::FRandRange(-1.0f, 1.0f),
		FMath::FRandRange(-1.0f, 1.0f),
		FMath::FRandRange(-1.0f, 1.0f)
	).GetSafeNormal();

	// Calculate dot products
	float Dot000 = FVector::DotProduct(Grad000, FVector(XFrac, YFrac, ZFrac));
	float Dot001 = FVector::DotProduct(Grad001, FVector(XFrac, YFrac, ZFrac - 1.0f));
	float Dot010 = FVector::DotProduct(Grad010, FVector(XFrac, YFrac - 1.0f, ZFrac));
	float Dot011 = FVector::DotProduct(Grad011, FVector(XFrac, YFrac - 1.0f, ZFrac - 1.0f));
	float Dot100 = FVector::DotProduct(Grad100, FVector(XFrac - 1.0f, YFrac, ZFrac));
	float Dot101 = FVector::DotProduct(Grad101, FVector(XFrac - 1.0f, YFrac, ZFrac - 1.0f));
	float Dot110 = FVector::DotProduct(Grad110, FVector(XFrac - 1.0f, YFrac - 1.0f, ZFrac));
	float Dot111 = FVector::DotProduct(Grad111, FVector(XFrac - 1.0f, YFrac - 1.0f, ZFrac - 1.0f));

	// Trilinear interpolation
	float X00 = Lerp(Dot000, Dot100, U);
	float X01 = Lerp(Dot001, Dot101, U);
	float X10 = Lerp(Dot010, Dot110, U);
	float X11 = Lerp(Dot011, Dot111, U);

	float Y0_ = Lerp(X00, X10, V);
	float Y1_ = Lerp(X01, X11, V);

	return Lerp(Y0_, Y1_, W);
}

float UTS_ProceduralNoise::SimplexNoise(float X, float Y, float Z, const FTS_NoiseParameters& Parameters)
{
	// Apply frequency and offset
	float SampleX = (X + Parameters.Offset.X) * Parameters.Frequency;
	float SampleY = (Y + Parameters.Offset.Y) * Parameters.Frequency;
	float SampleZ = (Z + Parameters.Offset.Z) * Parameters.Frequency;

	// Skewing factors for 3D simplex
	const float F3 = 1.0f / 3.0f;
	const float G3 = 1.0f / 6.0f;

	// Skew input space to determine which simplex cell we're in
	float S = (SampleX + SampleY + SampleZ) * F3;
	int32 I = FMath::FloorToInt(SampleX + S);
	int32 J = FMath::FloorToInt(SampleY + S);
	int32 K = FMath::FloorToInt(SampleZ + S);

	float T = (I + J + K) * G3;
	float X0 = I - T;
	float Y0 = J - T;
	float Z0 = K - T;
	float X0_ = SampleX - X0;
	float Y0_ = SampleY - Y0;
	float Z0_ = SampleZ - Z0;

	// Determine which simplex we are in
	int32 I1, J1, K1;
	int32 I2, J2, K2;

	if (X0_ >= Y0_)
	{
		if (Y0_ >= Z0_)
		{
			I1 = 1; J1 = 0; K1 = 0;
			I2 = 1; J2 = 1; K2 = 0;
		}
		else if (X0_ >= Z0_)
		{
			I1 = 1; J1 = 0; K1 = 0;
			I2 = 1; J2 = 0; K2 = 1;
		}
		else
		{
			I1 = 0; J1 = 0; K1 = 1;
			I2 = 1; J2 = 0; K2 = 1;
		}
	}
	else
	{
		if (Y0_ < Z0_)
		{
			I1 = 0; J1 = 0; K1 = 1;
			I2 = 0; J2 = 1; K2 = 1;
		}
		else if (X0_ < Z0_)
		{
			I1 = 0; J1 = 1; K1 = 0;
			I2 = 0; J2 = 1; K2 = 1;
		}
		else
		{
			I1 = 0; J1 = 1; K1 = 0;
			I2 = 1; J2 = 1; K2 = 0;
		}
	}

	// Calculate noise contribution from each corner
	float T0 = 0.6f - X0_ * X0_ - Y0_ * Y0_ - Z0_ * Z0_;
	float N0 = 0.0f;
	if (T0 >= 0.0f)
	{
		T0 *= T0;
		N0 = T0 * T0 * FMath::FRandRange(-1.0f, 1.0f);
	}

	float T1 = 0.6f - (X0_ - I1) * (X0_ - I1) - (Y0_ - J1) * (Y0_ - J1) - (Z0_ - K1) * (Z0_ - K1);
	float N1 = 0.0f;
	if (T1 >= 0.0f)
	{
		T1 *= T1;
		N1 = T1 * T1 * FMath::FRandRange(-1.0f, 1.0f);
	}

	float T2 = 0.6f - (X0_ - I2) * (X0_ - I2) - (Y0_ - J2) * (Y0_ - J2) - (Z0_ - K2) * (Z0_ - K2);
	float N2 = 0.0f;
	if (T2 >= 0.0f)
	{
		T2 *= T2;
		N2 = T2 * T2 * FMath::FRandRange(-1.0f, 1.0f);
	}

	return 32.0f * (N0 + N1 + N2);
}

float UTS_ProceduralNoise::FractalNoise(float X, float Y, float Z, const FTS_NoiseParameters& Parameters)
{
	float Value = 0.0f;
	float Amplitude = Parameters.Amplitude;
	float Frequency = Parameters.Frequency;
	float MaxValue = 0.0f;

	for (int32 i = 0; i < Parameters.Octaves; i++)
	{
		FTS_NoiseParameters OctaveParams = Parameters;
		OctaveParams.Frequency = Frequency;
		OctaveParams.Amplitude = Amplitude;

		Value += PerlinNoise(X, Y, Z, OctaveParams) * Amplitude;
		MaxValue += Amplitude;

		Amplitude *= Parameters.Persistence;
		Frequency *= Parameters.Lacunarity;
	}

	return Value / MaxValue;
}

float UTS_ProceduralNoise::CombineNoiseLayers(float X, float Y, float Z, const TArray<FTS_NoiseLayer>& Layers)
{
	float TotalValue = 0.0f;
	float TotalWeight = 0.0f;

	for (const FTS_NoiseLayer& Layer : Layers)
	{
		if (Layer.bEnabled)
		{
			float LayerValue = FractalNoise(X, Y, Z, Layer.Parameters);
			TotalValue += LayerValue * Layer.Weight;
			TotalWeight += Layer.Weight;
		}
	}

	return TotalWeight > 0.0f ? TotalValue / TotalWeight : 0.0f;
}

float UTS_ProceduralNoise::GetTerrainHeight(float X, float Y, const FTS_NoiseParameters& Parameters)
{
	// Use 2D noise for terrain height
	float Height = FractalNoise(X, Y, 0.0f, Parameters);
	
	// Apply amplitude scaling
	Height *= Parameters.Amplitude;
	
	// Add base height
	Height += 100.0f; // Base terrain height
	
	return Height;
}

float UTS_ProceduralNoise::GetCaveDensity(float X, float Y, float Z, const FTS_NoiseParameters& Parameters)
{
	// Use 3D noise for cave generation
	float CaveNoise = FractalNoise(X, Y, Z, Parameters);
	
	// Convert to density (0.0 = solid, 1.0 = empty)
	float Density = (CaveNoise + 1.0f) * 0.5f;
	
	// Apply threshold for cave generation
	float Threshold = 0.3f; // Adjust for cave frequency
	return Density > Threshold ? 1.0f : 0.0f;
}

uint32 UTS_ProceduralNoise::Hash(uint32 Input)
{
	// Simple hash function for pseudo-random number generation
	Input ^= Input >> 16;
	Input *= 0x85ebca6b;
	Input ^= Input >> 13;
	Input *= 0xc2b2ae35;
	Input ^= Input >> 16;
	return Input;
}

float UTS_ProceduralNoise::Lerp(float A, float B, float T)
{
	return A + T * (B - A);
}

float UTS_ProceduralNoise::SmoothStep(float T)
{
	return T * T * (3.0f - 2.0f * T);
}

float UTS_ProceduralNoise::Fade(float T)
{
	return T * T * T * (T * (T * 6.0f - 15.0f) + 10.0f);
}
