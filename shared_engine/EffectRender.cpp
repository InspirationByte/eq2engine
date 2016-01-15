//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Effect renderer
//////////////////////////////////////////////////////////////////////////////////

#include "EffectRender.h"
#include "core_base_header.h"
#include "IDebugOverlay.h"
#include "eqParallelJobs.h"
#include "eqGlobalMutex.h"

#pragma todo("non-indexed material group - use CPFXRenderGroup")

ConVar r_sorteffects("r_sorteffects", "1", "Sorts effects. If you disable it, effects will not be sorted.", CV_ARCHIVE);



int _SortParticles(IEffect* const &elem0, IEffect* const &elem1)
{
	if(!elem0 || !elem1)
		return 0;

	IEffect* pEffect0 = elem0;
	IEffect* pEffect1 = elem1;

	return pEffect1->GetDistanceToCamera() > pEffect0->GetDistanceToCamera();
}

IEffect::IEffect() :	m_vOrigin(0.0f),
						m_fStartLifeTime(0),
						m_fLifeTime(0),
						m_atlEntry(NULL),
						m_atlGroup(NULL),
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
	m_numEffects.SetValue(0);
	memset(m_pEffectList, 0, sizeof(m_pEffectList));
}

void CEffectRenderer::RegisterEffectForRender(IEffect* pEffect)
{
	Threading::CEqMutex& mutex = Threading::GetGlobalMutex(Threading::MUTEXPURPOSE_RENDERER);
	Threading::CScopedMutex m(mutex);

	ASSERTMSG(pEffect != NULL, "RegisterEffectForRender - inserting NULL effect");

	if(m_numEffects.GetValue() >= MAX_VISIBLE_EFFECTS)
	{
		DevMsg(1, "Effect list overflow!\n");

		// effect list overflow
		pEffect->DestroyEffect();
		delete pEffect;
		return;
	}

	m_pEffectList[m_numEffects.GetValue()] = pEffect;
	m_numEffects.Increment();
}

void CEffectRenderer::DrawEffects(float dt)
{
	Threading::CEqMutex& mutex = Threading::GetGlobalMutex(Threading::MUTEXPURPOSE_RENDERER);
	Threading::CScopedMutex m(mutex);

	// sort particles
	if(r_sorteffects.GetBool())
		shellSort<IEffect*>(m_pEffectList, m_numEffects.GetValue(), _SortParticles);

	for(int i = 0; i < m_numEffects.GetValue(); i++)
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
	Threading::CEqMutex& mutex = Threading::GetGlobalMutex(Threading::MUTEXPURPOSE_RENDERER);
	Threading::CScopedMutex m(mutex);

	for(int i = 0; i < m_numEffects.GetValue(); i++)
	{
		m_pEffectList[i]->DestroyEffect();
		delete m_pEffectList[i];
	}

	m_numEffects.SetValue(0);
	memset(m_pEffectList, 0, sizeof(m_pEffectList));
}

void CEffectRenderer::RemoveEffect(int index)
{
	if ( index >= m_numEffects.GetValue() || index < 0 )
		return;

    Threading::CEqMutex& mutex = Threading::GetGlobalMutex(Threading::MUTEXPURPOSE_RENDERER);
	Threading::CScopedMutex m(mutex);

	IEffect* effect = m_pEffectList[index];

	m_numEffects.Decrement();

	if ( m_numEffects.GetValue() > 0 && index != m_numEffects.GetValue() )
	{
		m_pEffectList[index] = m_pEffectList[m_numEffects.GetValue()];
		m_pEffectList[m_numEffects.GetValue()] = NULL;
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
