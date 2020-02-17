//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Selection arbitary transformation dialog
//////////////////////////////////////////////////////////////////////////////////


#ifndef TRANSFORMSELECTIONDIALOG_H
#define TRANSFORMSELECTIONDIALOG_H

#include "wx_inc.h"
#include "EditorHeader.h"

class CTransformSelectionDialog : public wxDialog
{
public:
						CTransformSelectionDialog();

	bool				IsOverSelectionCenter();
	int					GetMode();

	Vector3D			GetModifyValue();

	void				OnButtonClick(wxCommandEvent& event);
	void				OnRadioSelect(wxCommandEvent& event);

protected:

	DECLARE_EVENT_TABLE();

	wxCheckBox*			m_pRotateScaleOverSelectionCenter;
	wxRadioButton*		m_pModes[3];
	wxTextCtrl*			m_pModifyValues[3];
};


#endif // TRANSFORMSELECTIONDIALOG_H