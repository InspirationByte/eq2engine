//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Eq UI manager
//////////////////////////////////////////////////////////////////////////////////

#ifndef EQUI_MANAGER_H
#define EQUI_MANAGER_H

#include "materialsystem/IMaterialSystem.h"
#include "math/Rectangle.h"
#include "utils/DkList.h"

class CEqUI_Panel;

class CEqUI_Manager
{
public:
						CEqUI_Manager();
						~CEqUI_Manager();

	void				Init();
	void				Shutdown();

	CEqUI_Panel*		GetRootPanel() const;
	void				SetRootPanel(CEqUI_Panel* pPanel);

	// the element loader
	CEqUI_Panel*		CreateElement( const char* pszTypeName );

	void				AddPanel(CEqUI_Panel* panel);
	void				DestroyPanel( CEqUI_Panel* pPanel );
	CEqUI_Panel*		FindPanel( const char* pszPanelName ) const;

	void				SetViewFrame(const IRectangle& rect);
	const IRectangle&	GetViewFrame() const;

	void				Render();

	bool				ProcessMouseEvents(float x, float y, int nMouseButtons, int nMouseFlags);
	bool				ProcessKeyboardEvents(int nKeyButtons,int nKeyFlags);

	void				DumpPanelsToConsole();
private:
	CEqUI_Panel*		m_rootPanel;

	DkList<CEqUI_Panel*>	m_allocatedPanels;

	IRectangle			m_viewFrameRect;
	IMaterial*			m_material;

};

extern CEqUI_Manager*	g_pEqUIManager;

#endif // EQUI_MANAGER_H
