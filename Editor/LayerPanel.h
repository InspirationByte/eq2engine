//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Layer panel
//////////////////////////////////////////////////////////////////////////////////

#ifndef LAYERS_H
#define LAYERS_H

#include "wx_inc.h"
#include "EditorHeader.h"

class CLayerPanelDataViewModel;

class CWorldLayerPanel : public wxPanel
{
public:
	CWorldLayerPanel();
	~CWorldLayerPanel();

	void	UpdateLayers();
	void	OnButton(wxCommandEvent &event);
	void	OnSelectRow(wxDataViewEvent &event);

	DECLARE_EVENT_TABLE();

protected:

	wxDataViewCtrl*				m_pLayerList;
	CLayerPanelDataViewModel*	m_pModel;
	int							m_nSelectedLayer;

};

#endif // LAYERS_H