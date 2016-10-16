//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Eq UI manager
//////////////////////////////////////////////////////////////////////////////////

#ifndef EQUI_MANAGER_H
#define EQUI_MANAGER_H

#include "equi_defs.h"

#include "materialsystem/IMaterialSystem.h"
#include "math/Rectangle.h"
#include "utils/DkList.h"

class IEqUIControl;
class CEqUI_Panel;

class CEqUI_Manager
{
public:
						CEqUI_Manager();
						~CEqUI_Manager();

	void				Init();
	void				Shutdown();

	CEqUI_Panel*		GetRootPanel() const;

	// the element loader
	IEqUIControl*		CreateElement( const char* pszTypeName );
	IEqUIControl*		CreateElement( EUIElementType type );

	void				AddPanel(CEqUI_Panel* panel);
	void				DestroyPanel( CEqUI_Panel* pPanel );
	CEqUI_Panel*		FindPanel( const char* pszPanelName ) const;

	void				SetViewFrame(const IRectangle& rect);
	const IRectangle&	GetViewFrame() const;

	void				SetFocus( IEqUIControl* focusTo );
	IEqUIControl*		GetFocus() const;

	bool				IsPanelsVisible() const;

	void				Render();

	bool				ProcessMouseEvents(float x, float y, int nMouseButtons, int flags);
	bool				ProcessKeyboardEvents(int nKeyButtons, int flags);

	void				DumpPanelsToConsole();

private:
	CEqUI_Panel*			m_rootPanel;

	IEqUIControl*			m_focus;

	DkList<CEqUI_Panel*>	m_panels;

	IRectangle				m_viewFrameRect;
	IMaterial*				m_material;

};

extern CEqUI_Manager*	g_pEqUIManager;

#endif // EQUI_MANAGER_H
