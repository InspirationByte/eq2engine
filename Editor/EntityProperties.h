//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Entity properties dialog
//////////////////////////////////////////////////////////////////////////////////

#ifndef ENTITYPROPERTIES_H
#define ENTITYPROPERTIES_H

#include "wx_inc.h"
#include "EditorHeader.h"

struct entProp_t
{
	EqString		key;
	EqString		value;
	edef_param_t*	value_desc;
};

class CEntityPropertiesPanel : public wxDialog
{
public:
	CEntityPropertiesPanel();

	void				UpdateSelection();

	void				OnClassSelect(wxCommandEvent &event);
	void				OnSetObjectName(wxCommandEvent &event);

	void				OnSelectParameter(wxListEvent &event);
	void				OnValueSet(wxCommandEvent &event);
	void				OnOpenModel(wxCommandEvent &event);

	void				InitOutputPanel(wxPanel* panel);

	void				OnSetTargetEnt(wxCommandEvent &event);

	void				OnOutputButtons(wxCommandEvent &event);
	void				OnSelectOutput(wxListEvent &event);

	void				OnSetLayer(wxCommandEvent &event);

	void				OnClose(wxCloseEvent &event);

	DECLARE_EVENT_TABLE();

protected:
	
	// keys

	wxListCtrl*				m_pKeyList;

	wxComboBox*				m_pClassList;
	wxComboBox*				m_pNameList;

	wxTextCtrl*				m_pKeyText;
	wxComboBox*				m_pValueText;

	wxButton*				m_pBrowseModelButton;
	wxButton*				m_pPickColorButton;

	DkList<entProp_t>		m_CurrentProps;

	int						m_nSelectedProp;
	eDef_ParamType_e		m_currentParamType;

	// outputs
	wxListCtrl*				m_pOutputList;

	wxComboBox*				m_pOutput;
	wxComboBox*				m_pTargetEntity;
	wxComboBox*				m_pTargetInput;
	wxTextCtrl*				m_pDelay;
	wxTextCtrl*				m_pFireTimes;
	wxTextCtrl*				m_pOutValue;

	wxPanel*				m_pPropertiesPanel;

	DkList<OutputData_t*>	m_CurrentOutputs;

	wxComboBox*				m_pLayer;
};

#endif // ENTITYPROPERTIES_H