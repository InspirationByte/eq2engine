//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Drivers editor - occluder placement
//////////////////////////////////////////////////////////////////////////////////

#include "UI_BlockEditor.h"
#include "EditorLevel.h"
#include "EditorMain.h"

#include "MaterialAtlasList.h"

float SnapFloat(int grid_spacing, float val)
{
	return round(val / grid_spacing) * grid_spacing;
}

Vector3D SnapVector(int grid_spacing, const Vector3D &vector)
{
	return Vector3D(
		SnapFloat(grid_spacing, vector.x),
		SnapFloat(grid_spacing, vector.y),
		SnapFloat(grid_spacing, vector.z));
}

//--------------------------------------------------------------------------------------

enum EToolbarEvents
{
	Event_Tools_Selection = 100,
	Event_Tools_Brush,
	Event_Tools_Polygon,
	Event_Tools_VertexManip,
	Event_Tools_Clipper,

	Event_Tools_MAX
};

extern Vector3D g_camera_target;

BEGIN_EVENT_TABLE(CUI_BlockEditor, wxPanel)
	EVT_MENU_RANGE(Event_Tools_Selection, Event_Tools_MAX, CUI_BlockEditor::ProcessAllMenuCommands)
END_EVENT_TABLE()

CUI_BlockEditor::CUI_BlockEditor(wxWindow* parent) : wxPanel(parent, -1, wxDefaultPosition, wxDefaultSize)
{
	wxBoxSizer* bSizer28;
	bSizer28 = new wxBoxSizer(wxHORIZONTAL);

	wxBoxSizer* bSizer29;
	bSizer29 = new wxBoxSizer(wxVERTICAL);

	ConstructToolbars(bSizer29);

	bSizer28->Add(bSizer29, 0, wxEXPAND, 5);

	m_settingsPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(120, -1), wxTAB_TRAVERSAL);
	bSizer28->Add(m_settingsPanel, 0, wxEXPAND | wxALL, 5);

	m_texPanel = new CMaterialAtlasList(this);

	bSizer28->Add(m_texPanel, 1, wxEXPAND | wxALL, 5);

	m_pSettingsPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 150), wxTAB_TRAVERSAL);
	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer(wxVERTICAL);

	wxStaticBoxSizer* sbSizer2;
	sbSizer2 = new wxStaticBoxSizer(new wxStaticBox(m_pSettingsPanel, wxID_ANY, wxT("Search and Filter")), wxVERTICAL);

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer(0, 2, 0, 0);
	fgSizer1->SetFlexibleDirection(wxBOTH);
	fgSizer1->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);

	fgSizer1->Add(new wxStaticText(m_pSettingsPanel, wxID_ANY, wxT("Name"), wxDefaultPosition, wxDefaultSize, 0), 0, wxALL, 5);

	m_filtertext = new wxTextCtrl(m_pSettingsPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(200, -1), 0);
	m_filtertext->SetMaxLength(0);
	fgSizer1->Add(m_filtertext, 0, wxBOTTOM | wxRIGHT | wxLEFT, 5);

	fgSizer1->Add(new wxStaticText(m_pSettingsPanel, wxID_ANY, wxT("Tags"), wxDefaultPosition, wxDefaultSize, 0), 0, wxALL, 5);

	m_pTags = new wxTextCtrl(m_pSettingsPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(200, -1), 0);
	m_pTags->SetMaxLength(0);
	fgSizer1->Add(m_pTags, 0, wxALL, 5);


	sbSizer2->Add(fgSizer1, 0, wxEXPAND, 5);

	m_onlyusedmaterials = new wxCheckBox(m_pSettingsPanel, wxID_ANY, wxT("SHOW ONLY USED MATERIALS"), wxDefaultPosition, wxDefaultSize, 0);
	sbSizer2->Add(m_onlyusedmaterials, 0, wxALL, 5);

	m_pSortByDate = new wxCheckBox(m_pSettingsPanel, wxID_ANY, wxT("SORT BY DATE"), wxDefaultPosition, wxDefaultSize, 0);
	sbSizer2->Add(m_pSortByDate, 0, wxALL, 5);


	bSizer10->Add(sbSizer2, 0, wxEXPAND, 5);

	wxStaticBoxSizer* sbSizer3;
	sbSizer3 = new wxStaticBoxSizer(new wxStaticBox(m_pSettingsPanel, wxID_ANY, wxT("Display")), wxVERTICAL);

	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer(0, 2, 0, 0);
	fgSizer2->SetFlexibleDirection(wxBOTH);
	fgSizer2->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);

	fgSizer2->Add(new wxStaticText(m_pSettingsPanel, wxID_ANY, wxT("Preview size"), wxDefaultPosition, wxDefaultSize, 0), 0, wxALL, 5);

	wxString m_pPreviewSizeChoices[] = { wxT("64"), wxT("128"), wxT("256") };
	int m_pPreviewSizeNChoices = sizeof(m_pPreviewSizeChoices) / sizeof(wxString);
	m_pPreviewSize = new wxChoice(m_pSettingsPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_pPreviewSizeNChoices, m_pPreviewSizeChoices, 0);
	m_pPreviewSize->SetSelection(0);
	fgSizer2->Add(m_pPreviewSize, 0, wxBOTTOM | wxRIGHT | wxLEFT, 5);

	m_pAspectCorrection = new wxCheckBox(m_pSettingsPanel, wxID_ANY, wxT("ASPECT CORRECTION"), wxDefaultPosition, wxDefaultSize, 0);
	fgSizer2->Add(m_pAspectCorrection, 0, wxALL, 5);


	sbSizer3->Add(fgSizer2, 0, wxEXPAND, 5);


	bSizer10->Add(sbSizer3, 0, wxEXPAND, 5);


	m_pSettingsPanel->SetSizer(bSizer10);
	m_pSettingsPanel->Layout();
	bSizer28->Add(m_pSettingsPanel, 0, wxALL | wxEXPAND, 5);

	this->SetSizer(bSizer28);
	this->Layout();

	m_filtertext->Connect(wxEVT_COMMAND_TEXT_UPDATED, (wxObjectEventFunction)&CUI_BlockEditor::OnFilterTextChanged, NULL, this);

	m_onlyusedmaterials->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED, (wxObjectEventFunction)&CUI_BlockEditor::OnChangePreviewParams, NULL, this);
	m_pSortByDate->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED, (wxObjectEventFunction)&CUI_BlockEditor::OnChangePreviewParams, NULL, this);
	m_pPreviewSize->Connect(wxEVT_COMMAND_CHOICE_SELECTED, (wxObjectEventFunction)&CUI_BlockEditor::OnChangePreviewParams, NULL, this);
	m_pAspectCorrection->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED, (wxObjectEventFunction)&CUI_BlockEditor::OnChangePreviewParams, NULL, this);

	//--------------------------------------------------

	m_cursorPos = vec3_zero;
	m_selectedTool = BlockEdit_Selection;

}

CUI_BlockEditor::~CUI_BlockEditor()
{

}

void CUI_BlockEditor::ConstructToolbars(wxBoxSizer* addTo)
{
	SetSizer(new wxBoxSizer(wxVERTICAL));

	wxToolBar *pToolBar = new wxToolBar(this, -1, wxDefaultPosition, wxSize(36, 400), wxTB_FLAT | wxTB_NODIVIDER | wxTB_VERTICAL/* | wxTB_HORZ_TEXT*/);//CreateToolBar(wxTB_FLAT | wxTB_NODIVIDER | wxTB_VERTICAL);

	wxBitmap toolBarBitmaps[15];

	toolBarBitmaps[0] = wxBitmap("Editor/resource/tool_select.bmp", wxBITMAP_TYPE_BMP);
	toolBarBitmaps[1] = wxBitmap("Editor/resource/tool_brush.bmp", wxBITMAP_TYPE_BMP);
	toolBarBitmaps[2] = wxBitmap("Editor/resource/tool_surface.bmp", wxBITMAP_TYPE_BMP);
	toolBarBitmaps[3] = wxBitmap("Editor/resource/tool_clipper.bmp", wxBITMAP_TYPE_BMP);
	toolBarBitmaps[4] = wxBitmap("Editor/resource/tool_vertexmanip.bmp", wxBITMAP_TYPE_BMP);

	int w = toolBarBitmaps[0].GetWidth(),
		h = toolBarBitmaps[0].GetHeight();

	// this call is actually unnecessary as the toolbar will adjust its tools
	// size to fit the biggest icon used anyhow but it doesn't hurt neither
	pToolBar->SetToolBitmapSize(wxSize(w, h));

	pToolBar->AddSeparator();

#define ADD_TOOLRADIO(event_id, text, bitmap) \
	pToolBar->AddTool(event_id, text, bitmap, wxNullBitmap, wxITEM_RADIO ,text, text);

	ADD_TOOLRADIO(Event_Tools_Selection, DKLOC("TOKEN_SELECTION", "Selection"), toolBarBitmaps[0]);
	ADD_TOOLRADIO(Event_Tools_Brush, DKLOC("TOKEN_BRUSH", "Block tool"), toolBarBitmaps[1]);
	ADD_TOOLRADIO(Event_Tools_Polygon, DKLOC("TOKEN_SURFACES", "Polygon tool"), toolBarBitmaps[2]);
	ADD_TOOLRADIO(Event_Tools_VertexManip, DKLOC("TOKEN_VERTEXMANIP", "Vertex manipulator"), toolBarBitmaps[3]);
	ADD_TOOLRADIO(Event_Tools_Clipper, DKLOC("TOKEN_CLIPPER", "Clipper tool"), toolBarBitmaps[4]);

	pToolBar->Realize();

	addTo->Add(pToolBar);
}

void CUI_BlockEditor::ProcessAllMenuCommands(wxCommandEvent& event)
{
	switch (event.GetId())
	{
	case Event_Tools_Selection:
		m_selectedTool = BlockEdit_Selection;
		break;
	case Event_Tools_Brush:
		m_selectedTool = BlockEdit_Brush;
		break;
	case Event_Tools_Polygon:
		m_selectedTool = BlockEdit_Polygon;
		break;
	case Event_Tools_VertexManip:
		m_selectedTool = BlockEdit_VertexManip;
		break;
	case Event_Tools_Clipper:
		m_selectedTool = BlockEdit_Clipper;
		break;
	}
}

void CUI_BlockEditor::OnChangePreviewParams(wxCommandEvent& event)
{
	int values[] =
	{
		128,
		256,
		512,
	};

	m_texPanel->SetPreviewParams(values[m_pPreviewSize->GetSelection()], m_pAspectCorrection->GetValue());

	m_texPanel->ChangeFilter(m_filtertext->GetValue(), m_pTags->GetValue(), m_onlyusedmaterials->GetValue(), m_pSortByDate->GetValue());
	m_texPanel->Redraw();
}

void CUI_BlockEditor::OnFilterTextChanged(wxCommandEvent& event)
{
	m_texPanel->ChangeFilter(m_filtertext->GetValue(), m_pTags->GetValue(), m_onlyusedmaterials->GetValue(), m_pSortByDate->GetValue());
	m_texPanel->Redraw();
}

// IEditorTool stuff

void CUI_BlockEditor::MouseEventOnTile(wxMouseEvent& event, hfieldtile_t* tile, int tx, int ty, const Vector3D& ppos)
{
	Vector3D cursorPos = SnapVector(1, ppos);

	// selecting brushes within region
	// moving
	// editing
	if (m_selectedTool == BlockEdit_Selection)
	{
	
	}
	else if (m_selectedTool == BlockEdit_Brush)
	{
		// draw bounding box and press enter to create brush
	}
	else if (m_selectedTool == BlockEdit_Polygon)
	{
		// draw polygon and press enter to create brushes
	}
	else if (m_selectedTool == BlockEdit_VertexManip)
	{
		// vertex selection and movement
	}
	else if (m_selectedTool == BlockEdit_Clipper)
	{
		// draw a plane using 2 vertices and split brushes
	}

	m_cursorPos = cursorPos;
}

void CUI_BlockEditor::ProcessMouseEvents(wxMouseEvent& event)
{
	CBaseTilebasedEditor::ProcessMouseEvents(event);

	if (event.ButtonUp())
		g_pEditorActionObserver->EndAction();
}

void CUI_BlockEditor::OnKey(wxKeyEvent& event, bool bDown)
{

}

void CUI_BlockEditor::OnRender()
{
	if (m_selectedTool == BlockEdit_Brush ||
		m_selectedTool == BlockEdit_Polygon)
	{
		debugoverlay->Sphere3D(m_cursorPos, 0.15f, ColorRGBA(1, 1, 0, 1), 0.0f);
	}

	materials->SetMatrix(MATRIXMODE_WORLD, identity4());
	CBaseTilebasedEditor::OnRender();
}

void CUI_BlockEditor::InitTool()
{
	m_texPanel->ReloadMaterialList();
}

void CUI_BlockEditor::OnLevelUnload()
{
	m_texPanel->SelectMaterial(NULL, 0);
	CBaseTilebasedEditor::OnLevelUnload();
}

void CUI_BlockEditor::Update_Refresh()
{
	m_texPanel->Redraw();
}