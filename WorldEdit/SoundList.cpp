//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Sound list dialog
//////////////////////////////////////////////////////////////////////////////////

#include "SoundList.h"
#include "GameSoundEmitterSystem.h"

#define BUTTON_SELECT		0
#define BUTTON_SEARCH		1
#define BUTTON_PLAY			2

//-----------------------------------------------------
// Entity list
//-----------------------------------------------------

BEGIN_EVENT_TABLE(CSoundList, wxDialog)
	//EVT_BUTTON(-1, CEntityList::OnButtonClick)
	EVT_LISTBOX(BUTTON_SELECT, CSoundList::OnButtonClick)
	EVT_LISTBOX_DCLICK(BUTTON_SELECT, CSoundList::OnDoubleClickList)
	EVT_CLOSE(CSoundList::OnClose)
	EVT_BUTTON(-1, CSoundList::OnButtonClick)
END_EVENT_TABLE()

CSoundList::CSoundList() : wxDialog(g_editormainframe, -1, DKLOC("TOKEN_SOUNDLIST", L"Sound list"), wxPoint(-1,-1),wxSize(310,630), wxCAPTION | wxSYSTEM_MENU | wxCLOSE_BOX)
{
	Iconize( false );

	m_pSndList = new wxListBox(this, BUTTON_SELECT, wxPoint(5,5), wxSize(290,450), 0, NULL, wxLB_SINGLE | wxLB_SORT);

	new wxStaticText(this, -1, DKLOC("TOKEN_SEARCHTOK", L"Search string"), wxPoint(5,460));
	m_pSearchText = new wxTextCtrl(this, -1, "", wxPoint(75,460), wxSize(210, 25));

	new wxStaticText(this, -1, DKLOC("TOKEN_SOUNDNAME", L"Sound name"), wxPoint(5,490));
	m_pSoundName = new wxTextCtrl(this, -1, "", wxPoint(75,490), wxSize(210, 25), wxTE_READONLY);

	new wxButton(this, BUTTON_SEARCH, DKLOC("TOKEN_SEARCH", L"Search"), wxPoint(5,550),wxSize(65,25));

	UpdateList();
}

void CSoundList::Reload()
{
	g_sounds->Shutdown();
	g_sounds->Init(2000.0f);

	UpdateList();
}

void CSoundList::OnClose(wxCloseEvent& event)
{
	UpdateList();
	Hide();
}

void CSoundList::UpdateList()
{
	m_pSndList->Clear();

	EqString search_str(m_pSearchText->GetValue().c_str().AsChar());

	// fill the list
	for(int i = 0; i < g_sounds->m_allSounds.numElem(); i++)
	{
		if(search_str.Length() == 0)
		{
			m_pSndList->AppendString( g_sounds->m_allSounds[i]->pszName );
		}
		else if(strstr(g_sounds->m_allSounds[i]->pszName, search_str.GetData()))
			m_pSndList->AppendString( g_sounds->m_allSounds[i]->pszName );
	}
}

void CSoundList::OnButtonClick(wxCommandEvent& event)
{
	if(event.GetId() == BUTTON_SEARCH)
	{
		UpdateList();
	}
	else
	{
		m_pSoundName->SetValue( m_pSndList->GetString(m_pSndList->GetSelection()) );
	}
}

void CSoundList::OnDoubleClickList(wxCommandEvent& event)
{
	if(m_pSndList->GetSelection() != -1)
	{
		g_sounds->StopAllSounds();

		EqString str( m_pSndList->GetString(m_pSndList->GetSelection()).c_str().AsChar() );

		EmitSound_t emit;
		emit.name = (char*)str.GetData();
		emit.fVolume = 1.0f;
		emit.nFlags = (EMITSOUND_FLAG_FORCE_2D | EMITSOUND_FLAG_FORCE_CACHED);

		if(g_sounds->EmitSound( &emit ) == CHAN_INVALID)
		{
			wxMessageBox(varargs("Missing wave file(s) for sound '%s', Can't play!", str.GetData()), wxT("ERROR"), wxOK | wxICON_ERROR | wxCENTRE, g_editormainframe);
		}
	}
}