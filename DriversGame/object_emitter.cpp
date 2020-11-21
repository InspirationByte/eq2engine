//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: emitter object
//////////////////////////////////////////////////////////////////////////////////

#include "object_emitter.h"
#include "ParticleEffects.h"

extern CPFXAtlasGroup* g_translParticles;
extern CPFXAtlasGroup* g_additPartcles;

EEffectEmitterType ResolveEmitterType(const char* type)
{
	if (!stricmp(type, "smoke"))
		return EFFECT_EMIT_SMOKE;
	else if (!stricmp(type, "debris"))
		return EFFECT_EMIT_DEBRIS;
	else if (!stricmp(type, "sparks"))
		return EFFECT_EMIT_SPARKS;
	else if (!stricmp(type, "water"))
		return EFFECT_EMIT_WATER;

	return EFFECT_EMIT_NOTVALID;
}

CObject_EffectEmitter::CObject_EffectEmitter()
{
	m_emitDelay = 0.0f;
}

CObject_EffectEmitter::CObject_EffectEmitter(kvkeybase_t* kvdata) : CObject_EffectEmitter()
{
	m_keyValues = kvdata;
}

CObject_EffectEmitter::~CObject_EffectEmitter()
{

}

void CObject_EffectEmitter::OnRemove()
{
	BaseClass::OnRemove();
}

void CObject_EffectEmitter::Spawn()
{
	const char* emitterType = KV_GetValueString(m_keyValues->FindKeyBase("type"), 0, nullptr);

	if (!emitterType)
	{
		MsgWarning("Emitter type not specified for def '%s'\n", KV_GetValueString(m_keyValues));
		m_state = GO_STATE_REMOVE;
		return;
	}

	m_type = ResolveEmitterType(emitterType);

	if (m_type == EFFECT_EMIT_NOTVALID)
	{
		MsgWarning("Invalid emitter type '%s' for def '%s'\n", emitterType, KV_GetValueString(m_keyValues));
		m_state = GO_STATE_REMOVE;
		return;
	}

	// smoke, debris, sparks
	m_emitterLifeTime = KV_GetValueFloat(m_keyValues->FindKeyBase("emitter_lifetime"), 0, -1);
	m_emitRate = KV_GetValueFloat(m_keyValues->FindKeyBase("emitrate"), 0, 1.0f);
	m_numParticles = KV_GetValueInt(m_keyValues->FindKeyBase("particles"), 0, 1);
	m_posSpread = KV_GetValueFloat(m_keyValues->FindKeyBase("position_spread"), 0, 0.0f);

	m_lifetime = KV_GetValueFloat(m_keyValues->FindKeyBase("lifetime"), 0, 1.0f);
	m_rotation = KV_GetValueFloat(m_keyValues->FindKeyBase("rotation"), 0, 0.0f);
	m_gravity = KV_GetValueFloat(m_keyValues->FindKeyBase("gravity"), 0, -1.0f);
	m_startVelocity = KV_GetValueFloat(m_keyValues->FindKeyBase("velocity"), 0, 5.0f);
	m_windDrift = KV_GetValueFloat(m_keyValues->FindKeyBase("winddrift"), 0, 5.0f);

	const Vector3D dir_angles = KV_GetVector3D(m_keyValues->FindKeyBase("dir_angles"), 0, vec3_zero);
	const float spreadAngle = KV_GetValueFloat(m_keyValues->FindKeyBase("spread_angle"), 0, 5.0f);

	m_angleDir = Vector4D(dir_angles, DEG2RAD(spreadAngle));

	m_startColor = KV_GetVector4D(m_keyValues->FindKeyBase("color"), 0, ColorRGBA(1, 0, 1, 1));
	m_endColor = KV_GetVector4D(m_keyValues->FindKeyBase("color2"), 0, m_startColor);

	if (m_type == EFFECT_EMIT_SMOKE ||
		m_type == EFFECT_EMIT_SPARKS ||
		m_type == EFFECT_EMIT_WATER)
	{
		m_startSize = KV_GetValueFloat(m_keyValues->FindKeyBase("start_size"), 0, 1.0f);
		m_endSize = KV_GetValueFloat(m_keyValues->FindKeyBase("end_size"), 0, m_startSize);
	}
	else if (m_type == EFFECT_EMIT_DEBRIS)
	{
		m_startSize = KV_GetValueFloat(m_keyValues->FindKeyBase("scale"), 0, 1.0f);
	}

	// get atlas textures
	if (m_type == EFFECT_EMIT_SMOKE || m_type == EFFECT_EMIT_DEBRIS)
	{
		kvkeybase_t* atlasKv = m_keyValues->FindKeyBase("atlas_textures");

		if (atlasKv)
		{
			for (int i = 0; i < atlasKv->ValueCount(); i++)
			{
				const char* atlasEntryName = KV_GetValueString(atlasKv, i);
				
				// search in translucent atlas
				TexAtlasEntry_t* entry = g_translParticles->FindEntry(atlasEntryName);
				if (entry)
					m_atlasEntryList.append(entry);
				else
					MsgWarning("Invalid atlas entry of translucents '%s' for def '%s'\n", atlasEntryName, KV_GetValueString(m_keyValues));
			}
		}
	}

	// baseclass spawn
	BaseClass::Spawn();
}

void CObject_EffectEmitter::Draw(int nRenderFlags)
{
	// nothing to do
}

void CObject_EffectEmitter::Simulate(float fDt)
{
	const Vector3D origin = GetOrigin();

	if (m_emitterLifeTime != -1)
	{
		m_emitterLifeTime -= fDt;

		if (m_emitterLifeTime < 0)
		{
			m_state = GO_STATE_REMOVE;
		}
	}

	m_emitDelay -= fDt;

	if (m_emitDelay < 0.0f)
		m_emitDelay = 1.0f / m_emitRate;
	else
		return;

	Vector3D windDrift(m_windDrift, 0, m_windDrift);
	float spreadAngle = m_angleDir.w;

	// make particles
	if (m_type == EFFECT_EMIT_SMOKE)
	{
		for (int i = 0; i < m_numParticles; i++)
		{
			Vector3D emitPosition = origin + Vector3D(RandomFloat(-m_posSpread, m_posSpread),
													  RandomFloat(-m_posSpread, m_posSpread),
													  RandomFloat(-m_posSpread, m_posSpread));

			Vector3D angleSpread = Vector3D(RandomFloat(-spreadAngle, spreadAngle), 
											RandomFloat(-spreadAngle, spreadAngle), 
											RandomFloat(-spreadAngle, spreadAngle));

			Vector3D angle = m_angleDir.xyz() + angleSpread;
			Matrix3x3 dir = rotateXYZ3(DEG2RAD(angle.x), DEG2RAD(angle.y), DEG2RAD(angle.z));

			int randomAtlasEntryIdx = RandomInt(0, m_atlasEntryList.numElem() - 1);

			CSmokeEffect* pSmoke = new CSmokeEffect(emitPosition, dir.rows[2] * m_startVelocity + windDrift,
													m_startSize, m_endSize,
													RandomFloat(m_lifetime * 0.5f, m_lifetime),
													g_translParticles, m_atlasEntryList[randomAtlasEntryIdx],
													RandomFloat(-m_rotation, m_rotation), Vector3D(0, RandomFloat(m_gravity*0.5f, m_gravity), 0),
													m_startColor.xyz(), m_endColor.xyz());

			effectrenderer->RegisterEffectForRender(pSmoke);
		}
	}
	else if (m_type == EFFECT_EMIT_DEBRIS)
	{
		for (int i = 0; i < m_numParticles; i++)
		{
			

			Vector3D emitPosition = origin + Vector3D(RandomFloat(-m_posSpread, m_posSpread),
													  RandomFloat(-m_posSpread, m_posSpread),
													  RandomFloat(-m_posSpread, m_posSpread));

			Vector3D angleSpread = Vector3D(RandomFloat(-spreadAngle, spreadAngle),
											RandomFloat(-spreadAngle, spreadAngle),
											RandomFloat(-spreadAngle, spreadAngle));

			Vector3D angle = m_angleDir.xyz() + angleSpread;
			Matrix3x3 dir = rotateXYZ3(DEG2RAD(angle.x), DEG2RAD(angle.y), DEG2RAD(angle.z));

			int randomAtlasEntryIdx = RandomInt(0, m_atlasEntryList.numElem() - 1);

			CFleckEffect* pFleck = new CFleckEffect(emitPosition, dir.rows[2] * m_startVelocity + windDrift,
													Vector3D(0, RandomFloat(m_gravity * 0.5f, m_gravity), 0), m_startSize,
													RandomFloat(m_lifetime * 0.5f, m_lifetime),
													i & 1 ? m_rotation : -m_rotation,
													m_startColor.xyz(), g_translParticles, m_atlasEntryList[randomAtlasEntryIdx]);

			effectrenderer->RegisterEffectForRender(pFleck);
		}
	}
	else if (m_type == EFFECT_EMIT_SPARKS)
	{
		CPFXAtlasGroup* effgroup = g_additPartcles;
		TexAtlasEntry_t* entry = effgroup->FindEntry("tracer");

		for (int i = 0; i < m_numParticles; i++)
		{
			Vector3D emitPosition = origin + Vector3D(RandomFloat(-m_posSpread, m_posSpread),
				RandomFloat(-m_posSpread, m_posSpread),
				RandomFloat(-m_posSpread, m_posSpread));

			Vector3D angleSpread = Vector3D(RandomFloat(-spreadAngle, spreadAngle), 
				RandomFloat(-spreadAngle, spreadAngle), 
				RandomFloat(-spreadAngle, spreadAngle));

			Vector3D angle = m_angleDir.xyz() + angleSpread;
			Matrix3x3 dir = rotateXYZ3(DEG2RAD(angle.x), DEG2RAD(angle.y), DEG2RAD(angle.z));

			int randomAtlasEntryIdx = RandomInt(0, m_atlasEntryList.numElem() - 1);

			CSparkLine* pSpark = new CSparkLine(emitPosition,
				dir.rows[2] * m_startVelocity,	// velocity
				Vector3D(0, RandomFloat(m_gravity * 0.5f, m_gravity), 0),		// gravity
				RandomFloat(m_startSize, m_endSize) * 8.0f, // len
				RandomFloat(m_startSize * 0.7f, m_startSize * 1.2f) * 0.001f, RandomFloat(m_endSize * 0.7f, m_endSize * 1.2f) * 0.001f, // sizes
				RandomFloat(m_lifetime * 0.5f, m_lifetime),// lifetime
				g_additPartcles, entry);  // group - texture

			effectrenderer->RegisterEffectForRender(pSpark);
		}
	}
	else if (m_type == EFFECT_EMIT_WATER)
	{

	}
}