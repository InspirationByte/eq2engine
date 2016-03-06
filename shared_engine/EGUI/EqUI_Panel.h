//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: EGUI panel
//////////////////////////////////////////////////////////////////////////////////

#ifndef EGUI_PANEL_H
#define EGUI_PANEL_H

#include "math/DkMath.h"
#include "math/Rectangle.h"
#include "utils/eqstring.h"
#include "utils/DkLinkedList.h"
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

static const char* s_equi_typestrings[] =
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

class IEqUIControl
{
public:

	IEqUIControl();

	virtual int				GetType() const = 0;

	// name and type
	const char*				GetName() const {return m_name.c_str();}
	void					SetName(const char* pszName) {m_name = pszName;}

	// visibility
	virtual void			Show() {m_visible = true;}
	virtual void			Hide() {m_visible = false;}

	virtual void			SetVisible(bool bVisible) {m_visible = bVisible;}
	virtual bool			IsVisible() const {return m_visible;}

	// activation
	virtual void			Enable(bool value) {m_enabled = value;}
	virtual bool			IsEnabled() const {return m_enabled;}

	// position and size
	void					SetSize(const IVector2D& size);
	void					SetPosition(const IVector2D& pos);

	const IVector2D&		GetSize() const;
	const IVector2D&		GetPosition() const;

	// clipping rectangle, size position
	void					SetRectangle(const IRectangle& rect);
	virtual IRectangle		GetRectangle() const;

protected:
	IVector2D				m_position;
	IVector2D				m_size;

	bool					m_visible;
	bool					m_enabled;
	EqString				m_name;
};

// eq panel class
class CEqUI_Panel : public IEqUIControl
{
public:
	CEqUI_Panel();
	~CEqUI_Panel();

	virtual void			InitFromKeyValues( kvkeybase_t* pSection = NULL );
	virtual void			Destroy();

	// child controls
	void					AddChild(CEqUI_Panel* pControl);
	void					RemoveChild(CEqUI_Panel* pControl);
	CEqUI_Panel*			FindChild( const char* pszName );
	void					ClearChilds(bool bFree = true);

	virtual int				GetType() const {return EQUI_PANEL;}

	// apperance
	virtual void			SetColor(const ColorRGBA &color);
	virtual void			GetColor(ColorRGBA &color) const;

	virtual void			SetSelectionColor(const ColorRGBA &color);
	virtual void			GetSelectionColor(ColorRGBA &color) const;

	// control and render
	virtual bool			ProcessMouseEvents(float x, float y, int nMouseButtons, int nMouseFlags);
	virtual bool			ProcessKeyboardEvents(int nKeyButtons,int nKeyFlags);
	virtual bool			ProcessCommand( const char* pszCommand );
	virtual bool			ProcessCommandExecute( const char* pszCommand );

	virtual void			Render();

protected:

	DkLinkedList<CEqUI_Panel*>	m_childPanels;		// child panels
	CEqUI_Panel*				m_parent;			// parent panel (null is a starting panel)

	ColorRGBA				m_color;
	ColorRGBA				m_selColor;

	CEqUI_Panel*			m_currentElement;	// keyboard, or mouseover element
};

#endif // EGUI_PANEL_H
