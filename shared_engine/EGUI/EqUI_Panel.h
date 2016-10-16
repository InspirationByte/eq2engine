//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: EGUI panel
//////////////////////////////////////////////////////////////////////////////////

#ifndef EGUI_PANEL_H
#define EGUI_PANEL_H

#include "equi_defs.h"
#include "IEqUIControl.h"


#include "utils/KeyValues.h"


// eq panel class
class CEqUI_Panel : public IEqUIControl
{
public:
	CEqUI_Panel();
	~CEqUI_Panel();

	virtual void			InitFromKeyValues( kvkeybase_t* pSection = NULL );
	virtual void			Destroy();

	virtual int				GetType() const {return EQUI_PANEL;}

	// apperance
	virtual void			SetColor(const ColorRGBA &color);
	virtual void			GetColor(ColorRGBA &color) const;

	virtual void			SetSelectionColor(const ColorRGBA &color);
	virtual void			GetSelectionColor(ColorRGBA &color) const;

	// control and render
	virtual bool			ProcessMouseEvents(float x, float y, int nMouseButtons, int flags);
	virtual bool			ProcessKeyboardEvents(int nKeyButtons, int flags);
	virtual bool			ProcessCommand( const char* pszCommand );
	virtual bool			ProcessCommandExecute( const char* pszCommand );

	// rendering
	virtual void			Render();

protected:
	ColorRGBA				m_color;
	ColorRGBA				m_selColor;

	IEqUIControl*			m_mouseOver;	// mouseover element
};

#endif // EGUI_PANEL_H
