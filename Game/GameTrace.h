//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Game tracer
//////////////////////////////////////////////////////////////////////////////////

#ifndef GAMETRACE_H
#define GAMETRACE_H

#include "idkphysics.h"

class BaseEntity;

// 
struct trace_t
{
	
	trace_t()
	{
		hitEnt = NULL;
		hitmaterial = NULL;
		fraction = 1.0f;

		//pt.hitObj = NULL;
		//pt.fraction = 1.0f;
	}

	Vector3D				origin;
	Vector3D				traceEnd;
	Vector3D				normal;

	BaseEntity*				hitEnt;
	phySurfaceMaterial_t*	hitmaterial;
	
	float					fraction;

	// physics engine trace
	internaltrace_t			pt;
};

void UTIL_TraceLine( Vector3D &tracestart, Vector3D &traceend, int groupmask, trace_t *trace, BaseEntity** pIgnoreList = NULL, int numIgnored = 0 );
void UTIL_TraceBox( Vector3D &tracestart, Vector3D &traceend, Vector3D& boxSize, int groupmask, trace_t *trace, BaseEntity** pIgnoreList = NULL, int numIgnored = 0, Matrix4x4 *externalBoxTransform = NULL );
void UTIL_TraceSphere( Vector3D &tracestart, Vector3D &traceend, float sphereRadius, int groupmask, trace_t *trace, BaseEntity** pIgnoreList = NULL, int numIgnored = 0 );

#endif // GAMETRACE_H