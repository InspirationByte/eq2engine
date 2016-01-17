//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Building editor for Driver Syndicate
//////////////////////////////////////////////////////////////////////////////////

#include "UI_BuildingConstruct.h"
#include "EditorMain.h"

CBuildingLayerEditDialog::CBuildingLayerEditDialog( wxWindow* parent ) 
	: wxDialog( parent, wxID_ANY, wxT("Building floor layer set constructor - <filename>"), wxDefaultPosition,  wxSize( 756,558 ), wxDEFAULT_DIALOG_STYLE | wxSTAY_ON_TOP )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer18;
	bSizer18 = new wxBoxSizer( wxHORIZONTAL );
	
	m_renderPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_renderPanel->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNTEXT ) );
	
	bSizer18->Add( m_renderPanel, 1, wxEXPAND | wxALL, 5 );
	
	m_panel18 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxSize( 200,-1 ), wxTAB_TRAVERSAL );
	wxStaticBoxSizer* sbSizer12;
	sbSizer12 = new wxStaticBoxSizer( new wxStaticBox( m_panel18, wxID_ANY, wxT("Floor layer set") ), wxVERTICAL );
	
	m_button16 = new wxButton( m_panel18, wxID_ANY, wxT("New"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer12->Add( m_button16, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_button14 = new wxButton( m_panel18, wxID_ANY, wxT("Open..."), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer12->Add( m_button14, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_button15 = new wxButton( m_panel18, wxID_ANY, wxT("Save..."), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer12->Add( m_button15, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	sbSizer12->Add( new wxStaticLine( m_panel18, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL ), 0, wxEXPAND | wxALL, 5 );
	
	m_button18 = new wxButton( m_panel18, wxID_ANY, wxT("Toggle preview"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer12->Add( m_button18, 0, wxALL, 5 );
	
	
	m_panel18->SetSizer( sbSizer12 );
	m_panel18->Layout();
	bSizer18->Add( m_panel18, 0, wxALL|wxEXPAND, 5 );
	
	
	this->SetSizer( bSizer18 );
	this->Layout();
	
	this->Centre( wxBOTH );
}

CBuildingLayerEditDialog::~CBuildingLayerEditDialog()
{
}

//-----------------------------------------------------------------------------

CUI_BuildingConstruct::CUI_BuildingConstruct( wxWindow* parent )
	 : wxPanel( parent, -1, wxDefaultPosition, wxDefaultSize), CBaseTilebasedEditor()
{
	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxHORIZONTAL );
	
	m_modelPicker = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_modelPicker->SetForegroundColour( wxColour( 0, 0, 0 ) );
	m_modelPicker->SetBackgroundColour( wxColour( 0, 0, 0 ) );
	
	bSizer9->Add( m_modelPicker, 1, wxEXPAND|wxALL, 5 );
	
	m_pSettingsPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxSize( -1,150 ), wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizer2;
	sbSizer2 = new wxStaticBoxSizer( new wxStaticBox( m_pSettingsPanel, wxID_ANY, wxT("Search and Filter") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	fgSizer1->Add( new wxStaticText( m_pSettingsPanel, wxID_ANY, wxT("Name"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	m_filtertext = new wxTextCtrl( m_pSettingsPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 150,-1 ), 0 );
	m_filtertext->SetMaxLength( 0 ); 
	fgSizer1->Add( m_filtertext, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	sbSizer2->Add( fgSizer1, 0, wxEXPAND, 5 );
	
	
	bSizer10->Add( sbSizer2, 0, wxEXPAND, 5 );
	
	wxGridSizer* gSizer2;
	gSizer2 = new wxGridSizer( 0, 3, 0, 0 );
	
	
	bSizer10->Add( gSizer2, 1, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizer20;
	sbSizer20 = new wxStaticBoxSizer( new wxStaticBox( m_pSettingsPanel, wxID_ANY, wxT("Sets") ), wxVERTICAL );
	
	m_button5 = new wxButton( m_pSettingsPanel, wxID_ANY, wxT("Create..."), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer20->Add( m_button5, 0, wxALL, 5 );
	
	m_button3 = new wxButton( m_pSettingsPanel, wxID_ANY, wxT("Edit..."), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer20->Add( m_button3, 0, wxALL, 5 );
	
	m_button8 = new wxButton( m_pSettingsPanel, wxID_ANY, wxT("Delete"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer20->Add( m_button8, 0, wxALL, 5 );
	
	
	bSizer10->Add( sbSizer20, 1, wxEXPAND, 5 );
	
	
	m_pSettingsPanel->SetSizer( bSizer10 );
	m_pSettingsPanel->Layout();
	bSizer9->Add( m_pSettingsPanel, 0, wxALL|wxEXPAND, 5 );
	
	
	this->SetSizer( bSizer9 );
	this->Layout();
	
	// Connect Events
	m_filtertext->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( CUI_BuildingConstruct::OnFilterChange ), NULL, this );
	m_button5->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CUI_BuildingConstruct::OnCreateClick ), NULL, this );
	m_button3->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CUI_BuildingConstruct::OnEditClick ), NULL, this );
	m_button8->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CUI_BuildingConstruct::OnDeleteClick ), NULL, this );

	m_layerEditDlg = new CBuildingLayerEditDialog(g_pMainFrame);
}

CUI_BuildingConstruct::~CUI_BuildingConstruct()
{
}

// Virtual event handlers, overide them in your derived class
void CUI_BuildingConstruct::OnFilterChange( wxCommandEvent& event )
{

}

void CUI_BuildingConstruct::OnCreateClick( wxCommandEvent& event )
{
	// TODO: newLayer
	m_layerEditDlg->ShowModal();
}

void CUI_BuildingConstruct::OnEditClick( wxCommandEvent& event )
{
	// TODO: set existing layers
	m_layerEditDlg->ShowModal();
}

void CUI_BuildingConstruct::OnDeleteClick( wxCommandEvent& event )
{

}

//------------------------------------------------------------------------

void CUI_BuildingConstruct::InitTool()
{

}

void CUI_BuildingConstruct::ShutdownTool()
{

}

void CUI_BuildingConstruct::Update_Refresh()
{

}

void CUI_BuildingConstruct::OnKey(wxKeyEvent& event, bool bDown)
{

}

//------------------------------------------------------------------------

void CUI_BuildingConstruct::MouseEventOnTile( wxMouseEvent& event, hfieldtile_t* tile, int tx, int ty, const Vector3D& ppos )
{
	
}

void CUI_BuildingConstruct::OnRender()
{
	
}