//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Triggers
//////////////////////////////////////////////////////////////////////////////////

#include "BaseEngineHeader.h"
#include "model.h"
#include "Effects.h"

class CBaseRadialTrigger : public BaseEntity
{
public:
	DEFINE_CLASS_BASE(BaseEntity);

	CBaseRadialTrigger()
	{
		m_fRadius = 128;
		m_fDelay = 0.0f;
		m_fNextTriggerTime = 0.0f;
		m_pTriggered = NULL;
	}

	void Activate()
	{
		BaseEntity::Activate();
	}

	void Update(float dt)
	{
		if(m_fNextTriggerTime > gpGlobals->curtime)
			return;

		BaseEntity* pTriggered = NULL;

		for(int i = 0; i < ents->numElem(); i++)
		{
			BaseEntity* pCheckEnt = (BaseEntity*)ents->ptr()[i];

			if(pCheckEnt == this)
				continue;

			if(pCheckEnt->Classify() != ENTCLASS_PLAYER)
				continue; // we want player

			if(length(GetAbsOrigin() - pCheckEnt->GetAbsOrigin()) > m_fRadius)
				continue;

			variable_t var;
			m_OutputOnTrigger.FireOutput(var, pCheckEnt, this, 0);

			m_fNextTriggerTime = gpGlobals->curtime + m_fDelay;

			pTriggered = pCheckEnt;

			break; // only single object can trigger
		}

		if(pTriggered != NULL)
		{
			// if there is no objects that triggered before
			if(m_pTriggered == NULL)
			{
				m_pTriggered = pTriggered;

				// start triggering
				variable_t var;
				m_OutputOnStartTrigger.FireOutput(var, m_pTriggered, this, 0);
			}
		}
		else
		{
			if(m_pTriggered)
			{
				variable_t var;
				m_OutputOnEndTrigger.FireOutput(var, m_pTriggered, this, 0);

				// this object is no longer inside
				m_pTriggered = NULL;
			}
		}
	}
protected:
	DECLARE_DATAMAP();

	CBaseEntityOutput	m_OutputOnTrigger;
	CBaseEntityOutput	m_OutputOnStartTrigger;
	CBaseEntityOutput	m_OutputOnEndTrigger;

	BaseEntity*			m_pTriggered;

	float m_fRadius;
	float m_fDelay;
	float m_fNextTriggerTime;
};

// declare save data
BEGIN_DATAMAP(CBaseRadialTrigger)

	DEFINE_KEYFIELD( m_fRadius,	"radius", VTYPE_FLOAT),
	DEFINE_KEYFIELD( m_fDelay,	"delay", VTYPE_FLOAT),

	DEFINE_OUTPUT( "OnTrigger", m_OutputOnTrigger),
	DEFINE_OUTPUT( "OnStartTrigger", m_OutputOnStartTrigger),
	DEFINE_OUTPUT( "OnEndTrigger", m_OutputOnEndTrigger),

	DEFINE_FIELD( m_fNextTriggerTime, VTYPE_FLOAT),

END_DATAMAP()

DECLARE_ENTITYFACTORY(trigger_once_radial, CBaseRadialTrigger)

/*
DECLARE_ENTITYFACTORY(trigger_multiple, CBaseTrigger)

class CTriggerRemove : public BaseEntity
{
public:
	CTriggerRemove()
	{
		m_pModel = NULL;
	}

	void Activate()
	{
		if(!m_pModel)
		{
			int mdl = g_pModelCache->PrecacheModel(m_pszModelName.getData());
			m_pModel = g_pModelCache->GetModel(mdl);
		}

		BaseEntity::Activate();
	}

	void OnRemove()
	{

	}

	void Update(float dt)
	{
		DkList<BaseEntity*> TriggeredList;

		for(int i = 0; i < m_pModel->GetBrushCount(); i++)
		{
			for(int k = 0; k < ents->numElem(); k++)
			{
				BaseEntity* pCheckEnt = (BaseEntity*)ents->ptr()[k];
				if(!stricmp(pCheckEnt->GetClassname(), "worldspawn") || !stricmp(pCheckEnt->GetClassname(), "light") || pCheckEnt == this)
				{
					continue;
				}

				bool result = true;

				for(int j = 0; j < m_pModel->GetBrushes()[i].numFaces; j++)
				{
					if(m_pModel->GetBrushes()[i].faces[j]->Distance(pCheckEnt->GetAbsOrigin()) >= 0)
					{
						result = false;
						break;
					}
				}

				if(result && m_pModel->GetBrushes()[i].numFaces > 0)
				{
					TriggeredList.addUnique(pCheckEnt);
				}
			}
		}

		for(int i = 0; i < TriggeredList.numElem(); i++)
		{
			if(!stricmp(TriggeredList[i]->GetClassname(), "player"))
			{
				Msg("Actor removed!\n");
			}

			TriggeredList[i]->SetState(IEntity::ENTITY_REMOVE);
		}
	}

private:
	IEqModel*		m_pModel;
};


// KILL THIS!!!
DECLARE_ENTITYFACTORY(trigger_hurt, CTriggerRemove)
*/