//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Eq UI manager
//////////////////////////////////////////////////////////////////////////////////

#ifndef EQUI_MANAGER_H
#define EQUI_MANAGER_H

#include "materialsystem/IMaterialSystem.h"
#include "math/rectangle.h"
#include "utils/dklist.h"

class CEqUI_Panel;

class CEqUI_Manager
{
public:
						CEqUI_Manager();
						~CEqUI_Manager();

	void				Init();
	void				Shutdown();

	CEqUI_Panel*		GetRootPanel();
	void				SetRootPanel(CEqUI_Panel* pPanel);

	// the element loader
	CEqUI_Panel*		CreateElement( const char* pszTypeName );
	void				DestroyElement( CEqUI_Panel* pPanel );

	CEqUI_Panel*		FindPanel( const char* pszPanelName );
	void				DumpPanelsToConsole();

	void				SetViewFrame(const IRectangle& rect);
	IRectangle			GetViewFrame();

	void				Render();

	bool				ProcessMouseEvents(float x, float y, int nMouseButtons, int nMouseFlags);
	bool				ProcessKeyboardEvents(int nKeyButtons,int nKeyFlags);
private:
	CEqUI_Panel*		m_rootPanel;

	DkList<CEqUI_Panel*>	m_allocatedPanels;

	IRectangle			m_viewFrameRect;
	IMaterial*			m_material;
	
};

extern CEqUI_Manager*	g_pEqUIManager;

#endif // EQUI_MANAGER_H