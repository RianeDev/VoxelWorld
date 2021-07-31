// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "VoxelActor.generated.h"


struct FMeshSection
{
	TArray<FVector>Vertices;
	TArray<int32>Triangles;
	TArray<FVector>Normals;
	TArray<FVector2D>UVs;
	TArray<FProcMeshTangent>Tangents;
	TArray<FColor>VertexColor;

	int32 elementID = 0;
};

UCLASS()
class VOXELWORLD_API AVoxelActor : public AActor
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray <UMaterialInterface *> Materials;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true))
		int32 RandomSeed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true))
		int32 VoxelSize = 200;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true))
		int32 ChunkLineElements = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true))
		int32 ChunkXIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true))
		int32 ChunkYIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float xMult = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float yMult = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float zMult = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float weight = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float freq = 1;

	UPROPERTY()
		int32 ChunkTotalElements;

	UPROPERTY()
		int32 ChunkZElements;

	UPROPERTY()
		int32 ChunkLineElementsP2;

	UPROPERTY()
		int32 VoxelSizeHalf;

	UPROPERTY()
		TArray <int32> ChunkFields;

	UPROPERTY()
		UProceduralMeshComponent* proceduralComponent;

	UFUNCTION(BlueprintNativeEvent)
		TArray <int32> calculateNoise();
	virtual TArray <int32> calculateNoise_Implementation();

	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void SetVoxel(FVector localPos, int32 value);

	// Sets default values for this actor's properties
	AVoxelActor();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void OnConstruction(const FTransform & Transform) override;


	void GenerateChunks();

	void UpdateMesh();
};
