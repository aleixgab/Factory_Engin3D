#include "Application.h"
#include "ModuleGeometryManager.h"

#include "glew-2.1.0/include/GL/glew.h"

#include "Assimp/include/cimport.h"
#include "Assimp/include/scene.h"
#include "Assimp/include/postprocess.h"
#include "Assimp/include/cfileio.h"

#include "DevIL/includex86/IL/il.h"
#include "DevIL/includex86/IL/ilu.h"
#include "DevIL/includex86/IL/ilut.h"


#pragma comment( lib, "DevIL/libx86/DevIL.lib" )
#pragma comment( lib, "DevIL/libx86/ILU.lib" )
#pragma comment( lib, "DevIL/libx86/ILUT.lib" )

#include "GameObject.h"



ModuleGeometry::ModuleGeometry(Application* app, bool start_enabled) : Module(app, start_enabled)
{
}

ModuleGeometry::~ModuleGeometry()
{}

// Load assets
bool ModuleGeometry::Start()
{
	LOG("Loading Geometry manager");


	ilutRenderer(ILUT_OPENGL);
	ilInit();
	iluInit();
	ilutInit();
	ilutRenderer(ILUT_OPENGL);
	bool ret = true;


	LoadDefaultScene();
	
	return ret;
}


update_status ModuleGeometry::PreUpdate(float dt)
{
	update_status status = UPDATE_CONTINUE;

	return status;
}

// Load assets
bool ModuleGeometry::CleanUp()
{
	return true;
}

Mesh* ModuleGeometry::LoadMesh(char* path)
{
	Mesh* mesh = nullptr;
	if (path != nullptr)
	{
		char* filePath = path;
		const aiScene* scene = aiImportFile(filePath, aiProcessPreset_TargetRealtime_MaxQuality);

		if (scene != nullptr)
		{
			if (scene->HasMeshes())
			{
				mesh = new Mesh();
				numFaces = 0u;

				for (int i = 0; i < scene->mNumMeshes; ++i)
				{
					aiMesh* currentMesh = scene->mMeshes[i];

					MeshBuffer newCurrentBuffer;
					newCurrentBuffer.vertex.size = currentMesh->mNumVertices * 3;
					newCurrentBuffer.vertex.buffer = new float[newCurrentBuffer.vertex.size];

					memcpy(newCurrentBuffer.vertex.buffer, currentMesh->mVertices, sizeof(float) * newCurrentBuffer.vertex.size);

					for (int aaa = 0; aaa < newCurrentBuffer.vertex.size / 3; ++aaa)
					{
						LOG("X = %.2f, Y = %.2f, Z = %.2f", newCurrentBuffer.vertex.buffer[aaa * 3 + 0], newCurrentBuffer.vertex.buffer[aaa * 3 + 1], newCurrentBuffer.vertex.buffer[aaa * 3 + 2]);
					}

					LOG("New mesh loaded with %d vertices", newCurrentBuffer.vertex.size);

					glGenBuffers(1, (GLuint*)&(newCurrentBuffer.vertex.id));
					glBindBuffer(GL_ARRAY_BUFFER, newCurrentBuffer.vertex.id);
					glBufferData(GL_ARRAY_BUFFER, sizeof(float) * newCurrentBuffer.vertex.size, newCurrentBuffer.vertex.buffer, GL_STATIC_DRAW);

					if (currentMesh->HasFaces())
					{

						numFaces += currentMesh->mNumFaces;
						newCurrentBuffer.index.size = currentMesh->mNumFaces * 3;
						newCurrentBuffer.index.buffer = new uint[newCurrentBuffer.index.size];

						for (uint index = 0; index < currentMesh->mNumFaces; ++index)
						{
							if (currentMesh->mFaces[index].mNumIndices != 3)
								LOG("WARNING, geometry faces != 3 indices")
							else
							{
								LOG("X = %i, Y = %i, Z = %i", currentMesh->mFaces[index].mIndices[0], currentMesh->mFaces[index].mIndices[1], currentMesh->mFaces[index].mIndices[2]);
								memcpy(&newCurrentBuffer.index.buffer[index * 3], currentMesh->mFaces[index].mIndices, sizeof(uint) * 3);
							}
						}
						glGenBuffers(1, (GLuint*)&(newCurrentBuffer.index.id));
						glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, newCurrentBuffer.index.id);
						glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint) * newCurrentBuffer.index.size, newCurrentBuffer.index.buffer, GL_STATIC_DRAW);
					}

					if (currentMesh->HasTextureCoords(0))
					{
						float* textCoords = new float[currentMesh->mNumVertices * 2];
						for (int currVertices = 0; currVertices < currentMesh->mNumVertices; ++currVertices)
						{
							textCoords[currVertices * 2] = currentMesh->mTextureCoords[0][currVertices].x;
							textCoords[currVertices * 2 + 1] = currentMesh->mTextureCoords[0][currVertices].y;
						}

						glGenBuffers(1, &newCurrentBuffer.texture.id);
						glBindBuffer(GL_ARRAY_BUFFER, newCurrentBuffer.texture.id);
						glBufferData(GL_ARRAY_BUFFER, currentMesh->mNumVertices * sizeof(float) * 2, textCoords, GL_STATIC_DRAW);
						glBindBuffer(GL_ARRAY_BUFFER, 0);


						delete[] textCoords;
					}


					mesh->buffers.push_back(newCurrentBuffer);
				}

				currentMeshBB = LoadBoundingBox(mesh);
				LOG("Loaded geometry with %i faces", numFaces);
			}

			aiReleaseImport(scene);
		}

		else
			LOG("Error loading geometry %s", filePath);

	}
	return mesh;
}

void ModuleGeometry::UpdateMesh(char* path)
{
	Mesh* tempMesh = LoadMesh(path);
	if (tempMesh != nullptr)
		if (!tempMesh->buffers.empty())
		{
			Mesh* mesh = (Mesh*)currentMesh->GetComponent(ComponentType::ComponentType_GEOMETRY);
			currentMesh->RemoveComponent(mesh);

			MeshInfo info;
			info.mesh = tempMesh;
			currentMesh->AddComponent(ComponentType_GEOMETRY, &info);
		}
}

AABB* ModuleGeometry::LoadBoundingBox(Mesh* mesh)
{
	AABB* boundingBox = nullptr;
	if (mesh != nullptr)
	{
		float3 max, min;
		std::vector<MeshBuffer>::iterator iterator = mesh->buffers.begin();
		max.x = (*iterator).vertex.buffer[0];
		max.y = (*iterator).vertex.buffer[1];
		max.z = (*iterator).vertex.buffer[2];

		min = max;


		for (iterator; iterator != mesh->buffers.end(); ++iterator)
		{
			int vertexSize = (*iterator).vertex.size / 3;
			float* buffer = (*iterator).vertex.buffer;
			for (int i = 0; i < vertexSize; ++i)
			{
				Higher(max.x, buffer[i * 3]);
				Higher(max.y, buffer[i * 3 + 1]);
				Higher(max.z, buffer[i * 3 + 2]);

				Lower(min.x, buffer[i * 3]);
				Lower(min.y, buffer[i * 3 + 1]);
				Lower(min.z, buffer[i * 3 + 2]);
			}
		}

		boundingBox = new AABB(min, max);

		App->camera->Position = CalcBBPos(boundingBox);
		App->camera->Look(CalcBBPos(boundingBox), mesh->GetPos(), false);
	}
	return boundingBox;
}

float3 ModuleGeometry::CalcBBPos(math::AABB* boundingBox) const
{
	float3 distance{ 0,0,0 };
	if (boundingBox != nullptr)
	{
		float3 size = boundingBox->Size();

		float reScale = 1.25;
		distance.x = (size.x / 2) / math::Tan(0.33333333333 * reScale);
		distance.y = (size.y / 2) / math::Tan(0.33333333333 * reScale);
		distance.z = (size.z / 2) / math::Tan(0.33333333333 * reScale);
	}
	return distance;
}

float3 ModuleGeometry::GetBBPos() const
{
	float3 distance{ 0,0,0 };
	if (currentMeshBB != nullptr)
	{
		float3 size = currentMeshBB->Size();

		float reScale = 1.25;
		distance.x = (size.x / 2) / math::Tan(0.33333333333 * reScale);
		distance.y = (size.y / 2) / math::Tan(0.33333333333 * reScale);
		distance.z = (size.z / 2) / math::Tan(0.33333333333 * reScale);
	}
	return distance;
}

float3 ModuleGeometry::GetCurrentMeshPivot() const
{
	return currentMesh->GetPos();
}


void ModuleGeometry::Higher(float& val1, float val2)
{
	val1 = val1 > val2 ? val1 : val2;
}

void ModuleGeometry::Lower(float& val1, float val2)
{
	val1 = val1 < val2 ? val1 : val2;
}

//Geometry* ModuleGeometry::LoadPrimitive(PrimitiveTypes type)
//{
//	//TODO
//	return nullptr;
//}

uint ModuleGeometry::LoadTexture(char* path) const
{
	bool isSuccess = true;
	uint newTextureID = 0;

	ilGenImages(1, &newTextureID);
	ilBindImage(newTextureID);

	if ((bool)ilLoadImage(path))
	{
		ILinfo info;
		iluGetImageInfo(&info);
		if (info.Origin == IL_ORIGIN_UPPER_LEFT)
		{
			iluFlipImage();
		}

		if (ilConvertImage(IL_RGB, IL_UNSIGNED_BYTE))
		{
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

			glGenTextures(1, &newTextureID);
			glBindTexture(GL_TEXTURE_2D, newTextureID);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

			glTexImage2D(GL_TEXTURE_2D, 0, ilGetInteger(IL_IMAGE_FORMAT), ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT),
				0, ilGetInteger(IL_IMAGE_FORMAT), GL_UNSIGNED_BYTE, ilGetData());
		}
		else
		{
			isSuccess = false;
			LOG("Image conversion error %s", iluErrorString(ilGetError()));
		}
	}
	else
	{
		isSuccess = false;
		LOG("Error loading texture %s", iluErrorString(ilGetError()));
	}

	ilDeleteImages(1, &newTextureID);

	if (!isSuccess)
		newTextureID = 0;

	return newTextureID;
}

void ModuleGeometry::UpdateTexture(char* path)
{
	uint tempTexture = LoadTexture(path);
	if (tempTexture != 0)
	{
		if (textureID != 0)
		{
			glDeleteTextures(1, &textureID);
		}
		textureID = tempTexture;
	}
}


// Update
update_status ModuleGeometry::Update(float dt)
{
	return UPDATE_CONTINUE;
}

update_status ModuleGeometry::PostUpdate(float dt)
{
	return UPDATE_CONTINUE;
}

void ModuleGeometry::Draw3D(bool fill, bool wire) const
{
	//PrimitivePlane plane;
	//plane.color = { 1, 1, 1, 1 };
	//plane.axis = true;
	//plane.Render();

	if (currentMesh != nullptr)
	{
		Mesh* currentGeometry = (Mesh*)currentMesh->GetComponent(ComponentType_GEOMETRY);
		if (currentGeometry)
		{
			currentGeometry->fill = true;
			currentGeometry->wire = false;
			currentGeometry->Render();
		}
	}
}

void ModuleGeometry::LoadDefaultScene()
{
	currentMesh = new GameObject(App->gameObject->root);
	MeshInfo info;
	info.mesh = LoadMesh("assets/models/BakerHouse.fbx");
	currentMesh->AddComponent(ComponentType::ComponentType_GEOMETRY, &info);
	textureID = LoadTexture("assets/textures/Baker_house.dds");
}

ModuleGameObjectManager::ModuleGameObjectManager(Application * app, bool start_enabled) : Module(app, start_enabled)
{
}

ModuleGameObjectManager::~ModuleGameObjectManager()
{

}

bool ModuleGameObjectManager::Start()
{
	root = new GameObject(float3::zero, Quat::identity, float3::one);

	return true;
}

update_status ModuleGameObjectManager::PreUpdate(float dt)
{
	return UPDATE_CONTINUE;
}

update_status ModuleGameObjectManager::Update(float dt)
{
	for (list<GameObject*>::iterator iterator = gameObjects.begin(); iterator != gameObjects.end(); ++iterator)
	{
		(*iterator)->Update(dt);
	}

	return UPDATE_CONTINUE;
}

update_status ModuleGameObjectManager::PostUpdate(float dt)
{
	for (list<GameObject*>::iterator iterator = gameObjects.begin(); iterator != gameObjects.end(); ++iterator)
	{
		if ((*iterator)->toDelete)
		{
			delete (*iterator);
		}
	}
	return UPDATE_CONTINUE;
}

bool ModuleGameObjectManager::CleanUp()
{
	CleanAllGameObjects();

	return true;
}

void ModuleGameObjectManager::CleanAllGameObjects()
{
	// Deleting a GameObject will cause all his childs to be deleted
	// If we delete the root GameObject, all the GameObjects will be deletet recursively
	if (root != nullptr)
	{
		delete root;
		root = nullptr;
	}
}


GameObject* ModuleGameObjectManager::CreateGameObject(float3 position, Quat rotation, float3 scale, GameObject* father)
{
	GameObject* newGameObject = nullptr;

	if (father == nullptr)
		father = root;

	newGameObject = new GameObject(position, rotation, scale, father);

	return newGameObject;
}

GameObject* ModuleGameObjectManager::CreateGameObject(TransformInfo* info, GameObject* father)
{
	GameObject* newGameObject = nullptr;

	if (father == nullptr)
		father = root;

	newGameObject = new GameObject(info, father);

	return newGameObject;
}

GameObject* ModuleGameObjectManager::CreateGameObject(float4x4 transform, GameObject* father)
{
	GameObject* newGameObject = nullptr;

	if (father == nullptr)
		father = root;

	newGameObject = new GameObject(transform, father);

	return newGameObject;
}

