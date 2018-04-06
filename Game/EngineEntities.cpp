//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Engine entities
//////////////////////////////////////////////////////////////////////////////////

#include "BaseEngineHeader.h"
#include "EngineEntities.h"
#include "IGameRules.h"
#include "Effects.h"

#define ONE_BY_300 (0.00333)	// 1 / 300, magic constant. 128 is a default light radius

class CPointLight : public BaseEntity
{
public:
	
	// always do this type conversion
	typedef BaseEntity BaseClass;

	CPointLight()
	{
		m_bEnabled = true;
		m_fRadius = 300;
		color = ColorRGB(0.8f,0.8f,0.8f);
		m_bShadows = true;
		m_bMisc = false;
		m_bVolumetric = false;
		m_nFlicker = 0;
		m_vRadius3 = Vector3D(1);
		m_PointMatrix = identity3();
		m_fIntensity = 1.0f;

		m_pMaskTexture = NULL;

		m_szMaskTexture = "_matsys_white";

		m_pLightAreaList = NULL;
	}

	void Activate()
	{
		BaseClass::Activate();

		m_pLightAreaList = eqlevel->CreateRenderAreaList();

		m_nRooms = eqlevel->GetRoomsForSphere(GetAbsOrigin(), m_fRadius, m_pRooms);

		if(!m_nRooms)
			Msg("Light outside the world!\n");
	}

	void Precache()
	{
		m_pMaskTexture = g_pShaderAPI->LoadTexture(m_szMaskTexture.GetData(), TEXFILTER_TRILINEAR, TEXADDRESS_CLAMP, TEXFLAG_NOQUALITYLOD);
		m_pMaskTexture->Ref_Grab();
	}

	void OnRemove()
	{
		if(m_pMaskTexture)
			g_pShaderAPI->FreeTexture( m_pMaskTexture );

		if(m_pLightAreaList)
			eqlevel->DestroyRenderAreaList(m_pLightAreaList);

		BaseClass::OnRemove();
	}

	void OnPreRender()
	{
		if(!m_bEnabled)
			return;

		if( viewrenderer->GetAreaList() && viewrenderer->GetView() )
		{
			int view_rooms[2] = {-1, -1};
			viewrenderer->GetViewRooms(view_rooms);

			int cntr = 0;

			for(int i = 0; i < m_nRooms; i++)
			{
				if(m_pRooms[i] != -1)
				{
					if(!viewrenderer->GetAreaList()->area_list[ m_pRooms[i] ].doRender)
					{
						cntr++;
						continue;
					}

					if(!viewrenderer->GetAreaList()->area_list[ m_pRooms[i] ].IsSphereInside(m_vecAbsOrigin, m_fRadius))
					{
						cntr++;
						continue;
					}
				}
			}

			if(cntr == m_nRooms)
				return;
		}

		dlight_t *light = viewrenderer->AllocLight();

		SetLightDefaults(light);

		light->position = GetAbsOrigin();
		light->radius = m_fRadius;
		light->color = color;
		light->curintensity = m_fIntensity;
		light->intensity = m_fIntensity;

		light->extraData = m_pLightAreaList;

		if(!m_bShadows)
			light->nFlags |= LFLAG_NOSHADOWS;

		if(m_bMisc)
			light->nFlags |= LFLAG_MISCLIGHT;

		if(m_bVolumetric)
			light->nFlags |= LFLAG_VOLUMETRIC;

		//light->radius = m_vRadius3;

		if(m_nFlicker == 1)
		{
			float fac = sinf(sinf(gpGlobals->curtime*2))*0.5f - 0.5f;// + RandomFloat(-0.5f,0.1f);
			light->curintensity -= fac*1.5f;
		}
		else if(m_nFlicker == 2)
		{
			light->radius = fabs(sinf(gpGlobals->curtime*2.0f)) * m_fRadius;
		}

		light->pMaskTexture = m_pMaskTexture;
		light->lightRotation = m_PointMatrix;

		viewrenderer->AddLight(light);

		Vector3D f,r,u;

		f = m_PointMatrix.rows[2];
		r = m_PointMatrix.rows[0];
		u = m_PointMatrix.rows[1];

		debugoverlay->Line3D(GetAbsOrigin(),GetAbsOrigin()+r*90.0f,Vector4D(1,0,0, 1),Vector4D(1,0,0, 1));
		debugoverlay->Line3D(GetAbsOrigin(),GetAbsOrigin()+u*90.0f,Vector4D(0,1,0, 1),Vector4D(0,1,0, 1));
		debugoverlay->Line3D(GetAbsOrigin(),GetAbsOrigin()+f*90.0f,Vector4D(0,0,1, 1),Vector4D(0,0,1, 1));

		debugoverlay->Line3D(GetAbsOrigin()+f*90.0f,GetAbsOrigin()+f*85.0f+r*5.0f,Vector4D(0,0,1, 1),Vector4D(0,0,1, 1));
		debugoverlay->Line3D(GetAbsOrigin()+f*90.0f,GetAbsOrigin()+f*85.0f-r*5.0f,Vector4D(0,0,1, 1),Vector4D(0,0,1, 1));

		debugoverlay->Line3D(GetAbsOrigin()+f*90.0f,GetAbsOrigin()+f*85.0f+u*5.0f,Vector4D(0,0,1, 1),Vector4D(0,0,1, 1));
		debugoverlay->Line3D(GetAbsOrigin()+f*90.0f,GetAbsOrigin()+f*85.0f-u*5.0f,Vector4D(0,0,1, 1),Vector4D(0,0,1, 1));
	}

	void Toggle(inputdata_t &input)
	{
		m_bEnabled = !m_bEnabled;
	}

private:

	bool					m_bEnabled;
	float					m_fRadius;
	Vector3D				m_vRadius3;
	Vector3D				color;
	bool					m_bShadows;
	bool					m_bNoShadows;
	bool					m_bMisc;
	bool					m_bVolumetric;
	float					m_fIntensity;

	Matrix3x3				m_PointMatrix;

	int						m_nFlicker;

	ITexture*				m_pMaskTexture;

	EqString				m_szMaskTexture;

	renderAreaList_t*		m_pLightAreaList;

	int						m_pRooms[32];
	int						m_nRooms;

	DECLARE_DATAMAP();
};

// declare save data
BEGIN_DATAMAP(CPointLight)

	DEFINE_KEYFIELD( m_bEnabled,		"enabled", VTYPE_BOOLEAN ),
	DEFINE_KEYFIELD( color,			"color", VTYPE_VECTOR3D ),
	DEFINE_KEYFIELD( m_fIntensity,		"intensity", VTYPE_FLOAT ),
	DEFINE_KEYFIELD( m_fRadius,			"radius", VTYPE_FLOAT ),
	DEFINE_KEYFIELD( m_vRadius3,		"light_radius", VTYPE_ORIGIN ),
	DEFINE_KEYFIELD( m_bShadows,		"shadows", VTYPE_BOOLEAN ),
	DEFINE_KEYFIELD( m_bVolumetric,		"volumetric", VTYPE_BOOLEAN ),
	DEFINE_KEYFIELD( m_nFlicker,		"flicker", VTYPE_INTEGER ),
	DEFINE_KEYFIELD( m_szMaskTexture,	"mask", VTYPE_STRING ),
	DEFINE_KEYFIELD( m_bMisc,			"misc", VTYPE_BOOLEAN ),
	DEFINE_KEYFIELD( m_PointMatrix,		"rotation", VTYPE_MATRIX3X3 ),

	DEFINE_INPUTFUNC( "Toggle", Toggle )

END_DATAMAP()

DECLARE_ENTITYFACTORY(light_point,CPointLight)

DECLARE_CMD(make_light, "Creates light", CV_CHEAT)
{
	if(!viewrenderer->GetView())
		return;

	internaltrace_t tr;

	Vector3D forward;
	AngleVectors(g_pViewEntity->GetEyeAngles(), &forward);

	Vector3D pos = g_pViewEntity->GetEyeOrigin();

	physics->InternalTraceBox(pos, pos+forward*1000.0f, Vector3D(16,16,16), COLLISION_GROUP_WORLD, &tr);

	if(tr.fraction == 1.0f)
		return;

	CPointLight* pLight = (CPointLight*)entityfactory->CreateEntityByName("light_point");
	if(pLight)
	{
		pLight->SetAbsOrigin( tr.traceEnd );

		if(CMD_ARGC)
		{
			int cnt = 0;
			while(cnt < CMD_ARGC-1)
			{
				if(cnt+1 < CMD_ARGC)
				{
					Msg("key: %s\n", CMD_ARGV(cnt).c_str());
					pLight->SetKey(CMD_ARGV(cnt).c_str(), CMD_ARGV(cnt+1).GetData());
				}

				cnt += 2;
			}
		}

		pLight->Precache();
		pLight->Spawn();
		pLight->Activate();
	}
}

ConVar r_lights_correct_znear("r_lights_correct_znear", "1");

class CSpotLight : public BaseEntity
{
public:
	
	// always do this type conversion
	typedef BaseEntity BaseClass;

	CSpotLight()
	{
		m_fRadius = 300;
		color = ColorRGB(0.8f,0.8f,0.8f);
		m_fFov = 90;
		m_bShadows = true;
		m_bMisc = false;
		m_bVolumetric = false;
		m_bEnabled = true;
		m_nFlicker = 0;
		m_fIntensity = 1.0f;

		m_pMaskTexture = NULL;

		m_pLightAreaList = NULL;

		m_szMaskTexture = "_matsys_white";
	}

	void Activate()
	{
		BaseClass::Activate();

		m_pLightAreaList = eqlevel->CreateRenderAreaList();

		m_nRooms = eqlevel->GetRoomsForSphere(GetAbsOrigin(), m_fRadius, m_pRooms);

		if(!m_nRooms)
			Msg("Light outside the world!\n");
	}

	void Precache()
	{
		m_pMaskTexture = g_pShaderAPI->LoadTexture(m_szMaskTexture.GetData(), TEXFILTER_TRILINEAR, TEXADDRESS_CLAMP, TEXFLAG_NOQUALITYLOD);
		m_pMaskTexture->Ref_Grab();
	}

	void OnRemove()
	{
		if(m_pMaskTexture)
			g_pShaderAPI->FreeTexture(m_pMaskTexture);

		if(m_pLightAreaList)
			eqlevel->DestroyRenderAreaList(m_pLightAreaList);

		BaseClass::OnRemove();
	}

	void OnPreRender()
	{
		if(!m_bEnabled)
			return;

		if( viewrenderer->GetAreaList() && viewrenderer->GetView() )
		{
			int view_rooms[2] = {-1, -1};
			viewrenderer->GetViewRooms(view_rooms);

			int cntr = 0;

			for(int i = 0; i < m_nRooms; i++)
			{
				if(m_pRooms[i] != -1)
				{
					if(!viewrenderer->GetAreaList()->area_list[ m_pRooms[i] ].doRender)
					{
						cntr++;
						continue;
					}

					if(!viewrenderer->GetAreaList()->area_list[ m_pRooms[i] ].IsSphereInside(m_vecAbsOrigin, m_fRadius))
					{
						cntr++;
						continue;
					}
				}
			}

			if(cntr == m_nRooms)
				return;
		}

		dlight_t *light = viewrenderer->AllocLight();

		SetLightDefaults(light);

		light->nType = DLT_SPOT;
		light->angles = GetAbsAngles();
		light->position = GetAbsOrigin();
		light->radius = m_fRadius;
		light->color = color;
		light->curintensity = m_fIntensity;
		light->intensity = m_fIntensity;

		light->extraData = m_pLightAreaList;

		if(!m_bShadows)
			light->nFlags |= LFLAG_NOSHADOWS;

		if(m_bMisc)
			light->nFlags |= LFLAG_MISCLIGHT;

		if(m_bVolumetric)
			light->nFlags |= LFLAG_VOLUMETRIC;

		light->pMaskTexture = m_pMaskTexture;

		light->fovWH.z = m_fFov;

		light->radius = m_fRadius;

		if(m_nFlicker == 1)
		{
			float fac = sinf(12*sinf(gpGlobals->curtime*5))*0.5f - 0.5f + 1;// + RandomFloat(-0.1,0.1);
			light->color -= fac*0.6f;
		}
		else if(m_nFlicker == 2)
		{
			light->fovWH.z = fabs(sinf(gpGlobals->curtime*2.0f)) * m_fFov;
		}

		float fZNear = 1.0f;
		
		if(r_lights_correct_znear.GetBool())
			fZNear = m_fRadius * ONE_BY_300;

		Matrix4x4 spot_view = !rotateXYZ4(DEG2RAD(light->angles.x),DEG2RAD(light->angles.y),DEG2RAD(light->angles.z));
		Matrix4x4 spot_proj = perspectiveMatrixY(DEG2RAD(light->fovWH.z), light->fovWH.x,light->fovWH.y, fZNear, light->radius.x);
		
		light->lightRotation = transpose(spot_view.getRotationComponent());

		Vector3D f,r,u;
		f = spot_view.rows[2].xyz();
		r = spot_view.rows[0].xyz();
		u = spot_view.rows[1].xyz();

		debugoverlay->Line3D(light->position,light->position+r*90.0f,Vector4D(1,0,0, 1),Vector4D(1,0,0, 1));
		debugoverlay->Line3D(light->position,light->position+u*90.0f,Vector4D(0,1,0, 1),Vector4D(0,1,0, 1));
		debugoverlay->Line3D(light->position,light->position+f*90.0f,Vector4D(0,0,1, 1),Vector4D(0,0,1, 1));

		debugoverlay->Line3D(light->position+f*90.0f,light->position+f*85.0f+r*5.0f,Vector4D(0,0,1, 1),Vector4D(0,0,1, 1));
		debugoverlay->Line3D(light->position+f*90.0f,light->position+f*85.0f-r*5.0f,Vector4D(0,0,1, 1),Vector4D(0,0,1, 1));

		debugoverlay->Line3D(light->position+f*90.0f,light->position+f*85.0f+u*5.0f,Vector4D(0,0,1, 1),Vector4D(0,0,1, 1));
		debugoverlay->Line3D(light->position+f*90.0f,light->position+f*85.0f-u*5.0f,Vector4D(0,0,1, 1),Vector4D(0,0,1, 1));

		spot_view.translate(-light->position);

		light->lightWVP = spot_proj*spot_view;
		//light->lightView = spot_view;

		light->nFlags |= LFLAG_MATRIXSET;

		viewrenderer->AddLight(light);
	}

	void InputToggleFlicker(inputdata_t &data)
	{
		m_nFlicker = !m_nFlicker;
	}

private:

	Vector3D				color;
	float					m_fRadius;
	bool					m_bShadows;
	bool					m_bMisc;
	bool					m_bEnabled;
	float					m_fFov;
	bool					m_bVolumetric;
	float					m_fIntensity;

	int						m_nFlicker;

	ITexture*				m_pMaskTexture;

	EqString				m_szMaskTexture;

	renderAreaList_t*		m_pLightAreaList;

	int						m_pRooms[32];
	int						m_nRooms;

	DECLARE_DATAMAP();
};

// declare save data
BEGIN_DATAMAP(CSpotLight)

	DEFINE_KEYFIELD( m_bEnabled,			"enabled", VTYPE_BOOLEAN),
	DEFINE_KEYFIELD( color,				"color", VTYPE_VECTOR3D),
	DEFINE_KEYFIELD( m_fIntensity,			"intensity", VTYPE_FLOAT ),
	DEFINE_KEYFIELD( m_fRadius,				"radius", VTYPE_FLOAT),
	DEFINE_KEYFIELD( m_fFov,				"fov", VTYPE_FLOAT),
	DEFINE_KEYFIELD( m_bShadows,			"shadows", VTYPE_BOOLEAN),
	DEFINE_KEYFIELD( m_bVolumetric,			"volumetric", VTYPE_BOOLEAN),
	DEFINE_KEYFIELD( m_nFlicker,			"flicker", VTYPE_INTEGER),
	DEFINE_KEYFIELD( m_szMaskTexture,		"mask", VTYPE_STRING),
	DEFINE_KEYFIELD( m_bMisc,				"misc", VTYPE_BOOLEAN),

	DEFINE_INPUTFUNC( "ToggleFlicker",		InputToggleFlicker),

END_DATAMAP()

DECLARE_ENTITYFACTORY(light_spot, CSpotLight)

extern ConVar r_sun_osize;
extern ConVar r_sun_znear;
extern ConVar r_sun_zfar;

class CSunLight : public BaseEntity
{
public:
	
	// always do this type conversion
	typedef BaseEntity BaseClass;

	CSunLight()
	{
		color = ColorRGB(0.8f,0.8f,0.8f);
		m_bShadows = true;

		m_pLightAreaList = NULL;
	}

	void Activate()
	{
		BaseClass::Activate();

		m_pLightAreaList = eqlevel->CreateRenderAreaList();
	}

	void OnRemove()
	{
		if(m_pLightAreaList)
			eqlevel->DestroyRenderAreaList(m_pLightAreaList);

		BaseClass::OnRemove();
	}

	void OnPreRender()
	{
		dlight_t *light = viewrenderer->AllocLight();

		SetLightDefaults(light);

		light->nType = DLT_SUN;
		light->angles = GetAbsAngles();
		light->position = GetAbsOrigin();
		light->radius = -1;
		light->color = color;
		light->curintensity = 1.0f;
		light->intensity = 1.0f;

		if(!m_bShadows)
			light->nFlags |= LFLAG_NOSHADOWS;

		light->pMaskTexture = NULL;

		light->fovWH.z = -1;

		viewrenderer->AddLight( light );
	}

private:

	Vector3D				color;

	bool					m_bShadows;

	renderAreaList_t*		m_pLightAreaList;

	DECLARE_DATAMAP();
};

// declare save data
BEGIN_DATAMAP(CSunLight)

	DEFINE_KEYFIELD( color,				"color", VTYPE_VECTOR3D),
	DEFINE_KEYFIELD( m_bShadows,			"shadows", VTYPE_BOOLEAN),

END_DATAMAP()

DECLARE_ENTITYFACTORY(light_sun, CSunLight)

class CInfoNull : public BaseEntity
{

};

DECLARE_ENTITYFACTORY(info_null,CInfoNull)

// declare save data
BEGIN_DATAMAP(CWorldInfo)
	DEFINE_KEYFIELD( m_AmbientColor,	"ambient", VTYPE_VECTOR3D),
	DEFINE_KEYFIELD( m_fZFar,			"zfar", VTYPE_FLOAT),
	DEFINE_KEYFIELD( m_szMenuName,		"menu", VTYPE_STRING),

END_DATAMAP()

DECLARE_CMD(make_sun, "Creates sun", CV_CHEAT)
{
	if(!viewrenderer->GetView())
		return;

	internaltrace_t tr;

	Vector3D forward;
	AngleVectors(g_pViewEntity->GetEyeAngles(), &forward);

	CSunLight* pSun = (CSunLight*)entityfactory->CreateEntityByName("light_sun");

	if(pSun)
	{
		Vector3D pos = g_pViewEntity->GetEyeOrigin();
		Vector3D ang = g_pViewEntity->GetEyeAngles();

		pSun->SetAbsOrigin( pos );
		pSun->SetAbsAngles( ang );

		if(CMD_ARGC)
		{
			int cnt = 0;
			while(cnt < CMD_ARGC-1)
			{
				if(cnt+1 < CMD_ARGC)
				{
					Msg("key: %s\n", CMD_ARGV(cnt).c_str());
					pSun->SetKey(CMD_ARGV(cnt).c_str(), CMD_ARGV(cnt+1).c_str());
				}

				cnt += 2;
			}
		}

		pSun->Precache();
		pSun->Spawn();
		pSun->Activate();
	}
}

DECLARE_ENTITYFACTORY(worldinfo,CWorldInfo)

CWorldInfo::CWorldInfo()
{
	m_AmbientColor = Vector3D(0);
	m_fZFar = 2000;
	m_szMenuName = "";
}

ColorRGB CWorldInfo::GetAmbient()
{
	return m_AmbientColor;
}

float CWorldInfo::GetLevelRenderableDist()
{
	return m_fZFar;
}

extern void SetWorldSpawn(BaseEntity* pEntity);

void CWorldInfo::Activate()
{
	BaseClass::Activate();
	SetWorldSpawn(this);
}

const char* CWorldInfo::GetMenuName()
{
	return m_szMenuName.GetData();
}

void UTIL_AppendCylinder(int lengthSegs, int heightSegs, DkList<eqropevertex_t>& vertices, DkList<uint16>& indices, float fTexLength)
{
	heightSegs += 1; // we need an extra one here!

	int startvertex = vertices.numElem(); // if v is empty, the size is 0, so we are starting on an empty buffer, but if there is data, this will append it correctly

	int totalverts = lengthSegs*heightSegs;
	vertices.resize(startvertex + totalverts);

	float heightstride = 1.0f/((float)lengthSegs);
	float height = -heightstride;
	float deg = 0;

	float tex_size = float(heightSegs-1);

	for(int i = 0; i < lengthSegs; i++)
	{
		height += heightstride;

		for(int j = 0; j < heightSegs; j++)
		{
			eqropevertex_t vertex;
			vertex.point = Vector3D(cos(DEG2RAD(deg)), /*height*10*/0.0f, sin(DEG2RAD(deg) ) );
			vertex.texCoord_segIdx = Vector3D(float(j)/tex_size, ((float)i+1)/(fTexLength*0.015f*0.05f), (float)i);
			vertex.normal = normalize(vertex.point);

			vertices.append(vertex);

			deg += 360.0f/(float(heightSegs-1));
		}

		deg = 0.0f;
	}
	int nind = (6*(lengthSegs-1)*(heightSegs-1));

	int startindex = indices.numElem();

	indices.resize(nind + startindex);

	for(int i = 0; i < lengthSegs-1; i++)
	{
		for(int j = 0; j < heightSegs-1; j++)
		{
			indices.append(startvertex + i*heightSegs + j + 1);
			indices.append(startvertex + (i+1)*heightSegs + j);
			indices.append(startvertex + i*heightSegs + j);

			indices.append(startvertex + (i+1)*heightSegs + j + 1);
			indices.append(startvertex + (i+1)*heightSegs + j);
			indices.append(startvertex + i*heightSegs + j + 1);
		}
	}
}

struct ropequaternion_t
{
	Vector4D	quat;
	Vector4D	origin;
};

// Decal entity
class CEnvRopePoint : public BaseEntity
{
	// always do this type conversion
	typedef BaseEntity BaseClass;

public:
	CEnvRopePoint()
	{
		m_pRope = NULL;
		m_pParentRope = NULL;

		m_numSegments = 4;

		m_bHasGeometry = false;

		m_pRopeMaterial = NULL;

		m_nRooms = 0;

		m_szRopeMaterial = "ropes/rope_01";
	}

	void Precache()
	{
		m_pRopeMaterial = materials->GetMaterial(m_szRopeMaterial.GetData(), true);
		m_pRopeMaterial->Ref_Grab();
	}

	void CreateRopeSkinningGeometry(int node_count, float rope_len)
	{
		if(m_bHasGeometry)
			return;

		int start_vertex = g_rope_verts.numElem();
		int start_idx = g_rope_indices.numElem();

		// appends cylinder to the existing geometry buffer
		UTIL_AppendCylinder(node_count, 8, g_rope_verts, g_rope_indices, rope_len);
		
		//int numVerts = g_rope_verts.numElem() - start_vertex;
		int numIndices = g_rope_indices.numElem() - start_idx;

		m_startIndex = start_idx;
		m_startVertex = start_vertex;
		m_numIndices = numIndices;

		m_bHasGeometry = true;
	}

	void CreateLink()
	{
		CEnvRopePoint* pTargetRopePoint = dynamic_cast<CEnvRopePoint*>(GetTargetEntity());
		if(!pTargetRopePoint)
		{
			return;
		}

		//Msg("Creating link...\n");

		m_pRope = physics->CreateRope(GetAbsOrigin(), pTargetRopePoint->GetAbsOrigin(), m_numSegments);
		m_pRope->SetFullMass(25);
		m_pRope->SetNodeMass(0, 0);
		m_pRope->SetIterations(10);
		m_pRope->SetLinearStiffness(1.0f);
		m_pRope->SetNodeMass(m_pRope->GetNodeCount()-1, 0);

		pTargetRopePoint->SetParentRope( this );

		float len = length(GetAbsOrigin() - pTargetRopePoint->GetAbsOrigin());

		CreateRopeSkinningGeometry( m_pRope->GetNodeCount(), len );

		Vector3D bbox_center = (GetAbsOrigin() + pTargetRopePoint->GetAbsOrigin())*0.5f;
		Vector3D bbox_size = GetAbsOrigin() - pTargetRopePoint->GetAbsOrigin();

		m_nRooms = eqlevel->GetRoomsForSphere(bbox_center, length(bbox_size), m_pRooms);

		if(!m_nRooms)
			Msg("rope outside the world!\n");
	}

	void OnGameStart()
	{
		CheckTargetEntity();

		if(GetTargetEntity())
		{
			CreateLink();

			if(!m_pRope && m_pParentRope && GetTargetEntity()->PhysicsGetObject())
			{
				m_pParentRope->m_pRope->SetFullMass(25);
				m_pParentRope->m_pRope->SetNodeMass(0, 0);
				m_pParentRope->m_pRope->SetNodeMass(m_pParentRope->m_pRope->GetNodeCount()-1, 5);

				m_pParentRope->m_pRope->AttachObject(GetTargetEntity()->PhysicsGetObject(), m_pParentRope->m_pRope->GetNodeCount()-1);

				inputdata_t in;
				in.id = 0;
				in.value.SetInt(0);
				in.pCaller = GetTargetEntity();
				InputAttach(in);

				//MsgWarning("Attaching object\n");
			}
			else
			{
				if(!m_pRope)
				{
					MsgWarning("No parent rope or physics object (in %s)\n", GetName());

					SetState(ENTITY_REMOVE);
					return;
				}
			}
		}
	}

	void OnRemove()
	{
		if(m_bHasGeometry)
		{
			materials->FreeMaterial( m_pRopeMaterial );
		}

		if(m_pRope)
		{
			physics->DestroyPhysicsRope(m_pRope);
			m_pRope = NULL;
		}

		BaseClass::OnRemove();
	}

	void OnPreRender()
	{
		if(m_bHasGeometry)
		{
			g_modelList->AddRenderable( this );

			if(m_pRope && m_pRope->GetNodeCount() > 0)
			{
				m_vBBox.Reset();

				for(int i = 0; i < m_pRope->GetNodeCount(); i++)
					m_vBBox.AddVertex(m_pRope->GetNodePosition(i));

				//GetTargetEntity()->SetAbsOrigin( m_pRope->GetNodePosition(m_pRope->GetNodeCount()-1) );
			}
		}
	}

	// returns world transformation of this object
	Matrix4x4 GetRenderWorldTransform()
	{
		return identity4();
	}

	void Render(int nViewRenderFlags)
	{
		if(!m_bHasGeometry || !m_pRope)
			return;

		//if(viewrenderer->GetDrawMode() == VDM_SHADOW)
		//	return;

		if((nViewRenderFlags & VR_FLAG_NO_TRANSLUCENT) && (m_pRopeMaterial->GetFlags() & MATERIAL_FLAG_TRANSPARENT))
			return;

		if((nViewRenderFlags & VR_FLAG_NO_OPAQUE) && !(m_pRopeMaterial->GetFlags() & MATERIAL_FLAG_TRANSPARENT))
			return;

		// render if in frustum
		Matrix4x4 wvp;
		Matrix4x4 world = GetRenderWorldTransform();

		materials->SetMatrix(MATRIXMODE_WORLD, GetRenderWorldTransform());

		if(materials->GetLight() && materials->GetLight()->nFlags & LFLAG_MATRIXSET)
			wvp = materials->GetLight()->lightWVP * world;
		else 
			materials->GetWorldViewProjection(wvp);

		Volume frustum;
		frustum.LoadAsFrustum(wvp);
		
		Vector3D mins = GetBBoxMins();
		Vector3D maxs = GetBBoxMaxs();

		if(!frustum.IsBoxInside(mins.x,maxs.x,mins.y,maxs.y,mins.z,maxs.z))
			return;

		if( viewrenderer->GetAreaList() && viewrenderer->GetView() )
		{
			float bbox_size = length(mins-maxs);

			int view_rooms[2] = {-1, -1};
			viewrenderer->GetViewRooms(view_rooms);

			int cntr = 0;

			for(int i = 0; i < m_nRooms; i++)
			{
				if(m_pRooms[i] != -1)
				{
					if(!viewrenderer->GetAreaList()->area_list[ m_pRooms[i] ].doRender)
					{
						cntr++;
						continue;
					}

					if(!viewrenderer->GetAreaList()->area_list[ m_pRooms[i] ].IsSphereInside(m_vecAbsOrigin, bbox_size))
					{
						cntr++;
						continue;
					}
				}
			}

			if(cntr == m_nRooms)
				return;
		}
		
		materials->SetSkinningEnabled( true );

		if(!(nViewRenderFlags & VR_FLAG_NO_MATERIALS))
		{
			materials->BindMaterial( m_pRopeMaterial, false );
		}
		else
		{
			if(viewrenderer->GetDrawMode() == VDM_SHADOW)
			{
				viewrenderer->UpdateDepthSetup(false, VR_SKIN_SIMPLEMODEL);

				float fAlphatestFac = 0.0f;

				if(m_pRopeMaterial->GetFlags() & MATERIAL_FLAG_ALPHATESTED)
					fAlphatestFac = 50.0f;

				if(m_pRopeMaterial->GetFlags() & MATERIAL_FLAG_NOCULL)
					materials->SetCullMode(CULL_NONE);

				g_pShaderAPI->SetShaderConstantFloat("AlphaTestFactor", fAlphatestFac);
				g_pShaderAPI->SetTexture(m_pRopeMaterial->GetBaseTexture(0));
			}
		}

		// apply bones

		static ropequaternion_t quats[128];
		memset(quats,0,sizeof(ropequaternion_t)*128);

		int numNodes = m_pRope->GetNodeCount();

		Vector3D rope_pos = m_pRope->GetNodePosition(0);

		// Send all matrices as 4x3
		for(int i = 1; i < numNodes; i++)
		{
			Vector3D ropePoint = m_pRope->GetNodePosition(i);

			Vector3D ropeNormal = normalize(ropePoint - rope_pos);

			// kind of slowness
			Vector3D angle = VectorAngles(ropeNormal);
			Vector3D radangle = VDEG2RAD(angle);

			debugoverlay->Line3D(rope_pos, ropePoint, ColorRGBA(1,0,0,1), ColorRGBA(1,0,0,1));
			debugoverlay->Line3D(rope_pos, rope_pos+ropeNormal*10.0f, ColorRGBA(0,0,1,1), ColorRGBA(0,0,1,1));

			Matrix4x4 toAbsTransform = (rotateXYZ4(DEG2RAD(90),0.0f,DEG2RAD(90))*rotateZXY4(radangle.x,radangle.y,radangle.z))*scale4(1.0f, 1.0f, 1.0f);

			Matrix3x3 rot(toAbsTransform.getRotationComponent());

			quats[i-1].quat = Quaternion(rot).asVector4D();
			quats[i-1].origin = Vector4D(rope_pos, 1);

			quats[i].quat = Quaternion(rot).asVector4D();
			quats[i].origin = Vector4D(ropePoint, 1);

			rope_pos = ropePoint;
		}

		g_pShaderAPI->SetShaderConstantArrayVector4D("Bones", (Vector4D*)&quats[0].quat, numNodes*2);

		GR_SetupDynamicsIBO();

		materials->Apply();

		g_pShaderAPI->DrawIndexedPrimitives(PRIM_TRIANGLES, m_startIndex, m_numIndices, m_startVertex, m_numIndices);

		materials->SetSkinningEnabled( false );
	}

	void SetParentRope(CEnvRopePoint* pRope)
	{
		m_pParentRope = pRope;
	}

	CEnvRopePoint* GetParentRope()
	{
		return m_pParentRope;
	}

protected:
	
	IPhysicsRope*	m_pRope;
	CEnvRopePoint*	m_pParentRope;

	int				m_numSegments;

	int				m_startIndex;
	int				m_startVertex;
	int				m_numIndices;
	bool			m_bHasGeometry;

	IMaterial*		m_pRopeMaterial;

	EqString		m_szRopeMaterial;

	DECLARE_DATAMAP();
};

// declare save data
BEGIN_DATAMAP(CEnvRopePoint)
	DEFINE_KEYFIELD( m_numSegments, "segments", VTYPE_INTEGER),
	DEFINE_KEYFIELD( m_szRopeMaterial, "material", VTYPE_STRING),
END_DATAMAP()

DECLARE_ENTITYFACTORY(env_ropepoint, CEnvRopePoint)

class CDevText : public BaseEntity
{
	// always do this type conversion
	typedef BaseEntity BaseClass;

public:
	CDevText()
	{
		m_fRadius = 256;
		m_pText = "No text!";
	}

	void OnPreRender()
	{
		CBaseActor* pActor = (CBaseActor*)UTIL_EntByIndex(1);

		debugoverlay->Text3D(GetAbsOrigin(),m_fRadius, ColorRGBA(1), 0.0f, "%s", m_pText.GetData());
	}

	void Input_SetText(inputdata_t &data)
	{
		// if you want 
		m_pText = data.value.GetString();
	}

protected:
	float		m_fRadius;
	EqString	m_pText;
	DECLARE_DATAMAP();
};

// declare save data
BEGIN_DATAMAP(CDevText)
	DEFINE_KEYFIELD( m_fRadius, "radius", VTYPE_FLOAT),
	DEFINE_KEYFIELD( m_pText, "text", VTYPE_STRING),
	DEFINE_INPUTFUNC("SetText", Input_SetText),
END_DATAMAP()

DECLARE_ENTITYFACTORY(dev_text,CDevText)

class CInfoWorldDecal : public BaseEntity
{
	// always do this type conversion
	typedef BaseEntity BaseClass;

public:
	CInfoWorldDecal()
	{
		m_matName = "error";
		m_pDecalMaterial = NULL;
		m_pDecal = NULL;
	}

	void Precache()
	{
		m_pDecalMaterial = materials->GetMaterial(m_matName.GetData());
		m_pDecalMaterial->Ref_Grab();
	}

	void Spawn()
	{
		BaseClass::Spawn();

		decalmakeinfo_t info;

		if(m_project)
			info.flags |= MAKEDECAL_FLAG_TEX_NORMAL;

		info.pMaterial = m_pDecalMaterial;
		info.normal = m_normal;
		info.origin = GetAbsOrigin();
		info.size = m_size;
		info.texRotation = m_texrotate;
		info.texScale = m_texscale;

		m_pDecal = eqlevel->MakeStaticDecal( info );
	}

	void OnRemove()
	{
		if(m_pDecal)
			eqlevel->FreeStaticDecal(m_pDecal);

		BaseClass::OnRemove();
		materials->FreeMaterial(m_pDecalMaterial);
	}

protected:
	EqString		m_matName;
	IMaterial*		m_pDecalMaterial;
	staticdecal_t*	m_pDecal;

	Vector3D		m_size;
	Vector3D		m_normal;
	Vector2D		m_texscale;
	float			m_texrotate;

	bool			m_project;

	DECLARE_DATAMAP();
};

// declare save data
BEGIN_DATAMAP(CInfoWorldDecal)

	DEFINE_KEYFIELD( m_matName, "material", VTYPE_STRING),
	DEFINE_KEYFIELD( m_size, "size", VTYPE_VECTOR3D),
	DEFINE_KEYFIELD( m_normal, "normal", VTYPE_VECTOR3D),
	DEFINE_KEYFIELD( m_texscale, "texscale", VTYPE_VECTOR2D),
	DEFINE_KEYFIELD( m_texrotate, "texrotate", VTYPE_FLOAT),
	DEFINE_KEYFIELD( m_project, "projected", VTYPE_BOOLEAN),

END_DATAMAP()

DECLARE_ENTITYFACTORY(info_worlddecal,CInfoWorldDecal)
