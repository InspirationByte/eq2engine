//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: AI node and hint entities
//////////////////////////////////////////////////////////////////////////////////

#include "AINode.h"

ConVar ai_node_ground_linkdist("ai_node_ground_linkdist", "512", "Ground node linkage maximum distance");
ConVar ai_node_ground_traceheight("ai_node_ground_traceheight", "60", "Ground node linkage height to trace walkable space");

//-------------------------------------------------------
// Hint node entity
//-------------------------------------------------------

CAIHint::CAIHint()
{
	m_nHintType = HINT_INVALID;
	m_bInUse = false;
}

CAIHint::~CAIHint()
{

}

EntType_e CAIHint::EntType()
{
	return ENTTYPE_AI_HINT;
}

HintType_e CAIHint::GetHintType()
{
	return m_nHintType;
}

void CAIHint::SetIsUsed(bool bInUse)
{
	m_bInUse = bInUse;
}

bool CAIHint::IsUsed()
{
	return m_bInUse;
}

BEGIN_DATAMAP(CAIHint)
	DEFINE_KEYFIELD(m_nHintType, "type", VTYPE_INTEGER),
END_DATAMAP()

DECLARE_ENTITYFACTORY(ai_hint, CAIHint)

//-------------------------------------------------------
// AI ground node entity
//-------------------------------------------------------

CAINodeGround::CAINodeGround()
{

}

CAINodeGround::~CAINodeGround()
{

}

void CAINodeGround::OnMapSpawn()
{
	// check all objects in radius
	for(int i = 0; i < ents->numElem(); i++)
	{
		BaseEntity *pEnt = ents->ptr()[i];

		if(pEnt->EntType() == ENTTYPE_AI_NODEGROUND &&
			fabs(length( pEnt->GetAbsOrigin()-GetAbsOrigin() )) <= ai_node_ground_linkdist.GetFloat())
		{
			// trace to this node with some height
			Vector3D trace_start = GetAbsOrigin() + Vector3D(0,ai_node_ground_traceheight.GetFloat(),0);
			Vector3D trace_end = pEnt->GetAbsOrigin() + Vector3D(0,ai_node_ground_traceheight.GetFloat(),0);

			// trace with world only
			internaltrace_t tr;
			physics->InternalTraceLine( trace_start, trace_end, COLLISION_GROUP_WORLD, &tr );

			if(tr.fraction == 1.0f) // if no collision happens
				m_pLinkedNodes.append( (CAINodeGround*)pEnt );
		}
	}

	// append this to node list to graph list
	g_pAINavigator->AddGroundNode( this );

	BaseClass::OnMapSpawn();
}

void CAINodeGround::Spawn()
{
	BaseClass::Spawn();
}

EntType_e CAINodeGround::EntType()
{
	return ENTTYPE_AI_NODEGROUND;
}

BEGIN_DATAMAP(CAINodeGround)

END_DATAMAP()

DECLARE_ENTITYFACTORY(ainode_ground, CAINodeGround)