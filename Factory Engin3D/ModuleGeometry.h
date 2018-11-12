#ifndef __ModuleGeometryManager_H__
#define __ModuleGeometryManager_H__
#include "Module.h"

#include "Transform.h"
#include "Geometries.h"

#include "Assimp/include/matrix4x4.h"

#include <vector>

struct aiMesh;
struct aiScene;
struct aiNode;

class ModuleGeometry : public Module
{
public:
	ModuleGeometry(Application* app, bool start_enabled = true);
	~ModuleGeometry();

	bool Start();
	void LoadDefaultScene();
	update_status PostUpdate();

	void Draww(GameObject * object);

	bool CleanUp();

	void DistributeFile(char * file);

	MeshNode LoadMeshBuffer(const aiScene * scene, uint index, char* path);

	void LoadMeshTextureCoords(MeshBuffer &buffer, aiMesh* newMesh);

	void LoadMeshIndex(aiMesh* newMesh, MeshBuffer &buffer);

	void LoadMeshVertex(MeshBuffer &buffer, aiMesh* newMesh);

	MeshNode LoadMeshNode(const aiScene* scene, aiNode* node, char* path);

	float4x4 AiNatrixToFloatMat(const aiMatrix4x4 &aiMat);

	MeshNode LoadMesh(char* path);

	void SaveMeshImporter(MeshBuffer newCurrentBuffer, const char * path, uint uuid);

	vector<MeshBuffer*> LoadMeshImporter(const char * path, const vector<MeshNode>& nodes);

	GameObject* LoadGameObjectsFromMeshNode(MeshNode node, GameObject * father);

	GameObject * LoadEmptyGameObjectsFromMeshNode(MeshNode node, GameObject * father);

	void UpdateMesh(char * path);

	void SaveGameObjectJson(GameObject* object, JSON_Object* parent);

	AABB LoadBoundingBox(Buffer<float> vertex);

	float3 CalcBBPos(math::AABB* boundingBox) const;

	float3 GetBBPos() const;

	float3 GetCurrentMeshPivot() const;

	inline void Higher(float& val1, float val2);

	inline void Lower(float& val1, float val2);

	//Geometry* LoadPrimitive(PrimitiveTypes type);

	uint LoadTexture(char* path) const;

	void UpdateTexture(char* path);

public:
	GameObject* currentGameObject = nullptr;
	GameObject* bHouse = nullptr;

	vector<MeshNode> nodes;
	vector<MeshBuffer*> loadedMeshes;

	uint textureID = 0;
	uint numFaces = 0;

	char* droppedFileDir = nullptr;

	AABB currentMeshBB = AABB(float3::zero, float3::zero);

	string destination;

	GameObject* plane = nullptr;
};
#endif // !__ModuleGeometryManager_H__