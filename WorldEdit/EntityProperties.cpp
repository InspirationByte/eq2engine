//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Entity properties dialog
//////////////////////////////////////////////////////////////////////////////////

#include "SelectionEditor.h"
#include "EntityProperties.h"
#include "EditableEntity.h"
#include "MultiComparator.h"

// entity-specific panels for modifying

/*
class CChoicePanel : public wxPanel
{
	CChoicePanel() : wxPanel(g_editormainframe->GetEntityPropertiesPanel(), 0,0, 200,500)
	{

	}
}
*/

enum epanel_e
{
	EPANEL_CHANGECLASSNAME = 3001,
	EPANEL_CHANGENAME,
	EPANEL_SELECTKEY,
	EPANEL_SETVALUE,

	EPANEL_BROWSE,

	EPANEL_ADDOUTPUT,
	EPANEL_REMOVEOUTPUT,
	EPANEL_SETOUTPUT,

	EPANEL_SELECTOUTPUT,
	EPANEL_SETTARGETENTITY,

	EPANEL_SETLAYER,
};


BEGIN_EVENT_TABLE(CEntityPropertiesPanel, wxDialog)
	EVT_TEXT_ENTER(EPANEL_CHANGECLASSNAME, CEntityPropertiesPanel::OnClassSelect)

	EVT_TEXT_ENTER(EPANEL_CHANGENAME, CEntityPropertiesPanel::OnSetObjectName)

	EVT_LIST_ITEM_SELECTED(EPANEL_SELECTKEY, CEntityPropertiesPanel::OnSelectParameter)
	EVT_TEXT_ENTER(EPANEL_SETVALUE, CEntityPropertiesPanel::OnValueSet)
	EVT_COMBOBOX(EPANEL_SETVALUE, CEntityPropertiesPanel::OnValueSet)
	EVT_BUTTON(EPANEL_BROWSE, CEntityPropertiesPanel::OnOpenModel)

	EVT_LIST_ITEM_SELECTED(EPANEL_SELECTOUTPUT, CEntityPropertiesPanel::OnSelectOutput)
	EVT_COMBOBOX(EPANEL_SETTARGETENTITY, CEntityPropertiesPanel::OnSetTargetEnt)

	EVT_COMBOBOX(EPANEL_SETLAYER, CEntityPropertiesPanel::OnSetLayer)

	EVT_BUTTON(-1, CEntityPropertiesPanel::OnOutputButtons)

	EVT_CLOSE(CEntityPropertiesPanel::OnClose)
END_EVENT_TABLE()

CEntityPropertiesPanel::CEntityPropertiesPanel() : wxDialog(g_editormainframe,-1, DKLOC("TOKEN_OBJECTPROPS", "Object properties"), wxPoint( -1,-1), wxSize(400,500), wxCAPTION | wxSYSTEM_MENU | wxCLOSE_BOX)
{
	Iconize( false );

	m_pPropertiesPanel = new wxPanel(this, -1, wxPoint(0,60), wxSize(400,500));

	wxAuiNotebook *notebook = new wxAuiNotebook(m_pPropertiesPanel, -1, wxDefaultPosition, wxSize(600, 450),wxAUI_NB_TOP |wxAUI_NB_TAB_SPLIT | wxAUI_NB_TAB_MOVE | wxAUI_NB_SCROLL_BUTTONS |wxNO_BORDER);

	wxPanel* pEntityKeyPanel = new wxPanel(m_pPropertiesPanel, -1, wxPoint(0,0), wxSize(400,500));
	wxPanel* pEntityOutputPanel = new wxPanel(m_pPropertiesPanel, -1, wxPoint(0, 0), wxSize(400, 500));
	wxPanel* pObjectPropertiesPanel = new wxPanel(m_pPropertiesPanel, -1, wxPoint(0, 0), wxSize(400, 500));

	notebook->AddPage(pEntityKeyPanel, DKLOC("TOKEN_KEYS", "Keys"), true);
	notebook->AddPage(pEntityOutputPanel, DKLOC("TOKEN_OUTPUTS", "Outputs"), false);
	notebook->AddPage(pObjectPropertiesPanel, DKLOC("TOKEN_OBJECTPROPS", "Object properties"), false);

	new wxStaticText(this, -1, DKLOC("TOKEN_CLASSNAME", "Class name"), wxPoint(5,5));
	m_pClassList = new wxComboBox(this, EPANEL_CHANGECLASSNAME, "", wxPoint(65,5), wxSize(320,25), 0, NULL, wxTE_PROCESS_ENTER);

	new wxStaticText(this, -1, DKLOC("TOKEN_OBJECTNAME", "Object name"), wxPoint(5,30));
	m_pNameList = new wxComboBox(this, EPANEL_CHANGENAME, "", wxPoint(65,30), wxSize(320,25), 0, NULL, wxTE_PROCESS_ENTER);

	//-----------
	// Key panel
	//-----------

	m_pKeyList = new wxListCtrl(pEntityKeyPanel, EPANEL_SELECTKEY, wxPoint(5,5), wxSize(390,280),wxLC_REPORT | wxLC_SINGLE_SEL);

	wxListItem col0;
	col0.SetId(0);
	col0.SetText(DKLOC("TOKEN_KEY", "Key"));
	col0.SetWidth(100);

	wxListItem col1;
	col1.SetId(1);
	col1.SetText(DKLOC("TOKEN_VALUE", "Value"));
	col1.SetWidth(400);

	m_pKeyList->InsertColumn(0, col0);
	m_pKeyList->InsertColumn(1, col1);

	//m_pKeyList->SetItemCount();

	new wxStaticText(pEntityKeyPanel, -1, DKLOC("TOKEN_KEY", "Key"), wxPoint(5,285));
	m_pKeyText = new wxTextCtrl(pEntityKeyPanel, -1, "", wxPoint(75,285), wxSize(310, 25));

	new wxStaticText(pEntityKeyPanel, -1, DKLOC("TOKEN_VALUE", "Value"), wxPoint(5,310));
	m_pValueText = new wxComboBox(pEntityKeyPanel, EPANEL_SETVALUE, "", wxPoint(75,310), wxSize(310, 25), 0,NULL, wxTE_PROCESS_ENTER);

	m_pBrowseModelButton = new wxButton(pEntityKeyPanel, EPANEL_BROWSE, DKLOC("TOKEN_BROWSE", "Browse"), wxPoint(75,340),wxSize(90,25));
	m_pBrowseModelButton->Hide();

	m_pPickColorButton = new wxButton(pEntityKeyPanel, EPANEL_BROWSE, DKLOC("TOKEN_PICKCOLOR", "Pick color"), wxPoint(75,340),wxSize(90,25));
	m_pPickColorButton->Hide();

	//-----------
	// Output panel
	//-----------

	m_pOutputList = new wxListCtrl(pEntityOutputPanel, EPANEL_SELECTOUTPUT, wxPoint(5,5), wxSize(390,200),wxLC_REPORT | wxLC_SINGLE_SEL);

	InitOutputPanel(pEntityOutputPanel);

	//-----------
	// Object properties panel
	//-----------

	new wxStaticText(pObjectPropertiesPanel, -1, DKLOC("TOKEN_LAYER", "Layer"), wxPoint(5,5));
	m_pLayer = new wxComboBox(pObjectPropertiesPanel, EPANEL_SETLAYER, "", wxPoint(5,25), wxSize(310, 25), 0,NULL, wxTE_PROCESS_ENTER);
}

void CEntityPropertiesPanel::InitOutputPanel(wxPanel* panel)
{
	wxListItem col0;
	col0.SetId(0);
	col0.SetText(DKLOC("TOKEN_ENTITY", "Entity"));
	col0.SetWidth(60);

	wxListItem col1;
	col1.SetId(1);
	col1.SetText(DKLOC("TOKEN_ENTOUTPUT", "Output"));
	col1.SetWidth(60);

	wxListItem col2;
	col2.SetId(2);
	col2.SetText(DKLOC("TOKEN_ENTINPUT", "Input"));
	col2.SetWidth(60);

	wxListItem col3;
	col3.SetId(3);
	col3.SetText(DKLOC("TOKEN_DELAY", "Delay"));
	col3.SetWidth(60);

	wxListItem col4;
	col4.SetId(4);
	col4.SetText(DKLOC("TOKEN_FIRETIMES", "Fire times"));
	col4.SetWidth(60);

	wxListItem col5;
	col5.SetId(5);
	col5.SetText(DKLOC("TOKEN_VALUE", "Value"));
	col5.SetWidth(60);

	m_pOutputList->InsertColumn(0, col5);
	m_pOutputList->InsertColumn(0, col4);
	m_pOutputList->InsertColumn(0, col3);
	m_pOutputList->InsertColumn(0, col2);
	m_pOutputList->InsertColumn(0, col1);
	m_pOutputList->InsertColumn(0, col0);

	new wxStaticText(panel, -1, DKLOC("TOKEN_ENTOUTPUT", "Output"), wxPoint(5,210));
	m_pOutput = new wxComboBox(panel, -1, "", wxPoint(55,210), wxSize(130, 25), 0,NULL);

	new wxStaticText(panel, -1, DKLOC("TOKEN_ENTITY", "Entity"), wxPoint(190,210));
	m_pTargetEntity = new wxComboBox(panel, EPANEL_SETTARGETENTITY, "", wxPoint(240,210), wxSize(130, 25), 0, NULL, wxTE_PROCESS_ENTER);

	new wxStaticText(panel, -1, DKLOC("TOKEN_DELAY", "Delay"), wxPoint(5,240));
	m_pDelay = new wxTextCtrl(panel, -1, "0", wxPoint(55,240), wxSize(130, 25));

	new wxStaticText(panel, -1, DKLOC("TOKEN_INPUT", "Input"), wxPoint(190,240));
	m_pTargetInput = new wxComboBox(panel, -1, "", wxPoint(240,240), wxSize(130, 25), 0, NULL);

	new wxStaticText(panel, -1, DKLOC("TOKEN_VALUE", "Value"), wxPoint(190,270));
	m_pOutValue = new wxTextCtrl(panel, -1, "", wxPoint(240,270), wxSize(130, 25));

	new wxStaticText(panel, -1, DKLOC("TOKEN_FIRETIMES", "Fire times"), wxPoint(5,270));
	m_pFireTimes = new wxTextCtrl(panel, -1, "-1", wxPoint(55,270), wxSize(130, 25));

	new wxButton(panel, EPANEL_ADDOUTPUT, DKLOC("TOKEN_ADDOUTPUT", "Add"), wxPoint(5,300),wxSize(75,25));
	new wxButton(panel, EPANEL_REMOVEOUTPUT, DKLOC("TOKEN_REMOVEOUTPUT", "Remove"), wxPoint(80,300),wxSize(75,25));
	new wxButton(panel, EPANEL_SETOUTPUT, DKLOC("TOKEN_SETOUTPUT", "Set"), wxPoint(190,300),wxSize(75,25));
}

bool ChangeClassName(CBaseEditableObject* pObject, void* userdata)
{
	if(pObject->GetType() != EDITABLE_ENTITY)
		return false;

	CEditableEntity* pEntity = (CEditableEntity*)pObject;

	char* pszClassName = (char*)userdata;
	pEntity->SetClassName(pszClassName);
	pEntity->UpdateParameters();

	return false;
}

bool ChangeName(CBaseEditableObject* pObject, void* userdata)
{
	char* pszName = (char*)userdata;
	pObject->SetName(pszName);

	return false;
}

void CEntityPropertiesPanel::OnSelectParameter(wxListEvent &event)
{
	if(m_CurrentProps.numElem() == 0)
		return;

	m_pValueText->Clear();

	// HACK: because the list is flipped do this stupid hack
	int index = event.GetIndex();

	if(index != -1)
	{
		entProp_t& prop = m_CurrentProps[index];
		edef_param_t* value_desc = prop.value_desc;
		
		if(value_desc)
		{
			if(value_desc->type == PARAM_TYPE_CHOICE)
			{
				for(int i = 0; i < value_desc->choice.choice_num; i++)
					m_pValueText->Append( value_desc->choice.choice_list[i].desc.c_str() );
			}
			else if(value_desc->type == PARAM_TYPE_BOOL)
			{
				m_pValueText->Append("0");
				m_pValueText->Append("1");
			}

			m_currentParamType = value_desc->type;
			m_nSelectedProp = index;
		}
		else
			m_currentParamType = PARAM_TYPE_INVALID;

		if(m_currentParamType == PARAM_TYPE_MODEL || m_currentParamType == PARAM_TYPE_TEXTURE || m_currentParamType == PARAM_TYPE_MATERIAL)
			m_pBrowseModelButton->Show();
		else
			m_pBrowseModelButton->Hide();

		if(m_currentParamType == PARAM_TYPE_COLOR3 || m_currentParamType == PARAM_TYPE_COLOR4)
			m_pPickColorButton->Show();
		else
			m_pPickColorButton->Hide();

		m_pKeyText->SetValue(prop.key.c_str());

		if(prop.value.Length() > 0 && value_desc && value_desc->type == PARAM_TYPE_CHOICE)
		{
			int idx = atoi(prop.value.GetData());
			m_pValueText->SetValue(value_desc->choice.choice_list[idx].desc.c_str());
		}
		else
			m_pValueText->SetValue(prop.value.GetData());
	}
	else
	{
		m_pKeyText->SetValue("");
		m_pValueText->SetValue("");
	}
}

bool SetEntityValue(CBaseEditableObject* pObject, void* userdata)
{
	if(pObject->GetType() != EDITABLE_ENTITY)
		return false;

	CEditableEntity* pEntity = (CEditableEntity*)pObject;

	kvkeybase_t* pair = (kvkeybase_t*)userdata;

	pEntity->SetKey(pair->name, KV_GetValueString(pair));
	pEntity->UpdateParameters();

	return false;
}

void CEntityPropertiesPanel::OnValueSet(wxCommandEvent &event)
{
	((CSelectionBaseTool*)g_pSelectionTools[0])->BackupSelectionForUndo();

	kvkeybase_t pair;
	pair.SetName(m_pKeyText->GetValue().c_str());
	pair.SetValueAt(m_pValueText->GetValue().c_str(), 0);

	if (m_currentParamType == PARAM_TYPE_CHOICE && m_pValueText->GetSelection() != -1)
	{
		edef_param_t* value_desc = m_CurrentProps[m_nSelectedProp].value_desc;
		int selValueIdx = m_pValueText->GetSelection();

		if(selValueIdx != -1)
			pair.SetValueAt(value_desc->choice.choice_list[selValueIdx].value.c_str(), 0);
	}


	wxString classText(m_pClassList->GetValue());

	((CSelectionBaseTool*)g_pSelectionTools[0])->DoForEachSelectedObjects(SetEntityValue, &pair, true);

	UpdateSelection();
	UpdateAllWindows();

	m_pClassList->SetValue(classText);
}

void CEntityPropertiesPanel::OnOpenModel(wxCommandEvent &event)
{
	if(m_currentParamType == PARAM_TYPE_COLOR3 || m_currentParamType == PARAM_TYPE_COLOR4)
	{
		wxColourDialog* pDialog = new wxColourDialog(this);
		pDialog->GetColourData().SetChooseFull(true);

		entProp_t& currentProp = m_CurrentProps[m_nSelectedProp];

		if(currentProp.value.Length())
		{
			if(m_currentParamType == PARAM_TYPE_COLOR4)
			{
				ColorRGBA gColor;
				UTIL_StringToColor4(currentProp.value.c_str() );

				pDialog->GetColourData().SetColour(wxColour(gColor.x*255.0f,gColor.y*255.0f,gColor.z*255.0f,gColor.w*255.0f));
			}
			else
			{
				ColorRGB gColor;
				UTIL_StringToColor3(currentProp.value.c_str() );

				pDialog->GetColourData().SetColour(wxColour(gColor.x*255.0f,gColor.y*255.0f,gColor.z*255.0f));
			}
		}

		if( pDialog->ShowModal() != 0x000013EC ) // 0x000013EC, but why?
			return;
	
		wxColourData& colorData = pDialog->GetColourData();
		wxColour& col = colorData.GetColour();

		delete pDialog;

		ColorRGBA colorval(col.Red(), col.Green(), col.Blue(), col.Alpha());
		colorval *= (1.0f / 255.0f);

		

		((CSelectionBaseTool*)g_pSelectionTools[0])->BackupSelectionForUndo();

		kvkeybase_t pair;
		pair.SetName(m_pKeyText->GetValue().c_str());

		if(m_currentParamType == PARAM_TYPE_COLOR4)
			pair.SetValueAt(colorval, 0);
		else
			pair.SetValueAt(colorval.xyz(), 0);

		m_pValueText->SetValue(wxString(KV_GetValueString(&pair)));

		wxString classText = m_pClassList->GetValue();

		((CSelectionBaseTool*)g_pSelectionTools[0])->DoForEachSelectedObjects(SetEntityValue, &pair, true);

		UpdateSelection();
		UpdateAllWindows();

		m_pClassList->SetValue(classText);

		return;
	}

	wxFileDialog* file;
	
	if(m_currentParamType == PARAM_TYPE_TEXTURE)
		file = new wxFileDialog(NULL, "Open DDS texture", varargs("%s/materials", g_fileSystem->GetCurrentGameDirectory()), "*.dds", "Direct Draw Surface files (*.dds)|*.dds;", wxFD_FILE_MUST_EXIST | wxFD_OPEN);
	if(m_currentParamType == PARAM_TYPE_MATERIAL)
		file = new wxFileDialog(NULL, "Open material file", varargs("%s/materials", g_fileSystem->GetCurrentGameDirectory()), "*.mat", "Equilibrium Material file (*.mat)|*.mat;", wxFD_FILE_MUST_EXIST | wxFD_OPEN);
	else if(m_currentParamType == PARAM_TYPE_MODEL)
		file = new wxFileDialog(NULL, "Open EGF model", varargs("%s/models", g_fileSystem->GetCurrentGameDirectory()), "*.egf", "Equilibrium Graphics Files (*.egf)|*.egf;", wxFD_FILE_MUST_EXIST | wxFD_OPEN);

	if(file && (file->ShowModal() == wxID_OK))
	{
		EqString path(file->GetPath().c_str().AsChar());

		if(m_currentParamType == PARAM_TYPE_TEXTURE || m_currentParamType == PARAM_TYPE_MATERIAL)
			path = path.Path_Strip_Ext();

		FixSlashes((char*)path.GetData());
		char* sub = strstr((char*)path.GetData(), varargs("%s/", g_fileSystem->GetCurrentGameDirectory()));

		char* pszPath = (char*)path.c_str();
	
		int diff = (sub - pszPath);

		pszPath += diff + strlen(g_fileSystem->GetCurrentGameDirectory()) + 1;

		if(m_currentParamType == PARAM_TYPE_TEXTURE || m_currentParamType == PARAM_TYPE_MATERIAL)
			pszPath += strlen(materials->GetMaterialPath());

		((CSelectionBaseTool*)g_pSelectionTools[0])->BackupSelectionForUndo();

		kvkeybase_t pair;
		pair.SetName(m_pKeyText->GetValue().c_str());
		pair.SetValueAt(pszPath, 0);

		m_pValueText->SetValue(pszPath);

		wxString classText(m_pClassList->GetValue());

		((CSelectionBaseTool*)g_pSelectionTools[0])->DoForEachSelectedObjects(SetEntityValue, &pair, true);

		UpdateSelection();
		UpdateAllWindows();

		m_pClassList->SetValue(classText);

		delete file;
	}
}

void CEntityPropertiesPanel::OnClassSelect(wxCommandEvent &event)
{
	((CSelectionBaseTool*)g_pSelectionTools[0])->BackupSelectionForUndo();

	wxString classText(m_pClassList->GetValue());

	((CSelectionBaseTool*)g_pSelectionTools[0])->DoForEachSelectedObjects(ChangeClassName, (void*)classText.c_str().AsChar(), true);

	UpdateSelection();
	UpdateAllWindows();

	m_pClassList->SetValue(classText);
}

void CEntityPropertiesPanel::OnSetObjectName(wxCommandEvent &event)
{
	((CSelectionBaseTool*)g_pSelectionTools[0])->BackupSelectionForUndo();

	wxString nameText(m_pNameList->GetValue());

	((CSelectionBaseTool*)g_pSelectionTools[0])->DoForEachSelectedObjects(ChangeName, (void*)nameText.c_str().AsChar(), true);

	UpdateSelection();
	UpdateAllWindows();

	m_pNameList->SetValue(nameText);
}

//---------------------------
// entity properties
//---------------------------

struct propertycheckdata_t
{
	EqString			classname;
	bool				class_differs;

	DkList<entProp_t>	params;

	EqString			name;
	bool				name_differs;

};

void AddEntPropToList(entProp_t &prop, DkList<entProp_t> &list)
{
	for(int i = 0; i < list.numElem(); i++)
	{
		if(!list[i].key.CompareCaseIns(prop.key.GetData()))
		{
			if(prop.value.Length() == 0)
				return;

			// if zero length, update value (only when definition is available)
			if(list[i].value.Length() == 0 && list[i].value_desc)
			{
				list[i].value = prop.value.GetData();
			}
			else
			{
				int cm = list[i].value.CompareCaseIns(prop.value.GetData());

				if(cm != 0)
				{
					// the values is different
					list[i].value = "<differs>";
				}
			}

			return;
		}
	}

	// add to list
	list.append(prop);
}

bool CollectEntityParameters(CBaseEditableObject* pObject, void* userdata)
{
	propertycheckdata_t *pData = (propertycheckdata_t*)userdata;

	// copy name
	if(pData->name.Length() > 0)
	{
		if(pData->name.CompareCaseIns(pObject->GetName()))
			pData->name_differs = true;
	}

	pData->name = pObject->GetName();

	// so, other object don't have a
	if(pObject->GetType() != EDITABLE_ENTITY)
		return false;

	CEditableEntity* pEntity = (CEditableEntity*)pObject;

	// get entity params
	DkList<edkeyvalue_t>* pPairs =  pEntity->GetPairs();

	// get definition
	edef_entity_t* pDefinition = pEntity->GetDefinition();

	// copy class name
	if(pData->classname.Length() > 0)
	{
		if(pData->classname.CompareCaseIns(pEntity->GetClassname()))
			pData->class_differs = true;
	}

	pData->classname = pEntity->GetClassname();

	
	if(pDefinition)
	{
		DkList<edef_param_t*> paramList;
		EDef_GetFullParamList(paramList, pDefinition);
		
		for(int i = 0; i < paramList.numElem(); i++)
		{
			entProp_t prop;
			prop.key = paramList[i]->key;
			prop.value_desc = paramList[i];
			prop.value = "";
			AddEntPropToList(prop, pData->params);
		}
	}
	

	for(int i = 0; i < pPairs->numElem(); i++)
	{
		entProp_t prop;
		prop.key = pPairs->ptr()[i].name;
		prop.value_desc = EDef_FindParameter(pPairs->ptr()[i].name, pDefinition);
		prop.value = pPairs->ptr()[i].value;

		AddEntPropToList(prop, pData->params);
	}

	return false;
}

//---------------------------
// entity outputs/inputs
//---------------------------

struct outputcheckdata_t
{
	DkList<edef_output_t>	outputs;			// output list for selection
	DkList<OutputData_t*>	ent_output_data;	// entity output datas used by selection
};

struct targetinputdata_t
{
	char*					pszTargetName;
	DkList<edef_input_t>	inputs;
};

bool TargetInputList(CBaseEditableObject* pObject, void* userdata)
{
	if(pObject->GetType() != EDITABLE_ENTITY)
		return false;

	CEditableEntity* pEntity = (CEditableEntity*)pObject;

	if(!pEntity->GetName())
		return false;

	targetinputdata_t* pData = (targetinputdata_t*)userdata;

	if(!stricmp(pEntity->GetName(), pData->pszTargetName))
	{
		edef_entity_t* pEntDef = pEntity->GetDefinition();

		if(!pEntDef)
			return false;

		DkList<edef_input_t*> inputs;

		EDef_GetFullInputList(inputs, pEntDef);

		for(int i = 0; i < inputs.numElem(); i++)
		{
			bool bAdd = true;

			// don't add same elements
			for(int j = 0; j < pData->inputs.numElem(); j++)
			{
				if(!inputs[i]->name.CompareCaseIns(pData->inputs[j].name))
					bAdd = false;
			}

			if(bAdd)
				pData->inputs.append(*inputs[i]);
		}
	}

	return false;
}

bool CollectEntityInputOutputData(CBaseEditableObject* pObject, void* userdata)
{
	if(pObject->GetType() != EDITABLE_ENTITY)
		return false;

	CEditableEntity* pEntity = (CEditableEntity*)pObject;

	outputcheckdata_t* pOutData = (outputcheckdata_t*)userdata;

	for(int i = 0; i < pEntity->GetOutputs()->numElem(); i++)
	{
		pOutData->ent_output_data.append(&pEntity->GetOutputs()->ptr()[i]);
	}

	edef_entity_t* pEntDef = pEntity->GetDefinition();
	if(!pEntDef)
		return false;

	DkList<edef_output_t*> outputs;

	EDef_GetFullOutputList(outputs, pEntDef);

	for(int i = 0; i < outputs.numElem(); i++)
		pOutData->outputs.append(*outputs[i]);

	return false;
}

void AddUniqueToStringList(const char* pszName, DkList<const char*> &list)
{
	for(int i = 0; i < list.numElem(); i++)
	{
		if(!stricmp(list[i], pszName))
			return;
	}

	list.append(pszName);
}

void CEntityPropertiesPanel::OnSetTargetEnt(wxCommandEvent &event)
{
	if(m_pTargetEntity->GetValue().length() == 0)
		return;

	//if(event.GetInt() != 1)
	m_pTargetInput->Clear();

	//Msg("Input updates\n");

	char str[256];
	strcpy(str, m_pTargetEntity->GetValue().c_str());

	targetinputdata_t data;
	data.pszTargetName = str;

	for(int i = 0; i < g_pLevel->GetEditableCount(); i++)
		TargetInputList(g_pLevel->GetEditable(i), &data);

	DkList<const char*> list;

	for(int i = 0; i < data.inputs.numElem(); i++)
		AddUniqueToStringList(data.inputs[i].name.c_str(), list);

	for(int i = 0; i < list.numElem(); i++)
		m_pTargetInput->AppendString(list[i]);
}

bool AddOutputToSelection(CBaseEditableObject* pObject, void* userdata)
{
	if(pObject->GetType() != EDITABLE_ENTITY)
		return false;

	CEditableEntity* pEntity = (CEditableEntity*)pObject;

	OutputData_t newoutput = *((OutputData_t*)userdata);

	pEntity->GetOutputs()->append(newoutput);

	return false;
}

bool RemoveSelectedOutput(CBaseEditableObject* pObject, void* userdata)
{
	if(pObject->GetType() != EDITABLE_ENTITY)
		return false;

	CEditableEntity* pEntity = (CEditableEntity*)pObject;

	for(int i = 0; i < pEntity->GetOutputs()->numElem(); i++)
	{
		if((&pEntity->GetOutputs()->ptr()[i]) == (OutputData_t*)userdata)
		{
			pEntity->GetOutputs()->removeIndex(i);
			return false;
		}
	}

	return false;
}

void CEntityPropertiesPanel::OnOutputButtons(wxCommandEvent &event)
{
	long itemIndex = -1;
 
	for (;;) 
	{
		long tmpitemIndex = m_pOutputList->GetNextItem(itemIndex, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (tmpitemIndex == -1)
			break;

		itemIndex = tmpitemIndex;
	}

	switch(event.GetId())
	{
		case EPANEL_ADDOUTPUT:
		{
			OutputData_t newoutput;

			newoutput.fDelay = atof(m_pDelay->GetValue().c_str());
			newoutput.nFireTimes = atoi(m_pFireTimes->GetValue().c_str());
			newoutput.szOutputName = m_pOutput->GetValue().c_str();
			newoutput.szOutputTarget = m_pTargetEntity->GetValue().c_str();
			newoutput.szTargetInput = m_pTargetInput->GetValue().c_str();
			newoutput.szOutputValue = m_pOutValue->GetValue().c_str();

			// add output to each selected objects
			((CSelectionBaseTool*)g_pSelectionTools[0])->DoForEachSelectedObjects(AddOutputToSelection, &newoutput);

			UpdateSelection();
			break;
		}
		case EPANEL_REMOVEOUTPUT:
		{
			if(itemIndex < 0)
				return;

			((CSelectionBaseTool*)g_pSelectionTools[0])->DoForEachSelectedObjects(RemoveSelectedOutput,m_CurrentOutputs[itemIndex]);

			UpdateSelection();

			return;
		}
		case EPANEL_SETOUTPUT:
		{
			if(itemIndex < 0)
				return;

			OutputData_t* output = m_CurrentOutputs[itemIndex];

			output->fDelay = atof(m_pDelay->GetValue().c_str());
			output->nFireTimes = atoi(m_pFireTimes->GetValue().c_str());
			output->szOutputName = m_pOutput->GetValue().c_str();
			output->szOutputTarget = m_pTargetEntity->GetValue().c_str();
			output->szTargetInput = m_pTargetInput->GetValue().c_str();
			output->szOutputValue = m_pOutValue->GetValue().c_str();
			break;
		}
		default:
			return;
	}

	if(itemIndex < 0)
	{
		m_pOutput->Clear();
		m_pTargetInput->Clear();
		m_pDelay->SetValue(wxString("0.0"));
		m_pFireTimes->SetValue(wxString("-1"));
		m_pOutValue->SetValue(wxString(""));
		m_pTargetEntity->Clear();

		return;
	}

	//m_pOutput->Clear();
	m_pTargetInput->Clear();
	m_pDelay->SetValue(wxString("0.0"));
	m_pFireTimes->SetValue(wxString("-1"));
	m_pOutValue->SetValue(wxString(""));
	//m_pTargetEntity->Clear();

	UpdateSelection();

	m_pOutputList->SetItemState(itemIndex, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);

	if (itemIndex != -1)
	{
		OutputData_t* output = m_CurrentOutputs[itemIndex];

		m_pOutput->SetValue(output->szOutputName.c_str());
		m_pTargetEntity->SetValue(output->szOutputTarget.c_str());
		m_pDelay->SetValue(varargs("%f", output->fDelay));
		m_pFireTimes->SetValue(varargs("%d", output->nFireTimes));
		m_pOutValue->SetValue(output->szOutputValue.c_str());
	}

	//event.SetInt(1);

	OnSetTargetEnt(event);

	m_pTargetInput->SetValue(wxString(m_CurrentOutputs[itemIndex]->szTargetInput.GetData()));
}

void CEntityPropertiesPanel::OnSelectOutput(wxListEvent &event)
{
	int id = event.GetIndex();
	OutputData_t* output = m_CurrentOutputs[id];

	//m_pOutput->Clear();
	m_pTargetInput->Clear();
	m_pDelay->SetValue(wxString("0.0"));
	m_pFireTimes->SetValue(wxString("-1"));
	//m_pTargetEntity->Clear();

	m_pOutput->ChangeValue(output->szOutputName.c_str());
	m_pTargetEntity->ChangeValue(output->szOutputTarget.c_str());
	m_pDelay->ChangeValue(varargs("%f", output->fDelay));
	m_pFireTimes->ChangeValue(varargs("%d", output->nFireTimes));
	m_pOutValue->ChangeValue(output->szOutputValue.c_str());

	OnSetTargetEnt(event);

	m_pTargetInput->ChangeValue(wxString(m_CurrentOutputs[id]->szTargetInput.GetData()));
}

bool LayerCompare(CBaseEditableObject* pObject, void* userdata)
{
	CMultiTypeComparator<int>* layerTypeComp = (CMultiTypeComparator<int>*)userdata;

	layerTypeComp->CompareValue(pObject->GetLayerID());

	return false;
}

void CEntityPropertiesPanel::UpdateSelection()
{
	// layers
	m_pLayer->Clear();

	if(((CSelectionBaseTool*)g_pSelectionTools[0])->GetSelectedObjects()->numElem())
	{
		for(int i = 0; i < g_pLevel->GetLayerCount(); i++)
			m_pLayer->AppendString(wxString(g_pLevel->GetLayer(i)->layer_name.GetData()));

		CMultiTypeComparator<int> layerTypeComp;

		((CSelectionBaseTool*)g_pSelectionTools[0])->DoForEachSelectedObjects(LayerCompare, &layerTypeComp);

		if(!layerTypeComp.IsDiffers())
			m_pLayer->SetValue(g_pLevel->GetLayer(layerTypeComp.GetValue())->layer_name.GetData());
	}

	// keys
	m_CurrentProps.clear();

	m_pKeyList->DeleteAllItems();

	m_pClassList->Clear();
	m_pNameList->Clear();

	for(int i = 0; i < g_defs.numElem(); i++)
	{
		if(g_defs[i]->showinlist)
			m_pClassList->AppendString( wxString(g_defs[i]->classname.GetData()) );
	}

	propertycheckdata_t entData;
	entData.class_differs = false;
	entData.name_differs = false;
	
	((CSelectionBaseTool*)g_pSelectionTools[0])->DoForEachSelectedObjects(CollectEntityParameters, &entData);

	for(int i = 0; i < entData.params.numElem(); i++)
	{
		entProp_t& prop = entData.params[i];
		m_CurrentProps.append(prop);

		long item_id = -1;

		if(prop.value_desc && prop.value_desc->description.c_str()[0] != '#')
			item_id = m_pKeyList->InsertItem(m_pKeyList->GetItemCount(), wxString(prop.value_desc->description.GetData()));
		else
			item_id = m_pKeyList->InsertItem(m_pKeyList->GetItemCount(), wxString(prop.key.GetData()));

		if(prop.value.Length() > 0 && prop.value_desc && prop.value_desc->type == PARAM_TYPE_CHOICE)
		{
			int idx = atoi(prop.value.GetData());

			m_pKeyList->SetItem(item_id, 1, prop.value_desc->choice.choice_list[idx].desc.c_str());
		}
		else
			m_pKeyList->SetItem(item_id, 1, prop.value.c_str());
	}

	// 
	m_pClassList->SetValue("");

	if(!entData.class_differs)
		m_pClassList->SetValue(entData.classname.c_str());
	else
		m_pClassList->SetValue("<differs>");

	// Update name list
	DkList<const char*> name_strings;

	for(int i = 0; i < g_pLevel->GetEditableCount(); i++)
	{
		CBaseEditableObject* pEnt = g_pLevel->GetEditable(i);

		char* pszTarget = (char*)pEnt->GetName();

		if(strlen(pszTarget) > 0)
			AddUniqueToStringList( pszTarget, name_strings );
	}

	for(int i = 0; i < name_strings.numElem(); i++)
		m_pNameList->AppendString( wxString(name_strings[i]) );

	if(!entData.name_differs)
		m_pNameList->SetValue(wxString(entData.name.GetData()));
	else
		m_pNameList->SetValue("<differs>");

	// update output list

	m_CurrentOutputs.clear();

	m_pOutputList->DeleteAllItems();
	m_pOutput->Clear();
	m_pTargetInput->Clear();
	m_pDelay->SetValue(wxString("0.0"));
	m_pFireTimes->SetValue(wxString("-1"));
	m_pOutValue->SetValue(wxString(""));

	m_pTargetEntity->Clear();

	DkList<const char*> target_strings;

	for(int i = 0; i < g_pLevel->GetEditableCount(); i++)
	{
		if(g_pLevel->GetEditable(i)->GetType() != EDITABLE_ENTITY)
			continue;

		CEditableEntity* pEnt = (CEditableEntity*)g_pLevel->GetEditable(i);

		char* pszTarget = (char*)pEnt->GetName();

		if(strlen(pszTarget) > 0)
			AddUniqueToStringList( pszTarget, target_strings );
	}

	for(int i = 0; i < target_strings.numElem(); i++)
		m_pTargetEntity->AppendString( wxString(target_strings[i]) );

	outputcheckdata_t outData;

	// collect datas from them
	((CSelectionBaseTool*)g_pSelectionTools[0])->DoForEachSelectedObjects(CollectEntityInputOutputData, &outData);

	if( !entData.class_differs )
	{
		target_strings.clear();

		for(int i = 0; i < outData.outputs.numElem(); i++)
			AddUniqueToStringList(outData.outputs[i].name.c_str(), target_strings);

		for(int i = 0; i < target_strings.numElem(); i++)
			m_pOutput->AppendString(wxString(target_strings[i]));

		m_CurrentOutputs.append(outData.ent_output_data);

		for(int i = 0; i < outData.ent_output_data.numElem(); i++)
		{
			OutputData_t* output = outData.ent_output_data[i];

			long item_id = m_pOutputList->InsertItem(m_pOutputList->GetItemCount(), output->szOutputTarget.c_str());

			// target, output, input, delay, fires, value

			m_pOutputList->SetItemData(item_id, i);

			m_pOutputList->SetItem(item_id, 1, output->szOutputName.c_str());
			m_pOutputList->SetItem(item_id, 2, output->szTargetInput.c_str());
			m_pOutputList->SetItem(item_id, 3, varargs("%f", output->fDelay));
			m_pOutputList->SetItem(item_id, 4, varargs("%d", output->nFireTimes));
			m_pOutputList->SetItem(item_id, 5, output->szOutputValue.c_str());
		}
	}
}

void CEntityPropertiesPanel::OnClose(wxCloseEvent &event)
{
	Hide();
}

bool ChangeLayersOfEdiatbles(CBaseEditableObject* pObject, void* userdata)
{
	int nLayerID = *(int*)userdata;

	pObject->SetLayer(nLayerID);

	return false;
}

void CEntityPropertiesPanel::OnSetLayer(wxCommandEvent &event)
{
	int nLayerID = g_pLevel->FindLayer(m_pLayer->GetValue().c_str());

	if(nLayerID == -1)
		return;

	// collect datas from them
	((CSelectionBaseTool*)g_pSelectionTools[0])->DoForEachSelectedObjects(ChangeLayersOfEdiatbles, &nLayerID);
}