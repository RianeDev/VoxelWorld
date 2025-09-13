/**
 * @file TS_ChunkManager.h
 * @brief Simple chunk management for TerraScape MVP
 * @author Keves
 * @version 1.0
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProceduralMeshComponent.h"
#include "Async/AsyncWork.h"
#include "TS_VoxelTypes.h"
#include "TS_MaterialData.h"
#include "TS_WorldGenerator.h"
#include "TS_ChunkManager.generated.h"

/**
 * @brief Async task for generating chunk meshes
 */
class TERRA_SCAPE_API FTS_AsyncMeshGenerationTask : public FNonAbandonableTask
{
public:
	FTS_AsyncMeshGenerationTask(
		const FIntVector& InChunkID,
		const TArray<FTS_Voxel>& InVoxelData,
		int32 InChunkSize,
		float InVoxelSize,
		const FVector& InChunkWorldPos,
		UTS_MaterialManager* InMaterialManager,
		int32 InLODLevel = 0
	)
		: ChunkID(InChunkID)
		, VoxelData(InVoxelData)
		, ChunkSize(InChunkSize)
		, VoxelSize(InVoxelSize)
		, ChunkWorldPos(InChunkWorldPos)
		, MaterialManager(InMaterialManager)
		, LODLevel(InLODLevel)
	{
	}

	// FNonAbandonableTask interface
	void DoWork();
	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FTS_AsyncMeshGenerationTask, STATGROUP_ThreadPoolAsyncTasks);
	}

	// Results
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FColor> Colors;
	UMaterialInterface* MaterialInterface = nullptr;

private:
	FIntVector ChunkID;
	TArray<FTS_Voxel> VoxelData;
	int32 ChunkSize;
	float VoxelSize;
	FVector ChunkWorldPos;
	UTS_MaterialManager* MaterialManager;
	int32 LODLevel;

	void GenerateChunkMesh();
	FTS_Voxel GetVoxelAt(int32 X, int32 Y, int32 Z) const;
	bool IsVoxelSolid(int32 X, int32 Y, int32 Z) const;
};

/**
 * @brief Simple chunk manager for MVP - handles basic chunk creation and storage
 */
UCLASS(Blueprintable, meta=(BlueprintSpawnableComponent))
class TERRA_SCAPE_API UTS_ChunkManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UTS_ChunkManager();

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	/** Chunk size (32x32x32 voxels for streaming - balanced performance and memory) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | Chunk Settings")
	int32 ChunkSize = 32;

	/** Size of each voxel in world units (e.g., 100 = 100cm per voxel) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | Chunk Settings")
	float VoxelSize = 100.0f;

	/** Internal gap adjustment - automatically calculated to close gaps between chunks */
	float ChunkGap = 0.0f;

	/** Material manager for handling voxel materials */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | Materials")
	UTS_MaterialManager* MaterialManager;

	/** World generator for procedural terrain generation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | Procedural")
	UTS_WorldGenerator* WorldGenerator;

	/** Enable procedural generation instead of test voxels */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | Procedural")
	bool bUseProceduralGeneration = true;

	/** Material data table for voxel materials */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | Materials")
	UDataTable* MaterialDataTable;

	/** Async mesh generation tasks */
	TMap<FIntVector, FAsyncTask<FTS_AsyncMeshGenerationTask>*> AsyncMeshTasks;

	/** Queue of chunks waiting for mesh generation when async task limit is reached */
	TArray<FIntVector> PendingMeshGenerationQueue;

	/** Maximum number of concurrent async tasks to prevent thread pool exhaustion */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | Performance")
	int32 MaxConcurrentAsyncTasks = 8;

	/** LOD (Level of Detail) settings */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | LOD")
	bool bEnableLOD = true;

	/** Distance thresholds for LOD levels (in world units) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | LOD")
	float LOD0Distance = 2000.0f; // Full detail

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | LOD")
	float LOD1Distance = 4000.0f; // Reduced detail

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | LOD")
	float LOD2Distance = 8000.0f; // Minimal detail

	/** Player reference for distance calculations */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerraScape | LOD")
	AActor* PlayerReference;

	/** Simple Blueprint functions for MVP testing */
	UFUNCTION(BlueprintCallable, Category = "TerraScape | Chunks")
	void CreateChunk(const FIntVector& ChunkID);

	/** Check for completed async mesh generation tasks */
	void CheckAsyncMeshTasks();

	/** Update LOD for all chunks based on player distance */
	UFUNCTION(BlueprintCallable, Category = "TerraScape | LOD")
	void UpdateChunkLOD();

	/** Get LOD level for a chunk based on distance to player */
	UFUNCTION(BlueprintCallable, Category = "TerraScape | LOD")
	int32 GetChunkLODLevel(const FIntVector& ChunkID) const;

	/** Set player reference for LOD calculations */
	UFUNCTION(BlueprintCallable, Category = "TerraScape | LOD")
	void SetPlayerReference(AActor* Player);

	/** Calculate chunk world position (single source of truth) */
	FVector CalculateChunkWorldPosition(const FIntVector& ChunkID) const;

	/** Generate a grid of chunks around a center point */
	UFUNCTION(BlueprintCallable, Category = "TerraScape | Bulk Generation")
	void GenerateChunkGrid(const FIntVector& CenterChunk, int32 GridSize);

	/** Clear all chunks */
	UFUNCTION(BlueprintCallable, Category = "TerraScape | Bulk Generation")
	void ClearAllChunks();

	UFUNCTION(BlueprintCallable, Category = "TerraScape | Chunks")
	void DeleteChunk(const FIntVector& ChunkID);

	UFUNCTION(BlueprintCallable, Category = "TerraScape | Chunks")
	bool IsChunkLoaded(const FIntVector& ChunkID) const;

	UFUNCTION(BlueprintCallable, Category = "TerraScape | Chunks")
	int32 GetLoadedChunkCount() const;

private:
	/** Simple map to store loaded chunks */
	TMap<FIntVector, FTS_Chunk> LoadedChunks;

	/** Simple map to store voxel data per chunk */
	TMap<FIntVector, TArray<FTS_Voxel>> ChunkVoxelData;

	/** Map to store mesh components for each chunk */
	TMap<FIntVector, UProceduralMeshComponent*> ChunkMeshes;

	/** Map to store current LOD level for each chunk */
	TMap<FIntVector, int32> ChunkLODLevels;

	/** Generate simple test voxels for a chunk */
	TArray<FTS_Voxel> GenerateTestVoxels(const FIntVector& ChunkID);

	/** Generate procedural voxel data for a chunk */
	TArray<FTS_Voxel> GenerateProceduralVoxels(const FIntVector& ChunkID);

	/** Enable or disable procedural generation */
	void SetProceduralGenerationEnabled(bool bEnabled);

	/** Get world generator instance */
	UTS_WorldGenerator* GetWorldGenerator() const;

	/** Generate mesh for a chunk with face culling */
	void GenerateChunkMesh(const FIntVector& ChunkID);

	/** Get voxel at local position within chunk */
	FTS_Voxel GetVoxelAt(const FIntVector& ChunkID, int32 X, int32 Y, int32 Z) const;

	/** Check if voxel is solid (for face culling) */
	bool IsVoxelSolid(const FIntVector& ChunkID, int32 X, int32 Y, int32 Z) const;
};