//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Level loading dialog
//////////////////////////////////////////////////////////////////////////////////

#ifndef LOADLEVDIALOG_H
#define LOADLEVDIALOG_H

#include "EditorHeader.h"

#include <wx/gauge.h>


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

class CLoadingDialog : public wxDialog 
{
public:
	CLoadingDialog( wxWindow* parent);
	~CLoadingDialog();

	void			SetText(const wxString& text);
	void			SetPercentage(int percent);
	
protected:

	wxStaticText*	m_statusLabel;
	wxGauge*		m_statusGauge;
};

#endif // LOADLEVDIALOG_H