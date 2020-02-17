//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium physics objects
//////////////////////////////////////////////////////////////////////////////////

#ifndef IPHYSICSCONTROLLER_H
#define IPHYSICSCONTROLLER_H

enum controllertype_t
{
	CONTROLLER_INVALID = 0,

	CONTROLLER_CHARACTER,
	CONTROLLER_VEHICLE,
};

class IPhysicsController
{
public:
	virtual controllertype_t	GetType() = 0;
	virtual bool				IsOnGround() = 0;
	virtual void SetOrigin(Vector3D pos) = 0;
	virtual Vector3D GetOrigin() = 0;
	virtual void Destroy() = 0;
};

#endif //IPHYSICSCONTROLLER_H