//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: Driver Syndicate Editor - prefab manager
//////////////////////////////////////////////////////////////////////////////////

#include "UI_PrefabMgr.h"
#include "EditorLevel.h"

CUI_PrefabManager::CUI_PrefabManager( wxWindow* parent ) : wxPanel( parent, -1, wxDefaultPosition, wxDefaultSize )
{
	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxHORIZONTAL );
	
	m_prefabList = new wxListBox( this, wxID_ANY, wxDefaultPosition, wxSize( 300,-1 ), 0, NULL, 0 ); 
	bSizer10->Add( m_prefabList, 0, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizer28;
	bSizer28 = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizer2;
	sbSizer2 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxT("Search and Filter") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	fgSizer1->Add( new wxStaticText( this, wxID_ANY, wxT("Name"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	m_filtertext = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 150,-1 ), 0 );
	m_filtertext->SetMaxLength( 0 ); 
	fgSizer1->Add( m_filtertext, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	sbSizer2->Add( fgSizer1, 0, wxEXPAND, 5 );
	
	
	bSizer28->Add( sbSizer2, 0, 0, 5 );
	
	wxStaticBoxSizer* sbSizer20;
	sbSizer20 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxT("Prefabs") ), wxVERTICAL );
	
	m_newbtn = new wxButton( this, wxID_ANY, wxT("Create"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer20->Add( m_newbtn, 0, wxALL, 5 );
	
	m_editbtn = new wxButton( this, wxID_ANY, wxT("Edit"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer20->Add( m_editbtn, 0, wxALL, 5 );
	
	m_delbtn = new wxButton( this, wxID_ANY, wxT("Delete"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer20->Add( m_delbtn, 0, wxALL, 5 );
	
	
	bSizer28->Add( sbSizer20, 1, 0, 5 );
	
	
	bSizer10->Add( bSizer28, 1, wxEXPAND, 5 );
	
	
	this->SetSizer( bSizer10 );
	this->Layout();
	
	// Connect Events
	m_prefabList->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( CUI_PrefabManager::OnBeginPrefabPlacement ), NULL, this );
	m_filtertext->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( CUI_PrefabManager::OnFilterChange ), NULL, this );
	m_newbtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CUI_PrefabManager::OnNewPrefabClick ), NULL, this );
	m_editbtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CUI_PrefabManager::OnEditPrefabClick ), NULL, this );
	m_delbtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CUI_PrefabManager::OnDeletePrefabClick ), NULL, this );

	m_mode = EPREFAB_READY;
}

CUI_PrefabManager::~CUI_PrefabManager()
{
	// Disconnect Events
	m_prefabList->Disconnect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( CUI_PrefabManager::OnBeginPrefabPlacement ), NULL, this );
	m_filtertext->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( CUI_PrefabManager::OnFilterChange ), NULL, this );
	m_newbtn->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CUI_PrefabManager::OnNewPrefabClick ), NULL, this );
	m_editbtn->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CUI_PrefabManager::OnEditPrefabClick ), NULL, this );
	m_delbtn->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CUI_PrefabManager::OnDeletePrefabClick ), NULL, this );
}

		
// Virtual event handlers, overide them in your derived class
void CUI_PrefabManager::OnBeginPrefabPlacement( wxCommandEvent& event )
{

}

void CUI_PrefabManager::OnFilterChange( wxCommandEvent& event )
{

}

void CUI_PrefabManager::OnNewPrefabClick( wxCommandEvent& event )
{

}

void CUI_PrefabManager::OnEditPrefabClick( wxCommandEvent& event )
{

}

void CUI_PrefabManager::OnDeletePrefabClick( wxCommandEvent& event )
{

}

// IEditorTool stuff
void CUI_PrefabManager::MouseEventOnTile( wxMouseEvent& event, hfieldtile_t* tile, int tx, int ty, const Vector3D& ppos  )
{

}

void CUI_PrefabManager::OnKey(wxKeyEvent& event, bool bDown)
{

}

void CUI_PrefabManager::OnRender()
{
	materials->SetMatrix(MATRIXMODE_WORLD, identity4());

	if(m_selectedRegion)
	{
		CHeightTileFieldRenderable* field = m_selectedRegion->m_heightfield[0];

		field->DebugRender(false, m_mouseOverTileHeight);
	}

	CBaseTilebasedEditor::OnRender();
}

void CUI_PrefabManager::OnLevelUnload()
{

}

void CUI_PrefabManager::OnLevelLoad()
{

}

void CUI_PrefabManager::InitTool()
{
	
}

void CUI_PrefabManager::ReloadTool()
{

}

void CUI_PrefabManager::Update_Refresh()
{

}