//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Ragdoll utilites, etc
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "ragdoll.h"
#include "studio/StudioGeom.h"

#include "render/IDebugOverlay.h"
#include "animating/BoneSetup.h"
#include "physics/PhysicsCollisionGroup.h"
#include "dkphysics/IDkPhysics.h"

#define RAGDOLL_LINEAR_LIMIT (0.25)

// finds far parent bone in ragdoll
int ragdoll_t::ComputeAndGetFarParentOf(int bone)
{
	const int parentBone = m_pReferenceModel->GetJoint(bone).parent;

	if(parentBone != -1)
	{
		if(m_pBoneToRagdollIndices[parentBone] == -1)
		{
			// continue hierarchy
			return ComputeAndGetFarParentOf(parentBone);
		}
		else
		{
			// this is a needed parent with a ragdoll part.
			return parentBone;
		}
	}

	return -1;
}

#define COLLIDE_RAGDOLL (COLLISION_GROUP_WORLD | COLLISION_GROUP_OBJECTS | COLLISION_GROUP_PROJECTILES)

ragdoll_t* CreateRagdoll(CEqStudioGeom* pModel)
{
	if(!pModel)
		return nullptr;

	const studioPhysData_t& physModel = pModel->GetPhysData();
	const studioHdr_t& studio = pModel->GetStudioHdr();

	const int type = physModel.usageType;

	if(type == PHYSMODEL_USAGE_RAGDOLL)
	{
		ragdoll_t *newRagdoll = PPNew ragdoll_t;
		newRagdoll->Init();

		newRagdoll->m_pReferenceModel = pModel;

		int numPhysJoints = physModel.numJoints;
		newRagdoll->m_numBones = numPhysJoints;

		int numParts = physModel.numObjects;
		newRagdoll->m_numParts = numParts;

		// build joint remap table
		for(int i = 0; i < numPhysJoints; i++)
		{
			for(int j = 0; j < studio.numBones; j++)
			{
				if(!stricmp(pModel->GetJoint(j).bone->name, physModel.joints[i].name))
				{
					// assign index
					newRagdoll->m_pBoneToVisualIndices[i] = j;
					newRagdoll->m_pBoneToRagdollIndices[j] = i;

					continue;
				}
			}
		}

		// build far parental table
		for(int i = 0; i < studio.numBones; i++)
		{
			newRagdoll->m_pBoneMerge_far_parents[i] = newRagdoll->ComputeAndGetFarParentOf(i);

			Matrix4x4 transform = identity4;

			const int real_parent = newRagdoll->m_pBoneMerge_far_parents[i];
			if(real_parent != -1)
			{
				transform = pModel->GetJoint(i).absTrans * !pModel->GetJoint(real_parent).absTrans;
			}

			newRagdoll->m_pBoneMerge_Transformations[i] = transform;
		}

		// create objects of ragdoll
		for(int i = 0; i < numParts; i++)
		{
			newRagdoll->m_pParts[i] = physics->CreateObject( &physModel, i);

			newRagdoll->m_pParts[i]->SetContents( COLLISION_GROUP_DEBRIS );
			newRagdoll->m_pParts[i]->SetCollisionMask( COLLIDE_RAGDOLL | COLLISION_GROUP_DEBRIS );

			newRagdoll->m_pParts[i]->SetSleepTheresholds(20,20);
			newRagdoll->m_pParts[i]->SetDamping(0.01f,0.05f);
			newRagdoll->m_pParts[i]->SetActivationState(PS_ACTIVE);

			newRagdoll->m_nBodyParts[i] = physModel.objects[i].object.bodyPartId;

			newRagdoll->m_pParts[i]->SetUserData((void*)newRagdoll->m_nBodyParts[i]);
		}

		// create joints
		for(int i = 0; i < numPhysJoints; i++)
		{
			int object_partindexA = physModel.joints[i].objA;
			int object_partindexB = physModel.joints[i].objB;

			IPhysicsObject* partA = newRagdoll->m_pParts[object_partindexA];
			IPhysicsObject*	partB = newRagdoll->m_pParts[object_partindexB];

			// get a bone transformation
			const Matrix4x4& bone_transform = pModel->GetJoint(newRagdoll->m_pBoneToVisualIndices[i]).absTrans;

			const Vector3D linkA_pos = bone_transform.rows[3].xyz() - newRagdoll->m_pParts[object_partindexA]->GetPosition();
			const Vector3D linkB_pos = bone_transform.rows[3].xyz() - newRagdoll->m_pParts[object_partindexB]->GetPosition();

			Matrix4x4 local_a = bone_transform;
			Matrix4x4 local_b = bone_transform;

			local_a.setTranslation( linkA_pos );
			local_b.setTranslation( linkB_pos );

			// create constraints
			newRagdoll->m_pJoints[i] = physics->CreateJoint(partA, partB, local_a, local_b, true);

			// set limits
			newRagdoll->m_pJoints[i]->SetAngularLowerLimit(physModel.joints[i].minLimit);
			newRagdoll->m_pJoints[i]->SetAngularUpperLimit(physModel.joints[i].maxLimit);

			newRagdoll->m_pJoints[i]->SetLinearLowerLimit( Vector3D(-RAGDOLL_LINEAR_LIMIT) );
			newRagdoll->m_pJoints[i]->SetLinearUpperLimit( Vector3D(RAGDOLL_LINEAR_LIMIT) );
		}

		return newRagdoll;
	}
	else
	{
		MsgError("Invalid physics model usage for %s (excepted PHYSMODEL_USAGE_RAGDOLL)\n", pModel->GetName());
	}

	return nullptr;
}

void ragdoll_t::GetBoundingBox(Vector3D &mins, Vector3D &maxs) const
{
	BoundingBox aabb;

	for(int i = 0; i < m_numParts; i++)
	{
		if (!m_pParts[i])
			continue;
		Vector3D partAABBMins;
		Vector3D partAABBMaxs;

		m_pParts[i]->GetBoundingBox(partAABBMins, partAABBMaxs);

		aabb.AddVertex(partAABBMins);
		aabb.AddVertex(partAABBMaxs);
	}

	mins = aabb.minPoint;
	maxs = aabb.maxPoint;
}

Vector3D ragdoll_t::GetPosition() const
{
	Vector3D ragdoll_bboxmin;
	Vector3D ragdoll_bboxmax;

	GetBoundingBox(ragdoll_bboxmin, ragdoll_bboxmax);

	return (ragdoll_bboxmin+ragdoll_bboxmax)*0.5f;
}

void ragdoll_t::GetVisualBonesTransforms(Matrix4x4 *bones) const
{
	Matrix4x4 offsetTranslate = identity4;
	offsetTranslate.setTranslation(-GetPosition());

	const studioHdr_t& studio = m_pReferenceModel->GetStudioHdr();

	for(int i = 0; i < studio.numBones; i++)
	{
		const int ragdoll_joint_index = m_pBoneToRagdollIndices[i];

		if(ragdoll_joint_index != -1)
		{
			Matrix4x4 bone_global = m_pJoints[ragdoll_joint_index]->GetGlobalTransformA();
			bones[i] = bone_global*offsetTranslate;
		}
		else
		{
			const int far_parent = m_pBoneMerge_far_parents[i];

			if(far_parent != -1)
			{
				Matrix4x4 bone_transform = m_pBoneMerge_Transformations[i];

				bones[i] = bone_transform * bones[far_parent];
			}
		}
	}
}

// sets bone tranformations (useful for animated death, etc)
// you can setup from here a global transform
void ragdoll_t::SetBoneTransform(Matrix4x4 *bones, const Matrix4x4& translation)
{
	// set part transform
	for(int i = 0; i < m_numParts; i++)
	{
		if(m_pParts[i])
			m_pParts[i]->SetTransformFromMatrix( (!m_pJoints[i]->GetFrameTransformA() * bones[m_pBoneToVisualIndices[i]])*translation);
	}

	RefreshRagdollVisuals();
}

void ragdoll_t::Translate(const Vector3D &move)
{
	for(int i = 0; i < m_numParts; i++)
	{
		if(m_pParts[i])
			m_pParts[i]->SetPosition(m_pParts[i]->GetPosition() + move);
	}

	RefreshRagdollVisuals();
}

void ragdoll_t::RefreshRagdollVisuals()
{
	// refresh joint transform
	for(int i = 0; i < m_numBones; i++)
	{
		if(m_pJoints[i])
		{
			m_pJoints[i]->UpdateTransform();
		}
	}
}

// wakes ragdoll
void ragdoll_t::Wake()
{
	// set part transform
	for(int i = 0; i < m_numParts; i++)
	{
		if(m_pParts[i])
			m_pParts[i]->WakeUp();
	}

	RefreshRagdollVisuals();
}

void DestroyRagdoll(ragdoll_t* ragdoll)
{
	if(!ragdoll)
		return;

	// destroy all bones and objects
	for(int i = 0; i < ragdoll->m_numBones; i++)
	{
		if(ragdoll->m_pJoints[i])
		{
			physics->DestroyPhysicsJoint(ragdoll->m_pJoints[i]);
		}
	}

	for(int i = 0; i < ragdoll->m_numParts; i++)
	{
		if(ragdoll->m_pParts[i])
		{
			physics->DestroyPhysicsObject(ragdoll->m_pParts[i]);
		}
	}

	delete ragdoll;
}