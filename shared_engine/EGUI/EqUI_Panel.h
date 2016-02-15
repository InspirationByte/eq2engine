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
	EQUI_IMAGE,
	EQUI_TEXTINPUT,
	
	EQUI_TYPES,
};

static char* s_equi_typestrings[] = 
{
	"panel",
	"button",
	"image",
	"textinput",
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
class CEqUI_Panel
{
public:
	CEqUI_Panel();

	virtual void			InitFromKeyValues( kvkeybase_t* pSection = NULL );
	virtual void			Shutdown();

	void					AddChild(CEqUI_Panel* pControl);
	void					RemoveChild(CEqUI_Panel* pControl);
	void					ClearChilds(bool bFree = true);

	// returns child control 
	CEqUI_Panel*			GetControl( const char* pszName );

	const char*				GetName() const;
	void					SetName(const char* pszName);

	virtual int				GetType() const {return EQUI_PANEL;}

	virtual void			SetColor(const ColorRGBA &color);
	virtual void			GetColor(ColorRGBA &color) const;

	virtual void			SetSelectionColor(const ColorRGBA &color);
	virtual void			GetSelectionColor(ColorRGBA &color) const;

	virtual void			Show();
	virtual void			Hide();

	virtual void			SetVisible(bool bVisible);
	virtual bool			IsVisible() const;

	virtual void			Render();

	virtual void			Enable();
	virtual void			Disable();

	virtual bool			IsEnabled() const;

	virtual bool			ProcessMouseEvents(float x, float y, int nMouseButtons, int nMouseFlags);
	virtual bool			ProcessKeyboardEvents(int nKeyButtons,int nKeyFlags);

	void					SetSize(const IVector2D& size);
	void					SetPosition(const IVector2D& pos);

	void					SetRect(const IRectangle& rect);

	const IVector2D&		GetSize() const;
	const IVector2D&		GetPosition() const;

	// clipping rectangle, size position
	virtual IRectangle		GetRectangle() const;

	virtual bool			ProcessCommand( const char* pszCommand );

	virtual bool			ProcessCommandExecute( const char* pszCommand );

protected:

	IVector2D				m_position;
	IVector2D				m_size;

	bool					m_visible;
	bool					m_enabled;
	EqString				m_name;

	DkLinkedList<CEqUI_Panel*>	m_childPanels;		// child panels
	CEqUI_Panel*				m_parent;			// parent panel (null is a starting panel)

	ColorRGBA				m_color;
	ColorRGBA				m_selColor;

	CEqUI_Panel*			m_currentElement;	// keyboard, or mouseover element
};

#endif // EGUI_PANEL_H