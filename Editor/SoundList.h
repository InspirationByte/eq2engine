//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Sound list dialog
//////////////////////////////////////////////////////////////////////////////////

#ifndef SOUNDLIST_H
#define SOUNDLIST_H

#include "wx_inc.h"
#include "EditorHeader.h"

class CSoundList : public wxDialog
{
public:
	CSoundList();

	void		Reload();
	void		UpdateList();
	void		OnButtonClick(wxCommandEvent& event);
	void		OnDoubleClickList(wxCommandEvent& event);
	void		OnClose(wxCloseEvent& event);

protected:

	DECLARE_EVENT_TABLE();

	wxListBox*					m_pSndList;

	wxTextCtrl*					m_pSearchText;
	wxTextCtrl*					m_pSoundName;
};

#endif // SOUNDLIST_H