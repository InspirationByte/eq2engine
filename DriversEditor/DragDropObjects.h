//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: 'Drivers' level editor
//////////////////////////////////////////////////////////////////////////////////

#ifndef DRAGDROPOBJECTS_H
#define DRAGDROPOBJECTS_H

#include "wx/dnd.h"

enum EDragDropPointerType
{
	DRAGDROP_PTR_MATERIAL = 0,			// IMaterial
	DRAGDROP_PTR_COMPOSITE_MATERIAL,	// composite material and atlas, matAtlasElem_t
	DRAGDROP_PTR_OBJECTCONTAINER,		// object container
};

class IMaterial;
struct matAtlasElem_t;
struct objectcont_t;

class CPointerDataObject : public wxCustomDataObject
{
	friend class CPointerDropTarget;

public:
	CPointerDataObject() : wxCustomDataObject(wxDataFormat("eqEditPointer"))
	{}

	EDragDropPointerType	GetPointerType() const {return m_data.type;}
	void					SetPointerType(EDragDropPointerType type) {m_data.type = type;}

	void					SetMaterial(IMaterial* mat) {m_data.ptr = mat; m_data.type = DRAGDROP_PTR_MATERIAL;Store();}
	void					SetCompositeMaterial(matAtlasElem_t* mat) {m_data.ptr = mat; m_data.type = DRAGDROP_PTR_COMPOSITE_MATERIAL;Store();}
	void					SetModelContainer(objectcont_t* obj) {m_data.ptr = obj; m_data.type = DRAGDROP_PTR_OBJECTCONTAINER;Store();}

	IMaterial*				GetMaterial()			{return (IMaterial*)m_data.ptr;}
	matAtlasElem_t*			GetCompositeMaterial()	{return (matAtlasElem_t*)m_data.ptr;}
	objectcont_t*			GetModelContainer()		{return (objectcont_t*)m_data.ptr;}
protected:

	struct ptrdata_t
	{
		EDragDropPointerType	type;
		void*					ptr;
	};

	void					Store() {SetData(sizeof(ptrdata_t), &m_data);}
	void					Restore() {memcpy(&m_data, GetData(), sizeof(ptrdata_t));}

	ptrdata_t				m_data;
};


class CPointerDropTarget : public wxDropTarget
{
public:
    CPointerDropTarget()
	{
		SetDataObject(new CPointerDataObject());
	}

    virtual bool OnDropPoiner(wxCoord x, wxCoord y, void* ptr, EDragDropPointerType type) = 0;

    wxDragResult OnData(wxCoord x, wxCoord y, wxDragResult def)
	{
		if(!GetData())
			return wxDragError;

		CPointerDataObject* obj = (CPointerDataObject*)GetDataObject();
		obj->Restore();

		if(OnDropPoiner(x,y,obj->m_data.ptr, obj->m_data.type))
			return def;

		return wxDragNone;
	}
};


#endif // DRAGDROPOBJECTS_H