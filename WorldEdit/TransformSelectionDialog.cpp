//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Selection arbitary transformation dialog
//////////////////////////////////////////////////////////////////////////////////

#include "TransformSelectionDialog.h"

BEGIN_EVENT_TABLE(CTransformSelectionDialog, wxDialog)
	EVT_BUTTON(-1, CTransformSelectionDialog::OnButtonClick)
	EVT_RADIOBUTTON(-1, CTransformSelectionDialog::OnRadioSelect)
END_EVENT_TABLE()

CTransformSelectionDialog::CTransformSelectionDialog() : wxDialog(g_editormainframe, -1,"Arbitary transformation", wxPoint(-1,-1),wxSize(200,200), wxCAPTION | wxSYSTEM_MENU | wxCLOSE_BOX)
{
	Iconize( false );

	m_pModes[0] = new wxRadioButton(this, -1, "Move", wxPoint(5,5));
	m_pModes[1] = new wxRadioButton(this, -1, "Rotate", wxPoint(5,30));
	m_pModes[2] = new wxRadioButton(this, -1, "Scale", wxPoint(5,60));

	m_pModifyValues[0] = new wxTextCtrl(this, -1, "1.0", wxPoint(80,5), wxSize(80, 25));
	m_pModifyValues[1] = new wxTextCtrl(this, -1, "1.0", wxPoint(80,30), wxSize(80, 25));
	m_pModifyValues[2] = new wxTextCtrl(this, -1, "1.0", wxPoint(80,60), wxSize(80, 25));

	m_pRotateScaleOverSelectionCenter = new wxCheckBox(this, -1, "Transform over selection center", wxPoint(5,90));

	new wxButton(this, wxID_OK, "Apply", wxPoint(120, 120), wxSize(50, 25));
}

bool CTransformSelectionDialog::IsOverSelectionCenter()
{
	return m_pRotateScaleOverSelectionCenter->GetValue();
}

int CTransformSelectionDialog::GetMode()
{
	for(int i = 0; i < 3; i++)
	{
		if( m_pModes[i]->GetValue() )
			return i;
	}

	return 0;
}

void CTransformSelectionDialog::OnButtonClick(wxCommandEvent& event)
{
	if(event.GetId() == wxID_CANCEL)
	{
		EndModal(wxID_CANCEL);
	}
	else if(event.GetId() == wxID_OK)
	{
		EndModal(wxID_OK);
	}
}

void CTransformSelectionDialog::OnRadioSelect(wxCommandEvent& event)
{
	if(GetMode() == 0)
	{
		m_pModifyValues[0]->SetValue("0.0");
		m_pModifyValues[1]->SetValue("0.0");
		m_pModifyValues[2]->SetValue("0.0");
	}
	else if(GetMode() == 1)
	{
		m_pModifyValues[0]->SetValue("0.0");
		m_pModifyValues[1]->SetValue("0.0");
		m_pModifyValues[2]->SetValue("0.0");
	}
	else if(GetMode() == 2)
	{
		m_pModifyValues[0]->SetValue("1.0");
		m_pModifyValues[1]->SetValue("1.0");
		m_pModifyValues[2]->SetValue("1.0");
	}
}

Vector3D CTransformSelectionDialog::GetModifyValue()
{
	return Vector3D(atof(m_pModifyValues[0]->GetValue()),atof(m_pModifyValues[1]->GetValue()),atof(m_pModifyValues[2]->GetValue()));
}