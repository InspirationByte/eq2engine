//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Editor level
//////////////////////////////////////////////////////////////////////////////////

#ifndef EDITORLEVEL_H
#define EDITORLEVEL_h

#include "eqlevel.h"
#include "materialsystem/IMaterialSystem.h"
#include "RenderList.h"
#include "IVirtualStream.h"
#include "UndoObserver.h"
#include "IViewRenderer.h"
#include "grid.h"

class CEditorViewRender;
class CBaseEditableObject;
class CBaseRenderableObject;
struct EqBrushWinding_t;

//---------------------------------------------
// Object selection picker flags
//---------------------------------------------
enum NearestRayObjectFlag_e
{
	NR_FLAG_BRUSHES			= (1 << 0),
	NR_FLAG_SURFACEMODS		= (1 << 1),
	NR_FLAG_ENTITIES		= (1 << 2),
	NR_FLAG_GROUP			= (1 << 3),
	NR_FLAG_INCLUDEGROUPPED	= (1 << 4),		// include hidden objects

	NR_FLAG_TRACEALL		= ( NR_FLAG_BRUSHES | NR_FLAG_SURFACEMODS | NR_FLAG_ENTITIES | NR_FLAG_GROUP )
};

//---------------------------------------------
// Level layer structure
//---------------------------------------------

struct edworldlayer_t
{
	DkList<int>						object_ids;		// objects of this layer
	EqString						layer_name;		// layer name

	bool							visible;
	bool							active;
};

struct drawlist_struct_t
{
	CEditorViewRender*	pViewRender;
	bool				bWireframe;
};

//---------------------------------------------
// Clipboard action mode
//---------------------------------------------
enum ClipboardMode_e
{
	CLIPBOARD_COPY = 0,
	CLIPBOARD_PASTE,
	CLIPBOARD_CLEAR,
};

//---------------------------------------------
// Editor level
// Also used as render list, so it can be used
// with Deferred Shading
//---------------------------------------------
class CEditorLevel : public CBasicRenderList
{
public:
	friend class CBaseEditableObject;

	CEditorLevel();

	void							CleanLevel(bool bExitCleanup = false);

	void							SetLevelName(const char* prefix);
	const char*						GetLevelName();
	bool							Save();
	bool							Load();

	void							CreateNew();

	void							AddEditable(CBaseEditableObject* pEditable, bool bRecordAction = false);
	void							RemoveEditable(CBaseEditableObject* pEditable);

	int								GetLayerCount();
	edworldlayer_t*					GetLayer(int nLayer);
	edworldlayer_t*					CreateLayer();
	void							RemoveLayer(int nLayer);
	int								FindLayer(const char* pszName);

	int								GetEditableCount();
	CBaseEditableObject*			GetEditable(int id);

	CBaseEditableObject*			GetEditableByUniqueID(int nId);

	IVertexFormat*					GetLevelVertexFormat();

	IMaterial*						GetFlatMaterial();

	void							Update(int nViewRenderFlags, void* userdata);
	void							Render(int nViewRenderFlags, void* userdata);

	CBaseEditableObject*			GetRayNearestObject(const Vector3D &start, const Vector3D &end, Vector3D &pos = Vector3D(0), int nNRFlags = NR_FLAG_TRACEALL);

	void							HideSelectedObjects();
	void							UnhideAll();

	void							NotifyUpdate();
	bool							IsNeedsSave();
	bool							IsSavedOnDisk();

	//void							ImportMapFile(const char* pFileName);

	void							BuildWorld();

	void							ClipboardAction(ClipboardMode_e mode);

	bool							SaveObjects(IVirtualStream* pStream, CBaseEditableObject** pObjects, int nObjects);
	bool							LoadObjects(IVirtualStream* pStream, DkList<CBaseEditableObject*> &pLoadedObjects);

	void							Undo();

	void							AddAction(CObjectAction* pAction);

protected:
	DkList<CBaseEditableObject*>	m_pEditableList;
	DkList<edworldlayer_t*>			m_pLayers;

	IVertexFormat*					m_pLevelvertexformat;

	IMaterial*						m_pFlatMaterial;

	EqString						m_szLevelName;

	bool							m_bNeedsSave;
	bool							m_bSavedOnDisk;

	CMemoryStream*					m_pClipboardStream;

	DkList<CObjectAction*>			m_pUndoActionList;
};

extern CEditorLevel *g_pLevel;

#endif // EDITORLEVEL_H