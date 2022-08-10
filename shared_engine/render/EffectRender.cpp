//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
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

static int _SortParticles(IEffect* const &effect0, IEffect* const &effect1)
{
	return effect0->GetDistanceToCamera() - effect1->GetDistanceToCamera();
}

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
	m_numEffects = 0;
	memset(m_pEffectList, 0, sizeof(m_pEffectList));
}

void CEffectRenderer::AddEffect(IEffect* pEffect)
{
	CScopedMutex m(s_effectRenderMutex);

	ASSERT_MSG(pEffect != nullptr, "RegisterEffectForRender - inserting NULL effect");

	if(m_numEffects >= MAX_VISIBLE_EFFECTS)
	{
		DevMsg(DEVMSG_CORE, "Effect list overflow!\n");

		// effect list overflow
		pEffect->DestroyEffect();
		delete pEffect;
		return;
	}

	m_pEffectList[m_numEffects++] = pEffect;
}

void CEffectRenderer::DrawEffects(float dt)
{
	CScopedMutex m(s_effectRenderMutex);

	// sort particles
	quickSort<IEffect*>(m_pEffectList, _SortParticles, 0, m_numEffects-1);

	for(int i = 0; i < m_numEffects; i++)
	{
        if(!m_pEffectList[i])
        {
			RemoveEffect(i);
			i--;
			continue;
        }

		if(!m_pEffectList[i]->DrawEffect(dt))
		{
			RemoveEffect(i);
			i--;
		}
	}
}

void CEffectRenderer::RemoveAllEffects()
{
	CScopedMutex m(s_effectRenderMutex);

	for(int i = 0; i < m_numEffects; i++)
	{
		m_pEffectList[i]->DestroyEffect();
		delete m_pEffectList[i];
	}

	m_numEffects = 0;
	memset(m_pEffectList, 0, sizeof(m_pEffectList));
}

void CEffectRenderer::RemoveEffect(int index)
{
	if ( index >= m_numEffects || index < 0 )
		return;

	CScopedMutex m(s_effectRenderMutex);

	IEffect* effect = m_pEffectList[index];

	--m_numEffects;

	if ( m_numEffects > 0 && index != m_numEffects )
	{
		m_pEffectList[index] = m_pEffectList[m_numEffects];
		m_pEffectList[m_numEffects] = nullptr;
	}

	if(effect)
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

static CEffectRenderer g_effectRenderer;
CEffectRenderer* effectrenderer = &g_effectRenderer;
