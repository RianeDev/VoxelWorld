/**
 * @file TS_ChunkManager.cpp
 * @brief Simple chunk management implementation for TerraScape MVP
 * @author Keves
 * @version 1.0
 */

#include "TS_ChunkManager.h"
#include "Engine/Engine.h"
#include "ProceduralMeshComponent.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TS_MaterialData.h"
#include "TS_WorldGenerator.h"
#include "Async/AsyncWork.h"

UTS_ChunkManager::UTS_ChunkManager()
{
	// Enable ticking to check for completed async tasks
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f; // Check every 100ms
	
	// Initialize properties (required for UPROPERTY)
	ChunkSize = 32;
	VoxelSize = 100.0f; // 100 units per voxel
	MaxConcurrentAsyncTasks = 8;
	
	// Automatically set ChunkGap to half chunk size to close gaps
	ChunkGap = -(ChunkSize * VoxelSize) / 2.0f; // -1600 for default values
	
	// Initialize LOD settings
	bEnableLOD = true;
	LOD0Distance = 2000.0f; // Full detail
	LOD1Distance = 4000.0f; // Reduced detail
	LOD2Distance = 8000.0f; // Minimal detail
	PlayerReference = nullptr;
	
	// Create material manager
	MaterialManager = CreateDefaultSubobject<UTS_MaterialManager>(TEXT("MaterialManager"));

	// Create world generator
	WorldGenerator = CreateDefaultSubobject<UTS_WorldGenerator>(TEXT("WorldGenerator"));
}

void UTS_ChunkManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Check for completed async mesh generation tasks
	CheckAsyncMeshTasks();
	
	// Update LOD for chunks (less frequently to avoid performance impact)
	static float LODUpdateTimer = 0.0f;
	LODUpdateTimer += DeltaTime;
	if (LODUpdateTimer >= 0.5f) // Update LOD every 500ms
	{
		UpdateChunkLOD();
		LODUpdateTimer = 0.0f;
	}
}

void UTS_ChunkManager::CreateChunk(const FIntVector& ChunkID)
{
	// Don't create if already exists
	if (LoadedChunks.Contains(ChunkID))
	{
		UE_LOG(LogTemp, Warning, TEXT("Chunk %s already exists"), *ChunkID.ToString());
		return;
	}

	// Create chunk data
	FTS_Chunk NewChunk;
	NewChunk.ChunkID = ChunkID;
	// Calculate chunk world position using single source of truth
	NewChunk.WorldPosition = CalculateChunkWorldPosition(ChunkID);
	NewChunk.bIsLoaded = true;

	// Generate voxels (procedural or test)
	TArray<FTS_Voxel> VoxelData = bUseProceduralGeneration ? GenerateProceduralVoxels(ChunkID) : GenerateTestVoxels(ChunkID);

	// Store the chunk and its data
	LoadedChunks.Add(ChunkID, NewChunk);
	ChunkVoxelData.Add(ChunkID, VoxelData);

	// Create mesh component
	UProceduralMeshComponent* MeshComp = NewObject<UProceduralMeshComponent>(this);
	MeshComp->RegisterComponent();
	MeshComp->AttachToComponent(GetOwner()->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
	MeshComp->SetWorldLocation(NewChunk.WorldPosition);
	
	ChunkMeshes.Add(ChunkID, MeshComp);
	
	UE_LOG(LogTemp, Log, TEXT("Created chunk %s at position %s"), 
		*ChunkID.ToString(), *NewChunk.WorldPosition.ToString());
	
	// Check if we're at the limit of concurrent async tasks
	if (AsyncMeshTasks.Num() >= MaxConcurrentAsyncTasks)
	{
		UE_LOG(LogTemp, Warning, TEXT("Too many concurrent async tasks (%d/%d), adding chunk %s to queue"), 
			AsyncMeshTasks.Num(), MaxConcurrentAsyncTasks, *ChunkID.ToString());
		// Add to queue for later mesh generation
		PendingMeshGenerationQueue.Add(ChunkID);
		return;
	}
	
	// Debug: Check voxel data before async generation
	int32 SolidVoxels = 0;
	for (const FTS_Voxel& Voxel : VoxelData)
	{
		if (Voxel.IsSolid())
		{
			SolidVoxels++;
		}
	}
	UE_LOG(LogTemp, Log, TEXT("Chunk %s: %d solid voxels out of %d total, VoxelSize=%.1f, ChunkSize=%d"), 
		*ChunkID.ToString(), SolidVoxels, VoxelData.Num(), VoxelSize, ChunkSize);

	// Ensure MaterialManager is initialized before starting async task
	if (MaterialManager && MaterialDataTable && !MaterialManager->IsInitialized())
	{
		MaterialManager->InitializeMaterialDataTable(MaterialDataTable);
		UE_LOG(LogTemp, Log, TEXT("TerraScape: Material data table initialized for async task"));
	}

	// Get LOD level for this chunk
	int32 LODLevel = GetChunkLODLevel(ChunkID);
	
	
	// Start async mesh generation
	FAsyncTask<FTS_AsyncMeshGenerationTask>* AsyncTask = new FAsyncTask<FTS_AsyncMeshGenerationTask>(
		ChunkID, VoxelData, ChunkSize, VoxelSize, NewChunk.WorldPosition, MaterialManager, LODLevel);
	
	AsyncTask->StartBackgroundTask();
	AsyncMeshTasks.Add(ChunkID, AsyncTask);
	
	UE_LOG(LogTemp, Log, TEXT("Started async mesh generation for chunk %s (%d/%d tasks)"), 
		*ChunkID.ToString(), AsyncMeshTasks.Num(), MaxConcurrentAsyncTasks);

	UE_LOG(LogTemp, Log, TEXT("Created chunk %s at world position %s (mesh component positioned at: %s)"), 
		*ChunkID.ToString(), *NewChunk.WorldPosition.ToString(), *MeshComp->GetComponentLocation().ToString());
}

void UTS_ChunkManager::DeleteChunk(const FIntVector& ChunkID)
{
	if (!LoadedChunks.Contains(ChunkID))
	{
		UE_LOG(LogTemp, Warning, TEXT("Chunk %s does not exist"), *ChunkID.ToString());
		return;
	}

	// Cancel any pending async mesh generation task
	if (FAsyncTask<FTS_AsyncMeshGenerationTask>* AsyncTask = AsyncMeshTasks.FindRef(ChunkID))
	{
		AsyncTask->Cancel();
		delete AsyncTask;
		AsyncMeshTasks.Remove(ChunkID);
	}

	// Remove mesh component if it exists
	if (ChunkMeshes.Contains(ChunkID))
	{
		UProceduralMeshComponent* MeshComp = ChunkMeshes[ChunkID];
		if (MeshComp)
		{
			MeshComp->DestroyComponent();
		}
		ChunkMeshes.Remove(ChunkID);
	}

	// Remove chunk and its data
	LoadedChunks.Remove(ChunkID);
	ChunkVoxelData.Remove(ChunkID);

	UE_LOG(LogTemp, Log, TEXT("Deleted chunk %s"), *ChunkID.ToString());
}

bool UTS_ChunkManager::IsChunkLoaded(const FIntVector& ChunkID) const
{
	return LoadedChunks.Contains(ChunkID);
}

int32 UTS_ChunkManager::GetLoadedChunkCount() const
{
	return LoadedChunks.Num();
}

TArray<FTS_Voxel> UTS_ChunkManager::GenerateTestVoxels(const FIntVector& ChunkID)
{
	// Create simple test pattern for MVP - Minecraft-style terrain
	TArray<FTS_Voxel> VoxelData;
	const int32 TotalVoxels = ChunkSize * ChunkSize * ChunkSize;
	VoxelData.Reserve(TotalVoxels);

	for (int32 Z = 0; Z < ChunkSize; Z++)
	{
		for (int32 Y = 0; Y < ChunkSize; Y++)
		{
			for (int32 X = 0; X < ChunkSize; X++)
			{
				FTS_Voxel NewVoxel;
				
				// Calculate world position of this voxel
				FVector WorldPos = FVector(
					ChunkID.X * ChunkSize + X,
					ChunkID.Y * ChunkSize + Y,
					ChunkID.Z * ChunkSize + Z
				) * VoxelSize;
				
				// Create continuous terrain - simple ground plane at Z=0 with some thickness
				float GroundHeight = 0.0f; // Ground level
				float TerrainThickness = 4.0f * VoxelSize; // 4 voxels thick (400 units)
				
				// Simple continuous terrain - if we're at or below ground level, make it solid
				if (WorldPos.Z <= GroundHeight && WorldPos.Z > (GroundHeight - TerrainThickness))
				{
					NewVoxel.MaterialID = 1; // Solid (ground)
				}
				else
				{
					NewVoxel.MaterialID = 0; // Air (sky)
				}

				VoxelData.Add(NewVoxel);
			}
		}
	}

	return VoxelData;
}

void UTS_ChunkManager::GenerateChunkMesh(const FIntVector& ChunkID)
{
	if (!ChunkVoxelData.Contains(ChunkID))
	{
		return;
	}

	// Create or get existing mesh component
	UProceduralMeshComponent* MeshComp = nullptr;
	if (ChunkMeshes.Contains(ChunkID))
	{
		MeshComp = ChunkMeshes[ChunkID];
	}
	else
	{
		// Create new mesh component
		MeshComp = NewObject<UProceduralMeshComponent>(GetOwner());
		MeshComp->RegisterComponent();
		ChunkMeshes.Add(ChunkID, MeshComp);
	}

	// Generate mesh data with face culling
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FColor> Colors;

	const TArray<FTS_Voxel>& VoxelData = ChunkVoxelData[ChunkID];
	// Calculate chunk world position using single source of truth
	const FVector ChunkWorldPos = CalculateChunkWorldPosition(ChunkID);
	
	// Calculate LOD step size (for compatibility - this function uses LOD 0 by default)
	int32 LODStep = 1;
	float LODVoxelSize = VoxelSize * LODStep;

	// Face directions for culling (these are normalized directions, not LOD-scaled)
	const FVector FaceDirections[6] = {
		FVector(1, 0, 0),   // Right
		FVector(-1, 0, 0),  // Left
		FVector(0, 1, 0),   // Forward
		FVector(0, -1, 0),  // Back
		FVector(0, 0, 1),   // Up
		FVector(0, 0, -1)   // Down
	};

	const FVector FaceNormals[6] = {
		FVector(1, 0, 0),
		FVector(-1, 0, 0),
		FVector(0, 1, 0),
		FVector(0, -1, 0),
		FVector(0, 0, 1),
		FVector(0, 0, -1)
	};

	// Generate faces only for visible voxels
	for (int32 Z = 0; Z < ChunkSize; Z++)
	{
		for (int32 Y = 0; Y < ChunkSize; Y++)
		{
			for (int32 X = 0; X < ChunkSize; X++)
			{
				// Only generate faces for solid voxels
				if (!IsVoxelSolid(ChunkID, X, Y, Z))
				{
					continue;
				}

				// Check each face direction
				for (int32 FaceIndex = 0; FaceIndex < 6; FaceIndex++)
				{
					FVector NeighborPos = FVector(X, Y, Z) + FaceDirections[FaceIndex];
					
					// Check if neighbor is air (outside chunk or empty)
					bool bNeighborIsAir = false;
					if (NeighborPos.X < 0 || NeighborPos.X >= ChunkSize ||
						NeighborPos.Y < 0 || NeighborPos.Y >= ChunkSize ||
						NeighborPos.Z < 0 || NeighborPos.Z >= ChunkSize)
					{
						// Outside chunk bounds - treat as air
						bNeighborIsAir = true;
					}
					else
					{
						// Check if neighbor voxel is air
						bNeighborIsAir = !IsVoxelSolid(ChunkID, NeighborPos.X, NeighborPos.Y, NeighborPos.Z);
					}

					// Only generate face if neighbor is air (face culling)
					if (bNeighborIsAir)
					{
						// Generate quad for this face
						FVector BasePos = ChunkWorldPos + FVector(X * VoxelSize, Y * VoxelSize, Z * VoxelSize);
						FVector Normal = FaceNormals[FaceIndex];
						
						// Define quad vertices based on face direction (consistent counter-clockwise order)
						TArray<FVector> QuadVertices;
						QuadVertices.SetNum(4);
						
						if (FaceIndex == 0) // Right
						{
							QuadVertices[0] = BasePos + FVector(VoxelSize, 0, 0);
							QuadVertices[1] = BasePos + FVector(VoxelSize, 0, VoxelSize);
							QuadVertices[2] = BasePos + FVector(VoxelSize, VoxelSize, VoxelSize);
							QuadVertices[3] = BasePos + FVector(VoxelSize, VoxelSize, 0);
						}
						else if (FaceIndex == 1) // Left
						{
							QuadVertices[0] = BasePos + FVector(0, VoxelSize, 0);
							QuadVertices[1] = BasePos + FVector(0, VoxelSize, VoxelSize);
							QuadVertices[2] = BasePos + FVector(0, 0, VoxelSize);
							QuadVertices[3] = BasePos + FVector(0, 0, 0);
						}
						else if (FaceIndex == 2) // Forward
						{
							QuadVertices[0] = BasePos + FVector(0, VoxelSize, 0);
							QuadVertices[1] = BasePos + FVector(VoxelSize, VoxelSize, 0);
							QuadVertices[2] = BasePos + FVector(VoxelSize, VoxelSize, VoxelSize);
							QuadVertices[3] = BasePos + FVector(0, VoxelSize, VoxelSize);
						}
						else if (FaceIndex == 3) // Back
						{
							QuadVertices[0] = BasePos + FVector(VoxelSize, 0, 0);
							QuadVertices[1] = BasePos + FVector(0, 0, 0);
							QuadVertices[2] = BasePos + FVector(0, 0, VoxelSize);
							QuadVertices[3] = BasePos + FVector(VoxelSize, 0, VoxelSize);
						}
						else if (FaceIndex == 4) // Up
						{
							QuadVertices[0] = BasePos + FVector(0, 0, VoxelSize);
							QuadVertices[1] = BasePos + FVector(0, VoxelSize, VoxelSize);
							QuadVertices[2] = BasePos + FVector(VoxelSize, VoxelSize, VoxelSize);
							QuadVertices[3] = BasePos + FVector(VoxelSize, 0, VoxelSize);
						}
						else if (FaceIndex == 5) // Down
						{
							QuadVertices[0] = BasePos + FVector(0, 0, 0);
							QuadVertices[1] = BasePos + FVector(VoxelSize, 0, 0);
							QuadVertices[2] = BasePos + FVector(VoxelSize, VoxelSize, 0);
							QuadVertices[3] = BasePos + FVector(0, VoxelSize, 0);
						}

						// Add vertices
						int32 StartIndex = Vertices.Num();
						Vertices.Append(QuadVertices);
						
						// Add triangles (two triangles per quad) - counter-clockwise winding for outward normals
						Triangles.Add(StartIndex);
						Triangles.Add(StartIndex + 1);
						Triangles.Add(StartIndex + 2);
						
						Triangles.Add(StartIndex);
						Triangles.Add(StartIndex + 2);
						Triangles.Add(StartIndex + 3);

						// Add normals
						for (int32 i = 0; i < 4; i++)
						{
							Normals.Add(Normal);
						}

						// Add UVs (simple mapping)
						UVs.Add(FVector2D(0, 0));
						UVs.Add(FVector2D(1, 0));
						UVs.Add(FVector2D(1, 1));
						UVs.Add(FVector2D(0, 1));

						// Add vertex colors from material system
						FColor VertexColor = FColor(0, 255, 0); // Default green
						if (MaterialManager)
						{
							// Get color from material data (for now, use material ID 1 for grass)
							VertexColor = MaterialManager->GetVertexColor(1);
						}
						
						// Debug: Log the vertex color being used
						if (X == 0 && Y == 0 && Z == 0 && FaceIndex == 0) // Log only for first voxel, first face
						{
							UE_LOG(LogTemp, Warning, TEXT("Using vertex color: R=%d, G=%d, B=%d, A=%d"), 
								VertexColor.R, VertexColor.G, VertexColor.B, VertexColor.A);
						}
						
						for (int32 i = 0; i < 4; i++)
						{
							Colors.Add(VertexColor);
						}
					}
				}
			}
		}
	}

	// Create the mesh
	if (Vertices.Num() > 0)
	{
		MeshComp->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, Colors, TArray<FProcMeshTangent>(), true);
		
		// Use material manager to get the appropriate material
		UMaterialInterface* MaterialInterface = nullptr;
		if (MaterialManager)
		{
			// Initialize MaterialManager with data table if not already done
			if (MaterialDataTable && !MaterialManager->IsInitialized())
			{
				MaterialManager->InitializeMaterialDataTable(MaterialDataTable);
				UE_LOG(LogTemp, Log, TEXT("TerraScape: Material data table initialized"));
			}
			
			// For now, use material ID 1 (grass) for all chunks
			MaterialInterface = MaterialManager->GetMaterialInterface(1);
			UE_LOG(LogTemp, Warning, TEXT("MaterialManager found, MaterialInterface: %s"), 
				MaterialInterface ? TEXT("Valid") : TEXT("NULL"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("MaterialManager is NULL!"));
		}
		
		if (!MaterialDataTable)
		{
			UE_LOG(LogTemp, Warning, TEXT("TerraScape: No Material Data Table set! Please assign DT_VoxelMaterials in Blueprint."));
		}
		
		if (MaterialInterface)
		{
			// Use the material from the data table
			MeshComp->SetMaterial(0, MaterialInterface);
			UE_LOG(LogTemp, Warning, TEXT("Using material from data table"));
		}
		else
		{
			// No material specified in data table - use vertex colors only
			// Create a basic material that supports vertex colors
			UMaterial* BaseMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
			if (BaseMaterial)
			{
				UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
				if (DynamicMaterial)
				{
					// Enable vertex color support
					DynamicMaterial->SetScalarParameterValue(TEXT("UseVertexColors"), 1.0f);
					MeshComp->SetMaterial(0, DynamicMaterial);
					UE_LOG(LogTemp, Warning, TEXT("Using dynamic material with vertex colors"));
				}
				else
				{
					MeshComp->SetMaterial(0, BaseMaterial);
					UE_LOG(LogTemp, Warning, TEXT("Using base material"));
				}
			}
			else
			{
				// Last resort: no material at all (should use vertex colors)
				MeshComp->SetMaterial(0, nullptr);
				UE_LOG(LogTemp, Warning, TEXT("No material - using vertex colors only"));
			}
		}
		
		MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
}

FTS_Voxel UTS_ChunkManager::GetVoxelAt(const FIntVector& ChunkID, int32 X, int32 Y, int32 Z) const
{
	if (!ChunkVoxelData.Contains(ChunkID))
	{
		return FTS_Voxel(); // Return air voxel
	}

	if (X < 0 || X >= ChunkSize || Y < 0 || Y >= ChunkSize || Z < 0 || Z >= ChunkSize)
	{
		return FTS_Voxel(); // Return air voxel for out of bounds
	}

	const TArray<FTS_Voxel>& VoxelData = ChunkVoxelData[ChunkID];
	int32 Index = Z * ChunkSize * ChunkSize + Y * ChunkSize + X;
	
	if (Index >= 0 && Index < VoxelData.Num())
	{
		return VoxelData[Index];
	}
	
	return FTS_Voxel(); // Return air voxel
}

bool UTS_ChunkManager::IsVoxelSolid(const FIntVector& ChunkID, int32 X, int32 Y, int32 Z) const
{
	FTS_Voxel Voxel = GetVoxelAt(ChunkID, X, Y, Z);
	return Voxel.IsSolid();
}

void UTS_ChunkManager::CheckAsyncMeshTasks()
{
	// Check for completed async tasks
	TArray<FIntVector> CompletedTasks;
	
	for (auto& TaskPair : AsyncMeshTasks)
	{
		FIntVector ChunkID = TaskPair.Key;
		FAsyncTask<FTS_AsyncMeshGenerationTask>* Task = TaskPair.Value;
		
		if (Task && Task->IsDone())
		{
			// Task completed, apply the mesh
			FTS_AsyncMeshGenerationTask& TaskResult = Task->GetTask();
			
			// Find the mesh component for this chunk
			if (UProceduralMeshComponent* MeshComp = ChunkMeshes.FindRef(ChunkID))
			{
				// Apply the generated mesh data
				if (TaskResult.Vertices.Num() > 0)
				{
					MeshComp->CreateMeshSection(0, TaskResult.Vertices, TaskResult.Triangles, 
						TaskResult.Normals, TaskResult.UVs, TaskResult.Colors, 
						TArray<FProcMeshTangent>(), true);
					
					if (TaskResult.MaterialInterface)
					{
						MeshComp->SetMaterial(0, TaskResult.MaterialInterface);
					}
					
					MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
					
					UE_LOG(LogTemp, Log, TEXT("Async mesh generation completed for chunk %s"), 
						*ChunkID.ToString());
				}
			}
			
			// Clean up the task
			delete Task;
			CompletedTasks.Add(ChunkID);
		}
	}
	
	// Remove completed tasks
	for (const FIntVector& ChunkID : CompletedTasks)
	{
		AsyncMeshTasks.Remove(ChunkID);
	}
	
	// Process pending mesh generation queue if we have available slots
	while (PendingMeshGenerationQueue.Num() > 0 && AsyncMeshTasks.Num() < MaxConcurrentAsyncTasks)
	{
		FIntVector QueuedChunkID = PendingMeshGenerationQueue[0];
		PendingMeshGenerationQueue.RemoveAt(0);
		
		UE_LOG(LogTemp, Log, TEXT("Processing queued chunk %s from pending queue"), *QueuedChunkID.ToString());
		
		// Start mesh generation for the queued chunk
		if (LoadedChunks.Contains(QueuedChunkID) && ChunkVoxelData.Contains(QueuedChunkID))
		{
			TArray<FTS_Voxel> VoxelData = ChunkVoxelData[QueuedChunkID];
			
			// Get LOD level for this chunk
			int32 LODLevel = GetChunkLODLevel(QueuedChunkID);
			
			// Create async task for mesh generation
			FAsyncTask<FTS_AsyncMeshGenerationTask>* AsyncTask = new FAsyncTask<FTS_AsyncMeshGenerationTask>(
				QueuedChunkID, VoxelData, ChunkSize, VoxelSize, CalculateChunkWorldPosition(QueuedChunkID), MaterialManager, LODLevel);
			
			AsyncTask->StartBackgroundTask();
			AsyncMeshTasks.Add(QueuedChunkID, AsyncTask);
			
			UE_LOG(LogTemp, Log, TEXT("Started queued async mesh generation for chunk %s (%d/%d tasks)"), 
				*QueuedChunkID.ToString(), AsyncMeshTasks.Num(), MaxConcurrentAsyncTasks);
		}
	}
}

void UTS_ChunkManager::GenerateChunkGrid(const FIntVector& CenterChunk, int32 GridSize)
{
	if (GridSize <= 0 || GridSize > 100)
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid grid size: %d (must be 1-100)"), GridSize);
		return;
	}

	int32 ChunksGenerated = 0;
	int32 TotalChunks = GridSize * GridSize;
	
	UE_LOG(LogTemp, Log, TEXT("Generating chunk grid: center=%s, size=%dx%d (%d chunks)"), 
		*CenterChunk.ToString(), GridSize, GridSize, TotalChunks);

	// Calculate half size for centering
	int32 HalfSize = GridSize / 2;
	int32 StartX = CenterChunk.X - HalfSize;
	int32 StartY = CenterChunk.Y - HalfSize;

	// Generate 2D grid for chunk streaming (Z=0 only for continuous terrain)
	for (int32 X = 0; X < GridSize; X++)
	{
		for (int32 Y = 0; Y < GridSize; Y++)
		{
			FIntVector ChunkID(StartX + X, StartY + Y, 0); // Always Z=0 for continuous terrain
			
			// Only create if it doesn't already exist
			if (!LoadedChunks.Contains(ChunkID))
			{
				CreateChunk(ChunkID);
				ChunksGenerated++;
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Generated %d new chunks in continuous 2D grid"), ChunksGenerated);
}

void UTS_ChunkManager::ClearAllChunks()
{
	int32 ChunksToDelete = LoadedChunks.Num();
	int32 QueuedChunks = PendingMeshGenerationQueue.Num();
	
	UE_LOG(LogTemp, Log, TEXT("Clearing all %d chunks and %d queued chunks"), ChunksToDelete, QueuedChunks);

	// Clear pending mesh generation queue FIRST to prevent new chunks from being created
	PendingMeshGenerationQueue.Empty();
	
	// Cancel any running async tasks
	for (auto& TaskPair : AsyncMeshTasks)
	{
		FAsyncTask<FTS_AsyncMeshGenerationTask>* Task = TaskPair.Value;
		if (Task)
		{
			Task->Cancel();
			delete Task;
		}
	}
	AsyncMeshTasks.Empty();

	// Get all chunk IDs before deleting (to avoid iterator issues)
	TArray<FIntVector> ChunkIDs;
	LoadedChunks.GetKeys(ChunkIDs);

	// Delete each chunk
	for (const FIntVector& ChunkID : ChunkIDs)
	{
		DeleteChunk(ChunkID);
	}

	UE_LOG(LogTemp, Log, TEXT("Cleared all chunks, queue, and async tasks"));
}

// Async Mesh Generation Task Implementation
void FTS_AsyncMeshGenerationTask::DoWork()
{
	GenerateChunkMesh();
}

void FTS_AsyncMeshGenerationTask::GenerateChunkMesh()
{
	// Safety check: Ensure chunk size is reasonable
	if (ChunkSize <= 0 || ChunkSize > 256)
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid chunk size: %d"), ChunkSize);
		return;
	}

	// Safety check: Ensure voxel data is valid
	if (VoxelData.Num() != ChunkSize * ChunkSize * ChunkSize)
	{
		UE_LOG(LogTemp, Error, TEXT("Voxel data size mismatch: expected %d, got %d"), 
			ChunkSize * ChunkSize * ChunkSize, VoxelData.Num());
		return;
	}

	// Debug: Count solid voxels in async task
	int32 SolidVoxels = 0;
	for (const FTS_Voxel& Voxel : VoxelData)
	{
		if (Voxel.IsSolid())
		{
			SolidVoxels++;
		}
	}
	UE_LOG(LogTemp, Log, TEXT("Async task for chunk %s: %d solid voxels out of %d total, VoxelSize=%.1f"), 
		*ChunkID.ToString(), SolidVoxels, VoxelData.Num(), VoxelSize);

	// Face directions for culling (normalized directions, not scaled by VoxelSize)
	const FVector FaceDirections[6] = {
		FVector(1, 0, 0),   // Right
		FVector(-1, 0, 0),  // Left
		FVector(0, 1, 0),   // Forward
		FVector(0, -1, 0),  // Back
		FVector(0, 0, 1),   // Up
		FVector(0, 0, -1)   // Down
	};

	const FVector FaceNormals[6] = {
		FVector(1, 0, 0),
		FVector(-1, 0, 0),
		FVector(0, 1, 0),
		FVector(0, -1, 0),
		FVector(0, 0, 1),
		FVector(0, 0, -1)
	};

	// ChunkWorldPos is now passed to the async task
	UE_LOG(LogTemp, Log, TEXT("Async task chunk world position: %s (ChunkID=%s, ChunkSize=%d, VoxelSize=%.1f)"), 
		*ChunkWorldPos.ToString(), *ChunkID.ToString(), ChunkSize, VoxelSize);
	
	// Debug: Log first voxel position to verify alignment
	if (Vertices.Num() > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("First vertex position: %s (should be at ChunkWorldPos: %s)"), 
			*Vertices[0].ToString(), *ChunkWorldPos.ToString());
	}
	
	// Debug: Log chunk bounds
	FVector ChunkMin = ChunkWorldPos;
	FVector ChunkMax = ChunkWorldPos + FVector(ChunkSize * VoxelSize, ChunkSize * VoxelSize, ChunkSize * VoxelSize);
	UE_LOG(LogTemp, Log, TEXT("Chunk bounds: Min=%s, Max=%s, Size=%s"), 
		*ChunkMin.ToString(), *ChunkMax.ToString(), *FVector(ChunkSize * VoxelSize, ChunkSize * VoxelSize, ChunkSize * VoxelSize).ToString());
	
	// Performance tracking
	int32 TotalVoxelsProcessed = 0;
	int32 MaxVoxelsPerFrame = ChunkSize * ChunkSize * ChunkSize; // Allow processing all voxels in chunk

	// Calculate LOD step size (skip voxels for lower detail)
	int32 LODStep = 1;
	switch (LODLevel)
	{
		case 0: LODStep = 1; break;  // Full detail
		case 1: LODStep = 2; break;  // Half detail
		case 2: LODStep = 4; break;  // Quarter detail
		case 3: LODStep = 8; break;  // Eighth detail
		default: LODStep = 1; break;
	}

	UE_LOG(LogTemp, Log, TEXT("Generating mesh for chunk %s with LOD level %d (step size %d)"), 
		*ChunkID.ToString(), LODLevel, LODStep);

	// Generate mesh data with LOD
	for (int32 X = 0; X < ChunkSize; X += LODStep)
	{
		for (int32 Y = 0; Y < ChunkSize; Y += LODStep)
		{
			for (int32 Z = 0; Z < ChunkSize; Z += LODStep)
			{
				// Safety check: Prevent infinite loops
				TotalVoxelsProcessed++;
				if (TotalVoxelsProcessed > MaxVoxelsPerFrame)
				{
					UE_LOG(LogTemp, Error, TEXT("Mesh generation exceeded safety limit (%d voxels), aborting"), MaxVoxelsPerFrame);
					return;
				}
				
				// Only generate faces for solid voxels
				if (!IsVoxelSolid(X, Y, Z))
				{
					continue;
				}

				// Check each face
				for (int32 FaceIndex = 0; FaceIndex < 6; FaceIndex++)
				{
					FVector NeighborPos = FVector(X, Y, Z) + FaceDirections[FaceIndex];
					bool bNeighborIsAir = true;

					// Check if neighbor is air (face culling)
					if (NeighborPos.X >= 0 && NeighborPos.X < ChunkSize &&
						NeighborPos.Y >= 0 && NeighborPos.Y < ChunkSize &&
						NeighborPos.Z >= 0 && NeighborPos.Z < ChunkSize)
					{
						bNeighborIsAir = !IsVoxelSolid(NeighborPos.X, NeighborPos.Y, NeighborPos.Z);
					}

					// Only generate face if neighbor is air (face culling)
					if (bNeighborIsAir)
					{
					// Generate quad for this face (scale by LOD step)
					FVector BasePos = ChunkWorldPos + FVector(X * VoxelSize, Y * VoxelSize, Z * VoxelSize);
					FVector Normal = FaceNormals[FaceIndex];
					float LODVoxelSize = VoxelSize * LODStep;
						
						// Define quad vertices based on face direction (consistent counter-clockwise order)
						TArray<FVector> QuadVertices;
						QuadVertices.SetNum(4);
						
						if (FaceIndex == 0) // Right
						{
							QuadVertices[0] = BasePos + FVector(VoxelSize, 0, 0);
							QuadVertices[1] = BasePos + FVector(VoxelSize, 0, VoxelSize);
							QuadVertices[2] = BasePos + FVector(VoxelSize, VoxelSize, VoxelSize);
							QuadVertices[3] = BasePos + FVector(VoxelSize, VoxelSize, 0);
						}
						else if (FaceIndex == 1) // Left
						{
							QuadVertices[0] = BasePos + FVector(0, VoxelSize, 0);
							QuadVertices[1] = BasePos + FVector(0, VoxelSize, VoxelSize);
							QuadVertices[2] = BasePos + FVector(0, 0, VoxelSize);
							QuadVertices[3] = BasePos + FVector(0, 0, 0);
						}
						else if (FaceIndex == 2) // Forward
						{
							QuadVertices[0] = BasePos + FVector(0, VoxelSize, 0);
							QuadVertices[1] = BasePos + FVector(VoxelSize, VoxelSize, 0);
							QuadVertices[2] = BasePos + FVector(VoxelSize, VoxelSize, VoxelSize);
							QuadVertices[3] = BasePos + FVector(0, VoxelSize, VoxelSize);
						}
						else if (FaceIndex == 3) // Back
						{
							QuadVertices[0] = BasePos + FVector(VoxelSize, 0, 0);
							QuadVertices[1] = BasePos + FVector(0, 0, 0);
							QuadVertices[2] = BasePos + FVector(0, 0, VoxelSize);
							QuadVertices[3] = BasePos + FVector(VoxelSize, 0, VoxelSize);
						}
						else if (FaceIndex == 4) // Up
						{
							QuadVertices[0] = BasePos + FVector(0, 0, VoxelSize);
							QuadVertices[1] = BasePos + FVector(0, VoxelSize, VoxelSize);
							QuadVertices[2] = BasePos + FVector(VoxelSize, VoxelSize, VoxelSize);
							QuadVertices[3] = BasePos + FVector(VoxelSize, 0, VoxelSize);
						}
						else if (FaceIndex == 5) // Down
						{
							QuadVertices[0] = BasePos + FVector(0, 0, 0);
							QuadVertices[1] = BasePos + FVector(VoxelSize, 0, 0);
							QuadVertices[2] = BasePos + FVector(VoxelSize, VoxelSize, 0);
							QuadVertices[3] = BasePos + FVector(0, VoxelSize, 0);
						}

						// Add vertices
						int32 StartIndex = Vertices.Num();
						Vertices.Append(QuadVertices);
						
						// Add triangles (two triangles per quad) - counter-clockwise winding for outward normals
						Triangles.Add(StartIndex);
						Triangles.Add(StartIndex + 1);
						Triangles.Add(StartIndex + 2);
						
						Triangles.Add(StartIndex);
						Triangles.Add(StartIndex + 2);
						Triangles.Add(StartIndex + 3);

						// Add normals
						for (int32 i = 0; i < 4; i++)
						{
							Normals.Add(Normal);
						}

						// Add UVs (simple mapping)
						UVs.Add(FVector2D(0, 0));
						UVs.Add(FVector2D(1, 0));
						UVs.Add(FVector2D(1, 1));
						UVs.Add(FVector2D(0, 1));

						// Add vertex colors from material system
						FColor VertexColor = FColor(0, 255, 0); // Default green
						if (MaterialManager && MaterialManager->IsInitialized())
						{
							// Get color from material data (for now, use material ID 1 for grass)
							VertexColor = MaterialManager->GetVertexColor(1);
						}
						else
						{
							UE_LOG(LogTemp, Warning, TEXT("MaterialManager not initialized, using default green color"));
						}
						for (int32 i = 0; i < 4; i++)
						{
							Colors.Add(VertexColor);
						}
					}
				}
			}
		}
	}

	// Set up material interface
	if (MaterialManager && MaterialManager->IsInitialized())
	{
		// Use material ID 1 (grass) for all chunks
		MaterialInterface = MaterialManager->GetMaterialInterface(1);
		UE_LOG(LogTemp, Log, TEXT("Async task: Using material from data table"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Async task: MaterialManager not initialized, no material will be set"));
	}

	// No need to offset vertices - the chunk world position is already offset

	// Debug: Log mesh generation results
	UE_LOG(LogTemp, Log, TEXT("Async mesh generation complete for chunk %s: %d vertices, %d triangles"), 
		*ChunkID.ToString(), Vertices.Num(), Triangles.Num());
}

FTS_Voxel FTS_AsyncMeshGenerationTask::GetVoxelAt(int32 X, int32 Y, int32 Z) const
{
	int32 Index = Z * ChunkSize * ChunkSize + Y * ChunkSize + X;
	
	if (Index >= 0 && Index < VoxelData.Num())
	{
		return VoxelData[Index];
	}
	
	return FTS_Voxel(); // Return air voxel
}

bool FTS_AsyncMeshGenerationTask::IsVoxelSolid(int32 X, int32 Y, int32 Z) const
{
	FTS_Voxel Voxel = GetVoxelAt(X, Y, Z);
	return Voxel.IsSolid();
}

// LOD Implementation
void UTS_ChunkManager::UpdateChunkLOD()
{
	if (!bEnableLOD || !PlayerReference)
	{
		return;
	}

	FVector PlayerLocation = PlayerReference->GetActorLocation();
	int32 ChunksUpdated = 0;

	// Check all loaded chunks for LOD updates
	for (auto& ChunkPair : LoadedChunks)
	{
		FIntVector ChunkID = ChunkPair.Key;
		int32 CurrentLOD = ChunkLODLevels.FindRef(ChunkID);
		int32 NewLOD = GetChunkLODLevel(ChunkID);

		// If LOD level changed, regenerate the chunk
		if (CurrentLOD != NewLOD)
		{
			ChunkLODLevels.Add(ChunkID, NewLOD);
			
			// Cancel existing async task if any
			if (FAsyncTask<FTS_AsyncMeshGenerationTask>** ExistingTask = AsyncMeshTasks.Find(ChunkID))
			{
				if (*ExistingTask)
				{
					(*ExistingTask)->Cancel();
					delete *ExistingTask;
				}
				AsyncMeshTasks.Remove(ChunkID);
			}

			// Regenerate chunk with new LOD
			if (ChunkVoxelData.Contains(ChunkID))
			{
				TArray<FTS_Voxel> VoxelData = ChunkVoxelData[ChunkID];
				
				// Check if we can start a new async task
				if (AsyncMeshTasks.Num() < MaxConcurrentAsyncTasks)
				{
					FAsyncTask<FTS_AsyncMeshGenerationTask>* AsyncTask = new FAsyncTask<FTS_AsyncMeshGenerationTask>(
						ChunkID, VoxelData, ChunkSize, VoxelSize, CalculateChunkWorldPosition(ChunkID), MaterialManager, NewLOD);
					
					AsyncTask->StartBackgroundTask();
					AsyncMeshTasks.Add(ChunkID, AsyncTask);
					
					UE_LOG(LogTemp, Log, TEXT("Updated chunk %s to LOD level %d"), *ChunkID.ToString(), NewLOD);
					ChunksUpdated++;
				}
				else
				{
					// Add to queue for later processing
					PendingMeshGenerationQueue.Add(ChunkID);
					UE_LOG(LogTemp, Warning, TEXT("Queued chunk %s for LOD update to level %d"), *ChunkID.ToString(), NewLOD);
				}
			}
		}
	}

	if (ChunksUpdated > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("Updated LOD for %d chunks"), ChunksUpdated);
	}
}

int32 UTS_ChunkManager::GetChunkLODLevel(const FIntVector& ChunkID) const
{
	if (!bEnableLOD || !PlayerReference)
	{
		return 0; // Full detail if LOD disabled
	}

	// Calculate chunk center position using single source of truth
	FVector ChunkCenter = CalculateChunkWorldPosition(ChunkID) + FVector(ChunkSize * VoxelSize * 0.5f);

	// Calculate distance to player
	FVector PlayerLocation = PlayerReference->GetActorLocation();
	float Distance = FVector::Dist(PlayerLocation, ChunkCenter);

	// Determine LOD level based on distance
	if (Distance <= LOD0Distance)
	{
		return 0; // Full detail
	}
	else if (Distance <= LOD1Distance)
	{
		return 1; // Reduced detail
	}
	else if (Distance <= LOD2Distance)
	{
		return 2; // Minimal detail
	}
	else
	{
		return 3; // Lowest detail (or could be unloaded)
	}
}

void UTS_ChunkManager::SetPlayerReference(AActor* Player)
{
	PlayerReference = Player;
	UE_LOG(LogTemp, Log, TEXT("Set player reference for LOD calculations: %s"), 
		Player ? *Player->GetName() : TEXT("None"));
}

FVector UTS_ChunkManager::CalculateChunkWorldPosition(const FIntVector& ChunkID) const
{
	float ChunkWorldSize = ChunkSize * VoxelSize;
	// Apply ChunkGap as spacing between chunks, not global offset
	return FVector(
		ChunkID.X * (ChunkWorldSize + ChunkGap), 
		ChunkID.Y * (ChunkWorldSize + ChunkGap), 
		ChunkID.Z * (ChunkWorldSize + ChunkGap)
	);
}

TArray<FTS_Voxel> UTS_ChunkManager::GenerateProceduralVoxels(const FIntVector& ChunkID)
{
	TArray<FTS_Voxel> VoxelData;
	VoxelData.SetNum(ChunkSize * ChunkSize * ChunkSize);

	if (!WorldGenerator)
	{
		UE_LOG(LogTemp, Warning, TEXT("WorldGenerator is null, falling back to test voxels"));
		return GenerateTestVoxels(ChunkID);
	}

	// Calculate chunk world position
	FVector ChunkWorldPos = CalculateChunkWorldPosition(ChunkID);

	// Generate voxels using world generator
	for (int32 X = 0; X < ChunkSize; X++)
	{
		for (int32 Y = 0; Y < ChunkSize; Y++)
		{
			for (int32 Z = 0; Z < ChunkSize; Z++)
			{
				// Calculate world coordinates for this voxel
				float WorldX = ChunkWorldPos.X + (X * VoxelSize);
				float WorldY = ChunkWorldPos.Y + (Y * VoxelSize);
				float WorldZ = ChunkWorldPos.Z + (Z * VoxelSize);

				// Generate voxel using world generator
				FTS_VoxelGenResult Result = WorldGenerator->GenerateVoxelAtLocation(WorldX, WorldY, WorldZ);

				// Calculate array index
				int32 Index = X + (Y * ChunkSize) + (Z * ChunkSize * ChunkSize);

				// Create voxel
				FTS_Voxel Voxel;
				Voxel.bIsSolid = Result.bIsSolid;
				Voxel.MaterialID = Result.MaterialID;
				VoxelData[Index] = Voxel;
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Generated procedural voxels for chunk %d,%d,%d"), ChunkID.X, ChunkID.Y, ChunkID.Z);
	return VoxelData;
}

void UTS_ChunkManager::SetProceduralGenerationEnabled(bool bEnabled)
{
	bUseProceduralGeneration = bEnabled;
	UE_LOG(LogTemp, Log, TEXT("Procedural generation %s"), bEnabled ? TEXT("enabled") : TEXT("disabled"));
}

UTS_WorldGenerator* UTS_ChunkManager::GetWorldGenerator() const
{
	return WorldGenerator;
}

