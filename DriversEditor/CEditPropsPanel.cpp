#include "CEditPropsPanel.h"

#include "heightfield.h"

CEditPropsPanel::CEditPropsPanel( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style ) : wxPanel( parent, id, pos, size, style )
{
	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxVERTICAL );
	
	wxString m_modeChoices[] = { wxT("Texture"), wxT("Height up"), wxT("Height down") };
	int m_modeNChoices = sizeof( m_modeChoices ) / sizeof( wxString );
	m_mode = new wxRadioBox( this, wxID_ANY, wxT("Mode"), wxDefaultPosition, wxDefaultSize, m_modeNChoices, m_modeChoices, 1, wxRA_SPECIFY_COLS );
	m_mode->SetSelection( 0 );
	bSizer7->Add( m_mode, 0, wxALL|wxEXPAND, 5 );
	
	
	bSizer5->Add( bSizer7, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxHORIZONTAL );
	
	wxGridSizer* gSizer1;
	gSizer1 = new wxGridSizer( 0, 3, 0, 0 );
	
	gSizer1->SetMinSize( wxSize( 15,15 ) ); 
	
	gSizer1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	wxButton* m_button1 = new wxButton( this, SETROT_0, wxT("0"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	m_button1->SetMinSize( wxSize( 30,-1 ) );
	
	gSizer1->Add( m_button1, 0, wxTOP|wxBOTTOM|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	gSizer1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	wxButton* m_button2 = new wxButton( this, SETROT_270, wxT("270"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	m_button2->SetMinSize( wxSize( 30,-1 ) );
	
	gSizer1->Add( m_button2, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	
	gSizer1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	wxButton* m_button3 = new wxButton( this, SETROT_90, wxT("90"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	m_button3->SetMinSize( wxSize( 30,-1 ) );
	
	gSizer1->Add( m_button3, 0, wxTOP|wxBOTTOM|wxRIGHT, 5 );
	
	
	gSizer1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	wxButton* m_button4 = new wxButton( this, SETROT_180, wxT("180"), wxDefaultPosition, wxDefaultSize, 0 );
	m_button4->SetMinSize( wxSize( 30,-1 ) );
	
	gSizer1->Add( m_button4, 0, wxTOP|wxBOTTOM|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	bSizer3->Add( gSizer1, 0, 0, 5 );
	
	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxVERTICAL );
	
	m_detached = new wxCheckBox( this, wxID_ANY, wxT("detached"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer4->Add( m_detached, 0, wxALL, 5 );
	
	m_addWall = new wxCheckBox( this, wxID_ANY, wxT("add wall"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer4->Add( m_addWall, 0, wxALL, 5 );
	
	m_wallCollide = new wxCheckBox( this, wxID_ANY, wxT("wall collision"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer4->Add( m_wallCollide, 0, wxALL, 5 );
	
	m_noCollide = new wxCheckBox( this, wxID_ANY, wxT("no collide"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer4->Add( m_noCollide, 0, wxALL, 5 );
	
	
	bSizer3->Add( bSizer4, 0, 0, 5 );
	
	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxVERTICAL );
	
	bSizer8->Add( new wxStaticText( this, wxID_ANY, wxT("Height"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	m_addheight = new wxSlider( this, wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_AUTOTICKS|wxSL_HORIZONTAL|wxSL_LABELS );
	bSizer8->Add( m_addheight, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	bSizer8->Add( new wxStaticText( this, wxID_ANY, wxT("Radius"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	m_radius = new wxSlider( this, wxID_ANY, 2, 0, 32, wxDefaultPosition, wxDefaultSize, wxSL_AUTOTICKS|wxSL_HORIZONTAL|wxSL_LABELS );
	bSizer8->Add( m_radius, 0, wxALL|wxEXPAND, 5 );

	bSizer3->Add( bSizer8, 1, wxEXPAND, 5 );
	
	
	bSizer5->Add( bSizer3, 1, wxEXPAND, 5 );
	
	
	this->SetSizer( bSizer5 );
	this->Layout();
	
	// Connect Events
	m_button1->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEditPropsPanel::OnClickRotation ), NULL, this );
	m_button2->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEditPropsPanel::OnClickRotation ), NULL, this );
	m_button3->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEditPropsPanel::OnClickRotation ), NULL, this );
	m_button4->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEditPropsPanel::OnClickRotation ), NULL, this );
}

CEditPropsPanel::~CEditPropsPanel()
{

}

void CEditPropsPanel::OnClickRotation( wxCommandEvent& event )
{
	switch(event.GetId())
	{
		case SETROT_0:
			m_rotation = 0;
			break;
		case SETROT_90:
			m_rotation = 1;
			break;
		case SETROT_180:
			m_rotation = 2;
			break;
		case SETROT_270:
			m_rotation = 3;
			break;
	}
}

int CEditPropsPanel::GetRotation()
{
	return m_rotation;
}

int CEditPropsPanel::GetHeightfieldFlags()
{
	int flags = 0;

	flags |= m_addWall->GetValue() ? EHTILE_ADDWALL : 0;
	flags |= m_wallCollide->GetValue() ? EHTILE_COLLIDE_WALL : 0;
	flags |= m_detached->GetValue() ? EHTILE_DETACHED : 0;
	flags |= m_noCollide->GetValue() ? EHTILE_NOCOLLIDE : 0;

	return flags;
}

int	CEditPropsPanel::GetAddHeight()
{
	return m_addheight->GetValue();
}

int	CEditPropsPanel::GetRadius()
{
	return m_radius->GetValue();
}

EEditMode CEditPropsPanel::GetEditMode()
{
	return (EEditMode)m_mode->GetSelection();
}