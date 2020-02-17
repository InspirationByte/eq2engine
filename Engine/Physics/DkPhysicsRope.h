//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium physics rope class
//////////////////////////////////////////////////////////////////////////////////

#ifndef DKPHYSROPE_H
#define DKPHYSROPE_H

#include "DebugInterface.h"
#include "IPhysicsRope.h"

#define __BT_SKIP_UINT64_H	// SDL2 support

#include "btBulletDynamicsCommon.h"
#include "BulletSoftBody/btSoftBodyHelpers.h"

class DkPhysicsRope : public IPhysicsRope
{
public:
	friend class		CPhysicsObject;
	friend class		DkPhysics;

	void				AttachObject(IPhysicsObject* pObject, int node_index, bool disable_self_collision = false);
	int					GetNodeCount();

	Vector3D			GetNodePosition(int node);
	Vector3D			GetNodeVelocity(int node);
	Vector3D			GetNodeNormal(int node);

	IPhysicsObject*		GetAttachedObject(int index);
	int					GetAttachedObjectCount();
	void				RemoveAttachedObject(int index);

	void				SetFullMass(float fMass);
	void				SetNodeMass(int node, float fMass);
	void				SetLinearStiffness(float stiffness);
	void				SetIterations(int num);

protected:

	btSoftBody*			m_pRopeBody;
};

#endif // DKPHYSROPE_H