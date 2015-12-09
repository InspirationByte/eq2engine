//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech physics joints system
//////////////////////////////////////////////////////////////////////////////////

#include "DkPhysicsJoint.h"
#include "physics/BulletConvert.h"

using namespace EqBulletUtils;

extern btDynamicsWorld *g_pPhysicsWorld;

DkPhysicsJoint::DkPhysicsJoint()
{
	m_pObjectA = NULL;
	m_pObjectB = NULL;

	m_pJointPointer = NULL;
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
	return ConvertMatrix4ToEq((btTransform)m_pJointPointer->getCalculatedTransformA());
}

Matrix4x4 DkPhysicsJoint::GetGlobalTransformB()
{
	return ConvertMatrix4ToEq((btTransform)m_pJointPointer->getCalculatedTransformB());
}

Matrix4x4 DkPhysicsJoint::GetFrameTransformA()
{
	return ConvertMatrix4ToEq((btTransform)m_pJointPointer->getFrameOffsetA());
}

Matrix4x4 DkPhysicsJoint::GetFrameTransformB()
{
	return ConvertMatrix4ToEq((btTransform)m_pJointPointer->getFrameOffsetB());
}

// setters

void DkPhysicsJoint::SetLinearLowerLimit(const Vector3D& linearLower)
{
	m_pJointPointer->setLinearLowerLimit(ConvertPositionToBullet(linearLower));
}

void DkPhysicsJoint::SetLinearUpperLimit(const Vector3D& linearUpper)
{
	m_pJointPointer->setLinearUpperLimit(ConvertPositionToBullet(linearUpper));
}

void DkPhysicsJoint::SetAngularLowerLimit(const Vector3D& angularLower)
{
	m_pJointPointer->setAngularLowerLimit(ConvertDKToBulletVectors(angularLower));
}

void DkPhysicsJoint::SetAngularUpperLimit(const Vector3D& angularUpper)
{
	m_pJointPointer->setAngularUpperLimit(ConvertDKToBulletVectors(angularUpper));
}

Vector3D DkPhysicsJoint::GetLinearLowerLimit()
{
	btVector3 linearLower;
	m_pJointPointer->getLinearLowerLimit(linearLower);

	return ConvertBulletToDKVectors(linearLower);
}

Vector3D DkPhysicsJoint::GetLinearUpperLimit()
{
	btVector3 linearUpper;
	m_pJointPointer->getLinearUpperLimit(linearUpper);

	return ConvertBulletToDKVectors(linearUpper);
}

Vector3D DkPhysicsJoint::GetAngularLowerLimit()
{
	btVector3 angularLower;
	m_pJointPointer->getAngularLowerLimit(angularLower);

	return ConvertBulletToDKVectors(angularLower);
}

Vector3D DkPhysicsJoint::GetAngularUpperLimit()
{
	btVector3 angularUpper;
	m_pJointPointer->getAngularUpperLimit(angularUpper);

	return ConvertBulletToDKVectors(angularUpper);
}

void DkPhysicsJoint::UpdateTransform()
{
	m_pJointPointer->calculateTransforms();
}