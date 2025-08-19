//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Effect renderer
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/ConVar.h"
#include "EffectRender.h"

DECLARE_CVAR(r_maxEffects, "4096", "Maximum effects in pool", CV_ARCHIVE);

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

CEffectRenderer::CEffectRenderer()
{
	m_effectList.reserve(r_maxEffects.GetInt());
}

void CEffectRenderer::AddEffect(IEffect* pEffect)
{
	ASSERT_MSG(pEffect, "AddEffect - inserting NULL effect");

	CScopedMutex m(s_effectRenderMutex);
	if(m_effectList.isFull())
	{
		DevMsg(DEVMSG_CORE, "Effect list overflow!\n");
		pEffect->Release();
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
			m_effectList.fastRemoveIndex(i--);
			continue;
        }

		if(!m_effectList[i]->DrawEffect(dt))
		{
			m_effectList[i]->Release();
			m_effectList.fastRemoveIndex(i--);
		}
	}
}

void CEffectRenderer::RemoveAllEffects()
{
	CScopedMutex m(s_effectRenderMutex);

	for (IEffect* eff : m_effectList)
		eff->Release();
	m_effectList.clear();
}

void CEffectRenderer::SetViewSortPosition(const Vector3D& origin)
{
	m_viewPos = origin;
}

Vector3D CEffectRenderer::GetViewSortPosition() const
{
	return m_viewPos;
}
