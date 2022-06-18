//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium physics joints system
//////////////////////////////////////////////////////////////////////////////////

#define __BT_SKIP_UINT64_H	// for SDL2
#include <btBulletDynamicsCommon.h>

#include "core/core_common.h"
#include "DkPhysicsJoint.h"
#include "physics/BulletConvert.h"

using namespace EqBulletUtils;

extern btDynamicsWorld *g_pPhysicsWorld;

DkPhysicsJoint::DkPhysicsJoint()
{
	m_pObjectA = nullptr;
	m_pObjectB = nullptr;

	m_pJointPointer = nullptr;
}

DkPhysicsJoint::~DkPhysicsJoint()
{
	if(m_pJointPointer)
	{
		g_pPhysicsWorld->removeConstraint(m_pJointPointer);
		delete m_pJointPointer;
	}
}

IPhysicsObject* DkPhysicsJoint::GetPhysicsObjectA()
{
	return m_pObjectA;
}

IPhysicsObject* DkPhysicsJoint::GetPhysicsObjectB()
{
	return m_pObjectB;
}

Matrix4x4 DkPhysicsJoint::GetGlobalTransformA()
{
	Matrix4x4 out;
	ConvertMatrix4ToEq(out, (btTransform)m_pJointPointer->getCalculatedTransformA());
	return out;
}

Matrix4x4 DkPhysicsJoint::GetGlobalTransformB()
{
	Matrix4x4 out;
	ConvertMatrix4ToEq(out, (btTransform)m_pJointPointer->getCalculatedTransformB());
	return out;
}

Matrix4x4 DkPhysicsJoint::GetFrameTransformA()
{
	Matrix4x4 out;
	ConvertMatrix4ToEq(out, (btTransform)m_pJointPointer->getFrameOffsetA());
	return out;
}

Matrix4x4 DkPhysicsJoint::GetFrameTransformB()
{
	Matrix4x4 out;
	ConvertMatrix4ToEq(out, (btTransform)m_pJointPointer->getFrameOffsetB());
	return out;
}

// setters

void DkPhysicsJoint::SetLinearLowerLimit(const Vector3D& linearLower)
{
	btVector3 vec;
	ConvertPositionToBullet(vec, linearLower);
	m_pJointPointer->setLinearLowerLimit(vec);
}

void DkPhysicsJoint::SetLinearUpperLimit(const Vector3D& linearUpper)
{
	btVector3 vec;
	ConvertPositionToBullet(vec, linearUpper);
	m_pJointPointer->setLinearUpperLimit(vec);
}

void DkPhysicsJoint::SetAngularLowerLimit(const Vector3D& angularLower)
{
	btVector3 vec;
	ConvertDKToBulletVectors(vec, angularLower);
	m_pJointPointer->setAngularLowerLimit(vec);
}

void DkPhysicsJoint::SetAngularUpperLimit(const Vector3D& angularUpper)
{
	btVector3 vec;
	ConvertDKToBulletVectors(vec, angularUpper);
	m_pJointPointer->setAngularUpperLimit(vec);
}

Vector3D DkPhysicsJoint::GetLinearLowerLimit()
{
	btVector3 linearLower;
	m_pJointPointer->getLinearLowerLimit(linearLower);

	Vector3D out;
	ConvertBulletToDKVectors(out, linearLower);
	return out;
}

Vector3D DkPhysicsJoint::GetLinearUpperLimit()
{
	btVector3 linearUpper;
	m_pJointPointer->getLinearUpperLimit(linearUpper);

	Vector3D out;
	ConvertBulletToDKVectors(out, linearUpper);
	return out;
}

Vector3D DkPhysicsJoint::GetAngularLowerLimit()
{
	btVector3 angularLower;
	m_pJointPointer->getAngularLowerLimit(angularLower);

	Vector3D out;
	ConvertBulletToDKVectors(out, angularLower);
	return out;
}

Vector3D DkPhysicsJoint::GetAngularUpperLimit()
{
	btVector3 angularUpper;
	m_pJointPointer->getAngularUpperLimit(angularUpper);

	Vector3D out;
	ConvertBulletToDKVectors(out, angularUpper);
	return out;
}

void DkPhysicsJoint::UpdateTransform()
{
	m_pJointPointer->calculateTransforms();
}