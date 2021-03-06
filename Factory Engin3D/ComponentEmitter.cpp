#include "Application.h"

#include "ComponentEmitter.h"
#include "Application.h"
#include "ModuleTime.h"

#include <vector>
#include "pcg-c-basic-0.9/pcg_basic.h"

#include "ModuleParticles.h"

ComponentEmitter::ComponentEmitter(GameObject* gameObject) : Component(gameObject, ComponentType_EMITTER)
{
	timer.Start();
	burstTime.Start();
	App->sceneIntro->octree.Insert(gameObject);
}

ComponentEmitter::ComponentEmitter(GameObject* gameObject, EmitterInfo* info) : Component(gameObject, ComponentType_EMITTER)
{
	if (info)
	{
		duration = info->duration;

		loop = info->loop;

		burst = info->burst;
		minPart = info->minPart;
		maxPart = info->maxPart;
		repeatTime = info->repeatTime;

		// posDifAABB
		posDifAABB = info->posDifAABB;
		gravity = info->gravity;

		// boxCreation
		boxCreation = info->boxCreation;
		// SphereCreation
		sphereCreation.r = info->SphereCreationRad;

		circleCreation.r = info->circleCreationRad;

		normalShapeType = info->shapeType;
		texture = info->texture;

		startValues = info->startValues;

		checkLife = info->checkLife;
		checkSpeed = info->checkSpeed;
		checkAcceleration = info->checkAcceleration;
		checkSize = info->checkSize;
		checkRotation = info->checkRotation;
		checkAngularAcceleration = info->checkAngularAcceleration;
		checkSizeOverTime = info->checkSizeOverTime;
		checkAngularVelocity = info->checkAngularVelocity;

		isParticleAnimated = info->isParticleAnimated;
		if (isParticleAnimated)
		{
			textureRows = info->textureRows;
			textureColumns = info->textureColumns;
			animationSpeed = info->animationSpeed;
		}
		dieOnAnimation = info->dieOnAnimation;

		drawAABB = info->drawAABB;

		isSubEmitter = info->isSubEmitter;
		subEmitter = info->subEmitter;
		subEmitterUUID = info->subEmitterUUID;

		rateOverTime = info->rateOverTime;

		startValues.subEmitterActive = info->subEmitterActive;

		if (gameObject && gameObject->transform)
		{
			gameObject->transform->originalBoundingBox = AABB::FromCenterAndSize(info->posDifAABB, info->sizeOBB);
			gameObject->transform->UpdateBoundingBox();
		}


	}
	gameObject->transform->UpdateBoundingBox();
	SetNewAnimation(textureRows, textureColumns);
	App->sceneIntro->octree.Insert(gameObject);
}


ComponentEmitter::~ComponentEmitter()
{
	if (App)
	{
		App->time->gameTimerList.remove(&timer);
		App->time->gameTimerList.remove(&burstTime);
		App->time->gameTimerList.remove(&loopTimer);
		App->time->gameTimerList.remove(&timeSimulating);

		App->particle->RemoveEmitter(this);
	}			
	ClearEmitter();
}

void ComponentEmitter::StartEmitter()
{
	if (!isSubEmitter)
	{
		timer.Start();
		burstTime.Start();
		loopTimer.Start();

		timeToParticle = 0.0f;
	}
}

void ComponentEmitter::ChangeGameState(GameState state)
{
	simulatedGame = state;
	if (state == GameState_PLAYING)
		state = GameState_NONE;
	else if (state == GameState_STOP)
		ClearEmitter();

	if (subEmitter && subEmitter->HasComponent(ComponentType_EMITTER))
		((ComponentEmitter*)(subEmitter->GetComponent(ComponentType_EMITTER)))->ChangeGameState(state);
}


void ComponentEmitter::Update()
{
	if (rateOverTime > 0)
	{
		float time = timer.ReadSec();
		if (time > timeToParticle && (loop || loopTimer.ReadSec() < duration))
		{
			if (App->time->gameState == GameState_PLAYING || simulatedGame == GameState_PLAYING || App->time->gameState == GameState_TICK)
			{
				int particlesToCreate = (time / (1.0f / rateOverTime));
				CreateParticles(particlesToCreate, normalShapeType,float3::zero);

				timeToParticle = (1.0f / rateOverTime);
				
				timer.Start();
			}

		}
	}
	float burstT = burstTime.ReadSec();
	if (burst && burstT > repeatTime)
	{
		if (App->time->gameState == GameState_PLAYING || simulatedGame == GameState_PLAYING || App->time->gameState == GameState_TICK)
		{
			int particlesToCreate = minPart;
			if (minPart != maxPart)
				particlesToCreate = (rand() % (maxPart - minPart)) + minPart;
			CreateParticles(particlesToCreate, burstType, float3::zero);
			//LOG("%i", particlesToCreate);
		}
		burstTime.Start();
	}

	//Used for SubEmitter. Create particles from ParticleEmiter death (On Emiter update because need to resize before Particle update)
	if (!newPositions.empty())
	{
		for (std::list<float3>::const_iterator iterator = newPositions.begin(); iterator != newPositions.end(); ++iterator)
		{
			CreateParticles(rateOverTime, normalShapeType, *iterator);
		}

		newPositions.clear();
	}

	// Use this condition to remove all particles from the component Emitter
	if (!emitterActive)
	{
		for (std::list<Particle*>::iterator iterator = particles.begin(); iterator != particles.end(); ++iterator)
		{
			(*iterator)->active = false;
		}

		particles.clear();
	}
}

void ComponentEmitter::ClearEmitter()
{
	for (std::list<Particle*>::iterator iterator = particles.begin(); iterator != particles.end(); ++iterator)
	{
		(*iterator)->active = false;
		(*iterator)->owner = nullptr;
	}

	App->particle->activeParticles -= particles.size();

	particles.clear();
}

void ComponentEmitter::SoftClearEmitter()
{
	App->particle->activeParticles -= particles.size();

	particles.clear();
}


void ComponentEmitter::CreateParticles(int particlesToCreate, ShapeType shapeType, const float3& pos)
{
	if (particlesToCreate == 0)
		++particlesToCreate;

	for (int i = 0; i < particlesToCreate; ++i)
	{
		int particleId = 0;
		if (App->particle->GetParticle(particleId))
		{
			float3 spawnPos = pos;
			spawnPos += RandPos(shapeType);

			App->particle->allParticles[particleId].SetActive(spawnPos, startValues, &texture, &particleAnimation.textureIDs, animationSpeed);

			App->particle->allParticles[particleId].owner = this;
			particles.push_back(&App->particle->allParticles[particleId]);
		}
		else
			break;
	}
}

float3 ComponentEmitter::RandPos(ShapeType shapeType)
{
	float3 spawn = float3::zero;
	float angle = 0.0f;
	float centerDist = 0.0f;

	switch (shapeType)
	{
	case ShapeType_BOX:
		spawn = boxCreation.RandomPointInside(App->randomMath);
		startValues.particleDirection = (float3::unitY * gameObject->transform->GetRotation().ToFloat3x3()).Normalized();
		break;

	case ShapeType_SPHERE:
		spawn = sphereCreation.RandomPointInside(App->randomMath);
		startValues.particleDirection = spawn.Normalized();
		break;

	case ShapeType_SPHERE_CENTER:
		startValues.particleDirection = sphereCreation.RandomPointInside(App->randomMath).Normalized();
		break;

	case ShapeType_SPHERE_BORDER:
		spawn = sphereCreation.RandomPointOnSurface(App->randomMath);
		startValues.particleDirection = spawn.Normalized();
		break;

	case ShapeType_CONE:

		angle = (2*pi) * pcg32_random() / MAXUINT;
		centerDist = (float)pcg32_random() / MAXUINT;

		circleCreation.pos = (float3::unitY * gameObject->transform->GetRotation().ToFloat3x3()).Normalized();
		circleCreation.normal = -circleCreation.pos;
		startValues.particleDirection = (circleCreation.GetPoint(angle,centerDist)).Normalized();
		break;
	default:
		break;
	}

	float3 global = float3::zero;
	if (gameObject)
		global = gameObject->GetGlobalPos();

	return spawn + global;
}

void ComponentEmitter::Inspector()
{
	if (ImGui::CollapsingHeader("Particle System", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ParticleValues();

		ParticleShape();

		ParticleColor();

		ParticleBurst();

		ParticleAABB();

		ParticleTexture();

		ParticleSubEmitter();

		if (ImGui::Button("Remove Particles", ImVec2(150, 25)))
			toDelete = true;
	}
}

void ComponentEmitter::ParticleValues()
{
	if (ImGui::CollapsingHeader("Particle Values", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::ShowHelpMarker("Active checkBox if you want a random number");

		ImGui::Checkbox("##Speed", &checkSpeed);
		ShowFloatValue(startValues.speed, checkSpeed, "Speed", 0.25f, 0.25f, 20.0f);

		ImGui::Checkbox("##Acceleration", &checkAcceleration);
		ShowFloatValue(startValues.acceleration, checkAcceleration, "Acceleration", 0.25f, -5.0f, 5.0f);

		ImGui::Checkbox("##Rotation", &checkRotation);
		ShowFloatValue(startValues.rotation, checkRotation, "Initial Rotation", 0.25f, -360.0f, 360.0f);

		ImGui::Checkbox("##AngularVelocity", &checkAngularVelocity);
		ShowFloatValue(startValues.angularVelocity, checkAngularVelocity, "Angular Velocity", 0.25f, -45.0f, 45.0f);

		ImGui::Checkbox("##AngularAcceleration", &checkAngularAcceleration);
		ShowFloatValue(startValues.angularAcceleration, checkAngularAcceleration, "Angular Acceleration", 0.25f, -45.0f, 45.0f);

		ImGui::Checkbox("##Lifetime", &checkLife);
		ShowFloatValue(startValues.life, checkLife, "Lifetime", 0.5f, 1.0f, 20.0f);

		ImGui::Checkbox("##Size", &checkSize);
		ShowFloatValue(startValues.size, checkSize, "Size", 0.1f, 0.1f, 5.0f);

		ImGui::Checkbox("##SizeOverTime", &checkSizeOverTime);
		ShowFloatValue(startValues.sizeOverTime, checkSizeOverTime, "SizeOverTime", 0.25f, -1.0f, 1.0f);

		ImGui::DragInt("Emition", &rateOverTime, 1.0f, 0.0f, 300.0f, "%.2f");

		ImGui::Separator();
		if (ImGui::Checkbox("Loop", &loop))
			loopTimer.Start();
		ImGui::DragFloat("Duration", &duration, 0.5f, 0.5f, 20.0f, "%.2f");
	}
}

void ComponentEmitter::ParticleShape()
{
	if (ImGui::CollapsingHeader("Particle Shape"))
	{
		ImGui::Separator();
		if (ImGui::BeginMenu("Change Shape"))
		{
			if (ImGui::MenuItem("Box"))
				normalShapeType = ShapeType_BOX;
			else if (ImGui::MenuItem("Sphere"))
				normalShapeType = ShapeType_SPHERE;
			else if (ImGui::MenuItem("Cone"))
				normalShapeType = ShapeType_CONE;
			ImGui::End();
		}


		float3 pos;
		switch (normalShapeType)
		{
		case ShapeType_BOX:
			ImGui::Text("Box");
			pos = boxCreation.Size();
			ImGui::DragFloat3("Box Size", &pos.x, 0.1f, 0.1f, 20.0f, "%.2f");

			boxCreation.SetFromCenterAndSize(boxCreation.CenterPoint(), pos);

			break;
		case ShapeType_SPHERE:
		case ShapeType_SPHERE_BORDER:
		case ShapeType_SPHERE_CENTER:
			ImGui::Text("Sphere");

			ImGui::Text("Particle emision from:");

			if (ImGui::RadioButton("Random", normalShapeType == ShapeType_SPHERE))
				normalShapeType = ShapeType_SPHERE;
			ImGui::SameLine();
			if (ImGui::RadioButton("Center", normalShapeType == ShapeType_SPHERE_CENTER))
				normalShapeType = ShapeType_SPHERE_CENTER;
			ImGui::SameLine();
			if (ImGui::RadioButton("Border", normalShapeType == ShapeType_SPHERE_BORDER))
				normalShapeType = ShapeType_SPHERE_BORDER;

			ImGui::DragFloat("Sphere Size", &sphereCreation.r, 0.25f, 1.0f, 20.0f, "%.2f");

			break;
		case ShapeType_CONE:
			ImGui::Text("Cone");
			ImGui::DragFloat("Sphere Size", &circleCreation.r, 0.25f, 0.25f, 20.0f, "%.2f");

			break;
		default:
			break;
		}
	}
}

void ComponentEmitter::ParticleColor()
{
	if (ImGui::CollapsingHeader("Particle Color"))

	{
		ImGui::Text("Particle Color");
		ImGui::SameLine();
		ImGui::ShowHelpMarker("Click color square for change it");
		std::vector<ColorTime> deleteColor;
		std::list<ColorTime>::iterator iter = startValues.color.begin();
		uint posList = 0u;
		while (iter != startValues.color.end())
		{
			//TODO: they must be able to change position
			if ((iter) == startValues.color.begin())
			{//Cant delete 1st color
				
				if (!EditColor(*iter))
					break;
				iter++;
			}
			else
			{
				if (!EditColor(*iter, posList))
					startValues.color.erase(iter++);
				else
					iter++;
			}
			++posList;
		}
		ImGui::Separator();
		ImGui::Checkbox("Color time", &startValues.timeColor);
		if (startValues.timeColor)
		{

			ImGui::DragInt("Position", &nextPos, 1.0f, 1, 100);
			ImGui::ColorPicker4("", &nextColor.x, ImGuiColorEditFlags_AlphaBar);
			if (ImGui::Button("Add Color", ImVec2(125, 25)))
			{
				ColorTime colorTime;
				colorTime.color = nextColor;
				colorTime.position = (float)nextPos / 100;
				colorTime.name = std::to_string((int)nextPos) + "%";
				startValues.color.push_back(colorTime);
				startValues.color.sort();
			}
		}
	}
}

void ComponentEmitter::ParticleBurst()
{
	if (ImGui::CollapsingHeader("Particle Burst"))
	{
		ImGui::Checkbox("Burst", &burst);
		if (ImGui::BeginMenu(burstTypeName.data()))
		{
			if (ImGui::MenuItem("Box"))
			{
				burstType = ShapeType_BOX;
				burstTypeName = "Box Burst";
			}
			else if (ImGui::MenuItem("Sphere"))
			{
				burstType = ShapeType_SPHERE_CENTER;
				burstTypeName = "Sphere Burst";
			}
			else if (ImGui::MenuItem("Cone"))
			{
				burstType = ShapeType_CONE;
				burstTypeName = "Cone Burst";
			}
			ImGui::End();
		}
		ImGui::DragInt("Min particles", &minPart, 1.0f, 0, 100);
		if (minPart > maxPart)
			maxPart = minPart;
		ImGui::DragInt("Max Particles", &maxPart, 1.0f, 0, 100);
		if (maxPart < minPart)
			minPart = maxPart;
		ImGui::DragFloat("Repeat Time", &repeatTime, 0.5f, 0.0f, 0.0f, "%.1f");

		ImGui::Separator();
	}
}

void ComponentEmitter::ParticleAABB()
{
	if (ImGui::CollapsingHeader("Particle BoundingBox"))
	{
		ImGui::Checkbox("Bounding Box", &drawAABB);
		if (drawAABB)
		{
			float3 size = gameObject->transform->originalBoundingBox.Size();
			if (ImGui::DragFloat3("Dimensions", &size.x, 1.0f, 0.0f, 0.0f, "%.0f"))
			{
				gameObject->transform->originalBoundingBox.SetFromCenterAndSize(posDifAABB, size);
				gameObject->transform->UpdateBoundingBox();
			}

			if (ImGui::DragFloat3("Pos", &posDifAABB.x, 1.0f, 0.0f, 0.0f, "%.0f"))
			{
				gameObject->transform->originalBoundingBox.SetFromCenterAndSize(posDifAABB, size);
				gameObject->transform->UpdateBoundingBox();
			}
		}
	}
}

void ComponentEmitter::ParticleTexture()
{
	if (ImGui::CollapsingHeader("Particle Texture", ImGuiTreeNodeFlags_FramePadding))
	{
		if (texture)
		{
			std::string name = texture->file;
			name = name.substr(name.find_last_of("\\") + 1);

			ImGui::Text("Loaded texture '%s'", name.data());
			ImGui::Text("Texture used %i times", texture->usage);

			ImGui::Image((void*)(intptr_t)texture->GetID(), ImVec2(256.0f, 256.0f), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));

			if (ImGui::BeginMenu("Change Texture"))
			{
				std::vector<Resource*> resource;
				App->resources->GetResources(resource, ResourceType::texture);

				for (std::vector<Resource*>::iterator iterator = resource.begin(); iterator != resource.end(); ++iterator)
				{
					if (ImGui::MenuItem((*iterator)->name.data()))
					{
						App->resources->Remove(texture);
						texture = nullptr;

						texture = ((ResourceTexture*)(*iterator));
						texture->usage++;
					}
				}
				ImGui::End();
			}
			if (ImGui::Button("Remove Texture", ImVec2(125, 25)))
			{
				App->resources->Remove(texture);
				texture = nullptr;
			}

		}
		else
		{
			ImGui::Text("No texture loaded");
			if (ImGui::BeginMenu("Add new Texture"))
			{
				std::vector<Resource*> resource;
				App->resources->GetResources(resource, ResourceType::texture);

				for (std::vector<Resource*>::iterator iterator = resource.begin(); iterator != resource.end(); ++iterator)
				{
					if (ImGui::MenuItem((*iterator)->name.data()))
					{
						texture = ((ResourceTexture*)(*iterator));
						texture->usage++;
					}
				}
				ImGui::End();
			}
		}

		ImGui::Separator();
		if (ImGui::Checkbox("Animated sprite", &isParticleAnimated))
		{
			if (!isParticleAnimated)
			{
				SetNewAnimation(1, 1);
				dieOnAnimation = false;
			}
			else
				SetNewAnimation(textureRows, textureColumns);
		}
		if (isParticleAnimated)
		{
			ImGui::DragFloat("Animation Speed", &animationSpeed, 0.001f, 0.0f, 5.0f, "%.3f");
			ImGui::DragInt("Rows", &textureRows, 1, 1, 10);
			ImGui::DragInt("Columns", &textureColumns, 1, 1, 10);

			ImGui::Checkbox("Kill particle with animation", &dieOnAnimation);
			if (dieOnAnimation)
			{
				checkLife = false;
				startValues.life.x = animationSpeed * particleAnimation.columns * particleAnimation.rows;
			}

			if (ImGui::Button("Calc Animation", ImVec2(150.0f, 25.0f)))
			{
				SetNewAnimation(textureRows, textureColumns);
			}
		}
		ImGui::Separator();
	}
}

void ComponentEmitter::ParticleSubEmitter()
{
	if (ImGui::Checkbox("SubEmitter", &startValues.subEmitterActive))
	{
		if (startValues.subEmitterActive)
		{
			if (subEmitter)
				subEmitter->SetActive(true);
			else
			{
				subEmitter = App->gameObject->CreateGameObject(float3::zero, Quat::identity, float3::one, gameObject, "SubEmition");
				EmitterInfo info;
				info.isSubEmitter = true;
				subEmitter->AddComponent(ComponentType_EMITTER, &info);
				AABB boundingBox = AABB();
				boundingBox.SetFromCenterAndSize(subEmitter->GetPos(), float3::one);
				subEmitter->SetABB(boundingBox);
				App->sceneIntro->octree.Insert(subEmitter);
			}
		}
		else
			subEmitter->SetActive(false);
	}
	ImGui::Separator();
}

void ComponentEmitter::SetNewAnimation(int row, int col)
{
	particleAnimation = App->resources->LoadTextureUV(row, col);
	for (std::list<Particle*>::iterator iterator = particles.begin(); iterator != particles.end(); ++iterator)
	{
		(*iterator)->currentFrame = 0;
	}
}

void ComponentEmitter::ShowFloatValue(float2& value, bool checkBox, const char* name, float v_speed, float v_min, float v_max)
{
	ImGui::SameLine();
	if (checkBox)
	{
		ImGui::PushItemWidth(70.0f);
		std::string str = "##";
		str.append(name);
		str.append("min");
		if (ImGui::DragFloat(str.data(), &value.x, v_speed, v_min, v_max, "%.2f"))
			CheckMinMax(value);
		ImGui::SameLine();
		if (ImGui::DragFloat(name, &value.y, v_speed, v_min, v_max, "%.2f"))
			CheckMinMax(value);
	}
	else
	{
		ImGui::PushItemWidth(148.0f);
		if (ImGui::DragFloat(name, &value.x, v_speed, v_min, v_max, "%.2f"))
			value.y = value.x;
	}
	ImGui::PopItemWidth();
}

void ComponentEmitter::CheckMinMax(float2& value)
{
	if (value.x > value.y)
		value.y = value.x;
}

bool ComponentEmitter::EditColor(ColorTime &colorTime, uint pos)
{
	bool ret = true;
	ImVec4 color = EqualsFloat4(colorTime.color);
	if (ImGui::ColorButton(colorTime.name.data(), color, ImGuiColorEditFlags_None, ImVec2(100, 20)))
		colorTime.changingColor = !colorTime.changingColor;

	if (!colorTime.changingColor)
	{
		ImGui::SameLine();
		ImGui::TextUnformatted(colorTime.name.data());
		if (pos > 0)
		{
			std::string colorStr = "Remove Color ";
			colorStr.append(std::to_string(pos));
			ImGui::SameLine();
			if (ImGui::Button(colorStr.data(), ImVec2(125, 25)))
				ret = false;
		}
		else if (!startValues.timeColor)
			ret = false;
	}
	else
		ImGui::ColorEdit4(colorTime.name.data(), &colorTime.color.x, ImGuiColorEditFlags_AlphaBar);
	return ret;
}

ImVec4 ComponentEmitter::EqualsFloat4(const float4 float4D)
{
	ImVec4 vec;
	vec.x = float4D.x;
	vec.y = float4D.y;
	vec.z = float4D.z;
	vec.w = float4D.w;
	return vec;
}

void ComponentEmitter::SaveComponent(JSON_Object* parent)
{
	json_object_set_number(parent, "Type", this->type);

	json_object_set_number(parent, "UUID", GetUUID());

	json_object_set_number(parent, "Time Created", GetTime());

	json_object_set_boolean(parent, "checkLife", checkLife);
	json_object_set_boolean(parent, "checkSpeed", checkSpeed);
	json_object_set_boolean(parent, "checkAcceleration", checkAcceleration);
	json_object_set_boolean(parent, "checkSize", checkSize);
	json_object_set_boolean(parent, "checkSizeOverTime", checkSizeOverTime);
	json_object_set_boolean(parent, "checkRotation", checkRotation);
	json_object_set_boolean(parent, "checkAngularAcceleration", checkAngularAcceleration);
	json_object_set_boolean(parent, "checkAngularVelocity", checkAngularVelocity);

	json_object_set_number(parent, "lifeMin", startValues.life.x);
	json_object_set_number(parent, "lifeMax", startValues.life.y);

	json_object_set_number(parent, "speedMin", startValues.speed.x);
	json_object_set_number(parent, "speedMax", startValues.speed.y);

	json_object_set_number(parent, "accelerationMin", startValues.acceleration.x);
	json_object_set_number(parent, "accelerationMax", startValues.acceleration.y);

	json_object_set_number(parent, "sizeMin", startValues.size.x);
	json_object_set_number(parent, "sizeMax", startValues.size.y);

	json_object_set_number(parent, "sizeOverTimeMin", startValues.sizeOverTime.x);
	json_object_set_number(parent, "sizeOverTimeMax", startValues.sizeOverTime.y);

	json_object_set_number(parent, "rotationMin", startValues.rotation.x);
	json_object_set_number(parent, "rotationMax", startValues.rotation.y);

	json_object_set_number(parent, "angularAccelerationMin", startValues.angularAcceleration.x);
	json_object_set_number(parent, "angularAccelerationMax", startValues.angularAcceleration.y);

	json_object_set_number(parent, "angularVelocityMin", startValues.angularVelocity.x);
	json_object_set_number(parent, "angularVelocityMax", startValues.angularVelocity.y);


	json_object_set_boolean(parent, "subEmitterActive", startValues.subEmitterActive);


	json_object_set_number(parent, "rateOverTime", rateOverTime);

	JSON_Value* colorValue = json_value_init_array();
	JSON_Array* color = json_value_get_array(colorValue);

	for (std::list<ColorTime>::const_iterator iterator = startValues.color.begin(); iterator != startValues.color.end(); ++iterator)
	{
		JSON_Value* newColor = json_value_init_object();
		JSON_Object* objCol = json_value_get_object(newColor);

		json_object_set_number(objCol, "colorX", (*iterator).color.x);
		json_object_set_number(objCol, "colorY", (*iterator).color.y);
		json_object_set_number(objCol, "colorZ", (*iterator).color.z);
		json_object_set_number(objCol, "colorW", (*iterator).color.w);

		json_object_set_number(objCol, "position", (*iterator).position);
		json_object_set_string(objCol, "name", (*iterator).name.data());


		json_array_append_value(color, newColor);
	}
		json_object_set_value(parent, "Colors", colorValue);

	// TODO: save colors
	json_object_set_number(parent, "timeColor", startValues.timeColor);

	json_object_set_boolean(parent, "subEmitterActive", startValues.subEmitterActive);

	json_object_set_number(parent, "particleDirectionX", startValues.particleDirection.x);
	json_object_set_number(parent, "particleDirectionY", startValues.particleDirection.y);
	json_object_set_number(parent, "particleDirectionZ", startValues.particleDirection.z);

	json_object_set_number(parent, "duration", duration);

	json_object_set_number(parent, "loop", loop);

	json_object_set_number(parent, "burst", burst);
	json_object_set_number(parent, "minPart", minPart);
	json_object_set_number(parent, "maxPart", maxPart);
	json_object_set_number(parent, "repeatTime", repeatTime);

	json_object_set_number(parent, "posDifAABBX",posDifAABB.x);
	json_object_set_number(parent, "posDifAABBY",posDifAABB.y);
	json_object_set_number(parent, "posDifAABBZ",posDifAABB.z);

	json_object_set_number(parent, "gravity", gravity);

	json_object_set_number(parent, "boxCreationMinX", boxCreation.minPoint.x);
	json_object_set_number(parent, "boxCreationMinY", boxCreation.minPoint.y);
	json_object_set_number(parent, "boxCreationMinZ", boxCreation.minPoint.z);

	json_object_set_number(parent, "boxCreationMaxX", boxCreation.maxPoint.x);
	json_object_set_number(parent, "boxCreationMaxY", boxCreation.maxPoint.y);
	json_object_set_number(parent, "boxCreationMaxZ", boxCreation.maxPoint.z);
	
	SaveNumberArray(parent, "boxCreationMin", boxCreation.minPoint.ptr(), 3);
	SaveNumberArray(parent, "boxCreationMax", boxCreation.maxPoint.ptr(), 3);

	json_object_set_number(parent, "SphereCreationRad", sphereCreation.r);

	json_object_set_number(parent, "circleCreationRad", circleCreation.r);

	json_object_set_number(parent, "shapeType", normalShapeType);

	if (texture)
	json_object_set_string(parent, "texture", texture->file.data());
	else
	json_object_set_string(parent, "texture", "noTexture");

	json_object_set_number(parent, "textureRows", textureRows);
	json_object_set_number(parent, "textureColumns", textureColumns);
	json_object_set_number(parent, "animationSpeed", animationSpeed);

	json_object_set_boolean(parent, "isParticleAnimated", isParticleAnimated);
	json_object_set_boolean(parent, "dieOnAnimation", dieOnAnimation);

	json_object_set_boolean(parent, "drawAABB", drawAABB);
	
	json_object_set_boolean(parent, "isSubEmitter", isSubEmitter);
	if(subEmitter)
	json_object_set_number(parent, "SubEmitter", subEmitter->GetUID());


	if (gameObject && gameObject->transform)
	{
		float3 bb = gameObject->transform->originalBoundingBox.Size();

		json_object_set_number(parent, "originalBoundingBoxSizeX", bb.x);
		json_object_set_number(parent, "originalBoundingBoxSizeY", bb.y);
		json_object_set_number(parent, "originalBoundingBoxSizeZ", bb.z);
	}
}

int ComponentEmitter::GetEmition() const
{
	return rateOverTime;
}

// todo event system to delete texture