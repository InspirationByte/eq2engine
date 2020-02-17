//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Build Options dialog
//////////////////////////////////////////////////////////////////////////////////

#ifndef BUILDOPTIONSDIALOG_H
#define BUILDOPTIONSDIALOG_H

#include "wx_inc.h"
#include "EditorHeader.h"

class CBuildOptionsDialog : public wxDialog
{
public:
						CBuildOptionsDialog();

	void				SaveMapSettings();
	void				LoadMapSettings();

	int					GetTreeMaxDepth();
	int					GetSectorBoxSize();
	bool				IsTreeBuildingDisabled();
	bool				IsBrushCSGEnabled();
	bool				IsOnlyEntitiesMode();
	bool				IsPhysicsOnly();

	bool				IsLightmapsDisabled();
	float				GetLightmapLuxelsPerMeter();
	int					GetLightmapSize();


	void				OnButtonClick(wxCommandEvent& event);

protected:

	DECLARE_EVENT_TABLE();

	wxCheckBox*			m_pDisableTreeCheck;

	wxCheckBox*			m_pBrushCSGCheck;
	wxCheckBox*			m_pOnlyEntsCheck;

	wxComboBox*			m_pSectorSizeSpin;
	wxSpinCtrl*			m_pTreeMaxDepthSpin;
	wxCheckBox*			m_pPhysicsOnlyCompile;

	wxCheckBox*			m_pCompileLightmaps;
	wxComboBox*			m_pLightmapSize;
	wxTextCtrl*			m_pLuxelsPerMeter;
};


#endif // BUILDOPTIONSDIALOG_H