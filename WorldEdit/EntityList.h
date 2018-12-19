//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Entity list dialog
//////////////////////////////////////////////////////////////////////////////////

#ifndef OBJECTLIST_H
#define OBJECTLIST_H

#include "wx_inc.h"
#include "EditorHeader.h"

class CEntityList : public wxDialog
{
public:
	CEntityList();

	void		RefreshObjectList();
	void		OnButtonClick(wxCommandEvent& event);
	void		OnClose(wxCloseEvent& event);

protected:

	DECLARE_EVENT_TABLE();

	wxListBox*					m_pEntList;

	wxTextCtrl*					m_pSearchClass;
	wxTextCtrl*					m_pSearchKey;
	wxTextCtrl*					m_pSearchValue;

	DkList<CBaseEditableObject*> m_listObjects;
};

class CGroupList : public wxDialog
{
public:
	CGroupList();

	void		RefreshObjectList();
	void		OnButtonClick(wxCommandEvent& event);
	void		OnClose(wxCloseEvent& event);

protected:

	DECLARE_EVENT_TABLE();

	wxListBox*					m_pGroupList;

	DkList<CBaseEditableObject*> m_listObjects;
};

#endif // OBJECTLIST_H