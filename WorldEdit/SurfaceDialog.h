//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Terrain patch, editable surface, mesh dialog
//////////////////////////////////////////////////////////////////////////////////

#ifndef EDITABLESURFACETERRAINPATCH_H
#define EDITABLESURFACETERRAINPATCH_H

#include "wx_inc.h"
#include "EditorHeader.h"
#include "MultiComparator.h"

class CTextureView;

enum EditableType_e;

class CSurfaceDialog : public wxDialog
{
public:
	CSurfaceDialog();

	void					UpdateSelection();
	void					UpdateSurfaces();	// updates surface, and stores back the data to dialog

	float					GetPainterSphereRadius();
	float					GetPainterPower();

	VertexPainterType_e		GetPainterType();

	int						GetPaintAxis();
	int						GetPaintTextureLayer();

	bool					TerrainWireframeEnabled();
	bool					IsSelectionMaskDisabled();

	DECLARE_EVENT_TABLE()

protected:

	void					InitSurfaceEditTab();
	void					InitTerrainEditTab();
	void					InitTextureReplacementTab();

	void					OnClose(wxCloseEvent& event);
	void					OnShow(wxShowEvent& event);


	void					OnNoteBookPageChange(wxAuiNotebookEvent& event);
	void					OnSelectPaintType(wxCommandEvent& event);
	void					OnSliderChange(wxScrollEvent& event);
	void					OnEvent_ToUpdate(wxCommandEvent& event);

	void					OnSpinUpTextureParams(wxSpinEvent& event);
	void					OnSpinDownTextureParams(wxSpinEvent& event);

	void					OnTextParamsChanged(wxCommandEvent& event);

	void					OnButton(wxCommandEvent& event);

	wxPanel*				m_surface_panel;
	wxPanel*				m_terrain_panel;
	wxPanel*				m_texturereplacement_panel;

	// surface panel

	wxTextCtrl*								m_pMove[2];
	wxTextCtrl*								m_pScale[2];
	wxTextCtrl*								m_pRotate;
	wxTextCtrl*								m_pTess;
	wxCheckBox*								m_pCustTexCoords;

	wxCheckBox*								m_pNoCollide;
	wxCheckBox*								m_pNoSubdivision;

	CTextureView*							m_texview;
	wxTextCtrl*								m_texname;

	wxCheckBox*								m_pDrawSelectionMask;

	// terain panel

	wxSlider*								m_pSphereRadius;
	wxSlider*								m_pSpherePower;

	wxTextCtrl*								m_pRadiusValue;
	wxTextCtrl*								m_pPowerValue;

	VertexPainterType_e						m_nPaintType;

	wxChoice*								m_pPaintAxis;
	wxChoice*								m_pPaintLayer;
	wxRadioBox*								m_pPainterSel;

	wxCheckBox*								m_pDrawWiredTerrain;


	// texture replacement panel
	wxListBox*								m_pSelectedTextureList;

	DkList<IMaterial*>						m_pUsedSelectedMaterials;

	CMultiTypeComparator<float>				shiftValX;
	CMultiTypeComparator<float>				shiftValY;

	CMultiTypeComparator<float>				scaleValX;
	CMultiTypeComparator<float>				scaleValY;

	CMultiTypeComparator<float>				rotateVal;
	CMultiTypeComparator<IMaterial*>		materialVal;

	CMultiTypeComparator<EditableType_e>	editableType; // this is needed to setup tesselation

	CMultiTypeComparator<int>				tesselationVal;

	CMultiTypeComparator<bool>				nocollide;
	CMultiTypeComparator<bool>				nosubdivision;
	CMultiTypeComparator<bool>				custtexcoords;
};

#endif // EDITABLESURFACETERRAINPATCH_H