#include "VoxelActor.h"


const int32 bTriangles[] = { 2,1,0,0,3,2 };
const FVector2D bUVs[] = { FVector2D(0.000000, 0.000000), FVector2D(0.00000, 1.00000), FVector2D(1.00000, 1.00000), FVector2D(1.00000,0.000000) };
const FVector bNormals0[] = { FVector(0,0,1), FVector(0,0,1), FVector(0,0,1), FVector(0,0,1) };
const FVector bNormals1[] = { FVector(0,0,-1), FVector(0,0,-1), FVector(0,0,-1), FVector(0,0,-1) };
const FVector bNormals2[] = { FVector(0,1,0),FVector(0,1,0) ,FVector(0,1,0) ,FVector(0,1,0) };
const FVector bNormals3[] = { FVector(0,-1,0),FVector(0,-1,0), FVector(0,-1,0), FVector(0,-1,0) };
const FVector bNormals4[] = { FVector(1,0,0),FVector(1,0,0), FVector(1,0,0), FVector(1,0,0) };
const FVector bNormals5[] = { FVector(-1,0,0),FVector(-1,0,0), FVector(-1,0,0), FVector(-1,0,0) };
const FVector bMask[] = { FVector(0.00000,0.00000,1.00000),FVector(0.00000,0.00000,-1.00000) ,FVector(0.00000,1.00000,0.00000) ,FVector(0.00000,-1.00000,0.00000), FVector(1.00000,0.0000,0.00000),FVector(-1.00000,0.0000,0.00000) };
// Sets default values
AVoxelActor::AVoxelActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AVoxelActor::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AVoxelActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AVoxelActor::OnConstruction(const FTransform& Transform)
{
	ChunkZElements = 80;
	//ChunkLineElementsP2 = ChunkLineElements * ChunkLineElements;
	//ChunkTotalElements = ChunkLineElementsP2 * ChunkZElements;
	ChunkLineElementsExt = ChunkLineElements + 2;
	ChunkTotalElements = ChunkLineElementsExt * ChunkLineElementsExt * ChunkZElements;
	ChunkLineElementsP2 = ChunkLineElements * ChunkLineElements;
	ChunkLineElementsP2Ext = ChunkLineElementsExt * ChunkLineElementsExt;

	VoxelSizeHalf = VoxelSize / 2;

	FString string = "Voxel_" + FString::FromInt(ChunkXIndex) + "_" + FString::FromInt(ChunkYIndex);
	FName Name = FName(string);

	proceduralComponent = NewObject<class UProceduralMeshComponent>(this, Name);
	proceduralComponent->RegisterComponent();

	RootComponent = proceduralComponent;
	RootComponent->SetWorldTransform(Transform);

	Super::OnConstruction(Transform);

	GenerateChunks();
	UpdateMesh();
}

void AVoxelActor::GenerateChunks()
{
	FRandomStream RandomStream = FRandomStream(RandomSeed);
	TArray<FIntVector> TreeCenters;

	ChunkFields.SetNumUninitialized(ChunkTotalElements);

	TArray <int32> noise = CalculateNoise();

	for (int32 x = 0; x < ChunkLineElementsExt; x++)
	{
		for (int32 y = 0; y < ChunkLineElementsExt; y++)
		{
			for (int32 z = 0; z < ChunkZElements; z++)
			{
				int32 index = x + (y * ChunkLineElementsExt) + (z * ChunkLineElementsP2Ext);

				//if (z == 31 + noise[x + y * ChunkLineElements] && RandomStream.FRand() < 0.01) TreeCenters.Add(FIntVector(x, y, z)); // get random for tree spawn

				if (z == 30 + noise[x + y * ChunkLineElementsExt]) ChunkFields[index] = 11; // grass
				else if (z == 29 + noise[x + y * ChunkLineElementsExt]) ChunkFields[index] = 12; // dirt
				else if (z < 29 + noise[x + y * ChunkLineElementsExt]) ChunkFields[index] = 13; // stone
				else ChunkFields[index] = 0;


				// if equal to 30, do voxel with 1st material in material array. 
				// if 29, do voxel with 2nd. etc.
				// eventually would be better to have as exposed pins to adjust in the actor BP!
			}
		}
	}

	// tree range (smaller so they don't spawn on edges)

	for (int32 x = 2; x < ChunkLineElementsExt-2; x++)
	{
		for (int32 y = 2; y < ChunkLineElementsExt-2; y++)
		{
			for (int32 z = 0; z < ChunkZElements; z++)
			{
				int32 index = x + (y * ChunkLineElementsExt) + (z * ChunkLineElementsP2Ext);
				if (RandomStream.FRand() < 0.05 && z == 31 + noise[x + y * ChunkLineElementsExt]) ChunkFields[index] = -1; // < 0.05 is chance of spawning, increase if more needed, etc
				if (RandomStream.FRand() < 0.05 && z == 31 + noise[x + y * ChunkLineElementsExt]) ChunkFields[index] = -2; // flowers
				if (RandomStream.FRand() < 0.01 && z == 31 + noise[x + y * ChunkLineElementsExt]) TreeCenters.Add(FIntVector(x, y, z));
			}
		}
	}

	// end tree range

	for (FIntVector TreeCenter : TreeCenters)
	{
		int32 tree_height = RandomStream.RandRange(3, 6); // determine height of tree trunk
		int32 randomX = RandomStream.RandRange(0, 2); // leaf shapes
		int32 randomY = RandomStream.RandRange(0, 2); // leaf shapes
		int32 randomZ = RandomStream.RandRange(0, 2); // leaf shapes

		// tree leaves (generated first, so that tree trunk does not spawn where leaves should be!)
		for (int32 tree_x = -2; tree_x < 3; tree_x++)
		{
			for (int32 tree_y = -2; tree_y < 3; tree_y++)
			{
				for (int32 tree_z = -2; tree_z < 3; tree_z++)
				{
					if (inRange(tree_x + TreeCenter.X + 1, ChunkLineElements + 1) && inRange(tree_y + TreeCenter.Y + 1, ChunkLineElements + 1) && inRange(tree_z + TreeCenter.Z + tree_height + 1, ChunkZElements))
					{
							float radius = FVector(tree_x * randomX, tree_y * randomY, tree_z * randomZ).Size();

							if (radius <= 2.8)
								if (RandomStream.FRand() < 0.5 || radius <= 1.4) // check if values in range of ChunkField size.
									ChunkFields[TreeCenter.X + tree_x + (ChunkLineElementsExt * (TreeCenter.Y + tree_y)) + (ChunkLineElementsP2Ext * (TreeCenter.Z + tree_z + tree_height))] = 1;
						
					}
				}
			}
		} // end of leaves
	
		// tree trunk
		for (int32 h = 0; h < tree_height; h++)
		{
			ChunkFields[TreeCenter.X + (TreeCenter.Y * ChunkLineElementsExt) + ((TreeCenter.Z + h) * ChunkLineElementsP2Ext)] = 14;
		}
	
	
	}

	// transparent materials and full TODO
}

void AVoxelActor::UpdateMesh()
{	
	TArray<FMeshSection> MeshSections;
	MeshSections.SetNum(Materials.Num());
	int32 el_num = 0;


	for (int x = 0; x < ChunkLineElements; x++)
	{
		for (int y = 0; y < ChunkLineElements; y++)
		{
			for (int z = 0; z < ChunkZElements; z++)
			{
				int32 index = (x + 1) + (ChunkLineElementsExt * (y + 1)) + (ChunkLineElementsP2Ext * z);
				int32 MeshIndex = ChunkFields[index];

				if (MeshIndex > 0) // ignore anything from 0-10 (transparent voxels)
				{
					MeshIndex--;

					TArray<FVector> &Vertices = MeshSections[MeshIndex].Vertices;
					TArray<int32> &Triangles = MeshSections[MeshIndex].Triangles;
					TArray<FVector> &Normals = MeshSections[MeshIndex].Normals;
					TArray<FVector2D> &UVs = MeshSections[MeshIndex].UVs;
					TArray<FProcMeshTangent> &Tangents = MeshSections[MeshIndex].Tangents;
					TArray<FColor> &VertexColor = MeshSections[MeshIndex].VertexColor;
					int32 elementID = MeshSections[MeshIndex].elementID;

					// Add faces, vertices, UVs and normals
					int triangle_num = 0;
					for (int i = 0; i < 6; i++)
					{
						int newIndex = index + bMask[i].X + (bMask[i].Y * ChunkLineElementsExt) + (bMask[i].Z * ChunkLineElementsP2Ext);

						bool flag = false;
						if (MeshIndex >= 20) flag = true;
						
						else if (newIndex < ChunkFields.Num() && newIndex >= 0)
							if (ChunkFields[newIndex] < 10 ) flag = true;
						

						if (flag)
						{
							Triangles.Add(bTriangles[0] + triangle_num + elementID);
							Triangles.Add(bTriangles[1] + triangle_num + elementID);
							Triangles.Add(bTriangles[2] + triangle_num + elementID);
							Triangles.Add(bTriangles[3] + triangle_num + elementID);
							Triangles.Add(bTriangles[4] + triangle_num + elementID);
							Triangles.Add(bTriangles[5] + triangle_num + elementID);
							triangle_num += 4; // add 4 vertices to next triangle

							switch (i)
							{
							case 0: {
								Vertices.Add(FVector(-VoxelSizeHalf + (x * VoxelSize), VoxelSizeHalf + (y * VoxelSize), VoxelSizeHalf + (z * VoxelSize)));
								Vertices.Add(FVector(-VoxelSizeHalf + (x * VoxelSize), -VoxelSizeHalf + (y * VoxelSize), VoxelSizeHalf + (z * VoxelSize)));
								Vertices.Add(FVector(VoxelSizeHalf + (x * VoxelSize), -VoxelSizeHalf + (y * VoxelSize), VoxelSizeHalf + (z * VoxelSize)));
								Vertices.Add(FVector(VoxelSizeHalf + (x * VoxelSize), VoxelSizeHalf + (y * VoxelSize), VoxelSizeHalf + (z * VoxelSize)));

								Normals.Append(bNormals0, UE_ARRAY_COUNT(bNormals0));
								break;
							}
							case 1: {
								Vertices.Add(FVector(VoxelSizeHalf + (x * VoxelSize), -VoxelSizeHalf + (y * VoxelSize), -VoxelSizeHalf + (z * VoxelSize)));
								Vertices.Add(FVector(-VoxelSizeHalf + (x * VoxelSize), -VoxelSizeHalf + (y * VoxelSize), -VoxelSizeHalf + (z * VoxelSize)));
								Vertices.Add(FVector(-VoxelSizeHalf + (x * VoxelSize), VoxelSizeHalf + (y * VoxelSize), -VoxelSizeHalf + (z * VoxelSize)));
								Vertices.Add(FVector(VoxelSizeHalf + (x * VoxelSize), VoxelSizeHalf + (y * VoxelSize), -VoxelSizeHalf + (z * VoxelSize)));

								Normals.Append(bNormals1, UE_ARRAY_COUNT(bNormals1));
								break;
							}
							case 2: {
								Vertices.Add(FVector(VoxelSizeHalf + (x * VoxelSize), VoxelSizeHalf + (y * VoxelSize), VoxelSizeHalf + (z * VoxelSize)));
								Vertices.Add(FVector(VoxelSizeHalf + (x * VoxelSize), VoxelSizeHalf + (y * VoxelSize), -VoxelSizeHalf + (z * VoxelSize)));
								Vertices.Add(FVector(-VoxelSizeHalf + (x * VoxelSize), VoxelSizeHalf + (y * VoxelSize), -VoxelSizeHalf + (z * VoxelSize)));
								Vertices.Add(FVector(-VoxelSizeHalf + (x * VoxelSize), VoxelSizeHalf + (y * VoxelSize), VoxelSizeHalf + (z * VoxelSize)));

								Normals.Append(bNormals2, UE_ARRAY_COUNT(bNormals2));
								break;
							}
							case 3: {
								Vertices.Add(FVector(-VoxelSizeHalf + (x * VoxelSize), -VoxelSizeHalf + (y * VoxelSize), VoxelSizeHalf + (z * VoxelSize)));
								Vertices.Add(FVector(-VoxelSizeHalf + (x * VoxelSize), -VoxelSizeHalf + (y * VoxelSize), -VoxelSizeHalf + (z * VoxelSize)));
								Vertices.Add(FVector(VoxelSizeHalf + (x * VoxelSize), -VoxelSizeHalf + (y * VoxelSize), -VoxelSizeHalf + (z * VoxelSize)));
								Vertices.Add(FVector(VoxelSizeHalf + (x * VoxelSize), -VoxelSizeHalf + (y * VoxelSize), VoxelSizeHalf + (z * VoxelSize)));

								Normals.Append(bNormals3, UE_ARRAY_COUNT(bNormals3));
								break;
							}
							case 4: {
								Vertices.Add(FVector(VoxelSizeHalf + (x * VoxelSize), -VoxelSizeHalf + (y * VoxelSize), VoxelSizeHalf + (z * VoxelSize)));
								Vertices.Add(FVector(VoxelSizeHalf + (x * VoxelSize), -VoxelSizeHalf + (y * VoxelSize), -VoxelSizeHalf + (z * VoxelSize)));
								Vertices.Add(FVector(VoxelSizeHalf + (x * VoxelSize), VoxelSizeHalf + (y * VoxelSize), -VoxelSizeHalf + (z * VoxelSize)));
								Vertices.Add(FVector(VoxelSizeHalf + (x * VoxelSize), VoxelSizeHalf + (y * VoxelSize), VoxelSizeHalf + (z * VoxelSize)));

								Normals.Append(bNormals5, UE_ARRAY_COUNT(bNormals4));
								break;
							}
							case 5: {
								Vertices.Add(FVector(-VoxelSizeHalf + (x * VoxelSize), VoxelSizeHalf + (y * VoxelSize), VoxelSizeHalf + (z * VoxelSize)));
								Vertices.Add(FVector(-VoxelSizeHalf + (x * VoxelSize), VoxelSizeHalf + (y * VoxelSize), -VoxelSizeHalf + (z * VoxelSize)));
								Vertices.Add(FVector(-VoxelSizeHalf + (x * VoxelSize), -VoxelSizeHalf + (y * VoxelSize), -VoxelSizeHalf + (z * VoxelSize)));
								Vertices.Add(FVector(-VoxelSizeHalf + (x * VoxelSize), -VoxelSizeHalf + (y * VoxelSize), VoxelSizeHalf + (z * VoxelSize)));

								Normals.Append(bNormals4, UE_ARRAY_COUNT(bNormals5));
								break;
							}
							}

							UVs.Append(bUVs, UE_ARRAY_COUNT(bUVs));

							//FColor color = FColor::RandomColor();

							//FColor color = FColor(RandomStream.FRand() * 256, RandomStream.FRand() * 256, RandomStream.FRand() * 256, rand() % 5);
							FColor color = FColor(255, 255, 255, i);
							VertexColor.Add(color); VertexColor.Add(color); VertexColor.Add(color); VertexColor.Add(color);
						}
					}


					el_num += triangle_num;
					MeshSections[MeshIndex].elementID += triangle_num;	
				}
				else if (MeshIndex == -1) // add grass
				{
					AddInstanceVoxel(FVector(x* VoxelSize, y* VoxelSize, z* VoxelSize));
				}

				else if (MeshIndex == -2) // add flowers
				{
					AddFoliageVoxel(FVector(x* VoxelSize, y* VoxelSize, z* VoxelSize));
				}
			}
		}
	}

	proceduralComponent->ClearAllMeshSections();
	for (int i = 0; i < MeshSections.Num(); i++)
	{
		if (MeshSections[i].Vertices.Num() > 0)
		proceduralComponent->CreateMeshSection(i, MeshSections[i].Vertices, MeshSections[i].Triangles, MeshSections[i].Normals, MeshSections[i].UVs, MeshSections[i].VertexColor, MeshSections[i].Tangents, true); // true/false is for collisions.
	}

	int s = 0;
	while (s < Materials.Num())
	{
		proceduralComponent->SetMaterial(s, Materials[s]);
		s++;
	}
}

TArray<int32> AVoxelActor::CalculateNoise()
{
	TArray<int32> noises;
	noises.Reserve(ChunkLineElementsExt * ChunkLineElementsExt);
	for (int32 y = -1; y <= ChunkLineElements; y++)
		{
			for (int32 x = -1; x <= ChunkLineElements; x++)
			{
				float NoiseValue =
				USimplexNoiseBPLibrary::SimplexNoise2D((ChunkXIndex * ChunkLineElements + x) * 0.01f, (ChunkYIndex * ChunkLineElements + y) * 0.01f) * 4 +
				USimplexNoiseBPLibrary::SimplexNoise2D((ChunkXIndex * ChunkLineElements + x) * 0.01f, (ChunkYIndex * ChunkLineElements + y) * 0.01f) * 8 +
				USimplexNoiseBPLibrary::SimplexNoise2D((ChunkXIndex * ChunkLineElements + x) * 0.004f, (ChunkYIndex * ChunkLineElements + y) * 0.004f) * 16 + 
				FMath::Clamp(USimplexNoiseBPLibrary::SimplexNoise2D((ChunkXIndex * ChunkLineElements + x) * 0.05f, (ChunkYIndex * ChunkLineElements + y) * 0.05f), 0.0f, 5.0f) * 4; //clamp 0-5
				noises.Add(FMath::FloorToInt(NoiseValue));
			}
		}
	return noises;
}

//TArray<int32> AVoxelActor::CalculateNoise_Implementation()
//{
//	TArray<int32> aa;
//	aa.SetNum(ChunkLineElementsP2);
//	return aa;
//}

void AVoxelActor::SetVoxel(FVector localPos, int32 value)
{
	int32 x = localPos.X / VoxelSize + 1;
	int32 y = localPos.Y / VoxelSize + 1;
	int32 z = localPos.Z / VoxelSize;

	int32 index = x + (y * ChunkLineElementsExt) + (z * ChunkLineElementsP2Ext);

	ChunkFields[index] = value;

	UpdateMesh();
}

bool AVoxelActor::inRange(int32 value, int32 range)
{
	return (value >= 0 && value < range); // return true if value is 0 or higher and is lower than range
}

void AVoxelActor::AddInstanceVoxel_Implementation(FVector InstanceLocation)  // grass
{

}

void AVoxelActor::AddFoliageVoxel_Implementation(FVector InstanceLocation)  //  flowers
{

}