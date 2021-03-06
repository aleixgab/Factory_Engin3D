#include "Application.h"

#include "ComponentCamera.h"
#include "GameObject.h"


Camera::Camera(GameObject* gameObject) : Component(gameObject)
{
	frustum.type = PerspectiveFrustum;

	if (gameObject != nullptr && gameObject->transform != nullptr)
	{
		frustum.pos = gameObject->transform->GetPos();
		frustum.front = gameObject->transform->GetRotation() * float3::unitZ;
		frustum.up = gameObject->transform->GetRotation() * float3::unitY;
	}
	else
	{
		frustum.pos = float3::zero;
		frustum.front = float3::unitZ;
		frustum.up = float3::unitY;
	}

	frustum.nearPlaneDistance = 1.0f;
	frustum.farPlaneDistance = 1000.0f;
	frustum.verticalFov = 60.0f * DEGTORAD;
	frustum.horizontalFov = 2.f * atan(tan(frustum.verticalFov * 0.5f) * (SCREEN_WIDTH / SCREEN_HEIGHT));
}

Camera::Camera(GameObject* gameObject, CameraInfo* info) : Component(gameObject)
{
	frustum.type = PerspectiveFrustum;

	frustum.pos = info->position;

	frustum.front = float3::unitZ;
	frustum.up = float3::unitY;

	frustum.nearPlaneDistance = info->nearPlaneDistance;
	frustum.farPlaneDistance = info->farPlaneDistance;
	frustum.verticalFov = info->verticalFov;
	frustum.horizontalFov = info->horizontalFov;
}

void Camera::UpdateFrust()
{
	if (gameObject != nullptr && gameObject->transform != nullptr)
	{
		frustum.pos = gameObject->transform->GetPos();
		frustum.front = gameObject->transform->GetRotation() * float3::unitZ;
		frustum.up = gameObject->transform->GetRotation() * float3::unitY;
	}
	else
	{
		frustum.pos = float3::zero;
		frustum.front = float3::unitZ;
		frustum.up = float3::unitY;
	}
}

void Camera::Inspector()
{
	if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (frustum.type != InvalidFrustum)
		{
			char* camType = frustum.type == PerspectiveFrustum ? "Perspective" : "Orthographic";
 			ImGui::Text("Camera type: ", camType);
			ImGui::DragFloat("Far plane distance", &frustum.farPlaneDistance, 10, frustum.nearPlaneDistance, 2000.0f);
			ImGui::DragFloat("Near plane distance", &frustum.nearPlaneDistance, 1, 0.1, frustum.farPlaneDistance);

			float fov = frustum.verticalFov * RADTODEG;
			if (ImGui::DragFloat("FOV", &fov, 1, 55, 120))
			{
				frustum.verticalFov = fov * DEGTORAD;
				frustum.horizontalFov = 2.f * atan(tan(frustum.verticalFov * 0.5f) * (float(App->window->width) / App->window->height));
			}
		}
		else
		{
			ImGui::Text("Invalid camera type");
		}
	}
}


float4x4 Camera::GetViewMatrix()
{
	float4x4 matrix;

	matrix = frustum.ViewMatrix();
	matrix.Transpose();

	return matrix;
}

float4x4 Camera::GetProjectionMatrix()
{
	float4x4 matrix;

	matrix = frustum.ProjectionMatrix();
	matrix.Transpose();

	return matrix;
}

void Camera::SetPos(float x, float y, float z)
{
	frustum.pos = float3(x, y, z);
}
void Camera::SetPos(float3 pos)
{
	frustum.pos = pos;
}

float3 Camera::GetPos()
{
	return frustum.pos;
}

void Camera::SaveComponent(JSON_Object * parent)
{
	json_object_set_number(parent, "Type", this->type);

	json_object_set_number(parent, "UUID", GetUUID());

	json_object_set_number(parent, "Time Created", GetTime());

	SaveNumberArray(parent, "pos", frustum.pos.ptr(), 3);

	//Pos
	JSON_Value* position = json_value_init_object();
	JSON_Object* positionObj = json_value_get_object(position);

	json_object_set_value(parent, "Position", position);

	json_object_set_number(positionObj, "X", frustum.pos.x);
	json_object_set_number(positionObj, "Y", frustum.pos.y);
	json_object_set_number(positionObj, "Z", frustum.pos.z);
	//

	json_object_set_number(parent, "nearPlaneDistance", frustum.nearPlaneDistance);
	json_object_set_number(parent, "farPlaneDistance", frustum.farPlaneDistance);
	json_object_set_number(parent, "verticalFov", frustum.verticalFov);
	json_object_set_number(parent, "horizontalFov", frustum.horizontalFov);
}