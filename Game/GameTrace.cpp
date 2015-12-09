//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Game tracer
//////////////////////////////////////////////////////////////////////////////////

#include "GameTrace.h"
#include "BaseEngineHeader.h"

void UTIL_TraceLine( Vector3D &tracestart, Vector3D &traceend, int groupmask, trace_t *trace, BaseEntity** pIgnoreList, int numIgnored)
{
	ASSERT(trace != NULL);

	// resolve physics objects from ents to ignore
	DkList<IPhysicsObject*> ignore_objs;
	ignore_objs.resize(numIgnored);

	for(int i = 0; i < numIgnored; i++)
	{
		if(pIgnoreList[i])
			ignore_objs.append( pIgnoreList[i]->PhysicsGetObject() );
	}

	physics->InternalTraceLine( tracestart, traceend, groupmask, &trace->pt, ignore_objs.ptr(), ignore_objs.numElem());

	// get a hit object
	if(trace->pt.hitObj)
	{
		trace->hitmaterial = trace->pt.hitObj->GetMaterial();
		trace->hitEnt = (BaseEntity*)trace->pt.hitObj->GetEntityObjectPtr();
	}
	else
	{
		trace->hitmaterial = physics->GetMaterialList()->ptr()[0];
		trace->hitEnt = NULL;
	}

	trace->origin = trace->pt.origin;
	trace->normal = trace->pt.normal;
	trace->traceEnd = trace->pt.traceEnd;
	trace->fraction = trace->pt.fraction;
}

void UTIL_TraceBox( Vector3D &tracestart, Vector3D &traceend, Vector3D& boxSize, int groupmask, trace_t *trace, BaseEntity** pIgnoreList, int numIgnored, Matrix4x4 *externalBoxTransform )
{
	ASSERT(trace != NULL);

	// resolve physics objects from ents to ignore
	DkList<IPhysicsObject*> ignore_objs;
	ignore_objs.resize(numIgnored);

	for(int i = 0; i < numIgnored; i++)
	{
		if(pIgnoreList[i] && pIgnoreList[i]->PhysicsGetObject())
			ignore_objs.append( pIgnoreList[i]->PhysicsGetObject() );
	}

	physics->InternalTraceBox( tracestart, traceend, boxSize, groupmask, &trace->pt, ignore_objs.ptr(), ignore_objs.numElem(), externalBoxTransform);

	// get a hit object
	if(trace->pt.hitObj)
	{
		trace->hitmaterial = trace->pt.hitObj->GetMaterial();
		trace->hitEnt = (BaseEntity*)trace->pt.hitObj->GetEntityObjectPtr();
	}
	else
		trace->hitmaterial = physics->GetMaterialList()->ptr()[0];

	trace->origin = trace->pt.origin;
	trace->normal = trace->pt.normal;
	trace->traceEnd = trace->pt.traceEnd;
	trace->fraction = trace->pt.fraction;
}

void UTIL_TraceSphere( Vector3D &tracestart, Vector3D &traceend, float sphereRadius, int groupmask, trace_t *trace, BaseEntity** pIgnoreList, int numIgnored)
{
	ASSERT(trace != NULL);

	// resolve physics objects from ents to ignore
	DkList<IPhysicsObject*> ignore_objs;
	ignore_objs.resize(numIgnored);

	for(int i = 0; i < numIgnored; i++)
	{
		if(pIgnoreList[i] && pIgnoreList[i]->PhysicsGetObject())
			ignore_objs.append( pIgnoreList[i]->PhysicsGetObject() );
	}

	physics->InternalTraceSphere( tracestart, traceend, sphereRadius, groupmask, &trace->pt, ignore_objs.ptr(), ignore_objs.numElem());

	// get a hit object
	if(trace->pt.hitObj)
	{
		trace->hitmaterial = trace->pt.hitObj->GetMaterial();
		trace->hitEnt = (BaseEntity*)trace->pt.hitObj->GetEntityObjectPtr();
	}
	else
		trace->hitmaterial = physics->GetMaterialList()->ptr()[0];

	trace->origin = trace->pt.origin;
	trace->normal = trace->pt.normal;
	trace->traceEnd = trace->pt.traceEnd;
	trace->fraction = trace->pt.fraction;
}