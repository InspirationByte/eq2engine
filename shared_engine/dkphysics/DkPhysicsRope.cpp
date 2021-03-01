//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium physics rope class
//////////////////////////////////////////////////////////////////////////////////

#include "DkPhysicsRope.h"
#include "DkBulletObject.h"

void DkPhysicsRope::AttachObject(IPhysicsObject* pObject, int node_index, bool disable_self_collision)
{
	CPhysicsObject* pPhysicsObject = (CPhysicsObject*)pObject;

	m_pRopeBody->appendAnchor(node_index, pPhysicsObject->m_pPhyObjectPointer, disable_self_collision);
}

int DkPhysicsRope::GetNodeCount()
{
	return m_pRopeBody->m_nodes.size();
}

Vector3D DkPhysicsRope::GetNodePosition(int node)
{
	Vector3D out;
	ConvertPositionToEq(out, m_pRopeBody->m_nodes[node].m_x);
	return out;
}

Vector3D DkPhysicsRope::GetNodeVelocity(int node)
{
	Vector3D out;
	ConvertPositionToEq(out, m_pRopeBody->m_nodes[node].m_v);
	return out;
}

Vector3D DkPhysicsRope::GetNodeNormal(int node)
{
	Vector3D out;
	ConvertBulletToDKVectors(out, m_pRopeBody->m_nodes[node].m_n);
	return out;
}

IPhysicsObject* DkPhysicsRope::GetAttachedObject(int index)
{
	return (IPhysicsObject*)m_pRopeBody->m_anchors[index].m_body->getUserPointer();
}

int DkPhysicsRope::GetAttachedObjectCount()
{
	return m_pRopeBody->m_anchors.size();
}

void DkPhysicsRope::RemoveAttachedObject(int index)
{
	// FIXME: fix bullet std to do this
	//m_pRopeBody->m_anchors.remove(m_pRopeBody->m_anchors[index]);
}

void DkPhysicsRope::SetFullMass(float fMass)
{
	m_pRopeBody->setTotalMass(fMass);
}

void DkPhysicsRope::SetNodeMass(int node, float fMass)
{
	m_pRopeBody->setMass(node, fMass);
}

void DkPhysicsRope::SetLinearStiffness(float stiffness)
{
	m_pRopeBody->m_materials[0]->m_kLST = stiffness;
}

void DkPhysicsRope::SetIterations(int num)
{
	m_pRopeBody->m_cfg.piterations = num;
}