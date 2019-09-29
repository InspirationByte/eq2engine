//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Entity list
//////////////////////////////////////////////////////////////////////////////////

#include "EntityList.h"
#include "EditableEntity.h"
#include "GroupEditable.h"
#include "SelectionEditor.h"

#define BUTTON_SELECT		0
#define BUTTON_SEARCH		1

#define BUTTON_RENAME		2	// groups

//-----------------------------------------------------
// Entity list
//-----------------------------------------------------

BEGIN_EVENT_TABLE(CEntityList, wxDialog)
	//EVT_BUTTON(-1, CEntityList::OnButtonClick)
	EVT_LISTBOX(BUTTON_SELECT, CEntityList::OnButtonClick)
	EVT_CLOSE(CEntityList::OnClose)
	EVT_BUTTON(-1, CEntityList::OnButtonClick)
END_EVENT_TABLE()

CEntityList::CEntityList() : wxDialog(g_editormainframe, -1, DKLOC("TOKEN_ENTLIST", L"Entity list"), wxPoint( -1, -1), wxSize(310,630), wxCAPTION | wxSYSTEM_MENU | wxCLOSE_BOX)
{
	Iconize( false );

	m_pEntList = new wxListBox(this,BUTTON_SELECT,wxPoint(5,5),wxSize(290,450),0,NULL,wxLB_EXTENDED);
	//new wxButton(this, BUTTON_SELECT, DKLOC("TOKEN_ENTLSELECT", "Select"), wxPoint(5,455),wxSize(65,25));
	//new wxButton(this, BUTTON_SELECTALL, DKLOC("TOKEN_ENTLSELECTALL", "Select all"), wxPoint(75,455),wxSize(65,25));

	new wxStaticText(this, -1, DKLOC("TOKEN_CLASSNAME", L"Classname"), wxPoint(5,460));
	m_pSearchClass = new wxTextCtrl(this, -1, "", wxPoint(75,460), wxSize(210, 25));

	new wxStaticText(this, -1, DKLOC("TOKEN_KEY", L"Key"), wxPoint(5,490));
	m_pSearchKey = new wxTextCtrl(this, -1, "", wxPoint(75,490), wxSize(210, 25));

	new wxStaticText(this, -1, DKLOC("TOKEN_VALUE", L"Value"), wxPoint(5,520));
	m_pSearchValue = new wxTextCtrl(this, -1, "", wxPoint(75,520), wxSize(210, 25));

	new wxButton(this, BUTTON_SEARCH, DKLOC("TOKEN_SEARCH", L"Search"), wxPoint(5,550),wxSize(65,25));
}

void CEntityList::RefreshObjectList()
{
	m_pEntList->Clear();

	for(int i = 0; i < g_pLevel->GetEditableCount(); i++)
	{
		CBaseEditableObject* pObject = g_pLevel->GetEditable(i);
		if(pObject->GetType() == EDITABLE_ENTITY)
		{
			CEditableEntity* pEnt = (CEditableEntity*)pObject;

			if(m_pSearchClass->GetValue().length() > 0)
			{
				if(!strstr(pEnt->GetClassname(), m_pSearchClass->GetValue().c_str()))
					continue;
			}

			if(m_pSearchKey->GetValue().length() > 0)
			{
				char* pszValue = pEnt->GetKeyValue( (char*)m_pSearchKey->GetValue().c_str().AsChar() );
				if(pszValue == NULL)
					continue;

				// faster search method
				if(m_pSearchValue->GetValue().length() > 0)
				{
					if(!strstr(m_pSearchValue->GetValue().c_str(), pszValue))
						continue;
				}
			}
			else if(m_pSearchValue->GetValue().length() > 0)
			{
				bool bFound = false;

				// slower search method
				for(int j = 0; j < pEnt->GetPairs()->numElem(); j++)
				{
					if(!stricmp(pEnt->GetPairs()->ptr()[j].value, m_pSearchValue->GetValue().c_str()))
						bFound = true;
				}

				if(!bFound)
					continue;
			}

			bool shouldAddObjectName = strlen(pEnt->GetName()) > 0;
			if(!shouldAddObjectName)
				m_pEntList->Append(varargs("%s", pEnt->GetClassname()), pEnt);
			else
				m_pEntList->Append(varargs("%s  -  (%s)", pEnt->GetClassname(), pEnt->GetName()), pEnt);
		}
	}
}

void CEntityList::OnClose(wxCloseEvent& event)
{
	Hide();
}

void CEntityList::OnButtonClick(wxCommandEvent& event)
{
	if(event.GetId() == BUTTON_SEARCH)
	{
		RefreshObjectList();
		return;
	}

	wxArrayInt selected;
	m_pEntList->GetSelections(selected);

	DkList<CBaseEditableObject*> objects;

	for(size_t i = 0; i < selected.size(); i++)
	{
		CBaseEditableObject* selectedObject = (CBaseEditableObject*)m_pEntList->GetClientData(selected[i]);
		objects.append(selectedObject);
	}

	((CSelectionBaseTool*)g_pSelectionTools[0])->CancelSelection();
	((CSelectionBaseTool*)g_pSelectionTools[0])->SetSelectionState(SELECTION_PREPARATION);
	((CSelectionBaseTool*)g_pSelectionTools[0])->SelectObjects(true, objects.ptr(), objects.numElem());
}

//-----------------------------------------------------
// Group list
//-----------------------------------------------------

BEGIN_EVENT_TABLE(CGroupList, wxDialog)
	//EVT_BUTTON(-1, CEntityList::OnButtonClick)
	EVT_LISTBOX(BUTTON_SELECT, CGroupList::OnButtonClick)
	EVT_CLOSE(CGroupList::OnClose)
END_EVENT_TABLE()

CGroupList::CGroupList() : wxDialog(g_editormainframe, -1, DKLOC("TOKEN_GROUPLIST", L"Groups"), wxPoint(-1, -1), wxSize(310,510))
{
	m_pGroupList = new wxListBox(this,BUTTON_SELECT,wxPoint(5,5),wxSize(290,450),0,NULL,wxLB_EXTENDED);
	//new wxButton(this, BUTTON_RENAME, DKLOC("TOKEN_RENAME", "Rename"), wxPoint(5,455),wxSize(65,25));
	//new wxButton(this, BUTTON_SELECTALL, DKLOC("TOKEN_ENTLSELECTALL", "Select all"), wxPoint(75,455),wxSize(65,25));
}

void CGroupList::RefreshObjectList()
{
	m_pGroupList->Clear();

	for(int i = 0; i < g_pLevel->GetEditableCount(); i++)
	{
		CBaseEditableObject* pObject = g_pLevel->GetEditable(i);
		if(pObject->GetType() == EDITABLE_GROUP)
		{
			CEditableGroup* pGrp = (CEditableGroup*)pObject;

			const char* pszObjName = pGrp->GetName();

			if(strlen(pszObjName) == 0)
				pGrp->SetName(varargs("UnnamedGroup_%d\n", pGrp->m_id));

			m_pGroupList->Append(varargs("%s", pGrp->GetName()), pGrp);
		}
	}
}

void CGroupList::OnClose(wxCloseEvent& event)
{
	Hide();
}

void CGroupList::OnButtonClick(wxCommandEvent& event)
{
	wxArrayInt selected;
	m_pGroupList->GetSelections(selected);

	DkList<CBaseEditableObject*> objects;

	for (size_t i = 0; i < selected.size(); i++)
	{
		CBaseEditableObject* selectedObject = (CBaseEditableObject*)m_pGroupList->GetClientData(selected[i]);
		objects.append(selectedObject);
	}

	((CSelectionBaseTool*)g_pSelectionTools[0])->CancelSelection();
	((CSelectionBaseTool*)g_pSelectionTools[0])->SetSelectionState(SELECTION_PREPARATION);
	((CSelectionBaseTool*)g_pSelectionTools[0])->SelectObjects(true, objects.ptr(), objects.numElem());
}