/**
 * @file TS_TerraScapeManager.cpp
 * @brief Simple TerraScape manager implementation for MVP
 * @author Keves
 * @version 1.0
 */

#include "TS_TerraScapeManager.h"
#include "Components/SceneComponent.h"

ATS_TerraScapeManager::ATS_TerraScapeManager()
{
	// Set this actor to call Tick() every frame - disable for MVP
	PrimaryActorTick.bCanEverTick = false;

	// Create root component
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	// Create chunk manager component
	ChunkManager = CreateDefaultSubobject<UTS_ChunkManager>(TEXT("ChunkManager"));
}

void ATS_TerraScapeManager::BeginPlay()
{
	Super::BeginPlay();
	
	UE_LOG(LogTemp, Log, TEXT("TerraScape Manager started - MVP version"));
}

void ATS_TerraScapeManager::CreateTestChunk(const FIntVector& ChunkID)
{
	if (ChunkManager)
	{
		ChunkManager->CreateChunk(ChunkID);
	}
}

void ATS_TerraScapeManager::DeleteTestChunk(const FIntVector& ChunkID)
{
	if (ChunkManager)
	{
		ChunkManager->DeleteChunk(ChunkID);
	}
}

int32 ATS_TerraScapeManager::GetChunkCount() const
{
	if (ChunkManager)
	{
		return ChunkManager->GetLoadedChunkCount();
	}
	return 0;
}