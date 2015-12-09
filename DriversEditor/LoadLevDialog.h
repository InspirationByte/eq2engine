//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Level loading dialog
//////////////////////////////////////////////////////////////////////////////////

#ifndef LOADLEVDIALOG_H
#define LOADLEVDIALOG_H

#include "EditorHeader.h"

// level load dialog
class CLoadLevelDialog : public wxDialog
{
public:
				CLoadLevelDialog(wxWindow* parent);

	void		RebuildLevelList();

	char*		GetSelectedLevelString();

	DECLARE_EVENT_TABLE()

protected:

	void		OnButtonClick(wxCommandEvent& event);
	void		OnDoubleClick(wxCommandEvent& event);

	wxListBox*	m_levels;
};

#endif // LOADLEVDIALOG_H