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
	ChunkLineElementsP2 = ChunkLineElements * ChunkLineElements;
	ChunkTotalElements = ChunkLineElementsP2 * ChunkZElements;
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
	ChunkFields.SetNumUninitialized(ChunkTotalElements);

	TArray <int32> noise = calculateNoise();

	for (int x = 0; x < ChunkLineElements; x++)
	{
		for (int y = 0; y < ChunkLineElements; y++)
		{
			for (int z = 0; z < ChunkZElements; z++)
			{
				int32 index = x + (y * ChunkLineElements) + (z * ChunkLineElementsP2);

				if (z == 30 + noise[x + y * ChunkLineElements]) ChunkFields[index] = 1;
				else if (z == 29 + noise[x + y * ChunkLineElements]) ChunkFields[index] = 2;
				else if (z < 29 + noise[x + y * ChunkLineElements]) ChunkFields[index] = 3;
				else ChunkFields[index] = 0;
				// if equal to 30, do voxel with 2nd material in material array. 
				 // if over 30, do voxel with 1st.
				// eventually would be better to have as exposed pins to adjust in the actor BP!
			}
		}
	}
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
				int32 index = x + (y * ChunkLineElements) + (z * ChunkLineElementsP2);
				int32 MeshIndex = ChunkFields[index];

				if (MeshIndex > 0)
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
						int newIndex = index + bMask[i].X + (bMask[i].Y * ChunkLineElements) + (bMask[i].Z * ChunkLineElementsP2);
						bool flag = false;
						if (MeshIndex >= 20) flag = true;


						else if ((x + bMask[i].X < ChunkLineElements) && (x + bMask[i].X >= 0) && (y + bMask[i].Y < ChunkLineElements) && (y + bMask[i].Y >= 0))
						{
							if (newIndex < ChunkFields.Num() && newIndex >= 0)
								//if (ChunkFields[newIndex] >= 0) flag = true;
								if (ChunkFields[newIndex] < 1 ) flag = true;
								// Possibly not flagging properly. Tried multiple variations. None work.
						}

						else flag = true;

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

TArray<int32> AVoxelActor::calculateNoise_Implementation()
{
	TArray<int32> aa;
	aa.SetNum(ChunkLineElementsP2);
	return aa;
}

void AVoxelActor::SetVoxel(FVector localPos, int32 value)
{
	int32 x = localPos.X / VoxelSize;
	int32 y = localPos.Y / VoxelSize;
	int32 z = localPos.Z / VoxelSize;

	int32 index = x + (y * ChunkLineElements) + (z * ChunkLineElementsP2);

	ChunkFields[index] = value;

	UpdateMesh();
}