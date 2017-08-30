//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: Driver Syndicate Editor - prefab manager
//////////////////////////////////////////////////////////////////////////////////

#include "UI_PrefabMgr.h"
#include "EditorLevel.h"

#include "world.h"

extern CViewParams			g_pCameraParams;

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

	m_prefabNameDialog = new wxTextEntryDialog(this, DKLOC("TOKEN_PREFABNAME", L"Prefab name"), DKLOC("TOKEN_SPECIFYPREFAB NAME", L"Specify prefab name"));
	
	// Connect Events
	m_prefabList->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( CUI_PrefabManager::OnBeginPrefabPlacement ), NULL, this );
	m_filtertext->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( CUI_PrefabManager::OnFilterChange ), NULL, this );
	m_newbtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CUI_PrefabManager::OnNewPrefabClick ), NULL, this );
	m_editbtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CUI_PrefabManager::OnEditPrefabClick ), NULL, this );
	m_delbtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CUI_PrefabManager::OnDeletePrefabClick ), NULL, this );

	m_mode = PREFABMODE_READY;
	m_selPrefab = nullptr;
}

CUI_PrefabManager::~CUI_PrefabManager()
{
	m_prefabNameDialog->Destroy();

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
void CUI_PrefabManager::MouseEventOnTile( wxMouseEvent& event, hfieldtile_t* tile, int tx, int ty, const Vector3D& ppos )
{
	IVector2D globalTile;
	g_pGameWorld->m_level.LocalToGlobalPoint(IVector2D(tx,ty), m_selectedRegion, globalTile);

	if(m_mode == PREFABMODE_TILESELECTION && event.Dragging())
	{
		m_tileSelection.Reset();
		m_tileSelection.AddVertex(globalTile);
		m_tileSelection.AddVertex(m_startPoint);
	}

	if(event.ButtonDown(wxMOUSE_BTN_LEFT))
	{
		if(m_mode == PREFABMODE_READY || m_mode == PREFABMODE_CREATE_READY)
		{
			m_tileSelection.Reset();

			m_mode = PREFABMODE_TILESELECTION;
			m_startPoint = globalTile;
		}
	}
	else if(event.ButtonUp(wxMOUSE_BTN_LEFT))
	{
		if(m_mode == PREFABMODE_TILESELECTION)
		{
			m_tileSelection.Reset();
			m_tileSelection.AddVertex(globalTile);
			m_tileSelection.AddVertex(m_startPoint);

			m_mode = PREFABMODE_CREATE_READY;
		}
	}
}

void CUI_PrefabManager::OnKey(wxKeyEvent& event, bool bDown)
{
	if(m_mode == PREFABMODE_CREATE_READY)
	{
		if(event.m_keyCode == WXK_RETURN)
		{
			/*
			// get prefab name
			m_prefabNameDialog->SetValue("");

			if(m_prefabNameDialog->ShowModal() == wxID_OK)
			{
				EqString nameVal(m_prefabNameDialog->GetValue().mbc_str());

				if(nameVal.Length() == 0)
				{
					wxMessageBox("Name cannot be empty.", "Warning", wxOK | wxICON_EXCLAMATION | wxCENTRE, this);
					return;
				}

				// generate prefab
				CEditorLevel* generatedFromSel = g_pGameWorld->m_level.CreatePrefab(m_tileSelection.vleftTop, m_tileSelection.vrightBottom, PREFAB_ALL);

				m_mode = PREFABMODE_READY;
			}*/

			wxMessageBox("Making prefab.", "Warning", wxOK | wxICON_EXCLAMATION | wxCENTRE, this);

			if(m_selPrefab)
			{
				m_selPrefab->Cleanup();
				delete m_selPrefab;
			}

			m_selPrefab = g_pGameWorld->m_level.CreatePrefab(m_tileSelection.vleftTop, m_tileSelection.vrightBottom, PREFAB_ALL);
		}
	}
}

void CUI_PrefabManager::OnRender()
{
	materials->SetMatrix(MATRIXMODE_WORLD, identity4());

	if(m_selectedRegion)
	{
		CHeightTileFieldRenderable* field = m_selectedRegion->m_heightfield[0];

		field->DebugRender(false, m_mouseOverTileHeight);
	}

	Vector3D bboxMin = g_pGameWorld->m_level.GlobalTilePointToPosition(m_tileSelection.vleftTop);
	Vector3D bboxMax = g_pGameWorld->m_level.GlobalTilePointToPosition(m_tileSelection.vrightBottom);

	BoundingBox bbox;
	bbox.AddVertex(bboxMin);
	bbox.AddVertex(bboxMax);

	debugoverlay->Box3D(bbox.minPoint-HFIELD_POINT_SIZE*0.5f, bbox.maxPoint+HFIELD_POINT_SIZE*0.5f, ColorRGBA(1,1,0,1), 0.0f);

	if(m_selPrefab)
	{
		materials->SetMatrix(MATRIXMODE_WORLD2, translate(0.0f,1.0f,0.0f));

		m_selPrefab->Ed_Render(g_pCameraParams.GetOrigin(), g_pGameWorld->m_viewprojection);

		materials->SetMatrix(MATRIXMODE_WORLD2, identity4());
	}

	CBaseTilebasedEditor::OnRender();
}

void CUI_PrefabManager::OnLevelUnload()
{

}

void CUI_PrefabManager::OnLevelLoad()
{
	m_mode = PREFABMODE_READY;
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