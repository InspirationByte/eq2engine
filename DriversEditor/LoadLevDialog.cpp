//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Level loading dialog
//////////////////////////////////////////////////////////////////////////////////

#include "LoadLevDialog.h"

#define BUTTON_OPEN			1000
#define BUTTON_CANCEL		1001
//#define BUTTON_NEW		1002

BEGIN_EVENT_TABLE(CLoadLevelDialog, wxDialog)
	EVT_BUTTON(-1, CLoadLevelDialog::OnButtonClick)
	EVT_LISTBOX_DCLICK(-1, CLoadLevelDialog::OnDoubleClick)
END_EVENT_TABLE()

CLoadLevelDialog::CLoadLevelDialog(wxWindow* parent) : wxDialog(parent, -1, DKLOC("TOKEN_CHOOSELEVEL", L"Choose a level to edit"),wxPoint(-1,-1),wxSize(400,680),wxCAPTION | wxSYSTEM_MENU | wxCLOSE_BOX | wxCENTER_ON_SCREEN)
{
	Iconize( false );

	m_levels = new wxListBox(this,-1,wxPoint(5,5), wxSize(380,600));

	new wxButton(this, BUTTON_OPEN, DKLOC("TOKEN_LOAD", L"Load"), wxPoint(5, 610),wxSize(50,25));
	new wxButton(this, BUTTON_CANCEL, DKLOC("TOKEN_CANCEL", L"Cancel"), wxPoint(60, 610),wxSize(50,25));
	//new wxButton(this, BUTTON_NEW, DKLOC("TOKEN_CREATENEW", "Create new"), wxPoint(115, 610),wxSize(70,25));

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

	EqString level_dir(GetFileSystem()->GetCurrentGameDirectory());
	level_dir = level_dir + EqString("/levels/*.lev");

	WIN32_FIND_DATAA wfd;
	HANDLE hFile;

	hFile = FindFirstFileA(level_dir.GetData(), &wfd);
	if(hFile != NULL)
	{
		while(1) 
		{
			EqString filename = wfd.cFileName;

			if( !(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
			{

				m_levels->Append(filename.Path_Strip_Ext().c_str());
			}

			if(!FindNextFileA(hFile, &wfd))
				break;
		}

		FindClose(hFile);
	}
}