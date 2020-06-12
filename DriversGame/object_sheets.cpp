//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: flying sheets, leaves and newspapers
//////////////////////////////////////////////////////////////////////////////////

#include "object_sheets.h"
#include "world.h"
#include "eqParallelJobs.h"

#include "EqParticles.h"

#include "Shiny.h"

#define SHEET_COUNT							15
#define SHEET_GRAVITY						(-28.0f)
#define SHEET_MAX_FALL_VELOCITY				(-1.6f)
#define SHEET_MIN_AFFECTION_VELOCITY		(3.5f)
#define SHEET_MIN_AFFECTION_HEIGHT			(1.2f)

#define SHEET_ANGULAR_SCALE					(2.5f)
#define SHEET_VELOCITY_SCALE				(5.0f)
#define SHEETS_MAX_RAIN_DENSITY				(5.0f)
#define SHEET_REST_TIMEFACTOR				(4.0f)

extern CPFXAtlasGroup* g_translParticles;

CObject_Sheets::CObject_Sheets( kvkeybase_t* kvdata )
{
	m_keyValues = kvdata;
	m_hfObject = nullptr;
	m_initDelay = 2.0f;
}

CObject_Sheets::~CObject_Sheets()
{

}

void CObject_Sheets::OnRemove()
{
	BaseClass::OnRemove();

	if (m_hfObject)
	{
		g_pPhysics->m_physics.DestroyGhostObject(m_hfObject->m_object);
		m_hfObject->m_object = nullptr;
		delete m_hfObject;
		m_hfObject = nullptr;
	}
}

void CObject_Sheets::Spawn()
{
	BaseClass::Spawn();

	float radius = KV_GetValueFloat(m_keyValues->FindKeyBase("radius"), 0, 2.0f);

	CEqCollisionObject* triggerObject = new CEqCollisionObject();
	m_hfObject = new CPhysicsHFObject(triggerObject, this);

	triggerObject->Initialize( radius );

	triggerObject->SetCollideMask(0);
	triggerObject->SetContents(OBJECTCONTENTS_DEBRIS);

	triggerObject->SetPosition(GetOrigin());

	triggerObject->SetUserData(this);

	m_bbox = triggerObject->m_aabb_transformed;

	// Next call will add NO_RAYCAST, COLLISION_LIST and DISABLE_RESPONSE flags automatically
	g_pPhysics->m_physics.AddGhostObject(triggerObject);

	m_wasInit = InitSheets();
}

bool CObject_Sheets::InitSheets()
{
	kvkeybase_t* textures = m_keyValues->FindKeyBase("textures");

	float radius = KV_GetValueFloat(m_keyValues->FindKeyBase("radius"), 0, 2.0f);
	int count = KV_GetValueInt(m_keyValues->FindKeyBase("count"), 0, SHEET_COUNT);
	float scale = KV_GetValueFloat(m_keyValues->FindKeyBase("scale"), 0, 1.0f);
	float weight = KV_GetValueFloat(m_keyValues->FindKeyBase("weight"), 0, 1.0f);

	for(int i = 0; i < count; i++)
	{
		const char* textureName = KV_GetValueString(textures, RandomInt(0, textures->values.numElem()-1), "news1");

		sheetpart_t part;
		part.atlas = g_translParticles;	// TODO: other atlas
		part.entry = part.atlas->FindEntry( textureName );
		part.scale = scale;
		part.weight = weight;

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

void CObject_Sheets::OnPhysicsCollide(const CollisionPairData_t& pair)
{
	CEqCollisionObject* obj = pair.GetOppositeTo(m_hfObject->m_object);

	if (!obj)
		return;

	if (g_pGameWorld->m_envConfig.rainDensity > SHEETS_MAX_RAIN_DENSITY)
		return;

	// apply force to the object
	if (obj->IsDynamic())
	{
		CEqRigidBody* body = (CEqRigidBody*)obj;

		for (int i = 0; i < m_sheets.numElem(); i++)
		{
			sheetpart_t& sheet = m_sheets[i];

			float vel = length(body->GetVelocityAtWorldPoint(sheet.origin)) - SHEET_MIN_AFFECTION_VELOCITY;

			if (vel > 0.0f && fsimilar(sheet.origin.y, GetOrigin().y, SHEET_MIN_AFFECTION_HEIGHT))
			{
				float bodyToSheetDist = (float)length(body->GetPosition() - sheet.origin);

				float distToBodyFac = RemapValClamp(bodyToSheetDist, 0.0f, 3.5f, 0.0f, 1.0f);
				distToBodyFac = 1.0f - powf(distToBodyFac, 2.0f);

				sheet.velocity += (vel * distToBodyFac * SHEET_VELOCITY_SCALE * body->GetLastFrameTime()) / sheet.weight;
			}
		}
	}
}

void CObject_Sheets::UpdateSheets(float fDt)
{
	//g_worldGlobals.decalsQueue.Increment();

	for (int i = 0; i < m_sheets.numElem(); i++)
	{
		sheetpart_t& sheet = m_sheets[i];

		sheet.origin += vec3_up * sheet.velocity * fDt;
		sheet.angle += fabs(sheet.velocity) * fDt;

		CollisionData_t coll;
		if (!g_pPhysics->TestLine(sheet.origin + Vector3D(0, 4.0f, 0), sheet.origin - Vector3D(0, 0.1f, 0), coll, OBJECTCONTENTS_SOLID_GROUND))
		{
			//if(fDt > 0.0f && sheet.velocity > 0.0f)
			//	sheet.velocity *= 0.85f;

			const float maxFallVelocity = SHEET_MAX_FALL_VELOCITY;

			sheet.velocity += SHEET_GRAVITY * fDt;
			if (sheet.velocity < maxFallVelocity)
				sheet.velocity = maxFallVelocity;
		}
		else
		{
			sheet.velocity -= sign(sheet.velocity) * SHEET_REST_TIMEFACTOR * fDt;

			if (fabs(sheet.velocity) <= fDt * SHEET_REST_TIMEFACTOR)
				sheet.velocity = 0.0f;

			sheet.origin = coll.position + Vector3D(0, 0.05f, 0);
		}
	}

	bool canRender = g_pGameWorld->m_occludingFrustum.IsBoxVisible(m_bbox);
	m_bbox.Reset();

	PFXVertex_t* sheetQuad;

	ColorRGBA color(g_pGameWorld->m_info.ambientColor + g_pGameWorld->m_info.sunColor);

	for (int i = 0; i < m_sheets.numElem(); i++)
	{
		sheetpart_t& sheet = m_sheets[i];

		float featherAngle = sin(sheet.angle*2.0f) * fabs(clamp(sheet.velocity, -1.0f, 1.0f))*0.25f;
		featherAngle += SHEET_ANGULAR_SCALE * sinf(featherAngle);

		Vector3D sheetPos = sheet.origin + Vector3D(sin(sheet.angle)*1.5f, 0.015f, -cos(sheet.angle)*1.5f);
		m_bbox.AddVertex(sheetPos);

		if (!canRender)
			continue;

		Vector3D vRight, vUp;
		Vector3D sheetAngle(-90.0f + featherAngle * 65.0f, (featherAngle + sheet.angle)*25.0f, sheet.angle + featherAngle * 55.0f);

		AngleVectors(sheetAngle, NULL, &vUp, &vRight);

		Rectangle_t texCoords = sheet.entry->rect;

		Vector2D size(texCoords.GetSize() * sheet.scale);

		if (sheet.atlas->AllocateGeom(4, 4, &sheetQuad, NULL, true) != -1)
		{
			sheetQuad[0].point = sheetPos + (vUp * size.x) + (size.y * vRight);
			sheetQuad[0].texcoord = Vector2D(texCoords.vrightBottom.x, texCoords.vrightBottom.y);
			sheetQuad[0].color = color;

			sheetQuad[1].point = sheetPos + (vUp * size.x) - (size.y * vRight);
			sheetQuad[1].texcoord = Vector2D(texCoords.vrightBottom.x, texCoords.vleftTop.y);
			sheetQuad[1].color = color;

			sheetQuad[2].point = sheetPos - (vUp * size.x) + (size.y * vRight);
			sheetQuad[2].texcoord = Vector2D(texCoords.vleftTop.x, texCoords.vrightBottom.y);
			sheetQuad[2].color = color;

			sheetQuad[3].point = sheetPos - (vUp * size.x) - (size.y * vRight);
			sheetQuad[3].texcoord = Vector2D(texCoords.vleftTop.x, texCoords.vleftTop.y);
			sheetQuad[3].color = color;
		}
	}
}

void CObject_Sheets::SheetsUpdateJob(void *data, int i)
{
	CObject_Sheets* thisSheet = (CObject_Sheets*)data;
	thisSheet->UpdateSheets(g_pGameWorld->GetFrameTime());

	if (g_worldGlobals.mt.sheetsQueue.Decrement() == 0)
		g_worldGlobals.mt.sheetsCompleted.Raise();
}

void CObject_Sheets::Simulate( float fDt )
{
	BaseClass::Simulate(fDt);
	PROFILE_FUNC();

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

	if(g_worldGlobals.mt.sheetsQueue.Increment())
		g_worldGlobals.mt.sheetsCompleted.Clear();

	g_parallelJobs->AddJob(SheetsUpdateJob, this);
	g_parallelJobs->Submit();

}
