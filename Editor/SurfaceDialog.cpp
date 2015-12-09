//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Terrain patch, editable surface, mesh dialog
//////////////////////////////////////////////////////////////////////////////////

#include "SurfaceDialog.h"
#include "SelectionEditor.h"

#include "EditableSurface.h"
#include "EqBrush.h"
#include "RenderWindows.h"

class CTextureView: public wxPanel
{
public:
    CTextureView(wxWindow* parent, const wxPoint& pos, const wxSize& size) : wxPanel( parent, -1, pos, size, wxTAB_TRAVERSAL)
	{
		m_pMaterial = NULL;
	}

	void OnIdle(wxIdleEvent &event)
	{
		Redraw();
	}

	void OnEraseBackground(wxEraseEvent& event)
	{

	}
		
	//void OnMouseMotion(wxMouseEvent& event);
	//void OnMouseScroll(wxMouseEvent& event);
	//void OnMouseClick(wxMouseEvent& event);

	// render material in this tiny preview window
	void Redraw()
	{
		int w, h;
		GetSize(&w, &h);

		g_pShaderAPI->SetViewport(0, 0, w,h);

		materials->GetConfiguration().wireframeMode = false;

		materials->SetAmbientColor(color4_white);

		if(materials->BeginFrame())
		{
			g_pShaderAPI->Clear(true,true,false, ColorRGBA(0,0,0, 1));
			materials->Setup2D(w,h);

			if(m_pMaterial != NULL)
			{
				Vertex2D_t verts[] = {MAKETEXQUAD(0, 0, w, h, 0)};

				BlendStateParam_t params;
				params.alphaTest = false;
				params.alphaTestRef = 1.0f;
				params.blendEnable = false;
				params.srcFactor = BLENDFACTOR_ONE;
				params.dstFactor = BLENDFACTOR_ZERO;
				params.mask = COLORMASK_ALL;
				params.blendFunc = BLENDFUNC_ADD;

				materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, verts, 4, m_pMaterial->GetBaseTexture(), color4_white, &params);
			}

			materials->EndFrame((HWND)GetHWND());
		}
	}

	void SetMaterial(IMaterial* pMaterial)
	{
		m_pMaterial = pMaterial;
	}

	DECLARE_EVENT_TABLE()

protected:
	IMaterial*	m_pMaterial;
};

BEGIN_EVENT_TABLE(CTextureView, wxPanel)
    EVT_ERASE_BACKGROUND(CTextureView::OnEraseBackground)
    EVT_IDLE(CTextureView::OnIdle)
	//EVT_MOTION(CTextureView::OnMouseMotion)
	//EVT_MOUSEWHEEL(CTextureView::OnMouseScroll)
	//EVT_LEFT_DOWN(CTextureView::OnMouseClick)
END_EVENT_TABLE()

enum TextureJustify_e
{
	TEXJUSTIFY_FIT = 0,
	TEXJUSTIFY_CENTER,
	TEXJUSTIFY_LEFT,
	TEXJUSTIFY_TOP,
	TEXJUSTIFY_RIGHT,
	TEXJUSTIFY_BOTTOM
};

enum SurfDialogEventIds_e
{
	SDE_JUSTIFY_FIT = 100,
	SDE_JUSTIFY_CENTER,
	SDE_JUSTIFY_LEFT,
	SDE_JUSTIFY_TOP,
	SDE_JUSTIFY_RIGHT,
	SDE_JUSTIFY_BOTTOM,

	SDE_SCALEX,
	SDE_SCALEY,
	SDE_MOVEX,
	SDE_MOVEY,
	SDE_ROTATE,

	SDE_SURFTESS,
	SDE_FLAGS,

	SDE_RENDER_UPDATE,

	SDE_POWERUPDATE,
	SDE_RADUISUPDATE,

	SDE_PAINTERSELECT,

	SDE_TEXTURAXISFROM3DVIEW,
};

BEGIN_EVENT_TABLE(CSurfaceDialog, wxDialog)
	EVT_CLOSE(CSurfaceDialog::OnClose)
	EVT_SHOW(CSurfaceDialog::OnShow)
	EVT_AUINOTEBOOK_PAGE_CHANGED(-1, CSurfaceDialog::OnNoteBookPageChange)
	EVT_CHECKBOX(-1,CSurfaceDialog::OnEvent_ToUpdate)
	EVT_RADIOBOX(SDE_PAINTERSELECT,  CSurfaceDialog::OnSelectPaintType)
	EVT_COMMAND_SCROLL(SDE_POWERUPDATE, CSurfaceDialog::OnSliderChange)
	EVT_COMMAND_SCROLL(SDE_RADUISUPDATE, CSurfaceDialog::OnSliderChange)
	EVT_SPIN_UP(-1, CSurfaceDialog::OnSpinUpTextureParams)
	EVT_SPIN_DOWN(-1, CSurfaceDialog::OnSpinDownTextureParams)
	EVT_TEXT_ENTER(-1, CSurfaceDialog::OnTextParamsChanged)
	EVT_BUTTON(SDE_TEXTURAXISFROM3DVIEW, CSurfaceDialog::OnButton)
END_EVENT_TABLE()

CSurfaceDialog::CSurfaceDialog() : wxDialog(g_editormainframe, -1, DKLOC("TOKEN_SURFDIALOG_TITLE", "Surface dialog"), wxPoint(-1,-1),wxSize(615, 354), wxCAPTION | wxCLOSE_BOX)
{

	/*
	// build tabs
	wxAuiNotebook *notebook = new wxAuiNotebook(this, -1, wxDefaultPosition, wxSize(615, 324),wxAUI_NB_TOP |wxAUI_NB_TAB_SPLIT | wxAUI_NB_TAB_MOVE | wxAUI_NB_SCROLL_BUTTONS |wxNO_BORDER);

	m_surface_panel = new wxPanel(this, 0,0,512,304);
	m_terrain_panel = new wxPanel(this, 0,0,512,304);
	m_texturereplacement_panel = new wxPanel(this, 0,0,512,512);

	notebook->AddPage(m_surface_panel, DKLOC("TOKEN_SURFACE", "Surface"), true);
	notebook->AddPage(m_terrain_panel, DKLOC("TOKEN_TERRAIN", "Terrain"), false);
	notebook->AddPage(m_texturereplacement_panel, DKLOC("TOKEN_USEDMATERIALS", "Used materials"), false);
	*/

	wxBoxSizer* bSizer16;
	bSizer16 = new wxBoxSizer( wxVERTICAL );
	
	wxAuiNotebook* notebook = new wxAuiNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_TOP |wxAUI_NB_TAB_SPLIT | wxAUI_NB_TAB_MOVE | wxAUI_NB_SCROLL_BUTTONS |wxNO_BORDER );
	m_surface_panel = new wxPanel( notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	notebook->AddPage( m_surface_panel, DKLOC("TOKEN_SURFACE", "Surface"), true, wxNullBitmap );
	m_terrain_panel = new wxPanel( notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	notebook->AddPage( m_terrain_panel, DKLOC("TOKEN_TERRAIN", "Terrain"), false, wxNullBitmap );
	m_texturereplacement_panel = new wxPanel( notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	notebook->AddPage( m_texturereplacement_panel, DKLOC("TOKEN_REPLACETEX", "Replace"), false, wxNullBitmap );
	
	bSizer16->Add( notebook, 1, wxEXPAND|wxBOTTOM, 5 );
	
	
	this->SetSizer( bSizer16 );
	this->Layout();

	InitSurfaceEditTab();
	InitTerrainEditTab();
	InitTextureReplacementTab();
}

void CSurfaceDialog::InitSurfaceEditTab()
{
	/*
	new wxStaticText(m_surface_panel, -1, DKLOC("TOKEN_SCALE", "Scale:"), wxPoint(15,5),wxSize(50,16));
	new wxStaticText(m_surface_panel, -1, DKLOC("TOKEN_SHIFT", "Shift:"), wxPoint(85,5),wxSize(50,16));
	new wxStaticText(m_surface_panel, -1, DKLOC("TOKEN_ROTATION", "Rotation:"), wxPoint(155,5),wxSize(50,16));

	new wxStaticText(m_surface_panel, -1, "X:", wxPoint(5,20),wxSize(10,15));
	new wxStaticText(m_surface_panel, -1, "Y:", wxPoint(5,45),wxSize(10,15));

	m_pScale[0] = new wxTextCtrl(m_surface_panel, SDE_SCALEX, "", wxPoint(15,20),  wxSize(35,20), wxTE_PROCESS_ENTER | wxTAB_TRAVERSAL);
	m_pScale[1] = new wxTextCtrl(m_surface_panel, SDE_SCALEY, "", wxPoint(15,45),  wxSize(35,20), wxTE_PROCESS_ENTER | wxTAB_TRAVERSAL);

	new wxSpinButton(m_surface_panel, SDE_SCALEX, wxPoint(50,20), wxSize(15,20));
	new wxSpinButton(m_surface_panel, SDE_SCALEY, wxPoint(50,45), wxSize(15,20));

	m_pMove[0] = new wxTextCtrl(m_surface_panel, SDE_MOVEX, "", wxPoint(85,20),  wxSize(35,20), wxTE_PROCESS_ENTER | wxTAB_TRAVERSAL);
	m_pMove[1] = new wxTextCtrl(m_surface_panel, SDE_MOVEY, "", wxPoint(85,45),  wxSize(35,20), wxTE_PROCESS_ENTER | wxTAB_TRAVERSAL);

	new wxSpinButton(m_surface_panel, SDE_MOVEX, wxPoint(120,20), wxSize(15,20));
	new wxSpinButton(m_surface_panel, SDE_MOVEY, wxPoint(120,45), wxSize(15,20));

	m_pRotate = new wxTextCtrl(m_surface_panel, SDE_ROTATE, "", wxPoint(155,20),  wxSize(35,20), wxTE_PROCESS_ENTER | wxTAB_TRAVERSAL);
	new wxSpinButton(m_surface_panel, SDE_ROTATE, wxPoint(190,20), wxSize(15,20));

	new wxStaticText(m_surface_panel, -1, DKLOC("TOKEN_TESS", "Tesselation:"), wxPoint(4,94),wxSize(70,16));
	m_pTess = new wxTextCtrl(m_surface_panel, SDE_SURFTESS, "", wxPoint(75,91),  wxSize(35,20), wxTE_PROCESS_ENTER | wxTAB_TRAVERSAL);
	new wxSpinButton(m_surface_panel, SDE_SURFTESS, wxPoint(110,91), wxSize(15,20));

	m_pNoCollide = new wxCheckBox(m_surface_panel, SDE_FLAGS, DKLOC("TOKEN_NOCOLLIDE", "No collsion"), wxPoint(4,116),wxSize(120,16));
	m_pNoSubdivision = new wxCheckBox(m_surface_panel, SDE_FLAGS, DKLOC("TOKEN_NOSUBDIV", "No subdivision"), wxPoint(4,141),wxSize(120,16));

	m_pCustTexCoords = new wxCheckBox(m_surface_panel, SDE_FLAGS, DKLOC("TOKEN_CUSTTEXCOORD", "Custom tex. coords"), wxPoint(4,166),wxSize(120,16));
	//m_pCustTexCoords->Disable();

	m_pDrawSelectionMask = new wxCheckBox(m_surface_panel, SDE_RENDER_UPDATE, DKLOC("TOKEN_SELMASK", "Show Selection mask"), wxPoint(4,206),wxSize(120,16));
	m_pDrawSelectionMask->SetValue(true);

	m_texview = new CTextureView(m_surface_panel, wxPoint(215,5), wxSize(128,128));
	m_texname = new wxTextCtrl(m_surface_panel,-1," ", wxPoint(215,134), wxSize(256,25));
	m_texname->Disable();

	new wxButton(m_surface_panel, SDE_TEXTURAXISFROM3DVIEW, DKLOC("TOKEN_TEXAXIS3DVIEW", "Set TexAxis from 3D"), wxPoint(5,240), wxSize(75,25));

	/*
	new wxStaticText(m_surface_panel, -1, DKLOC("TOKEN_JUSTIFY", "Justify"), wxPoint(80,5),wxSize(80,16));

	wxButton* pJustifyFit = new wxButton(m_surface_panel, SDE_JUSTIFY_FIT, "F", wxPoint(15,5), wxSize(25,25));
	wxButton* pJustifyTop = new wxButton(m_surface_panel, SDE_JUSTIFY_TOP, "T", wxPoint(40,5), wxSize(25,25));
	wxButton* pJustifyCtr = new wxButton(m_surface_panel, SDE_JUSTIFY_CENTER, "C", wxPoint(40,30), wxSize(25,25));
	wxButton* pJustifyBtm = new wxButton(m_surface_panel, SDE_JUSTIFY_BOTTOM, "B", wxPoint(40,55), wxSize(25,25));
	wxButton* pJustifyLft = new wxButton(m_surface_panel, SDE_JUSTIFY_LEFT, "L", wxPoint(15,30), wxSize(25,25));
	wxButton* pJustifyRgt = new wxButton(m_surface_panel, SDE_JUSTIFY_RIGHT, "R", wxPoint(65,30), wxSize(25,25));
	*/

	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer25;
	bSizer25 = new wxBoxSizer( wxVERTICAL );
	
	wxFlexGridSizer* fgSizer6;
	fgSizer6 = new wxFlexGridSizer( 0, 4, 0, 0 );
	fgSizer6->SetFlexibleDirection( wxBOTH );
	fgSizer6->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	
	fgSizer6->Add( 0, 0, 1, wxEXPAND, 5 );

	fgSizer6->Add( new wxStaticText( m_surface_panel, wxID_ANY, DKLOC("TOKEN_SCALE", "Scale:"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	fgSizer6->Add( new wxStaticText( m_surface_panel, wxID_ANY, DKLOC("TOKEN_SHIFT", "Shift:"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	fgSizer6->Add( new wxStaticText( m_surface_panel, wxID_ANY, DKLOC("TOKEN_ROTATION", "Rotation:"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	fgSizer6->Add( new wxStaticText( m_surface_panel, wxID_ANY, wxT("X"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxHORIZONTAL );
	
	m_pScale[0] = new wxTextCtrl( m_surface_panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 45,-1 ), 0 );
	bSizer9->Add( m_pScale[0], 0, wxBOTTOM|wxLEFT, 5 );
	
	bSizer9->Add( new wxSpinButton( m_surface_panel, SDE_SCALEX, wxDefaultPosition, wxSize( 15,20 ), 0 ), 0, wxRIGHT, 5 );
	
	
	fgSizer6->Add( bSizer9, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer91;
	bSizer91 = new wxBoxSizer( wxHORIZONTAL );
	
	m_pMove[0] = new wxTextCtrl( m_surface_panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 45,-1 ), 0 );
	bSizer91->Add( m_pMove[0], 0, wxBOTTOM|wxLEFT, 5 );
	
	bSizer91->Add( new wxSpinButton( m_surface_panel, SDE_MOVEX, wxDefaultPosition, wxSize( 15,20 ), 0 ), 0, wxRIGHT, 5 );
	
	
	fgSizer6->Add( bSizer91, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer92;
	bSizer92 = new wxBoxSizer( wxHORIZONTAL );
	
	m_pRotate = new wxTextCtrl( m_surface_panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 45,-1 ), 0 );
	bSizer92->Add( m_pRotate, 0, wxBOTTOM|wxLEFT, 5 );
	
	bSizer92->Add( new wxSpinButton( m_surface_panel, SDE_ROTATE, wxDefaultPosition, wxSize( 15,20 ), 0 ), 0, wxRIGHT, 5 );
	
	
	fgSizer6->Add( bSizer92, 1, wxEXPAND, 5 );
	

	fgSizer6->Add( new wxStaticText( m_surface_panel, wxID_ANY, wxT("Y"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	wxBoxSizer* bSizer93;
	bSizer93 = new wxBoxSizer( wxHORIZONTAL );
	
	m_pScale[1] = new wxTextCtrl( m_surface_panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 45,-1 ), 0 );
	bSizer93->Add( m_pScale[1], 0, wxBOTTOM|wxLEFT, 5 );
	
	bSizer93->Add( new wxSpinButton( m_surface_panel, SDE_SCALEY, wxDefaultPosition, wxSize( 15,20 ), 0 ), 0, wxRIGHT, 5 );
	
	
	fgSizer6->Add( bSizer93, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer94;
	bSizer94 = new wxBoxSizer( wxHORIZONTAL );
	
	m_pMove[1] = new wxTextCtrl( m_surface_panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 45,-1 ), 0 );
	bSizer94->Add( m_pMove[1], 0, wxBOTTOM|wxLEFT, 5 );
	
	bSizer94->Add( new wxSpinButton( m_surface_panel, SDE_MOVEY, wxDefaultPosition, wxSize( 15,20 ), 0 ), 0, wxRIGHT, 5 );
	
	
	fgSizer6->Add( bSizer94, 1, wxEXPAND, 5 );
	
	
	bSizer25->Add( fgSizer6, 0, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizer5;
	sbSizer5 = new wxStaticBoxSizer( new wxStaticBox( m_surface_panel, wxID_ANY,  DKLOC("TOKEN_SETTINGS", "Settings") ), wxVERTICAL );
	
	m_pNoCollide = new wxCheckBox( m_surface_panel, SDE_FLAGS, DKLOC("TOKEN_NOCOLLIDE", "No collsion"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer5->Add( m_pNoCollide, 0, wxALL, 5 );
	
	m_pNoSubdivision = new wxCheckBox( m_surface_panel, SDE_FLAGS, DKLOC("TOKEN_NOSUBDIV", "No subdivision"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer5->Add( m_pNoSubdivision, 0, wxALL, 5 );
	
	wxBoxSizer* bSizer95;
	bSizer95 = new wxBoxSizer( wxHORIZONTAL );
	
	//m_pEnableTess = new wxCheckBox( m_surface_panel, wxID_ANY, wxT("TESSELATION"), wxDefaultPosition, wxDefaultSize, 0 );
	//bSizer95->Add( m_pEnableTess, 0, wxALL, 5 );
	
	m_pTess = new wxTextCtrl( m_surface_panel, SDE_SURFTESS, wxEmptyString, wxDefaultPosition, wxSize( 45,-1 ), 0 );
	bSizer95->Add( m_pTess, 0, wxBOTTOM|wxLEFT, 5 );
	
	bSizer95->Add( new wxSpinButton( m_surface_panel, SDE_SURFTESS, wxDefaultPosition, wxSize( 15,20 ), 0 ), 0, wxRIGHT, 5 );
	
	
	sbSizer5->Add( bSizer95, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer29;
	bSizer29 = new wxBoxSizer( wxHORIZONTAL );
	
	m_pCustTexCoords = new wxCheckBox( m_surface_panel, SDE_FLAGS, DKLOC("TOKEN_CUSTTEXCOORD", "Manual tex. coords"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer29->Add( m_pCustTexCoords, 0, wxALL, 5 );
	
	bSizer29->Add( new wxButton( m_surface_panel, SDE_TEXTURAXISFROM3DVIEW, DKLOC("TOKEN_PROJECT", "Project"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxRIGHT, 5 );
	
	
	sbSizer5->Add( bSizer29, 1, wxEXPAND, 5 );
	
	
	bSizer25->Add( sbSizer5, 1, wxEXPAND, 5 );
	
	m_pDrawSelectionMask = new wxCheckBox( m_surface_panel, SDE_RENDER_UPDATE, DKLOC("TOKEN_SELMASK", "Show Selection mask"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer25->Add( m_pDrawSelectionMask, 0, wxALL, 5 );
	m_pDrawSelectionMask->SetValue(true); 
	
	
	bSizer4->Add( bSizer25, 1, 0, 5 );
	
	wxBoxSizer* bSizer19;
	bSizer19 = new wxBoxSizer( wxVERTICAL );
	
	m_texview = new CTextureView(m_surface_panel, wxDefaultPosition, wxSize(256,256));
	m_texview->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_CAPTIONTEXT ) );
	m_texview->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_CAPTIONTEXT ) );
	
	bSizer19->Add( m_texview, 0, wxALL, 5 );
	
	wxBoxSizer* bSizer20;
	bSizer20 = new wxBoxSizer( wxHORIZONTAL );
	
	m_texname = new wxTextCtrl( m_surface_panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	bSizer20->Add( m_texname, 1, wxBOTTOM|wxLEFT, 5 );
	
	//bSizer20->Add( new wxButton( this, SDE_SHOWMATERIALSELECTOR, wxT("..."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT ), 0, wxBOTTOM|wxRIGHT, 5 );
	
	
	bSizer19->Add( bSizer20, 1, wxEXPAND, 5 );
	
	
	bSizer4->Add( bSizer19, 0, wxEXPAND, 5 );
	
	
	m_terrain_panel->SetSizer( bSizer4 );
	m_terrain_panel->Layout();
}

void CSurfaceDialog::OnButton(wxCommandEvent& event)
{
	if(event.GetId() == SDE_TEXTURAXISFROM3DVIEW)
	{
		for(int i = 0; i < g_pLevel->GetEditableCount(); i++)
		{
			if((g_pLevel->GetEditable(i)->GetType() != EDITABLE_DECAL))
			{
				if(g_pLevel->GetEditable(i)->GetType() > EDITABLE_TERRAINPATCH)
					continue;
			}

			CEditableSurface* pEditable = (CEditableSurface*)g_pLevel->GetEditable(i);

			bool bChanged = false;

			for(int j = 0; j < pEditable->GetSurfaceTextureCount(); j++)
			{
				if(!(pEditable->GetSurfaceTexture(j)->nFlags & STFL_SELECTED))
					continue;

				Vector3D forward;
				AngleVectors(g_editormainframe->GetViewWindow(0)->GetActiveView()->GetView()->GetAngles(),&forward);

				pEditable->GetSurfaceTexture(j)->Plane.normal = -forward;

				bChanged = true;
			}

			if(bChanged)
				pEditable->UpdateSurfaceTextures();
		}

		g_editormainframe->GetViewWindow(0)->NotifyUpdate();
	}
}

#define LIST_ID 2400

void CSurfaceDialog::InitTextureReplacementTab()
{
	m_pSelectedTextureList = new wxListBox(m_texturereplacement_panel, LIST_ID, wxPoint(5,5), wxSize(250,325), 0, NULL, wxLB_MULTIPLE);
}

void CSurfaceDialog::OnSpinUpTextureParams(wxSpinEvent& event)
{
	switch(event.GetId())
	{
		case SDE_MOVEX:
		{
			float value = atof(m_pMove[0]->GetValue().c_str());
			value += 10;
			m_pMove[0]->SetValue(varargs("%g", value));
			break;
		}
		case SDE_MOVEY:
		{
			float value = atof(m_pMove[1]->GetValue().c_str());
			value += 10;
			m_pMove[1]->SetValue(varargs("%g", value));
			break;
		}
		case SDE_SCALEX:
		{
			float value = atof(m_pScale[0]->GetValue().c_str());
			value += 0.1f;
			m_pScale[0]->SetValue(varargs("%g", value));
			break;
		}
		case SDE_SCALEY:
		{
			float value = atof(m_pScale[1]->GetValue().c_str());
			value += 0.1f;
			m_pScale[1]->SetValue(varargs("%g", value));
			break;
		}
		case SDE_ROTATE:
		{
			int value = atoi(m_pRotate->GetValue().c_str());
			value += 1;
			m_pRotate->SetValue(varargs("%i", value));
			break;
		}
	}

	UpdateSurfaces();
}

void CSurfaceDialog::OnSpinDownTextureParams(wxSpinEvent& event)
{
	switch(event.GetId())
	{
		case SDE_MOVEX:
		{
			float value = atof(m_pMove[0]->GetValue().c_str());
			value -= 10;
			m_pMove[0]->SetValue(varargs("%g", value));
			break;
		}
		case SDE_MOVEY:
		{
			float value = atof(m_pMove[1]->GetValue().c_str());
			value -= 10;
			m_pMove[1]->SetValue(varargs("%g", value));
			break;
		}
		case SDE_SCALEX:
		{
			float value = atof(m_pScale[0]->GetValue().c_str());
			value -= 0.1f;
			m_pScale[0]->SetValue(varargs("%g", value));
			break;
		}
		case SDE_SCALEY:
		{
			float value = atof(m_pScale[1]->GetValue().c_str());
			value -= 0.1f;
			m_pScale[1]->SetValue(varargs("%g", value));
			break;
		}
		case SDE_ROTATE:
		{
			int value = atoi(m_pRotate->GetValue().c_str());
			value -= 1;
			m_pRotate->SetValue(varargs("%i", value));
			break;
		}
	}

	UpdateSurfaces();
}

void CSurfaceDialog::OnTextParamsChanged(wxCommandEvent& event)
{
	UpdateSurfaces();
}

void CSurfaceDialog::InitTerrainEditTab()
{
	/*
	m_pSphereRadius = new wxSlider(m_terrain_panel,SDE_RADUISUPDATE+100, 16,1,512,wxPoint(4,4),wxSize(165, 20));
	m_pSpherePower = new wxSlider(m_terrain_panel,SDE_POWERUPDATE+100, 5,1,100,wxPoint(4,29),wxSize(165, 20));

	m_pRadiusLabel = new wxStaticText(m_terrain_panel, -1, "Radius: 16", wxPoint(169,4),wxSize(80,16));
	m_pPowerLabel = new wxStaticText(m_terrain_panel, -1, "Power: 5", wxPoint(169,29),wxSize(80,16));

	new wxStaticText(m_terrain_panel, -1, DKLOC("TOKEN_PAINTTYPE", "Paint type"), wxPoint(4,45),wxSize(80,16));

	wxRadioButton* pMainSelection = new wxRadioButton(m_terrain_panel, PAINTER_VERTEX_ADVANCE+100, DKLOC("TOKEN_VERTEXADV", "Vertex advance"), wxPoint(4, 61),wxSize(120,16));
	pMainSelection->SetValue(true);

	new wxRadioButton(m_terrain_panel, PAINTER_VERTEX_SMOOTH+100, DKLOC("TOKEN_VERTEXSMTH", "Vertex smooth"), wxPoint(4, 77),wxSize(120,16));
	new wxRadioButton(m_terrain_panel, PAINTER_VERTEX_SETLEVEL+100, DKLOC("TOKEN_VERTEXSETLEVEL", "Set level"), wxPoint(4, 93),wxSize(120,16));
	new wxRadioButton(m_terrain_panel, PAINTER_TEXTURETRANSITION+100, DKLOC("TOKEN_TEXTRANSITION", "Texture transiton"), wxPoint(4, 109),wxSize(120,16));

	wxString axis_choices[] = 
	{
		"X",
		"Y",
		"Z",
	};

	m_pPaintAxis = new wxComboBox(m_terrain_panel, -1, "Y", wxPoint(130,70),wxSize(65,16),3, axis_choices, wxCB_READONLY | wxCB_DROPDOWN);

	wxString layer_chioces[] = 
	{
		DKLOC("TOKEN_LAYER1", "Layer 1"),
		DKLOC("TOKEN_LAYER2", "Layer 2"),
		DKLOC("TOKEN_LAYER3", "Layer 3"),
		DKLOC("TOKEN_LAYER4", "Layer 4"),
	};

	m_pPaintLayer = new wxComboBox(m_terrain_panel, -1, layer_chioces[0],  wxPoint(130,93),wxSize(65,16), 4, layer_chioces, wxCB_READONLY | wxCB_DROPDOWN);

	m_pDrawWiredTerrain = new wxCheckBox(m_terrain_panel, -1, DKLOC("TOKEN_WIREMASK", "Show Wire mask"), wxPoint(4,125),wxSize(120,16));
	m_pDrawWiredTerrain->SetValue(true);
	*/

	wxBoxSizer* bSizer31;
	bSizer31 = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticBoxSizer* sbSizer7;
	sbSizer7 = new wxStaticBoxSizer( new wxStaticBox( m_terrain_panel, wxID_ANY, wxT("Surface painter") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizer7;
	fgSizer7 = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer7->SetFlexibleDirection( wxBOTH );
	fgSizer7->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	fgSizer7->Add( new wxStaticText( m_terrain_panel, wxID_ANY, wxT("Radius"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_pSphereRadius = new wxSlider( m_terrain_panel, SDE_RADUISUPDATE, 16, 1, 200, wxDefaultPosition, wxSize( 200,20 ), wxSL_HORIZONTAL );
	fgSizer7->Add( m_pSphereRadius, 0, wxTOP|wxRIGHT, 5 );
	
	m_pRadiusValue = new wxTextCtrl( m_terrain_panel, wxID_ANY, wxT("16"), wxDefaultPosition, wxSize( 40,-1 ), 0 );
	fgSizer7->Add( m_pRadiusValue, 0, wxBOTTOM, 5 );
	
	fgSizer7->Add( new wxStaticText( m_terrain_panel, wxID_ANY, wxT("Power"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	m_pSpherePower = new wxSlider( m_terrain_panel, SDE_POWERUPDATE, 10, 1, 200, wxDefaultPosition, wxSize( 200,20 ), wxSL_HORIZONTAL );
	fgSizer7->Add( m_pSpherePower, 0, wxTOP|wxRIGHT, 5 );
	
	m_pPowerValue = new wxTextCtrl( m_terrain_panel, wxID_ANY, wxT("10"), wxDefaultPosition, wxSize( 40,-1 ), 0 );
	fgSizer7->Add( m_pPowerValue, 0, wxBOTTOM, 5 );
	
	
	sbSizer7->Add( fgSizer7, 0, 0, 5 );
	
	wxBoxSizer* bSizer33;
	bSizer33 = new wxBoxSizer( wxHORIZONTAL );
	
	wxString m_pPainterSelChoices[] = { 
		DKLOC("TOKEN_VERTEXADV", "Vertex advance"), 
		DKLOC("TOKEN_VERTEXSMTH", "Vertex smooth"), 
		DKLOC("TOKEN_VERTEXSETLEVEL", "Set level"), 
		DKLOC("TOKEN_TEXTRANSITION", "Texture transiton")
	};

	int m_pPainterSelNChoices = sizeof( m_pPainterSelChoices ) / sizeof( wxString );
	m_pPainterSel = new wxRadioBox( m_terrain_panel, SDE_PAINTERSELECT, DKLOC("TOKEN_PAINTTYPE", "Paint type"), wxDefaultPosition, wxSize( 150,-1 ), m_pPainterSelNChoices, m_pPainterSelChoices, 1, wxRA_SPECIFY_COLS );
	m_pPainterSel->SetSelection( 0 );
	bSizer33->Add( m_pPainterSel, 0, wxALL, 5 );
	
	wxFlexGridSizer* fgSizer8;
	fgSizer8 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer8->SetFlexibleDirection( wxBOTH );
	fgSizer8->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	fgSizer8->Add( new wxStaticText( m_terrain_panel, wxID_ANY, wxT("Axis"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	wxString m_pPaintAxisChoices[] = { wxT("X"), wxT("Y"), wxT("Z") };
	int m_pPaintAxisNChoices = sizeof( m_pPaintAxisChoices ) / sizeof( wxString );
	m_pPaintAxis = new wxChoice( m_terrain_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_pPaintAxisNChoices, m_pPaintAxisChoices, 0 );
	m_pPaintAxis->SetSelection( 1 );
	fgSizer8->Add( m_pPaintAxis, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	fgSizer8->Add( new wxStaticText( m_terrain_panel, wxID_ANY, wxT("Layer"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	wxString m_pPaintLayerChoices[] = { 
		DKLOC("TOKEN_LAYER1", "Layer 1"),
		DKLOC("TOKEN_LAYER2", "Layer 2"),
		DKLOC("TOKEN_LAYER3", "Layer 3"),
		DKLOC("TOKEN_LAYER4", "Layer 4"),
	};
	int m_pPaintLayerNChoices = sizeof( m_pPaintLayerChoices ) / sizeof( wxString );
	m_pPaintLayer = new wxChoice( m_terrain_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_pPaintLayerNChoices, m_pPaintLayerChoices, 0 );
	m_pPaintLayer->SetSelection( 0 );
	fgSizer8->Add( m_pPaintLayer, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	
	bSizer33->Add( fgSizer8, 1, wxEXPAND, 5 );
	
	
	sbSizer7->Add( bSizer33, 1, wxEXPAND, 5 );
	
	m_pDrawWiredTerrain = new wxCheckBox( m_terrain_panel, wxID_ANY, DKLOC("TOKEN_WIREMASK", "Show Wire mask"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pDrawWiredTerrain->SetValue(true); 
	sbSizer7->Add( m_pDrawWiredTerrain, 0, wxALL, 5 );
	
	
	bSizer31->Add( sbSizer7, 0, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizer8;
	sbSizer8 = new wxStaticBoxSizer( new wxStaticBox( m_terrain_panel, wxID_ANY, wxT("Detail properties") ), wxVERTICAL );
	
	
	bSizer31->Add( sbSizer8, 1, wxEXPAND, 5 );
	
	
	m_terrain_panel->SetSizer( bSizer31 );
	m_terrain_panel->Layout();

	m_nPaintType = PAINTER_VERTEX_ADVANCE;
	
}

bool CSurfaceDialog::TerrainWireframeEnabled()
{
	return m_pDrawWiredTerrain->GetValue() && m_terrain_panel->IsShownOnScreen();
}

bool CSurfaceDialog::IsSelectionMaskDisabled()
{
	return !m_pDrawSelectionMask->GetValue() && this->IsShownOnScreen();
}

int CSurfaceDialog::GetPaintAxis()
{
	return m_pPaintAxis->GetSelection();
}

int CSurfaceDialog::GetPaintTextureLayer()
{
	return m_pPaintLayer->GetSelection();
}

void CSurfaceDialog::OnEvent_ToUpdate(wxCommandEvent& event)
{
	if(event.GetId() == SDE_FLAGS)
		UpdateSurfaces();

	g_editormainframe->UpdateAllWindows();
}

void CSurfaceDialog::OnClose(wxCloseEvent& event)
{
	Hide();
	g_editormainframe->UpdateAllWindows();
}

void CSurfaceDialog::OnShow(wxShowEvent& event)
{
	g_editormainframe->UpdateAllWindows();
}

void CSurfaceDialog::OnSelectPaintType(wxCommandEvent& event)
{
	m_nPaintType = (VertexPainterType_e)(m_pPainterSel->GetSelection()+1);
}

void CSurfaceDialog::OnSliderChange(wxScrollEvent& event)
{
	if(event.GetId() == SDE_RADUISUPDATE)
		m_pRadiusValue->SetValue(varargs("%d", m_pSphereRadius->GetValue()));

	if(event.GetId() == SDE_POWERUPDATE)
		m_pPowerValue->SetValue(varargs("%d", m_pSpherePower->GetValue()));

	g_editormainframe->UpdateAllWindows();
}

void CSurfaceDialog::OnNoteBookPageChange(wxAuiNotebookEvent& event)
{
	g_editormainframe->UpdateAllWindows();
}

void CSurfaceDialog::UpdateSelection()
{
	IMaterial* pSelectMaterial = NULL;

	m_texview->SetMaterial(NULL);

	m_pUsedSelectedMaterials.clear();

	editableType.Reset();
	editableType.CompareValue(EDITABLE_TERRAINPATCH);

	shiftValX.Reset();
	shiftValY.Reset();

	scaleValX.Reset();
	scaleValY.Reset();

	rotateVal.Reset();
	materialVal.Reset();

	tesselationVal.Reset();

	nocollide.Reset();
	nosubdivision.Reset();
	custtexcoords.Reset();

	for(int i = 0; i < g_pLevel->GetEditableCount(); i++)
	{
		CBaseEditableObject* pEditable = g_pLevel->GetEditable(i);

		for(int j = 0; j < pEditable->GetSurfaceTextureCount(); j++)
		{
			if(!(pEditable->GetSurfaceTexture(j)->nFlags & STFL_SELECTED))
				continue;

			Vector2D shift(pEditable->GetSurfaceTexture(j)->UAxis.offset,pEditable->GetSurfaceTexture(j)->VAxis.offset);
			Vector2D scale = pEditable->GetSurfaceTexture(j)->vScale;
			float rotate = pEditable->GetSurfaceTexture(j)->fRotation;
			IMaterial* pMat = pEditable->GetSurfaceTexture(j)->pMaterial;

			// add unique selected materials
			m_pUsedSelectedMaterials.addUnique(pMat);

			// add values to comparators
			shiftValX.CompareValue(shift.x);
			shiftValY.CompareValue(shift.y);

			scaleValX.CompareValue(scale.x);
			scaleValY.CompareValue(scale.y);

			rotateVal.CompareValue(rotate);
			materialVal.CompareValue(pMat);

			nocollide.CompareValue((pEditable->GetSurfaceTexture(j)->nFlags & STFL_NOCOLLIDE) > 0);
			nosubdivision.CompareValue((pEditable->GetSurfaceTexture(j)->nFlags & STFL_NOSUBDIVISION) > 0);
			custtexcoords.CompareValue((pEditable->GetSurfaceTexture(j)->nFlags & STFL_CUSTOMTEXCOORD) > 0);

			editableType.CompareValue(pEditable->GetType());

			//tesselationVal.CompareValue();
		}
	}

	m_pMove[0]->SetValue("");
	m_pMove[1]->SetValue("");

	m_pScale[0]->SetValue("");
	m_pScale[1]->SetValue("");

	m_pRotate->SetValue("");
	m_texname->SetValue("");

	/*
	m_pNoCollide->SetValue(false);
	m_pNoSubdivision->SetValue(false);
	m_pCustTexCoords->SetValue(false);
	*/

	m_texview->SetMaterial(NULL);

	m_pSelectedTextureList->Clear();

	if(!nocollide.IsDiffers())		m_pNoCollide->SetValue(nocollide.GetValue());
	if(!nosubdivision.IsDiffers())	m_pNoSubdivision->SetValue(nosubdivision.GetValue());
	if(!custtexcoords.IsDiffers())	m_pCustTexCoords->SetValue(custtexcoords.GetValue());
		
	if(!shiftValX.IsDiffers()) m_pMove[0]->SetValue(varargs("%g", shiftValX.GetValue()));
	if(!shiftValY.IsDiffers()) m_pMove[1]->SetValue(varargs("%g", shiftValY.GetValue()));

	if(!scaleValX.IsDiffers()) m_pScale[0]->SetValue(varargs("%g", scaleValX.GetValue()));
	if(!scaleValY.IsDiffers()) m_pScale[1]->SetValue(varargs("%g", scaleValY.GetValue()));

	if(!rotateVal.IsDiffers()) m_pRotate->SetValue(varargs("%g", rotateVal.GetValue()));

	if(!materialVal.IsDiffers())
	{
		m_texview->SetMaterial(materialVal.GetValue());
		m_texname->SetValue(wxString(materialVal.GetValue()->GetName()));
	}

	// update used textures dialog
	for(int i = 0; i < m_pUsedSelectedMaterials.numElem(); i++)
		m_pSelectedTextureList->AppendString( m_pUsedSelectedMaterials[i]->GetName() );
}

void CSurfaceDialog::UpdateSurfaces()
{
	/*
	shiftValX.Reset();
	shiftValY.Reset();

	scaleValX.Reset();
	scaleValY.Reset();

	rotateVal.Reset();
	materialVal.Reset();

	tesselationVal.Reset();

	nocollide.Reset();
	nosubdivision.Reset();
	custtexcoords.Reset();
	*/

	/*
	bool shift_differs[2];
	bool scale_differs[2];
	bool rotate_differs;

	memset(shift_differs, 0, sizeof(shift_differs));
	memset(scale_differs, 0, sizeof(scale_differs));
	rotate_differs = false;

	if(m_pMove[0]->GetValue().Length() == 0)
		shift_differs[0] = true;
	if(m_pMove[1]->GetValue().Length() == 0)
		shift_differs[1] = true;

	if(m_pScale[0]->GetValue().Length() == 0)
		scale_differs[0] = true;
	if(m_pScale[1]->GetValue().Length() == 0)
		scale_differs[1] = true;

	if(m_pRotate->GetValue().Length() == 0)
		rotate_differs = true;
		*/

	Vector2D	shift(atof(m_pMove[0]->GetValue().c_str()), atof(m_pMove[1]->GetValue().c_str()));
	Vector2D	scale(atof(m_pScale[0]->GetValue().c_str()), atof(m_pScale[1]->GetValue().c_str()));
	float		rotate = atof(m_pRotate->GetValue().c_str());

	for(int i = 0; i < g_pLevel->GetEditableCount(); i++)
	{
		CBaseEditableObject* pEditable = g_pLevel->GetEditable(i);

		bool bChanged = false;

		for(int j = 0; j < pEditable->GetSurfaceTextureCount(); j++)
		{
			if(!(pEditable->GetSurfaceTexture(j)->nFlags & STFL_SELECTED))
				continue;

			if(!shiftValX.IsDiffers())
				pEditable->GetSurfaceTexture(j)->UAxis.offset = shift.x;
			if(!shiftValY.IsDiffers())
				pEditable->GetSurfaceTexture(j)->VAxis.offset = shift.y;

			if(!scaleValX.IsDiffers())
				pEditable->GetSurfaceTexture(j)->vScale.x = scale.x;
			if(!scaleValY.IsDiffers())
				pEditable->GetSurfaceTexture(j)->vScale.y = scale.y;

			if(!rotateVal.IsDiffers())
				pEditable->GetSurfaceTexture(j)->fRotation = rotate;

			if(!nocollide.IsDiffers())
			{
				if(m_pNoCollide->GetValue())
					pEditable->GetSurfaceTexture(j)->nFlags |= STFL_NOCOLLIDE;
				else
					pEditable->GetSurfaceTexture(j)->nFlags &= ~STFL_NOCOLLIDE;
			}

			if(!nosubdivision.IsDiffers())
			{
				if(m_pNoSubdivision->GetValue())
					pEditable->GetSurfaceTexture(j)->nFlags |= STFL_NOSUBDIVISION;
				else
					pEditable->GetSurfaceTexture(j)->nFlags &= ~STFL_NOSUBDIVISION;
			}

			if(!custtexcoords.IsDiffers())
			{
				if(m_pCustTexCoords->GetValue())
					pEditable->GetSurfaceTexture(j)->nFlags |= STFL_CUSTOMTEXCOORD;
				else
					pEditable->GetSurfaceTexture(j)->nFlags &= ~STFL_CUSTOMTEXCOORD;
			}

			bChanged = true;
		}

		if(bChanged)
			pEditable->UpdateSurfaceTextures();
	}

	g_editormainframe->GetViewWindow(0)->NotifyUpdate();
}

VertexPainterType_e CSurfaceDialog::GetPainterType()
{
	return m_terrain_panel->IsShownOnScreen() ? m_nPaintType : PAINTER_NONE;
}

float CSurfaceDialog::GetPainterSphereRadius()
{
	float radius = atof(m_pRadiusValue->GetValue());

	if(radius > 0)
		return radius;

	return m_pSphereRadius->GetValue();
}

float CSurfaceDialog::GetPainterPower()
{
	float power = atof(m_pPowerValue->GetValue());

	if(power > 0)
		return power;

	return m_pSpherePower->GetValue();
}