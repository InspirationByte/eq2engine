//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Effect renderer
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IEqParallelJobs.h"
#include "core/ConVar.h"
#include "utils/TextureAtlas.h"
#include "EffectRender.h"

#include "render/IDebugOverlay.h"

using namespace Threading;
static CEqMutex s_effectRenderMutex;

CStaticAutoPtr<CEffectRenderer> g_effectRenderer;

void IEffect::SetSortOrigin(const Vector3D &origin)
{
	m_distToView = lengthSqr(origin - g_effectRenderer->m_viewPos);
}

void IEffect::InternalInit(const Vector3D& origin, float lifetime, PFXAtlasRef atlasRef)
{
	ASSERT(lifetime > F_EPS);

	m_origin = origin;
	SetSortOrigin(origin);

	m_lifeTimeRcp = 1.0f / lifetime;
	m_lifeTime = lifetime;

	m_atlasRef = atlasRef;
}

//-------------------------------------------------------------------------------------

void CEffectRenderer::AddEffect(IEffect* pEffect)
{
	CScopedMutex m(s_effectRenderMutex);

	ASSERT_MSG(pEffect != nullptr, "RegisterEffectForRender - inserting NULL effect");

	if(m_effectList.numElem() >= m_effectList.numAllocated())
	{
		DevMsg(DEVMSG_CORE, "Effect list overflow!\n");
		delete pEffect;
		return;
	}

	m_effectList.append(pEffect);
}

void CEffectRenderer::DrawEffects(float dt)
{
	PROF_EVENT("Effect Renderer Draw");

	CScopedMutex m(s_effectRenderMutex);

	// sort particles
	arraySort(m_effectList, [](IEffect* effect0, IEffect* effect1)
	{
		return sortCompare(effect1->GetDistanceToCamera(), effect0->GetDistanceToCamera());
	});

	for(int i = 0; i < m_effectList.numElem(); i++)
	{
        if(!m_effectList[i])
        {
			RemoveEffect(i--);
			continue;
        }

		if(!m_effectList[i]->DrawEffect(dt))
			RemoveEffect(i--);
	}
}

void CEffectRenderer::RemoveAllEffects()
{
	CScopedMutex m(s_effectRenderMutex);

	for(int i = 0; i < m_effectList.numElem(); i++)
		delete m_effectList[i];

	m_effectList.clear();
}

void CEffectRenderer::RemoveEffect(int index)
{
	if ( index >= m_effectList.numElem() || index < 0)
		return;

	CScopedMutex m(s_effectRenderMutex);

	IEffect* effect = m_effectList[index];
	if (m_effectList.fastRemoveIndex(index) && effect)
		delete effect;
}

void CEffectRenderer::SetViewSortPosition(const Vector3D& origin)
{
	m_viewPos = origin;
}

Vector3D CEffectRenderer::GetViewSortPosition() const
{
	return m_viewPos;
}
