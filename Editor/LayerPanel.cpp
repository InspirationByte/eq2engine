//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Layer panel
//////////////////////////////////////////////////////////////////////////////////

#include "SelectionEditor.h"
#include "EntityProperties.h"
#include "LayerPanel.h"

BEGIN_EVENT_TABLE(CWorldLayerPanel, wxPanel)
	EVT_BUTTON(-1,CWorldLayerPanel::OnButton)
	
	EVT_DATAVIEW_SELECTION_CHANGED(-1,CWorldLayerPanel::OnSelectRow)
END_EVENT_TABLE()

enum LayerPropType_e
{
	LAYERPROP_ACTIVE = 0,
	LAYERPROP_VISIBLE,
	LAYERPROP_NAME,

	LAYERPROP_NUM
};

class CLayerPanelDataViewModel : public wxDataViewVirtualListModel
{
public:
    CLayerPanelDataViewModel() : wxDataViewVirtualListModel(0)
    {

    }

	
    virtual unsigned int GetCount() const
    {
		//Reset(g_pLevel->GetLayerCount());
		return g_pLevel->GetLayerCount();
	}
	

    virtual unsigned int GetColumnCount() const
    {
		return LAYERPROP_NUM;
	}

    // as reported by wxVariant
    virtual wxString GetColumnType( unsigned int col ) const
    {
        if (col != LAYERPROP_NAME)
            return wxT("bool");

        return wxT("string");
    }

	bool IsEnabledByRow(unsigned int row, unsigned int col) const
	{
		if(row == 1 && col == 0)
			return false;

		return true;
	}

    virtual void GetValueByRow( wxVariant &variant, unsigned row, unsigned col ) const
    {
		edworldlayer_t* pLayer = g_pLevel->GetLayer(row);
		if(!pLayer)
			return;

		if(col == LAYERPROP_ACTIVE)
		{
			variant = pLayer->active;
		}
		else if(col == LAYERPROP_VISIBLE)
		{
			variant = pLayer->visible;
		}
		else if(col == LAYERPROP_NAME)
		{
			variant = pLayer->layer_name.GetData();
		}
    }

	virtual bool GetAttrByRow( unsigned int row, unsigned int col, wxDataViewItemAttr &attr ) const
	{
		return false;
	}

    virtual bool SetValueByRow( const wxVariant &value, unsigned row, unsigned col )
    {
		if(col == LAYERPROP_ACTIVE)
		{
			g_pLevel->GetLayer(row)->active = value.GetBool();
		}
		else if(col == LAYERPROP_VISIBLE)
		{
			g_pLevel->GetLayer(row)->visible = value.GetBool();
		}
		else if(col == LAYERPROP_NAME)
		{
			g_pLevel->GetLayer(row)->layer_name = value.GetString().c_str();
		}

		UpdateAllWindows();

		return true;
    }
};

enum LayerPanelEvent_e
{
	LAYERPANEL_ADD		= 2001,
	LAYERPANEL_REMOVE,
};

CWorldLayerPanel::CWorldLayerPanel() : wxPanel(g_editormainframe, 0, 0, 200, 400)
{
	wxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

	m_pLayerList = new wxDataViewCtrl(this, -1);//, wxPoint(5,5), wxSize(290,200));
	mainSizer->Add(m_pLayerList, 1, wxEXPAND, 5);

	m_pModel = new CLayerPanelDataViewModel(); 

	m_pLayerList->AssociateModel( m_pModel );

	m_pLayerList->AppendToggleColumn("active",
                                        LAYERPROP_ACTIVE,
                                        wxDATAVIEW_CELL_ACTIVATABLE,
                                        wxCOL_WIDTH_AUTOSIZE);

	m_pLayerList->AppendToggleColumn("visible",
                                        LAYERPROP_VISIBLE,
                                        wxDATAVIEW_CELL_ACTIVATABLE,
                                        wxCOL_WIDTH_AUTOSIZE);

	m_pLayerList->AppendTextColumn("layer name",
                                        LAYERPROP_NAME,
                                        wxDATAVIEW_CELL_EDITABLE,
                                        wxCOL_WIDTH_AUTOSIZE);

	wxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);

	buttonSizer->Add(new wxButton(this, LAYERPANEL_ADD, DKLOC("TOKEN_ADD", "Add"),wxPoint(5,210), wxSize(70,25)), 0, wxALL, 5);
	buttonSizer->Add(new wxButton(this, LAYERPANEL_REMOVE, DKLOC("TOKEN_REMOVE", "Remove"),wxPoint(80,210), wxSize(70,25)), 0, wxALL, 5);

	mainSizer->Add(buttonSizer, 1, wxEXPAND, 0);

	m_nSelectedLayer = -1;

	SetSizer(mainSizer);
	Layout();

	UpdateLayers();
}

CWorldLayerPanel::~CWorldLayerPanel()
{
	//Msg("KILLING\n");
	//delete m_pLayerList->GetModel();
	//m_pLayerList->Destroy();
}

void CWorldLayerPanel::UpdateLayers()
{
	m_pLayerList->Refresh();
	m_pModel->Reset( g_pLevel->GetLayerCount() );

	g_editormainframe->GetEntityPropertiesPanel()->UpdateSelection();
}

void CWorldLayerPanel::OnButton(wxCommandEvent &event)
{
	if(event.GetId() == LAYERPANEL_ADD)
	{
		g_pLevel->CreateLayer();
	}
	
	if(event.GetId() == LAYERPANEL_REMOVE && m_nSelectedLayer != -1)
	{
		if(m_nSelectedLayer < 1)
		{
			wxMessageBox(wxString(DKLOC("TOKEN_DEFAULTLAYER_REMOVED", "Default Layer cannot be removed!")), wxString("Warning"), wxOK | wxCENTRE | wxICON_WARNING, g_editormainframe);
			return;
		}

		int result = wxMessageBox(wxString(varargs((char*)DKLOC("TOKEN_REMOVE_LAYER", "Remove selected layer?\nAll objects bound to that layer will be attached to %s"), g_pLevel->GetLayer(0)->layer_name.GetData())), wxString("Warning"), wxYES_NO | wxCENTRE | wxICON_WARNING, g_editormainframe);

		if(result == wxNO)
			return;

		g_pLevel->RemoveLayer( m_nSelectedLayer );
		m_nSelectedLayer = -1;
	}
	
	UpdateLayers();
}

void CWorldLayerPanel::OnSelectRow(wxDataViewEvent &event)
{
	//Msg("%d %d %d\n", (int)m_pLayerList->GetSelection().GetID(), event.GetColumn(), event.GetInt());
	m_nSelectedLayer = ((int)m_pLayerList->GetSelection().GetID())-1;
}