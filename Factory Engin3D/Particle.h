#ifndef __Particle_H__
#define __Particle_H__

#include "MathGeoLib/Math/float3.h"
#include "MathGeoLib/Math/Quat.h"

#include "ParticlePlane.h"

struct ParticleTrans
{
	float3 position = float3::zero;
	Quat rotation = Quat::identity;
	float3 scale = float3::one;
};

class Particle
{
public:
	Particle();
	~Particle();

	float GetCamDistance() const;
	void Draw() const;
public:
	float lifeTime;
	float life;
	float3 speed;
	float rotationSpeed;

	ParticleTrans transform;

//private:
	ParticlePlane* plane = nullptr;
};

struct particleCompare
{
	bool operator()(const Particle* particle1, const Particle* particle2) const
	{
		return particle1->GetCamDistance() < particle2->GetCamDistance();
	}
};
#endif // !__Particle_H__
