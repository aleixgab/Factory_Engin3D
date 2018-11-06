#ifndef __ModulePicking_H__
#define __ModulePicking_H__

#include "Module.h"
#include "Globals.h"

class ModulePicking : public Module
{
public:
	ModulePicking(Application* app, bool start_enabled = true);
	~ModulePicking();

	bool Start();
	bool CleanUp();
	update_status Update(float dt);

	bool CheckMeshTri(Geometry * geometry, LineSegment & rayTransformed, float & smallerDist);

	float3 SetTrianglePoint(Buffer<float> vertex, Buffer<uint> index, uint idIndex);

public:
	RayLine rayDraw;
};

#endif // !__ModulePicking_H__