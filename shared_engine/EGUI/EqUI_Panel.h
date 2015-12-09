//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: EGUI panel
//////////////////////////////////////////////////////////////////////////////////

#ifndef EGUI_PANEL_H
#define EGUI_PANEL_H

#include "math/dkmath.h"
#include "math/rectangle.h"
#include "utils/eqstring.h"
#include "utils/dklinkedlist.h"
#include "utils/KeyValues.h"

enum EqUI_Type_e
{
	EQUI_INVALID	= -1,

	EQUI_PANEL		= 0,
	EQUI_BUTTON,
	
	EQUI_TYPES,
};

static char* s_equi_typestrings[] = 
{
	"panel",
	"button",
	NULL
};

static EqUI_Type_e EqUI_ResolveElementTypeString(const char* pszTypeName)
{
	for(int i = 0; i < EQUI_TYPES; i++)
	{
		if(!s_equi_typestrings[i])
			break;

		if(!stricmp(s_equi_typestrings[i], pszTypeName))
			return (EqUI_Type_e)i;
	}

	return EQUI_INVALID;
}

// eq panel class
class CEqPanel
{
public:
	CEqPanel();

	virtual void			InitFromKeyValues( kvkeybase_t* pSection = NULL );
	virtual void			Shutdown();

	void					AddControl(CEqPanel* pControl);
	void					RemoveControl(CEqPanel* pControl);
	void					ClearControls(bool bFree = true);

	// returns child control 
	CEqPanel*				GetControl( const char* pszName );

	char*					GetName();
	void					SetName(const char* pszName);

	virtual void			SetColor(const ColorRGBA &color);
	virtual void			GetColor(ColorRGBA &color);

	virtual void			SetSelectionColor(const ColorRGBA &color);
	virtual void			GetSelectionColor(ColorRGBA &color);

	virtual void			Show();
	virtual void			Hide();

	virtual void			SetVisible(bool bVisible);

	virtual bool			IsVisible();

	virtual void			Render();

	virtual void			Enable();
	virtual void			Disable();

	virtual bool			IsEnabled();

	virtual bool			ProcessMouseEvents(float x, float y, int nMouseButtons, int nMouseFlags);
	virtual bool			ProcessKeyboardEvents(int nKeyButtons,int nKeyFlags);

	void					SetSize(const IVector2D& size);
	void					SetPosition(const IVector2D& pos);

	void					SetRect(const IRectangle& rect);

	void					GetSize(IVector2D& size);
	void					GetPosition(IVector2D& pos);

	// clipping rectangle, size position
	virtual IRectangle		GetRectangle();

	virtual bool			ProcessCommand( const char* pszCommand );

	virtual bool			ProcessCommandExecute( const char* pszCommand );

	void					SetParent(CEqPanel* pParent);
	CEqPanel*				GetParent();

protected:

	IVector2D				m_vPosition;
	IVector2D				m_vSize;

	bool					m_bVisible;
	bool					m_bEnabled;
	EqString				m_szName;

	DkLinkedList<CEqPanel*>	m_ChildPanels;		// child panels
	CEqPanel*				m_pParent;			// parent panel (null is a starting panel)

	ColorRGBA				m_vColor;
	ColorRGBA				m_vSelColor;

	CEqPanel*				m_nCurrentElement;	// keyboard, or mouseover element
};

#endif // EGUI_PANEL_H