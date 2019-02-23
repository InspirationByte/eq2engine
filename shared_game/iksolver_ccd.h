//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Cyclic Continuous Difference (CCD) Ik Solver
//////////////////////////////////////////////////////////////////////////////////

#ifndef IKSOLVER_CCD
#define IKSOLVER_CCD

#include "math/DkMath.h"
#include "BoneSetup.h"

inline void IKLimitDOF( giklink_t* link )
{
	// FIXME: broken here
	// gimbal lock always occurent
	// better to use quaternions...

	Vector3D euler = eulers(link->quat);

	euler = VRAD2DEG(euler);

	// clamp to this limits
	euler = clamp(euler, link->l->mins, link->l->maxs);

	//euler = NormalizeAngles180(euler);

	// get all back
	euler = VDEG2RAD(euler);

	link->quat = Quaternion(euler.x,euler.y,euler.z);
}

#define IK_DISTANCE_EPSILON 0.05f

// solves Ik chain
inline bool SolveIKLinks(giklink_t& effector, Vector3D &target, float fDt, int numIterations = 100)
{
	Vector3D	rootPos, curEnd, targetVector, desiredEnd, curVector, crossResult;

	// start at the last link in the chain
	giklink_t* link = effector.parent;

	int nIter = 0;
	do
	{
		rootPos = link->absTrans.rows[3].xyz();
		curEnd = effector.absTrans.rows[3].xyz();

		desiredEnd = target;
		float dist = distance(curEnd, desiredEnd);

		// check distance
		if (dist > IK_DISTANCE_EPSILON)
		{
			// get directions
			curVector = normalize(curEnd - rootPos);
			targetVector = normalize(desiredEnd - rootPos);

			// get diff cosine between target vector and current bone vector
			float cosAngle = dot(targetVector, curVector);

			// check if we not in zero degrees
			if (cosAngle < 1.0)
			{
				// determine way of rotation
				crossResult = normalize(cross(targetVector, curVector));
				float turnAngle = acosf(cosAngle); // get the angle of dot product

				// damp using time
				turnAngle *= fDt * link->l->damping;

				Quaternion quat(turnAngle, crossResult);
				
				link->quat = link->quat * quat;
	
				// check limits
				IKLimitDOF( link );
			}

			if (!link->parent) 
				link = effector.parent; // restart
			else
				link = link->parent;
		}
	}while(nIter++ < numIterations && distance(curEnd, desiredEnd) > IK_DISTANCE_EPSILON);
			
	if (nIter >= numIterations)
		return false;

	return true;
}

#endif // IKSOLVER_CCD