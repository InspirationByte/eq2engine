//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Editable entity parameters interface class
//////////////////////////////////////////////////////////////////////////////////

#ifndef IEDITABLEENTITYPARAMETERS_H
#define IEDITABLEENTITYPARAMETERS_H

#include "EditableEntity.h"
#include "wx_inc.h"

class CEditorViewRender;

// renderable and editable variable for entities
class IEntityVariable
{
public:

	IEntityVariable(CEditableEntity* pEnt, char* pszValue)
	{
		m_pEntity = pEnt;
		SetValue(pszValue);
	}

	virtual void SetValue(char* pszValue)
	{
		m_variable.SetValue(pszValue);
		Update();
	}

	// linking keys
	void SetKey(char* pszKey)
	{
		strcpy(m_pszKey, pszKey);
	}

	virtual void OnRemove() {}

	char* GetKey()
	{
		return m_pszKey;
	}

	virtual void		Update() {}

	virtual void		Render(int nViewRenderFlags) = 0;
	virtual void		RenderGhost(int nViewRenderFlags) = 0;
	virtual void		Render2D(CEditorViewRender* pViewRender) {}

	virtual bool		UpdateManipulation2D(CEditorViewRender* pViewRender, wxMouseEvent& mouseEvents, Vector3D &delta3D, Vector2D &delta2D) {return false;}

protected:
	// typed variable is here
	evariable_t			m_variable;

	// entity to use with this renderable variable
	// TODO: classdata for editable entity
	CEditableEntity*	m_pEntity;

	char				m_pszKey[128];
};

IEntityVariable*	CreateRegisteredVariable(char* pszType, CEditableEntity* pEnt, char* pszValue);

#endif // IEDITABLEENTITYPARAMETERS_H