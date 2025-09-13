/**
 * @file TS_TerraScapeManager.h
 * @brief Simple TerraScape manager for MVP
 * @author Keves
 * @version 1.0
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TS_ChunkManager.h"
#include "TS_TerraScapeManager.generated.h"

/**
 * @brief Main TerraScape manager actor for MVP
 * Simple actor that manages chunks - keep it minimal for now
 */
UCLASS(Blueprintable, ClassGroup=(TerraScape), meta=(DisplayName="TerraScape Manager"))
class TERRA_SCAPE_API ATS_TerraScapeManager : public AActor
{
	GENERATED_BODY()
	
public:	
	ATS_TerraScapeManager();

protected:
	virtual void BeginPlay() override;

public:
	/** Chunk manager component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TerraScape | Components")
	UTS_ChunkManager* ChunkManager;

	/** Simple Blueprint functions for MVP testing */
	UFUNCTION(BlueprintCallable, Category = "TerraScape | Management")
	void CreateTestChunk(const FIntVector& ChunkID);

	UFUNCTION(BlueprintCallable, Category = "TerraScape | Management")
	void DeleteTestChunk(const FIntVector& ChunkID);

	UFUNCTION(BlueprintCallable, Category = "TerraScape | Management")
	int32 GetChunkCount() const;
};