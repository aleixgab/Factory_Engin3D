#include "GameObject.h"
#include "pcg-c-basic-0.9/pcg_basic.h"


GameObject::GameObject(GameObject* father, const char * name)
{
	TransformInfo* info = new TransformInfo();
	info->position = float3::zero;
	info->rotation = Quat::identity;
	info->scale = float3::one;
	info->whichInfo = UsingInfo_TRS;

	CreateGameObject(info);
	delete info;

	this->father = father;

	if (name != nullptr)
		this->name = name;
	else this->name = "noName";
}

GameObject::GameObject(float3 position, Quat rotation, float3 scale, GameObject* father, const char* name)
{
	TransformInfo* info = new TransformInfo();
	info->position = position;
	info->rotation = rotation;
	info->scale = scale;
	info->whichInfo = UsingInfo_TRS;

	CreateGameObject(info);
	delete info;

	this->father = father;

	if (name != nullptr)
		this->name = name;
	else this->name = "noName";
}


GameObject::~GameObject()
{
	transform = nullptr;

	RemoveComponents();

	RemoveChilds();
}

void GameObject::RemoveChilds()
{
	for (list<GameObject*>::iterator iterator = childs.begin(); iterator != childs.end(); ++iterator)
	{
		delete (*iterator);
	}
	childs.clear();
}

void GameObject::RemoveComponents()
{
	for (list<Component*>::iterator iterator = components.begin(); iterator != components.end(); ++iterator)
	{
		delete (*iterator);
	}
	components.clear();
}



void GameObject::RealDelete()
{
	transform = nullptr;

	if (father)
		father->childs.remove(this);

	father = nullptr;
	//delete this;
}



void GameObject::Update(float dt)
{
	if (isActive)
	{
		for (list<Component*>::iterator iterator = components.begin(); iterator != components.end(); ++iterator)
		{
			(*iterator)->Update(dt);
		}

		for (list<GameObject*>::iterator iterator = childs.begin(); iterator != childs.end(); ++iterator)
		{
			(*iterator)->Update(dt);
		}
	}
}

Component* GameObject::GetComponent(ComponentType type)
{
	Component* component = nullptr;

	for (list<Component*>::iterator iterator = components.begin(); iterator != components.end(); ++iterator)
	{
		if ((*iterator)->type == type)
		{
			component = (*iterator);
			break;
		}
	}

	return component;
}

list<Component*> GameObject::GetAllComponent(ComponentType type)
{
	list<Component*> component;

	for (list<Component*>::iterator iterator = components.begin(); iterator != components.end(); ++iterator)
	{
		if ((*iterator)->type == type)
		{
			components.push_back(*iterator);
		}
	}

	return component;
}

bool GameObject::HasComponent(ComponentType type)
{
	bool ret = false;
	for (list<Component*>::iterator iterator = components.begin(); iterator != components.end(); ++iterator)
	{
		if ((*iterator)->type == type)
		{
			ret = true;
			break;
		}
	}
	return ret;
}

void GameObject::RemoveComponent(Component* component)
{
	if (component != nullptr)
	{
		components.remove(component);
		delete component;
		component = nullptr;
	}
}


void GameObject::CreateGameObject(TransformInfo* info)
{
	UID = pcg32_random();

	this->transform = (Transform*)AddComponent(ComponentType_TRANSFORM, info);
	if(transform)
		transform->SetUUID(pcg32_random());
}

Component* GameObject::AddComponent(ComponentType type, ComponentInfo* info)
{
	Component* newComponent = nullptr;

	if (info)
		info->gameObject = this;

	switch (type)
	{
	case ComponentType_TRANSFORM:
		if (info)
			newComponent = (Component*)new Transform((TransformInfo*)info);
		break;
	case ComponentType_GEOMETRY:
		if (info)
			newComponent = (Component*)(((GeometryInfo*)info)->geometry);
		break;
	case ComponentType_CAMERA:
		newComponent = (Component*)new Camera(this);
		break;
	case ComponentType_TEXTURE:
		break;
	case ComponentType_LIGHT:
		break;
	default:
		break;
	}

	if (newComponent)
	{
		newComponent->type = type;
		newComponent->isActive = true;
		newComponent->gameObject = this;
		components.push_back(newComponent);
	}
	return newComponent;
}

float3 GameObject::GetPos() const
{
	if (transform)
		return transform->GetPos();

	return float3::zero;
}

float3 GameObject::GetGlobalPos() const
{
	if (transform)
		return transform->GetGlobalPos();

	return float3::zero;
}

float3 GameObject::GetScale() const
{
	if (transform)
		return transform->GetScale();

	return float3::one;
}

float3 GameObject::GetGlobalScale() const
{
	if (transform)
		return transform->GetGlobalScale();

	return float3::one;
}

float4x4 GameObject::GetGlobalMatrix() const
{
	float4x4 mat = float4x4::identity;
	if (transform)
	{
		mat = transform->GetMatrix();
	}

	return mat;
}

Quat GameObject::GetRotation() const
{
	if (transform)
		return transform->GetRotation().Normalized();
}

Quat GameObject::GetGlobalRotation() const
{
	if (transform)
		return transform->GetGlobalRotation().Normalized();
}

void GameObject::SetTransform(float4x4 trans)
{
	if (transform)
	{
		transform->SetTransform(trans);
	}
}

void GameObject::SetPos(float3 pos)
{
	if (transform)
	{
		transform->SetPos(pos);
	}
}


void GameObject::Move(float3 movement)
{
	if (transform)
		transform->Move(movement);
}

void GameObject::SetScale(float3 scale)
{
	if (transform)
		transform->SetScale(scale);
}

void GameObject::Scale(float3 scale)
{
	if (transform)
		transform->Scale(scale);
}

void GameObject::SetRotation(Quat rotation)
{
	if (transform)
		transform->SetRotation(rotation);
}

void GameObject::Rotate(Quat rotation)
{
	if (transform)
		transform->Rotate(rotation);
}

void GameObject::SetIdentity()
{
	if (transform)
	{
		transform->SetIdentity();
		transform->UpdateBoundingBox();
	}
}

const AABB* GameObject::GetAABB() const
{
	return &transform->boundingBox;
}

float3 GameObject::GetBBPos() const
{
	float3 distance{ 0,0,0 };
	if (transform)
	{
		float3 size = transform->boundingBox.Size();

		float3 reScale = GetScale();
		distance.x = (size.x / 2) / math::Tan((0.3333333 * reScale.x));
		distance.y = (size.y / 2) / math::Tan((0.3333333 * reScale.y));
		distance.z = (size.z / 2) / math::Tan((0.3333333 * reScale.z));
	}
	return distance + GetPos();
}

void GameObject::SetABB(AABB aabb)
{
	if (transform)
	{
		transform->originalBoundingBox = aabb;

		transform->UpdateBoundingBox();
	}
}

void GameObject::SetActive(bool active)
{
	for (list<GameObject*>::iterator iterator = childs.begin(); iterator != childs.end(); ++iterator)
	{
		(*iterator)->SetActive(active);
	}
	isActive = active;
}

void GameObject::SetObjectStatic(bool isStatic)
{
	for (list<GameObject*>::iterator iterator = childs.begin(); iterator != childs.end(); ++iterator)
	{
		(*iterator)->SetObjectStatic(isStatic);
	}
	this->isStatic = isStatic;
}

int GameObject::CreateRandomUID()
{
	return 0;
}

