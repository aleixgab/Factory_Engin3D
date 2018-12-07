#include "ManagerParticle.h"
#include <algorithm>

#include "Brofiler/Brofiler.h"

ManagerParticle::ManagerParticle()
{
}


ManagerParticle::~ManagerParticle()
{
}

void ManagerParticle::Draw()
{
	BROFILER_CATEGORY("__FUNCTION__", Profiler::Color::PapayaWhip);

	if (!particleList.empty())
	{
		particleList.sort(particleCompare());
		for (std::list<Particle*>::iterator iterator = particleList.begin(); iterator != particleList.end(); ++iterator)
		{
			(*iterator)->Draw();
		}
	}
}
