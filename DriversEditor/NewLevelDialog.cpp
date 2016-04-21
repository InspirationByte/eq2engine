//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: New level dialog
//////////////////////////////////////////////////////////////////////////////////

#include "NewLevelDialog.h"

///////////////////////////////////////////////////////////////////////////

CNewLevelDialog::CNewLevelDialog( wxWindow* parent ) : wxDialog( parent, -1, wxT("Level properties"), wxDefaultPosition, wxSize( 341, 252 ), wxDEFAULT_DIALOG_STYLE | wxCENTRE_ON_SCREEN )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer14;
	bSizer14 = new wxBoxSizer( wxVERTICAL );
	

	bSizer14->Add( new wxStaticText( this, wxID_ANY, wxT("Level dimensions:"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	wxFlexGridSizer* fgSizer71;
	fgSizer71 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer71->SetFlexibleDirection( wxBOTH );
	fgSizer71->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	

	fgSizer71->Add( new wxStaticText( this, wxID_ANY, wxT("Cells per region"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	m_cellsSizeSel = new wxComboBox( this, wxID_ANY, wxT("32x32"), wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY );
	m_cellsSizeSel->Append( wxT("32x32") );
	m_cellsSizeSel->Append( wxT("48x48") );
	m_cellsSizeSel->Append( wxT("64x64") );
	m_cellsSizeSel->Append( wxT("96x96") );
	m_cellsSizeSel->SetSelection( 0 );
	fgSizer71->Add( m_cellsSizeSel, 0, wxALL|wxEXPAND, 5 );
	
	
	bSizer14->Add( fgSizer71, 0, wxEXPAND, 5 );
	
	m_useImageTemplate = new wxCheckBox( this, wxID_ANY, wxT("Map template from image file"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer14->Add( m_useImageTemplate, 0, wxALL, 5 );
	
	wxFlexGridSizer* fgSizer7;
	fgSizer7 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer7->SetFlexibleDirection( wxBOTH );
	fgSizer7->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	fgSizer7->Add( new wxStaticText( this, wxID_ANY, wxT("Wide"), wxDefaultPosition, wxSize( 200,-1 ), 0 ), 0, wxALL, 5 );
	
	m_widthSel = new wxComboBox( this, wxID_ANY, wxT("Combo!"), wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	m_widthSel->Append( wxT("8") );
	m_widthSel->Append( wxT("16") );
	m_widthSel->Append( wxT("32") );
	m_widthSel->Append( wxT("48") );
	m_widthSel->Append( wxT("64") );
	m_widthSel->SetSelection( 1 );
	fgSizer7->Add( m_widthSel, 0, wxALL|wxEXPAND, 5 );

	fgSizer7->Add( new wxStaticText( this, wxID_ANY, wxT("Tall"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	m_heightTall = new wxComboBox( this, wxID_ANY, wxT("Combo!"), wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	m_heightTall->Append( wxT("8") );
	m_heightTall->Append( wxT("16") );
	m_heightTall->Append( wxT("32") );
	m_heightTall->Append( wxT("48") );
	m_heightTall->Append( wxT("64") );
	m_heightTall->SetSelection( 1 );
	fgSizer7->Add( m_heightTall, 0, wxALL|wxEXPAND, 5 );
	
	
	bSizer14->Add( fgSizer7, 0, wxEXPAND, 5 );
	
	m_imageTemplateFilename = new wxFilePickerCtrl( this, wxID_ANY, wxEmptyString, wxT("Select an image file"), wxT("Image files (*.png;*.tga)|*.png;*.tga"), wxDefaultPosition, wxDefaultSize, wxFLP_FILE_MUST_EXIST|wxFLP_OPEN|wxFLP_USE_TEXTCTRL );
	bSizer14->Add( m_imageTemplateFilename, 0, wxALL|wxEXPAND, 5 );
	
	bSizer14->Add( new wxButton( this, wxID_OK, wxT("Create"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL|wxALIGN_RIGHT, 5 );
	
	
	this->SetSizer( bSizer14 );
	this->Layout();
	
	this->Centre( wxBOTH );

	// Connect Events
	m_useImageTemplate->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CNewLevelDialog::OnCheckEnableTemplate ), NULL, this );

	bool enablement = m_useImageTemplate->GetValue();

	m_imageTemplateFilename->Enable(enablement);
	m_widthSel->Enable(!enablement);
	m_heightTall->Enable(!enablement);
}

CNewLevelDialog::~CNewLevelDialog()
{
	delete m_imageTemplateFilename;
	m_imageTemplateFilename = NULL;
}

static int cellSizeTable[] = 
{
	32,48,64,96,128
};

int CNewLevelDialog::GetLevelCellSize() const
{
	int sel = m_cellsSizeSel->GetSelection();

	return cellSizeTable[sel];
}

void CNewLevelDialog::GetLevelWideTall(int& wide, int& tall) const
{
	wide = atoi(m_widthSel->GetValue());
	tall = atoi(m_heightTall->GetValue());
}

const char*	CNewLevelDialog::GetLevelImageFileName()
{
	if(!m_useImageTemplate->GetValue())
		return NULL;

	m_filePath = m_imageTemplateFilename->GetPath().c_str().AsChar();

	return m_filePath.c_str();
}

void CNewLevelDialog::OnCheckEnableTemplate( wxCommandEvent& event )
{
	bool enablement = m_useImageTemplate->GetValue();

	m_imageTemplateFilename->Enable(enablement);
	m_widthSel->Enable(!enablement);
	m_heightTall->Enable(!enablement);
}