//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: emitter object
//////////////////////////////////////////////////////////////////////////////////

#ifndef OBJECT_EMITTER_H
#define OBJECT_EMITTER_H

#include "GameObject.h"
#include "world.h"

enum EEffectEmitterType
{
	EFFECT_EMIT_NOTVALID	= -1,
	EFFECT_EMIT_SMOKE		= 0,
	EFFECT_EMIT_DEBRIS,		// = 1,
	EFFECT_EMIT_SPARKS,		// = 2,
	EFFECT_EMIT_WATER,		// = 3,
};

class CObject_EffectEmitter : public CGameObject
{
public:
	DECLARE_CLASS(CObject_EffectEmitter, CGameObject)

	CObject_EffectEmitter();
	CObject_EffectEmitter(kvkeybase_t* kvdata);
	~CObject_EffectEmitter();

	void					OnRemove();
	void					Spawn();

	void					Draw(int nRenderFlags);

	void					Simulate(float fDt);

	int						ObjType() const { return GO_EMITTER; }

protected:

	// smoke, debris, sparks
	ColorRGBA					m_startColor;
	ColorRGBA					m_endColor;
	Vector4D					m_angleDir;				// direction angle + spread angle
	DkList<TexAtlasEntry_t*>	m_atlasEntryList;
	float						m_startVelocity;
	float						m_startSize;			// or scale for fleck
	float						m_endSize;
	float						m_lifetime;				// particle life time
	float						m_rotation;
	float						m_gravity;
	float						m_windDrift;

	// emitter
	EEffectEmitterType			m_type;
	float						m_emitterLifeTime;		// time before it will be removed
	float						m_emitRate;				// emit rate per second
	int							m_numParticles;			// number of particles to emit at once
	float						m_posSpread;

	float						m_emitDelay;
};


#ifndef __INTELLISENSE__
OOLUA_PROXY(CObject_EffectEmitter, CGameObject)

OOLUA_PROXY_END
#endif //  __INTELLISENSE__

#endif // OBJECT_EMITTER_H