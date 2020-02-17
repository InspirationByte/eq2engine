//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: New level dialog
//////////////////////////////////////////////////////////////////////////////////

#ifndef NEWLEVELDIALOG_H
#define NEWLEVELDIALOG_H

#include "EditorHeader.h"

#include <wx/filepicker.h>

class CNewLevelDialog : public wxDialog 
{
public:
	CNewLevelDialog( wxWindow* parent);
	~CNewLevelDialog();

	int			GetLevelCellSize() const;
	void		GetLevelWideTall(int& wide, int& tall) const;

	const char*	GetLevelImageFileName();

	void		OnCheckEnableTemplate( wxCommandEvent& event );
	
protected:
	wxComboBox* m_widthSel;
	wxComboBox* m_heightTall;
	wxComboBox* m_cellsSizeSel;

	EqString	m_filePath;

	wxCheckBox* m_useImageTemplate;
	wxFilePickerCtrl* m_imageTemplateFilename;

	bool		m_doRedraw;
};

#endif //__NONAME_H__
