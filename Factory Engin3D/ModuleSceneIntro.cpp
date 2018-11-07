#include "Application.h"
#include "ModuleSceneIntro.h"
#include "ModuleImGui.h"
//#include "ModuleGeometryManager.h"

#include "Geometries.h"

#include "glew-2.1.0/include/GL/glew.h"

#include "Assimp/include/cimport.h"
#include "Assimp/include/scene.h"
#include "Assimp/include/postprocess.h"
#include "Assimp/include/cfileio.h"

#include "ImGuizmo/ImGuizmo.h"
#include <fstream>  

ModuleSceneIntro::ModuleSceneIntro(Application* app, bool start_enabled) : Module(app, start_enabled)
{
}

ModuleSceneIntro::~ModuleSceneIntro()
{}

// Load assets
bool ModuleSceneIntro::Start()
{
	LOG("Loading Intro assets");

	bool ret = true;

	App->camera->Move(float3(1.0f, 1.0f, 0.0f));
	quadtree.Create(AABB(float3(-5, 0, -5), float3(5, 5, 5)));

	return ret;
}

update_status ModuleSceneIntro::PreUpdate(float dt)
{
	update_status status = UPDATE_CONTINUE;

	if (App->input->GetKey(SDL_SCANCODE_1) == KEY_DOWN)
	{
		App->renderer3D->debugQuad = !App->renderer3D->debugQuad;
	}

	if (App->input->GetKey(SDL_SCANCODE_2) == KEY_DOWN)//Empty game object
	{
		float3 pos = math::float3((rand() % 100 )- 50, rand() % 15, (rand() % 100) - 50);

		GameObject* random = App->gameObject->CreateGameObject(pos);

		const float3 center(pos.x, pos.y, pos.z);
		const float3 size(float3::two);

		random->SetABB(AABB::FromCenterAndSize(center, size));

		quadtree.Insert(random);
	}

	if (App->input->GetKey(SDL_SCANCODE_3) == KEY_DOWN)
	{
		App->renderer3D->cameraCulling = !App->renderer3D->cameraCulling;
	}

	return status;
}

// Load assets
bool ModuleSceneIntro::CleanUp()
{	
	return true;
}

// Update
update_status ModuleSceneIntro::Update(float dt)
{
	ImGuizmo::Enable(true);


	//ImGui::PushID(3);
	ImGuiIO& io = ImGui::GetIO();
	ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

	ImGuizmo::Manipulate(App->camera->GetViewMatrix().ptr(), App->camera->GetProjectionMatrix().ptr(), guizOperation, guizMode, App->geometry->currentGameObject->transform->GetMatrix().Transposed().ptr());

	if (App->input->GetKey(SDL_SCANCODE_W) == KEY_DOWN)
		guizOperation = ImGuizmo::OPERATION::TRANSLATE;

	if (App->input->GetKey(SDL_SCANCODE_E) == KEY_DOWN)
		guizOperation = ImGuizmo::OPERATION::SCALE;
	
	if (App->input->GetKey(SDL_SCANCODE_R) == KEY_DOWN)
		guizOperation = ImGuizmo::OPERATION::ROTATE;
	
	if (App->input->GetKey(SDL_SCANCODE_Q) == KEY_DOWN)
		guizOperation = ImGuizmo::OPERATION::BOUNDS;

	return UPDATE_CONTINUE;
}

update_status ModuleSceneIntro::PostUpdate(float dt)
{
	return UPDATE_CONTINUE;
}