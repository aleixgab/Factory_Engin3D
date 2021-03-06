#ifndef __Module_H__
#define __Module_H__

#include "parson/parson.h"

#include "Event.h"

class Application;
struct PhysBody3D;

class Module
{
private :
	bool enabled;

public:
	Application* App;

	Module(Application* parent, bool start_enabled = true) : App(parent)
	{}

	virtual ~Module()
	{}

	virtual bool Init() 
	{
		return true; 
	}

	virtual bool Start()
	{
		return true;
	}

	virtual update_status PreUpdate()
	{
		return UPDATE_CONTINUE;
	}

	virtual update_status Update()
	{
		return UPDATE_CONTINUE;
	}

	virtual update_status PostUpdate()
	{
		return UPDATE_CONTINUE;
	}

	virtual void ReceiveEvent(const Event& event)
	{}

	virtual update_status Save(JSON_Object* object)
	{
		return UPDATE_CONTINUE;
	}

	virtual update_status Load(JSON_Object* object)
	{
		return UPDATE_CONTINUE;
	}

	virtual bool CleanUp() 
	{ 
		return true; 
	}

	virtual void OnCollision(PhysBody3D* body1, PhysBody3D* body2)
	{}
};
#endif // !__Module_H__