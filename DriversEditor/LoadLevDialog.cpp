//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Level loading dialog
//////////////////////////////////////////////////////////////////////////////////

#include "LoadLevDialog.h"
#include "ILocalize.h"
#include "IFileSystem.h"

#define BUTTON_OPEN			1000
#define BUTTON_CANCEL		1001
//#define BUTTON_NEW		1002

BEGIN_EVENT_TABLE(CLoadLevelDialog, wxDialog)
	EVT_BUTTON(-1, CLoadLevelDialog::OnButtonClick)
	EVT_LISTBOX_DCLICK(-1, CLoadLevelDialog::OnDoubleClick)
END_EVENT_TABLE()

CLoadLevelDialog::CLoadLevelDialog(wxWindow* parent) : wxDialog(parent, -1, DKLOC("TOKEN_CHOOSELEVEL", "Choose a level to edit"),wxPoint(-1,-1),wxSize(400,680),wxCAPTION | wxSYSTEM_MENU | wxCLOSE_BOX | wxCENTER_ON_SCREEN)
{
	Iconize( false );

	m_levels = new wxListBox(this,-1,wxPoint(5,5), wxSize(380,600));

	wxButton* btnLoad = new wxButton(this, BUTTON_OPEN, DKLOC("TOKEN_LOAD", "Load"), wxPoint(5, 610),wxSize(50,25));
	new wxButton(this, BUTTON_CANCEL, DKLOC("TOKEN_CANCEL", "Cancel"), wxPoint(60, 610),wxSize(50,25));
	//new wxButton(this, BUTTON_NEW, DKLOC("TOKEN_CREATENEW", "Create new"), wxPoint(115, 610),wxSize(70,25));

	btnLoad->SetDefault();

	RebuildLevelList();
}

void CLoadLevelDialog::OnDoubleClick(wxCommandEvent& event)
{
	if(m_levels->GetSelection() != -1)
		EndModal(wxID_OK);
}

void CLoadLevelDialog::OnButtonClick(wxCommandEvent& event)
{
	if(event.GetId() == BUTTON_CANCEL || event.GetId() == wxID_CANCEL)
	{
		EndModal(wxID_CANCEL);
	}
	else if(event.GetId() == BUTTON_OPEN)
	{
		EndModal(wxID_OK);
	}
	else if(event.GetId() == wxID_CLOSE)
	{
		EndModal(wxID_CANCEL);
	}
}

char* CLoadLevelDialog::GetSelectedLevelString()
{
	static char selectedlevelname[128];
	strcpy(selectedlevelname, m_levels->GetStringSelection().c_str());

	return selectedlevelname;
}

void CLoadLevelDialog::RebuildLevelList()
{
	m_levels->Clear();

	EqString level_dir(g_fileSystem->GetCurrentGameDirectory());
	level_dir = level_dir + EqString("/levels/*.lev");

	DKFINDDATA* findData = nullptr;
	char* foundFile = (char*)g_fileSystem->FindFirst(level_dir.c_str(), &findData, SP_ROOT);

	if (foundFile)
	{
		EqString fileName(foundFile);

		if (!g_fileSystem->FindIsDirectory(findData))
			m_levels->Append(fileName.Path_Strip_Ext().c_str());

		while (foundFile = (char*)g_fileSystem->FindNext(findData))
		{
			fileName.Assign(foundFile);

			if (!g_fileSystem->FindIsDirectory(findData))
				m_levels->Append(fileName.Path_Strip_Ext().c_str());
		}

		g_fileSystem->FindClose(findData);
	}
}

//----------------------------------------------------------------------------------------------

CLoadingDialog::CLoadingDialog(wxWindow* parent) : wxDialog(parent, -1, DKLOC("TOKEN_PLSWAIT", "Please wait"),wxPoint(-1,-1),wxSize( 300,100 ),wxCAPTION | wxSTAY_ON_TOP | wxCENTER_ON_SCREEN)
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer24;
	bSizer24 = new wxBoxSizer( wxVERTICAL );
	
	m_statusLabel = new wxStaticText( this, wxID_ANY, wxT("Status"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
	m_statusLabel->Wrap( -1 );
	bSizer24->Add( m_statusLabel, 0, wxALL|wxEXPAND, 5 );
	
	m_statusGauge = new wxGauge( this, wxID_ANY, 100, wxDefaultPosition, wxDefaultSize, wxGA_HORIZONTAL );
	m_statusGauge->SetValue( 0 ); 
	bSizer24->Add( m_statusGauge, 0, wxALL|wxEXPAND, 5 );
	
	
	this->SetSizer( bSizer24 );
	this->Layout();
	
	this->Centre( wxBOTH );
}

CLoadingDialog::~CLoadingDialog()
{
}

void CLoadingDialog::SetText(const wxString& text)
{
	m_statusLabel->SetLabelText(text);
	Update();
}

void CLoadingDialog::SetPercentage(int percent)
{
	m_statusGauge->SetValue( percent );
	Update();
}