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

CStaticAutoPtr<CEffectRenderer> effectrenderer;

IEffect::IEffect() :	m_vOrigin(0.0f),
						m_fStartLifeTime(0),
						m_fLifeTime(0),
						m_atlEntry(nullptr),
						m_atlGroup(nullptr),
						m_fDistanceToView(0.0f)
{
}

void IEffect::SetSortOrigin(const Vector3D &origin)
{
	m_fDistanceToView = length(origin - effectrenderer->m_viewPos);
}

//-------------------------------------------------------------------------------------

CEffectRenderer::CEffectRenderer()
{
	m_effectList.clear();
}

void CEffectRenderer::AddEffect(IEffect* pEffect)
{
	CScopedMutex m(s_effectRenderMutex);

	ASSERT_MSG(pEffect != nullptr, "RegisterEffectForRender - inserting NULL effect");

	if(m_effectList.numElem() >= m_effectList.numAllocated())
	{
		DevMsg(DEVMSG_CORE, "Effect list overflow!\n");

		// effect list overflow
		pEffect->DestroyEffect();
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
	{
		m_effectList[i]->DestroyEffect();
		delete m_effectList[i];
	}
	m_effectList.clear();
}

void CEffectRenderer::RemoveEffect(int index)
{
	if ( index >= m_effectList.numElem() || index < 0)
		return;

	CScopedMutex m(s_effectRenderMutex);

	IEffect* effect = m_effectList[index];
	if (m_effectList.fastRemoveIndex(index) && effect)
	{
		effect->DestroyEffect();
		delete effect;
	}
}

void CEffectRenderer::SetViewSortPosition(const Vector3D& origin)
{
	m_viewPos = origin;
}

Vector3D CEffectRenderer::GetViewSortPosition() const
{
	return m_viewPos;
}
