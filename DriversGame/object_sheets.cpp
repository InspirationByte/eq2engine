//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: flying sheets, leaves and newspapers
//////////////////////////////////////////////////////////////////////////////////

#include "object_sheets.h"
#include "world.h"

#include "EqParticles.h"

#define SHEET_COUNT				15
#define SHEET_GRAVITY			(-3.0f)
#define SHEET_MIN_VELOCITY		(6.5f)
#define SHEET_VELOCITY_SCALE	(2.0f)

extern CPFXAtlasGroup* g_translParticles;

CObject_Sheets::CObject_Sheets( kvkeybase_t* kvdata ) : CGameObject()
{
	m_keyValues = kvdata;
	m_ghostObject = NULL;
	m_initDelay = 2.0f;
}

CObject_Sheets::~CObject_Sheets()
{

}

void CObject_Sheets::OnRemove()
{
	BaseClass::OnRemove();

	g_pPhysics->m_physics.DestroyGhostObject(m_ghostObject);
	m_ghostObject = NULL;
}

void CObject_Sheets::Spawn()
{
	BaseClass::Spawn();

	float radius = KV_GetValueFloat(m_keyValues->FindKeyBase("radius"), 0, 2.0f);

	m_ghostObject = new CEqCollisionObject();

	m_ghostObject->Initialize( radius );

	m_ghostObject->SetCollideMask(0);
	m_ghostObject->SetContents(OBJECTCONTENTS_DEBRIS);

	m_ghostObject->SetPosition(GetOrigin());

	m_ghostObject->SetUserData(this);

	m_bbox = m_ghostObject->m_aabb_transformed;

	// Next call will add NO_RAYCAST, COLLISION_LIST and DISABLE_RESPONSE flags automatically
	g_pPhysics->m_physics.AddGhostObject( m_ghostObject );

	m_wasInit = InitSheets();
}

bool CObject_Sheets::InitSheets()
{
	float radius = KV_GetValueFloat(m_keyValues->FindKeyBase("radius"), 0, 2.0f);
	kvkeybase_t* textures = m_keyValues->FindKeyBase("textures");

	for(int i = 0; i < SHEET_COUNT; i++)
	{
		const char* textureName = KV_GetValueString(textures, RandomInt(0, textures->values.numElem()-1), "news1");

		sheetpart_t part;
		part.atlas = g_translParticles;	// TODO: other atlas
		part.entry = part.atlas->FindEntry( textureName );

		Vector3D randomPos;
		AngleVectors(Vector3D(0.0f,RandomFloat(0.0f,360.0f),0.0f), &randomPos);

		float randRadius = RandomFloat(0.0f,1.0f)*radius;

		part.origin = GetOrigin() + randomPos*randRadius + Vector3D(0,5.0f,0);
		Vector3D endPos = GetOrigin() + randomPos*randRadius - Vector3D(0,5.0f,0);

		CollisionData_t coll;
		if( g_pPhysics->TestLine(part.origin, endPos, coll, OBJECTCONTENTS_SOLID_GROUND) )
		{
			part.origin = coll.position;
			part.angle = RandomFloat(-70.0f,80.0f);
			m_sheets.append( part );
		}
	}

	return m_sheets.numElem() > 0;
}

void CObject_Sheets::Simulate( float fDt )
{
	if(!m_wasInit)
	{
		m_initDelay -= fDt;

		if(m_initDelay <= 0.0f)
		{
			m_wasInit = InitSheets();
			m_initDelay = 2.0f;
		}

		return;
	}

	CEqRigidBody* body = NULL;

	bool canRender = g_pGameWorld->m_occludingFrustum.IsBoxVisible(m_bbox);

	m_bbox.Reset();

	if(m_ghostObject->m_collisionList.numElem() > 0 && m_ghostObject->m_collisionList[0].bodyB)
	{
		CEqCollisionObject* obj = m_ghostObject->m_collisionList[0].bodyB;

		if(obj->m_flags & BODY_ISCAR)
			body = (CEqRigidBody*)obj;
	}

	// we have to deal with cleaning up collision list
	m_ghostObject->m_collisionList.clear( false );

	PFXVertex_t* sheetQuad;

	const float sheetScale = 0.2f;
	ColorRGBA color = (g_pGameWorld->m_info.ambientColor+g_pGameWorld->m_info.sunColor);

	for( int i = 0; i < m_sheets.numElem(); i++ )
	{
		sheetpart_t& sheet = m_sheets[i];

		bool appliedForce = false;

		float featherAngle = sin( sheet.angle*2.0f );

		if( body )
		{
			float vel = length(body->GetVelocityAtWorldPoint( sheet.origin ));

			if(vel > SHEET_MIN_VELOCITY && (sheet.origin.y-GetOrigin().y) < 2.0f)
			{
				vel -= SHEET_MIN_VELOCITY;

				appliedForce = true;
				float distToBodyFac = clamp((float)length(body->GetPosition()-sheet.origin)/3.5f, 0.0f, 1.0f);

				distToBodyFac *= distToBodyFac;
				distToBodyFac = 1.0f-distToBodyFac;

				sheet.velocity += vel*distToBodyFac*SHEET_VELOCITY_SCALE*fDt;
				sheet.origin += Vector3D(0, sheet.velocity*fDt, 0);

				sheet.angle += sheet.velocity*fDt;
			}

			featherAngle *= clamp(fabs(sheet.velocity)*0.5f, 0.0f, 1.0f);
		}

		if(!appliedForce && sheet.velocity != 0.0f)
		{
			CollisionData_t coll;
			if( !g_pPhysics->TestLine(sheet.origin + Vector3D(0,10.0f, 0), sheet.origin - Vector3D(0,0.01f, 0), coll, OBJECTCONTENTS_SOLID_GROUND) )
			{
				if(fDt > 0.0f && sheet.velocity > 0.0f)
					sheet.velocity *= 0.85f;

				sheet.angle += fDt*SHEET_GRAVITY;

				sheet.velocity += SHEET_GRAVITY*fDt;
				sheet.origin += Vector3D(0, sheet.velocity*fDt, 0);
			}
			else
			{
				sheet.velocity = 0.0f;
				sheet.origin = coll.position;
				featherAngle = 0.0f;
			}
		}
		else if(sheet.velocity == 0.0f)
			featherAngle = 0.0f;

		featherAngle += sin(featherAngle);

		Vector3D sheetPos = sheet.origin + Vector3D(sin(sheet.angle)*1.5f, 0.015f, -cos(sheet.angle)*1.5f);
		m_bbox.AddVertex( sheetPos );

		if(!canRender)
			continue;

		Vector3D vRight, vUp;
		Vector3D sheetAngle(-90.0f+featherAngle*45.0f, sheet.angle*5.0f, sheet.angle+featherAngle*25.0f);

		AngleVectors(sheetAngle, NULL, &vUp, &vRight);

		Rectangle_t texCoords(0,0,1,1);
		texCoords = sheet.entry->rect;
		if(sheet.atlas->AllocateGeom(4,4, &sheetQuad, NULL, true) != -1)
		{
			sheetQuad[0].point = sheetPos + (vUp * sheetScale) + (sheetScale * vRight);
			sheetQuad[0].texcoord = Vector2D(texCoords.vrightBottom.x, texCoords.vrightBottom.y);
			sheetQuad[0].color = color;

			sheetQuad[1].point = sheetPos + (vUp * sheetScale) - (sheetScale * vRight);
			sheetQuad[1].texcoord = Vector2D(texCoords.vrightBottom.x, texCoords.vleftTop.y);
			sheetQuad[1].color = color;

			sheetQuad[2].point = sheetPos - (vUp * sheetScale) + (sheetScale * vRight);
			sheetQuad[2].texcoord = Vector2D(texCoords.vleftTop.x, texCoords.vrightBottom.y);
			sheetQuad[2].color = color;

			sheetQuad[3].point = sheetPos - (vUp * sheetScale) - (sheetScale * vRight);
			sheetQuad[3].texcoord = Vector2D(texCoords.vleftTop.x, texCoords.vleftTop.y);
			sheetQuad[3].color = color;
		}
	}
}
