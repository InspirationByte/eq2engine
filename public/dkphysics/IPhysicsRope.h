//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium physics rope class
//////////////////////////////////////////////////////////////////////////////////

#pragma once

class IPhysicsObject;

class IPhysicsRope
{
public:
	virtual ~IPhysicsRope() = default;

	virtual void				AttachObject(IPhysicsObject* pObject, int node_index, bool disable_self_collision = false) = 0;
	virtual int					GetNodeCount() = 0;

	virtual Vector3D			GetNodePosition(int node) = 0;
	virtual Vector3D			GetNodeVelocity(int node) = 0;
	virtual Vector3D			GetNodeNormal(int node) = 0;

	virtual IPhysicsObject*		GetAttachedObject(int index) = 0;
	virtual int					GetAttachedObjectCount() = 0;
	virtual void				RemoveAttachedObject(int index) = 0;

	virtual void				SetFullMass(float fMass) = 0; // sets full mass of the rope
	virtual void				SetNodeMass(int node, float fMass) = 0; // sets node mass. set mass to 0 if you want to freeze it.

	virtual void				SetLinearStiffness(float stiffness) = 0;
	virtual void				SetIterations(int num) = 0; // position iterations
};
