//-=-=-=-=-=-=-=-=-=-= Copyright (C) Damage Byte L.L.C 2009-2012 =-=-=-=-=-=-=-=-=-=-=-
//
// Description: Equilibrium Engine Editor main cycle
//
//-=-=-=-D-=-A-=-M-=-A-=-G-=-E-=-=-B-=-Y-=-T-=-E-=-=-S-=-T-=-U-=-D-=-I-=-O-=-S-=-=-=-=-

#include "wx_inc.h"
#include "ILocalize.h"

class CMainWindow : public wxFrame 
{
public:
	CMainWindow( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("EGFman"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 915,697 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );
		
	~CMainWindow();

protected:
	wxPanel* m_pRenderPanel;
	wxNotebook* m_notebook1;
	wxPanel* m_pModelPanel;
	wxPanel* m_pMotionPanel;
	wxStaticText* m_staticText3;
	wxComboBox* m_pMotionSelection;
	wxStaticText* m_staticText4;
	wxTextCtrl* m_pAnimFramerate;
	wxStaticText* m_staticText2;
	wxSlider* m_pTimeline;
	wxButton* m_button1;
	wxButton* m_button2;
	wxRadioBox* m_pAnimMode;
	wxPanel* m_pPhysicsPanel;
	wxStaticLine* m_staticline3;
	wxStaticText* m_staticText11;
	wxTextCtrl* m_pObjMass;
	wxStaticText* m_staticText12;
	wxTextCtrl* m_pObjSurfProps;
	wxStaticText* m_staticText5;
	wxComboBox* m_pPhysJoint;
	wxStaticText* m_staticText7;
	wxTextCtrl* m_pLimMinX;
	wxTextCtrl* m_pLimMaxX;
	wxStaticText* m_staticText8;
	wxTextCtrl* m_pLimMinY;
	wxTextCtrl* m_pLimMaxY;
	wxStaticText* m_staticText9;
	wxTextCtrl* m_pLimMinZ;
	wxTextCtrl* m_pLimMaxZ;
	wxStaticText* m_staticText14;
	wxTextCtrl* m_pSimTimescale;
	wxButton* m_button9;
	wxButton* m_button10;
	wxPanel* m_pIKPanel;
	wxStaticText* m_staticText10;
	wxComboBox* m_comboBox5;
	wxMenuBar* m_pMenu;
	wxMenu* m_menu1;
	wxMenu* m_menu2;
	wxMenu* m_menu3;
	wxMenu* m_menu4;

	
};
