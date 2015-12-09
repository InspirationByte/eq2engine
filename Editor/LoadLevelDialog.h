//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Level loading dialog
//////////////////////////////////////////////////////////////////////////////////

#ifndef LEVELLOADINGDIALOG_H
#define LEVELLOADINGDIALOG_H

#include "wx_inc.h"
#include "EditorHeader.h"

// level load dialog
class CLoadLevelDialog : public wxDialog
{
public:
	CLoadLevelDialog();

	void	RebuildLevelList();

	char*	GetSelectedLevelString();

	DECLARE_EVENT_TABLE()

protected:

	void					OnButtonClick(wxCommandEvent& event);
	void					OnDoubleClick(wxCommandEvent& event);

	wxListBox*				m_levels;
};

#endif // LEVELLOADINGDIALOG_H