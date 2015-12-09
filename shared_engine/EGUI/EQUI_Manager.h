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

class CEqPanel;

class CEqUI_Manager
{
public:
						CEqUI_Manager();
						~CEqUI_Manager();

	void				Init();
	void				Shutdown();

	CEqPanel*			GetRootPanel();
	void				SetRootPanel(CEqPanel* pPanel);

	// the element loader
	CEqPanel*			CreateElement( const char* pszTypeName );
	void				DestroyElement( CEqPanel* pPanel );

	CEqPanel*			FindPanel( const char* pszPanelName );
	void				DumpPanelsToConsole();

	void				SetViewFrame(const IRectangle& rect);
	IRectangle			GetViewFrame();

	void				Render();

	bool				ProcessMouseEvents(float x, float y, int nMouseButtons, int nMouseFlags);
	bool				ProcessKeyboardEvents(int nKeyButtons,int nKeyFlags);
private:
	CEqPanel*			m_pRootPanel;

	DkList<CEqPanel*>	m_AllocatedPanels;

	IRectangle			m_viewFrameRect;
	IMaterial*			m_material;
	
};

extern CEqUI_Manager*	g_pEqUIManager;

#endif // EQUI_MANAGER_H