//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Editor level
//	
//			Importing the model can be result of creation of a new surfaces
//				(counted by model mat. group count)
//			Editable surfaces is also will be terrain patches, surface sheets.
//			Welding support
//////////////////////////////////////////////////////////////////////////////////

#include "IDebugOverlay.h"
#include "IEqModel.h"
#include "SelectionEditor.h"

#include "GroupEditable.h"
#include "EqBrush.h"
#include "EditableSurface.h"
#include "EditableEntity.h"
#include "EditableDecal.h"

#include "EditorMainFrame.h"
#include "SurfaceDialog.h"
#include "BuildOptionsDialog.h"
#include "EntityList.h"

#include "BaseEditableObject.h"
#include "BaseRenderableObject.h"
#include "VertexTool.h"

#include "LayerPanel.h"

#include "EditorVersion.h"

#include "VirtualStream.h"


int s_objectid_increment = 0;

#define DEFAULT_LEVEL_NAME "Untitled"

// LEVEL

CEditorLevel::CEditorLevel()
{
	m_pLevelvertexformat = NULL;
	m_pFlatMaterial = NULL;

	m_pClipboardStream = NULL;

	m_szLevelName = DEFAULT_LEVEL_NAME;

	m_bNeedsSave = false;
	m_bSavedOnDisk = false;
}

void CEditorLevel::CleanLevel(bool bExitCleanup)
{
	if(m_pLevelvertexformat)
		g_pShaderAPI->DestroyVertexFormat(m_pLevelvertexformat);

	m_pLevelvertexformat = NULL;

	if(m_pFlatMaterial)
		materials->FreeMaterial(m_pFlatMaterial);

	m_pFlatMaterial = NULL;

	((CSelectionBaseTool*)g_pSelectionTools[0])->CancelSelection(true, bExitCleanup);

	for(int i = 0; i < m_pEditableList.numElem(); i++)
	{
		m_pEditableList[i]->OnRemove(true);
		delete m_pEditableList[i];
	}

	m_pEditableList.clear();

	for(int i = 0; i < m_pUndoActionList.numElem(); i++)
		delete m_pUndoActionList[i];

	m_pUndoActionList.clear();

	for(int i = 0; i < m_pLayers.numElem(); i++)
		delete m_pLayers[i];

	m_pLayers.clear();

	if(!bExitCleanup)
	{
		g_editormainframe->GetEntityListDialog()->RefreshObjectList();
		g_editormainframe->GetGroupListDialog()->RefreshObjectList();
	}
}

void CEditorLevel::SetLevelName(const char* prefix)
{
	m_szLevelName = prefix;
	m_bSavedOnDisk = false;
}

const char* CEditorLevel::GetLevelName()
{
	return m_szLevelName.GetData();
}

extern CEditorViewRender g_views[4];

bool CEditorLevel::Save()
{
	EqString leveldir(EqString("worlds/")+m_szLevelName);

	// make level formats
	// create directory first
	g_fileSystem->MakeDir(leveldir.GetData(), SP_MOD);

	EqString editor_level_file(leveldir + "/editorlevel.lvl");
	EqString editor_level_file_new(leveldir + "/level.edlvl");

	EqString leveleditordata(leveldir + "/surfdata");

	//IFile* pFile = g_fileSystem->Open( editor_level_file.getData(), "wb");

	g_editormainframe->SetStatusText(varargs("Saving level '%s'\n", editor_level_file_new.GetData()));
	Msg("Saving...\n");

	KeyValues kv;

	kvkeybase_t* pEditorLevelSec = kv.GetRootSection()->AddKeyBase("EditorLevel");
	kvkeybase_t* pCameraParams = pEditorLevelSec->AddKeyBase("Cameras");

	for(int i = 0; i < 4; i++)
	{
		kvkeybase_t* pCamera = pCameraParams->AddKeyBase(varargs("%d",i));
		pCamera->AddKeyBase("position", varargs("%g %g %g", g_views[i].GetView()->GetOrigin().x,g_views[i].GetView()->GetOrigin().y,g_views[i].GetView()->GetOrigin().z));
		pCamera->AddKeyBase("angles", varargs("%g %g %g", g_views[i].GetView()->GetAngles().x,g_views[i].GetView()->GetAngles().y,g_views[i].GetView()->GetAngles().z));
		pCamera->AddKeyBase("fov", varargs("%g", g_views[i].GetView()->GetFOV()));
	}

	for(int i = 0; i < m_pLayers.numElem(); i++)
	{
		kvkeybase_t* pLayerSection = pEditorLevelSec->AddKeyBase("layer", false);

		pLayerSection->AddKeyBase("name", (char*)m_pLayers[i]->layer_name.GetData());
		pLayerSection->AddKeyBase("visible", varargs("%d", m_pLayers[i]->visible));
		pLayerSection->AddKeyBase("active", varargs("%d", m_pLayers[i]->active));
	}

	g_fileSystem->MakeDir(leveleditordata.GetData(), SP_MOD);

	for(int i = 0; i < m_pEditableList.numElem(); i++)
	{
		g_editormainframe->SetStatusText(varargs("Saving object id='%d'\n", m_pEditableList[i]->m_id));

		kvkeybase_t* pObjectSec = pEditorLevelSec->AddKeyBase(s_editabletypestrings[m_pEditableList[i]->GetType()], false);
		pObjectSec->AddKeyBase("obj_id", varargs("%d", m_pEditableList[i]->GetObjectID()));
		pObjectSec->AddKeyBase("layer", varargs("%d", m_pEditableList[i]->GetLayerID()));
		pObjectSec->AddKeyBase("color", varargs("%g %g %g", m_pEditableList[i]->GetGroupColor().x,m_pEditableList[i]->GetGroupColor().y,m_pEditableList[i]->GetGroupColor().z));
		pObjectSec->AddKeyBase("hide", varargs("%d", (m_pEditableList[i]->GetFlags() & EDFL_HIDDEN)));
		pObjectSec->AddKeyBase("name", (char*)m_pEditableList[i]->GetName());

		m_pEditableList[i]->SaveToKeyValues(pObjectSec);
	}

	kv.SaveToFile(editor_level_file_new.GetData(), SP_MOD);

	m_bSavedOnDisk = true;
	m_bNeedsSave = false;

	Msg("Successfully saved %s.\n", editor_level_file_new.GetData());
	g_editormainframe->SetStatusText(varargs("'%s' successfully saved\n", editor_level_file_new.GetData()));

	/*
	IVirtualStream* pStream = OpenFileStream(VS_OPEN_WRITE, editor_level_file.getData());

	if(pStream)
	{
		Msg("Saving...\n");

		SaveObjects(pStream, m_pEditableList.ptr(), m_pEditableList.numElem());
		
		pStream->Destroy();
		DDelete(pStream);

		m_bSavedOnDisk = true;
		m_bNeedsSave = false;

		Msg("Successfully saved %s.\n", editor_level_file_new.getData());
	}
	else
	{
		MsgError("Unable to save world. Can't open file %s for write.\n", editor_level_file.getData());
		return false;
	}
	*/

	return true;
}

bool CEditorLevel::SaveObjects(IVirtualStream* pStream, CBaseEditableObject** pObjects, int nObjects)
{
	// write header
	int ident = MCHAR4('E','Q','L','E');
	int nObjectCount = nObjects;

	pStream->Write(&ident, 1, sizeof(int));
	pStream->Write(&nObjectCount, 1, sizeof(int));

	DkList<int> nWrittenObjectsGroupIds;

	// write level objects
	for(int i = 0; i < nObjects; i++)
	{
		// first, write object header
		int object_type = pObjects[i]->GetType();
		pStream->Write(&object_type,1,sizeof(int));

		char str[128];
		strcpy(str,pObjects[i]->GetName());
		pStream->Write(str, 1, 128);

		pObjects[i]->WriteObject(pStream);
		nWrittenObjectsGroupIds.append(pObjects[i]->GetGroupID());
	}

	return true;
}

#define EQEDIT_CLIPBOARD (CF_PRIVATEFIRST+1)

bool EDUTIL_StreamToClipboard(CMemoryStream* pStream)
{
	if(OpenClipboard( (HWND) g_editormainframe->GetHWND() ))
	{
		HGLOBAL hgBuffer;

		EmptyClipboard();
		hgBuffer = GlobalAlloc(GMEM_DDESHARE, pStream->GetSize() + sizeof(int) );

		ubyte* chBuffer = (ubyte*)GlobalLock( hgBuffer );

		// stream size is needed
		*((int*)chBuffer) = pStream->GetSize();

		// copy stream data
		memcpy(chBuffer + sizeof(int), pStream->GetBasePointer(), pStream->GetSize());

		GlobalUnlock(hgBuffer);
		SetClipboardData( EQEDIT_CLIPBOARD, hgBuffer );
		CloseClipboard();

		return true;
	}

	return false;
}

bool EDUTIL_ClipboardToStream(CMemoryStream* pStream)
{
	if(OpenClipboard( (HWND) g_editormainframe->GetHWND() ))
	{
		HANDLE hData = GetClipboardData( EQEDIT_CLIPBOARD );

		ubyte* chBuffer = (ubyte*)GlobalLock( hData );
		if(chBuffer == NULL)
		{
			GlobalUnlock(hData);
			CloseClipboard();
			return false;
		}

		int size = *((int*)chBuffer);

		pStream->Open(NULL, VS_OPEN_WRITE, 4096);

		// copy stream data back
		pStream->Write(chBuffer + sizeof(int), 1, size);

		pStream->Close();

		GlobalUnlock(hData);
		CloseClipboard();

		return true;
	}

	return false;
}

void CEditorLevel::ClipboardAction(ClipboardMode_e mode)
{
	if(!m_pClipboardStream)
	{
		// create memory stream
		m_pClipboardStream = (CMemoryStream*)OpenMemoryStream(VS_OPEN_WRITE, 0);
		m_pClipboardStream->Close();
	}

	switch(mode)
	{
		case CLIPBOARD_COPY:
		{
			// Base buffer size is 4 kbytes, don't realloc too much with small objects
			m_pClipboardStream->Open(NULL, VS_OPEN_WRITE, 4096);
			break;
		}
		case CLIPBOARD_CLEAR:
		{
			m_pClipboardStream->Open(NULL, VS_OPEN_WRITE, 0);
			m_pClipboardStream->Close();
			return;
		}
		case CLIPBOARD_PASTE:
		{
			// try to copy clipboard
			EDUTIL_ClipboardToStream( (CMemoryStream*)m_pClipboardStream );

			m_pClipboardStream->Open(NULL, VS_OPEN_READ, 0);
			break;
		}
	}

	if(m_pClipboardStream->GetSize() == 0 && mode == CLIPBOARD_PASTE)
	{
		MsgInfo("Clipboard is empty\n");
		return;
	}
	
	if(mode == CLIPBOARD_COPY)
	{
		DkList<CBaseEditableObject*> *pObjects = ((CSelectionBaseTool*)g_pSelectionTools[0])->GetSelectedObjects();

		// save as level
		SaveObjects(m_pClipboardStream, pObjects->ptr(), pObjects->numElem());

		// if we copied stream to windows clipboard, empty editor's one
		if( EDUTIL_StreamToClipboard( (CMemoryStream*)m_pClipboardStream ))
		{
			m_pClipboardStream->Open(NULL, VS_OPEN_WRITE, 0);
			m_pClipboardStream->Close();
		}
	}
	else if(mode == CLIPBOARD_PASTE)
	{
		DkList<CBaseEditableObject*> pObjects;

		// just load objects from our level, and also select them
		LoadObjects(m_pClipboardStream, pObjects);

		CObjectAction* pNewAction = new CObjectAction;
		pNewAction->Begin();
		pNewAction->SetObjectIsCreated();

		for(int i = 0; i < pObjects.numElem(); i++)
		{
			pNewAction->AddObject( pObjects[i] );
			m_pEditableList.append( pObjects[i] );
		}

		((CSelectionBaseTool*)g_pSelectionTools[0])->CancelSelection(true);
		((CSelectionBaseTool*)g_pSelectionTools[0])->SetSelectionState(SELECTION_PREPARATION);
		((CSelectionBaseTool*)g_pSelectionTools[0])->SelectObjects(true, pObjects.ptr(), pObjects.numElem());

		pNewAction->End();
		AddAction(pNewAction);
	}

	UpdateAllWindows();
}

/*

//
// A code to support QERadiant map files importing
// if you want to use it you need quakelib. Sorry.
// Loading code discovered in q3map2
//

inline Vector3D ConvertQ3ToDkVectors(Vector3D &input)
{
	return Vector3D(-input.x,input.z,-input.y);
}

void MapPlaneFromPoints(Vector3D &v0,Vector3D &v1,Vector3D &v2, Plane &pl)
{
	Vector3D t1 = v0-v1;
	Vector3D t2 = v2-v1;
	pl.normal = -normalize(cross(t1, t2));
	pl.offset = -dot(v0, pl.normal);
}

void CEditorLevel::ImportMapFile(const char* pFileName)
{
	LoadScriptFile(pFileName, -1);

	if(!GetToken(true))
	{
		MsgError("Nothing imported. Map is empty\n");
		return;
	}

	if(stricmp(token, "{"))
	{
		Msg("Invalid map file\n");
		return;
	}

	while(1)
	{
		// get initial token
		if(!GetToken(true))
			return;

		if(!stricmp(token, "}"))
			break;

		if(!stricmp(token, "{"))
		{
			// parse a brush or patch
			if(!GetToken(true))
				break;

			// check entity/object types
			if(!stricmp(token, "patchDef2"))
			{
				eqlevelvertex_t* pTempVerts = NULL;
				CEditableSurface* pSurface = DNew(CEditableSurface);

				MatchToken("{");

				// parse material
				GetToken(true);
				char material_name[256];
				strcpy(material_name, token);

				float info[5];

				Parse1DMatrix(5, info);

				int wide = info[0];
				int tall = info[1];

				pTempVerts = new eqlevelvertex_t[wide*tall];

				MatchToken("(");
				for(int j = 0; j < wide; j++)
				{
					MatchToken("(");
					for(int i = 0; i < tall; i++)
					{
						float vert_struct[5];
						Parse1DMatrix(5, vert_struct);
						pTempVerts[i * wide + j].position = Vector3D(vert_struct[0],vert_struct[1],vert_struct[2]);
						pTempVerts[i * wide + j].texcoord = Vector2D(vert_struct[3],vert_struct[4]);
						pTempVerts[i * wide + j].position = ConvertQ3ToDkVectors(pTempVerts[i * wide + j].position);
						pTempVerts[i * wide + j].normal = Vector3D(0,0,1);
						pTempVerts[i * wide + j].tangent = Vector3D(1,0,0);
						pTempVerts[i * wide + j].binormal = Vector3D(0,1,0);
						pTempVerts[i * wide + j].vertexPaint = Vector4D(1,0,0,0);
					}
					MatchToken(")");
				}
				MatchToken(")");

				MatchToken("}");
				MatchToken("}");

				pSurface->MakeCustomTerrainPatch(wide, tall, pTempVerts, 4);
				pSurface->SetMaterial(materials->FindMaterial(material_name,true));
				m_pEditableList.append(pSurface);

				delete [] pTempVerts;
				
			}
			else if(!stricmp(token, "patchDef3"))
			{
				SkipBracedSection();
			}
			else if(!stricmp(token, "terrainDef"))
			{
				SkipBracedSection();
			}
			else if(!stricmp(token, "brushDef"))
			{
				SkipBracedSection();
			}
			else if(!stricmp(token, "brushDef3"))
			{
				SkipBracedSection();
			}
			else
			{
				// parse brush
				UnGetToken();

				CEditableBrush* pNewBrush = DNew(CEditableBrush);

				ColorRGBA brush_col(0.5f+RandomFloat(0.0f,0.5f),0.5f+RandomFloat(0.0f,0.5f),0.5f+RandomFloat(0.0f,0.5f),1);

				// parse brush planes/triangle definitions
				while(1)
				{
					Texture_t face;

					if(!GetToken(true))
						break;

					if(!stricmp(token, "}"))
						break;

					UnGetToken();

					Vector3D triangle_to_plane[3];

					Parse1DMatrix(3, triangle_to_plane[0]);
					Parse1DMatrix(3, triangle_to_plane[1]);
					Parse1DMatrix(3, triangle_to_plane[2]);

					// convert to our coordinate system
					triangle_to_plane[0] = ConvertQ3ToDkVectors(triangle_to_plane[0]);
					triangle_to_plane[1] = ConvertQ3ToDkVectors(triangle_to_plane[1]);
					triangle_to_plane[2] = ConvertQ3ToDkVectors(triangle_to_plane[2]);
					
					char material_name[256];

					// read shader (material) name
					GetToken(false);
					strcpy(material_name, token);

					// parse texture parameters
					Vector2D	shift;
					Vector2D	scale;
					float		rotate;

					GetToken(false);
					shift[0] = atof(token);

					GetToken(false);
					shift[1] = atof(token);

					GetToken(false);
					rotate = atof(token);

					GetToken(false);
					scale[0] = atof(token);

					GetToken(false);
					scale[1] = atof(token);

					GetToken(false);
					GetToken(false);
					GetToken(false);

					// make plane from def
					//face.Plane = Plane(triangle_to_plane[0],triangle_to_plane[1],triangle_to_plane[2]);
					MapPlaneFromPoints(triangle_to_plane[0],triangle_to_plane[1],triangle_to_plane[2], face.Plane);

					//debugoverlay->Polygon3D(triangle_to_plane[0],triangle_to_plane[1],triangle_to_plane[2], brush_col, 100500);

					// make the U and V texture axes
					VectorVectors(face.Plane.normal, face.UAxis.normal, face.VAxis.normal);
					face.UAxis.offset = shift.x;
					face.VAxis.offset = shift.y;
					face.vScale = scale;
					face.fRotation = rotate;

					face.pMaterial = materials->FindMaterial(material_name, true);

					if(face.pMaterial->GetShader())
						face.pMaterial->GetShader()->InitParams();

					pNewBrush->AddFace(face);
				}

				pNewBrush->CreateFromPlanes();

				if(pNewBrush->GetFaceCount() == 0)
				{
					DDelete(pNewBrush);
					continue;
				}

				pNewBrush->CalculateBBOX();

				pNewBrush->UpdateRenderBuffer();

				m_pEditableList.append(pNewBrush);
			}
		}
	}

	m_bNeedsSave = true;
}
*/

CBaseEditableObject* CreateEmptyTypedObject(EditableType_e type)
{
	switch(type)
	{
		case EDITABLE_BRUSH:
			return new CEditableBrush;
		case EDITABLE_TERRAINPATCH:
		case EDITABLE_STATIC:
			return new CEditableSurface;
		case EDITABLE_ENTITY:
			return new CEditableEntity;
		case EDITABLE_GROUP:
			return new CEditableGroup;
			return new CEditableEntity;
		case EDITABLE_DECAL:
			return new CEditableDecal;
	}

	return NULL;
}

bool CEditorLevel::Load()
{
	EqString leveldir(EqString("worlds/")+m_szLevelName);
	EqString editor_level_file(leveldir + "/editorlevel.lvl");
	EqString editor_level_file_new(leveldir + "/level.edlvl");

	// try to open level file

	KeyValues kv;

	int nHasHiddenObjects = 0;

	g_editormainframe->SetStatusText(varargs("Loading level '%s'\n", editor_level_file_new.GetData()));

	if(kv.LoadFromFile(editor_level_file_new.GetData()))
	{
		MsgError("Loading level %s...\n", m_szLevelName.GetData());

		s_objectid_increment = 0;

		// clean this level
		CleanLevel();

		//DkList<CBaseEditableObject*> pObjects;

		kvkeybase_t* pSection = kv.GetRootSection()->FindKeyBase("EditorLevel", KV_FLAG_SECTION);

		m_pEditableList.resize(pSection->keys.numElem());

		// search for any layer section, if none - keek DefaultLayer
		if(!pSection->FindKeyBase("layer"))
		{
			edworldlayer_t* pLayer = CreateLayer();
			pLayer->layer_name = "DefaultLayer";
			pLayer->active = true;
			pLayer->visible = true;
		}

		for(int i = 0; i < pSection->keys.numElem(); i++)
		{
			if(!pSection->keys[i]->keys.numElem())
				continue;

			if(!stricmp("cameras", pSection->keys[i]->name))
			{
				kvkeybase_t* pCameraParams = pSection->keys[i];
				for(int i = 0; i < 4; i++)
				{
					kvkeybase_t* pCamera = pCameraParams->keys[i];
					g_views[i].GetView()->SetOrigin(UTIL_StringToColor3(pCamera->FindKeyBase("position")->values[0]));
					g_views[i].GetView()->SetAngles(UTIL_StringToColor3(pCamera->FindKeyBase("angles")->values[0]));
					g_views[i].GetView()->SetFOV(atof(pCamera->FindKeyBase("fov")->values[0]));
				}
			}
			else if(!stricmp("layer", pSection->keys[i]->name))
			{
				kvkeybase_t* pLayerParams = pSection->keys[i];

				edworldlayer_t* pLayer = CreateLayer();

				kvkeybase_t* pPair = pLayerParams->FindKeyBase("name");
				pLayer->layer_name = pPair->values[0];

				pPair = pLayerParams->FindKeyBase("visible");
				pLayer->visible = atoi(pPair->values[0]);

				pPair = pLayerParams->FindKeyBase("active");
				pLayer->active = atoi(pPair->values[0]);
			}
			else
			{
				kvkeybase_t* pObjectSection = pSection->keys[i];

				EditableType_e edType = ResolveEditableTypeFromString(pObjectSection->name);

				// create typed object
				CBaseEditableObject* pObject = CreateEmptyTypedObject(edType);

				if(!pObject)
				{
					MsgError("Unsupported object %d type (%d)!\n", i, edType);
					continue;
				}

				kvkeybase_t* pPair = pObjectSection->FindKeyBase("color");

				if(pPair)
				{
					ColorRGB grp_color = UTIL_StringToColor3(pPair->values[0]);
					pObject->SetGroupColor(grp_color);
				}

				pPair = pObjectSection->FindKeyBase("obj_id");

				// assign object id after reading to not be corrupt with groupped objects
				if(pPair)
				{
					pObject->m_id = atoi(pPair->values[0]);

					// increment used ID automatically
					if(pObject->m_id > s_objectid_increment)
						s_objectid_increment = pObject->m_id;
				}

				g_editormainframe->SetStatusText(varargs("Loading object id='%d'\n", pObject->m_id));

				pPair = pObjectSection->FindKeyBase("layer");
				if(pPair)
				{
					pObject->m_layer_id = atoi(pPair->values[0]);

					// assign object to layer
					m_pLayers[pObject->m_layer_id]->object_ids.append( pObject->m_id );
				}

				pPair = pObjectSection->FindKeyBase("name");
				if(pPair)
				{
					pObject->SetName(pPair->values[0]);
				}

				pPair = pObjectSection->FindKeyBase("hide");
				if(pPair)
				{
					bool bHidden = atoi(pPair->values[0]);
					pObject->AddFlags(atoi(pPair->values[0]) ? EDFL_HIDDEN : 0);

					if(bHidden)
						nHasHiddenObjects++;
				}
				
				// load this object from section
				pObject->LoadFromKeyValues(pObjectSection);

				if(pObject->GetType() == EDITABLE_GROUP)
				{
					CEditableGroup* pObj = (CEditableGroup*)pObject;
					if(pObj->GetGroupList()->numElem() <= 1)
					{
						pObj->OnRemove();
						delete pObj;
						continue;
					}
				}

				m_pEditableList.append(pObject);

				//pObjects.append(pObject);
				
				// almost done, next ahead
			}
		}

		g_editormainframe->SetStatusText(varargs("'%s' is successfully loaded\n", editor_level_file_new.GetData()));

		//for(int i = 0; i < pObjects.numElem(); i++)
		//	m_pEditableList.append(pObjects[i]);

		m_bSavedOnDisk = true;
		m_bNeedsSave = false;

		g_editormainframe->GetBuildOptionsDialog()->LoadMapSettings();
	}
	else
	{
		IFile* pStream = g_fileSystem->Open(editor_level_file.GetData(), "rb");

		// support old formats for now
		if(pStream)
		{
			s_objectid_increment = 0;

			MsgError("Loading level %s...\n", m_szLevelName.GetData());

			// clean this level
			CleanLevel();

			DkList<CBaseEditableObject*> pObjects;

			// just load objects from our level
			LoadObjects(pStream, pObjects);

			for(int i = 0; i < pObjects.numElem(); i++)
				m_pEditableList.append(pObjects[i]);

			g_fileSystem->Close(pStream);

			m_bSavedOnDisk = true;
			m_bNeedsSave = false;

			g_editormainframe->GetBuildOptionsDialog()->LoadMapSettings();
		}
		else
		{
			MsgError("Unable to open level. Can't open file %s for read.\n", editor_level_file.GetData());
			return false;
		}
	}

	if(nHasHiddenObjects)
		wxMessageBox(wxString(varargs((char*)DKLOC("TOKEN_HIDDENOBJECTS", "World has %d hidden objects!"), nHasHiddenObjects)), wxString("Warning"), wxOK | wxCENTRE | wxICON_WARNING, g_editormainframe);

	g_editormainframe->GetEntityListDialog()->RefreshObjectList();
	g_editormainframe->GetGroupListDialog()->RefreshObjectList();

	return true;
}

bool CEditorLevel::LoadObjects(IVirtualStream* pStream, DkList<CBaseEditableObject*> &pLoadedObjects)
{
	// write header
	int ident = -1;
	int nObjectCount = 0;

	pStream->Read(&ident, 1, sizeof(int));
	pStream->Read(&nObjectCount, 1, sizeof(int));

	if(ident != MCHAR4('E','Q','L','E'))
	{
		MsgError("Invalid editor level data, can't load.\n");
		return false;
	}

	// read level objects
	for(int i = 0; i < nObjectCount; i++)
	{
		int object_type = -1;
		pStream->Read(&object_type, 1, sizeof(int));

		// create typed object
		CBaseEditableObject* pObject = CreateEmptyTypedObject((EditableType_e)object_type);

		if(!pObject)
		{
			MsgError("Unsupported object %d type (%d)!\n", i, object_type);
			return false;
		}

		char str[128];
		str[0] = 0;
		pStream->Read(str, 1, 128);

		pObject->SetName(str);

		// then load it's data
		if(!pObject->ReadObject(pStream))
		{
			MsgError("Object error\n");
			return false;
		}

		pLoadedObjects.append(pObject);
	}


	// TODO last group id
	/*
	// save groups of saved objects
	int nObjectGroups = 0;

	pStream->Read(&nObjectGroups, 1, sizeof(int));

	for(int i = 0; i < nObjectGroups; i++)
	{
		int nGroupID = -1;
		pStream->Read(&pLoadedObjects[i], 1, sizeof(int));
		pLoadedObjects[i]->SetGroupID(nGroupID);
	}
	*/

	return true;
}

void CEditorLevel::NotifyUpdate()
{
	m_bNeedsSave = true;
}

bool CEditorLevel::IsNeedsSave()
{
	return m_bNeedsSave;
}

bool CEditorLevel::IsSavedOnDisk()
{
	return m_bSavedOnDisk;
}

void CEditorLevel::CreateNew()
{
	m_szLevelName = DEFAULT_LEVEL_NAME;
	CleanLevel();

	m_bNeedsSave = false;
	m_bSavedOnDisk = false;

	s_objectid_increment = 0;

	edworldlayer_t* pLayer = CreateLayer();
	pLayer->layer_name = "DefaultLayer";
	pLayer->active = true;
	pLayer->visible = true;
}

void CEditorLevel::AddAction(CObjectAction* pAction)
{
	m_pUndoActionList.append(pAction);
	m_bNeedsSave = true;

	g_editormainframe->GetEntityListDialog()->RefreshObjectList();
	g_editormainframe->GetGroupListDialog()->RefreshObjectList();
}

void CEditorLevel::AddEditable(CBaseEditableObject* pEditable, bool bRecordAction)
{
	if(!pEditable)
		return;

	m_pEditableList.append(pEditable);
	m_bNeedsSave = true;

	if(bRecordAction)
	{
		CObjectAction* pNewAction = new CObjectAction;

		pNewAction->Begin();

		pNewAction->SetObjectIsCreated();
		pNewAction->AddObject(pEditable);

		pNewAction->End();
		g_pLevel->AddAction(pNewAction);
	}

	g_editormainframe->GetEntityListDialog()->RefreshObjectList();
	g_editormainframe->GetGroupListDialog()->RefreshObjectList();
}

void CEditorLevel::RemoveEditable(CBaseEditableObject* pEditable)
{
	// deselect objects
	if(g_pSelectionTools[0])
	{
		DkList<CBaseEditableObject*> *selected_objects = ((CSelectionBaseTool*)g_pSelectionTools[0])->GetSelectedObjects();
		for(int i = 0; i < selected_objects->numElem(); i++)
		{
			if(selected_objects->ptr()[i] == pEditable)
			{
				selected_objects->removeIndex(i);
				i--;
			}
		}

		if(selected_objects->numElem() <= 0)
			((CSelectionBaseTool*)g_pSelectionTools[0])->CancelSelection();
	}

	/*
	// invalidate object
	for(int i = 0; i < m_pEditableList.numElem(); i++)
	{
		if(g_pLevel->GetEditable(i)->GetType() != EDITABLE_GROUP)
			continue;

		CEditableGroup* pGroup = (CEditableGroup*)m_pEditableList[i];

		pGroup->RemoveObject(pEditable);

		if(pGroup->GetGroupList()->numElem() == 0)
		{
			m_pEditableList.remove(pGroup);

			pGroup->OnRemove();
			delete pGroup;
			i--;
		}
	}*/

	// invalidate in-layer
	m_pLayers[pEditable->m_layer_id]->object_ids.remove(pEditable->m_id);

	m_pEditableList.remove(pEditable);
	pEditable->OnRemove();
	delete pEditable;

	g_editormainframe->GetEntityListDialog()->RefreshObjectList();
	g_editormainframe->GetGroupListDialog()->RefreshObjectList();
}

int	CEditorLevel::GetLayerCount()
{
	return m_pLayers.numElem();
}

edworldlayer_t* CEditorLevel::GetLayer(int nLayer)
{
	if(nLayer == -1)
		return NULL;

	return m_pLayers[nLayer];
}

edworldlayer_t* CEditorLevel::CreateLayer()
{
	edworldlayer_t* pNewLayer = new edworldlayer_t;
	pNewLayer->visible = true;
	pNewLayer->active = false;
	pNewLayer->layer_name = varargs("layer_%d", m_pLayers.numElem());

	m_pLayers.append(pNewLayer);

	//g_editormainframe->GetWorldLayerPanel()->Update();

	return pNewLayer;
}

void CEditorLevel::RemoveLayer(int nLayer)
{
	for(int i = nLayer+1; i < m_pLayers.numElem(); i++)
	{
		for(int j = 0; j < m_pLayers[i]->object_ids.numElem(); j++)
		{
			CBaseEditableObject* pObject = g_pLevel->GetEditableByUniqueID(m_pLayers[i]->object_ids[j]);

			if(pObject)
				pObject->m_layer_id--;
		}
		
	}

	// move layer objects to the main layer before deletion
	for(int i = 0; i < m_pLayers[nLayer]->object_ids.numElem(); i++)
	{
		CBaseEditableObject* pObject = GetEditableByUniqueID(m_pLayers[nLayer]->object_ids[i]);

		if(pObject)
			pObject->m_layer_id = 0;
	}

	delete m_pLayers[nLayer];
	m_pLayers.removeIndex(nLayer);
}

int	CEditorLevel::FindLayer(const char* pszName)
{
	for(int i = 0; i < m_pLayers.numElem(); i++)
	{
		if(!stricmp(m_pLayers[i]->layer_name.GetData(), pszName))
			return i;
	}

	return -1;
}

CBaseEditableObject* CEditorLevel::GetRayNearestObject(const Vector3D &start, const Vector3D &end, Vector3D &pos, int nNRFlags)
{
	float fNearest = 1.0f;
	CBaseEditableObject* pNearest = NULL;
	pos = vec3_zero;

	for(int i = 0; i < m_ObjectList.numElem(); i++)
	{
		CBaseEditableObject* pObject = (CBaseEditableObject*)m_ObjectList[i];

		// skip invisible layers
		if(!m_pLayers[pObject->m_layer_id]->visible)
			continue;

		// don't select hidden objects
		if(pObject->GetFlags() & EDFL_HIDDEN)
			continue;

		// don't select grouped objects if there is no special flag
		if((pObject->GetFlags() & EDFL_GROUPPED) && !(nNRFlags & NR_FLAG_INCLUDEGROUPPED))
			continue;

		if(!(nNRFlags & NR_FLAG_BRUSHES) && (pObject->GetType() == EDITABLE_BRUSH))
			continue;

		if(!(nNRFlags & NR_FLAG_ENTITIES) && ((pObject->GetType() == EDITABLE_ENTITY) || (pObject->GetType() == EDITABLE_DECAL)))
			continue;

		// select groups ?
		if(!(nNRFlags & NR_FLAG_GROUP) && (pObject->GetType() == EDITABLE_GROUP))
			continue;

		if(!(nNRFlags & NR_FLAG_SURFACEMODS) && ((pObject->GetType() == EDITABLE_STATIC) || (pObject->GetType() == EDITABLE_TERRAINPATCH)))
			continue;

		Vector3D intersec_point;
		float fraction = pObject->CheckLineIntersection(start, end, intersec_point);

		if(fraction < fNearest)
		{
			pos = intersec_point;
			fNearest = fraction;
			pNearest = pObject;
		}
	}

	return pNearest;
}

int CEditorLevel::GetEditableCount()
{
	return m_pEditableList.numElem();
}

CBaseEditableObject* CEditorLevel::GetEditable(int id)
{
	return m_pEditableList[id];
}

CBaseEditableObject* CEditorLevel::GetEditableByUniqueID(int nId)
{
	for(int i = 0; i < m_pEditableList.numElem(); i++)
	{
		if(m_pEditableList[i]->GetObjectID() == nId)
			return m_pEditableList[i];
	}

	return NULL;
}

IVertexFormat* CEditorLevel::GetLevelVertexFormat()
{
	if(!m_pLevelvertexformat)
	{
		VertexFormatDesc_t pFormat[] = {
			{ 0, 3, VERTEXATTRIB_POSITION, ATTRIBUTEFORMAT_FLOAT },	  // position
			{ 0, 2, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_FLOAT }, // texcoord 0

			{ 0, 3, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_FLOAT }, // Tangent (TC1)
			{ 0, 3, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_FLOAT }, // Binormal (TC2)
			{ 0, 3, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_FLOAT }, // Normal (TC3)

			{ 0, 4, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_FLOAT }, // vertex painting data (TC4)
		};

		m_pLevelvertexformat = g_pShaderAPI->CreateVertexFormat(pFormat, elementsOf(pFormat));
	}

	return m_pLevelvertexformat;
}

IMaterial* CEditorLevel::GetFlatMaterial()
{
	if(!m_pFlatMaterial)
		m_pFlatMaterial = materials->FindMaterial("flatcolor");

	return m_pFlatMaterial;
}

int DistanceCompare(const void* obj1, const void* obj2)
{
	CBaseEditableObject* pObjectA = *(CBaseEditableObject**)obj1;
	CBaseEditableObject* pObjectB = *(CBaseEditableObject**)obj2;

	return -UTIL_CompareFloats(pObjectA->m_fViewDistance, pObjectB->m_fViewDistance);
}

void CEditorLevel::Update(int nViewRenderFlags, void* userdata)
{
	drawlist_struct_t *drawOptions = (drawlist_struct_t*)userdata;

	m_ObjectList.clear();
	m_ObjectList.resize(2048); //editor has extended range

	Vector2D screen_dims;
	
	if(userdata)
		screen_dims = drawOptions->pViewRender->Get2DDimensions();

	Rectangle_t screen_rect(0,0, screen_dims.x,screen_dims.y);

	// compute distances
	for(int i = 0; i < m_pEditableList.numElem(); i++)
	{
		// skip invisible layers
		if(!m_pLayers[m_pEditableList[i]->m_layer_id]->visible)
			continue;

		// skip hidden objects
		if(m_pEditableList[i]->GetFlags() & EDFL_HIDDEN)
			continue;

		if((m_pEditableList[i]->GetType() == EDITABLE_DECAL || m_pEditableList[i]->GetType() == EDITABLE_ENTITY) && !g_editormainframe->IsObjectVisibiltyEnabled( VisObj_Entities ))
			continue;

		bool isSpecial =	(m_pEditableList[i]->GetFlags() & EDFL_AREAPORTAL) ||
							(m_pEditableList[i]->GetFlags() & EDFL_ROOM) || 
							(m_pEditableList[i]->GetFlags() & EDFL_CLIP);

		if(!isSpecial && m_pEditableList[i]->GetType() <= EDITABLE_BRUSH && !g_editormainframe->IsObjectVisibiltyEnabled( VisObj_Geometry ))
			continue;
		else if((m_pEditableList[i]->GetFlags() & EDFL_AREAPORTAL) && !g_editormainframe->IsObjectVisibiltyEnabled( VisObj_AreaPortals ))
			continue;
		else if((m_pEditableList[i]->GetFlags() & EDFL_ROOM) && !g_editormainframe->IsObjectVisibiltyEnabled( VisObj_Rooms ))
			continue;
		else if((m_pEditableList[i]->GetFlags() & EDFL_CLIP) && !g_editormainframe->IsObjectVisibiltyEnabled( VisObj_Clip ))
			continue;

		Vector3D mins = m_pEditableList[i]->GetBBoxMins();
		Vector3D maxs = m_pEditableList[i]->GetBBoxMaxs();

		if(userdata)
		{
			if(drawOptions->pViewRender->GetCameraMode() != CPM_PERSPECTIVE && m_pEditableList[i]->GetType() == EDITABLE_ENTITY)
			{
				// automatically hide entities
				Rectangle_t rect;
				SelectionRectangleFromBBox(drawOptions->pViewRender, mins, maxs, rect);

				if(g_editormainframe->IsClipModelsEnabled() && !screen_rect.IsFullyInside(rect))
					continue;
			}

			BoundingBox bbox(mins, maxs);

			Vector3D pos = drawOptions->pViewRender->GetView()->GetOrigin();

			float dist_to_camera;

			// clamp point in bbox
			if(!bbox.Contains(pos))
			{
				pos = bbox.ClampPoint(pos);
				dist_to_camera = length(pos - drawOptions->pViewRender->GetView()->GetOrigin());
			}
			else
			{
				dist_to_camera = length(pos - bbox.GetCenter());
			}
	
			m_pEditableList[i]->m_fViewDistance = dist_to_camera;
		}

		m_ObjectList.append(m_pEditableList[i]);
	}

	// sort list
	qsort(m_ObjectList.ptr(), m_ObjectList.numElem(), sizeof(CBaseEditableObject*), DistanceCompare);
}

void CEditorLevel::Render(int nViewRenderFlags, void* userdata)
{
	drawlist_struct_t *drawOptions = (drawlist_struct_t*)userdata;

	Matrix4x4 view, proj, viewProj;

	// update draw lists
	Update(nViewRenderFlags, userdata);

	if(!userdata)
	{
		proj = viewrenderer->GetMatrix(MATRIXMODE_PROJECTION);
		view = viewrenderer->GetMatrix(MATRIXMODE_VIEW);
	}
	else
	{
		drawOptions->pViewRender->GetViewProjection(view, proj);
	}

	
	viewProj = proj*view;

	Volume frustum_volume;
	frustum_volume.LoadAsFrustum(viewProj);

	if(userdata && drawOptions->bWireframe)
	{
		for(int i = 0; i < m_ObjectList.numElem(); i++)
		{
			CBaseEditableObject* pObject = (CBaseEditableObject*)m_ObjectList[i];

			if(!frustum_volume.IsBoxInside(	pObject->GetBBoxMins().x,
								pObject->GetBBoxMaxs().x,

								pObject->GetBBoxMins().y,
								pObject->GetBBoxMaxs().y,

								pObject->GetBBoxMins().z,
								pObject->GetBBoxMaxs().z))
								continue;

			drawOptions->pViewRender->SetupRenderMatrices();

			if(pObject->GetFlags() & EDFL_SELECTED)
				materials->SetAmbientColor(ColorRGBA(1,0,0,1));
			else
				materials->SetAmbientColor(ColorRGBA(pObject->GetGroupColor(),1));

			materials->GetConfiguration().wireframeMode = true;

			pObject->RenderGhost(vec3_zero, Vector3D(1), vec3_zero);
			
			// draw center handle for each object
			Vector3D center3D = (pObject->GetBBoxMins()+pObject->GetBBoxMaxs())*0.5f;
			Vector2D center2D;

			PointToScreen(center3D, center2D, viewProj, drawOptions->pViewRender->Get2DDimensions());
		}

		materials->Setup2D(drawOptions->pViewRender->Get2DDimensions().x, drawOptions->pViewRender->Get2DDimensions().y);

		for(int i = 0; i < m_ObjectList.numElem(); i++)
		{
			CBaseEditableObject* pObject = (CBaseEditableObject*)m_ObjectList[i];

			if(pObject->GetType() == EDITABLE_ENTITY)
			{
				if(!frustum_volume.IsBoxInside(	pObject->GetBBoxMins().x,
					pObject->GetBBoxMaxs().x,

					pObject->GetBBoxMins().y,
					pObject->GetBBoxMaxs().y,

					pObject->GetBBoxMins().z,
					pObject->GetBBoxMaxs().z))
					continue;

				Vector3D mins,maxs;
				mins = pObject->GetBBoxMins();
				maxs = pObject->GetBBoxMaxs();

				if((drawOptions->pViewRender->GetView()->GetFOV()/drawOptions->pViewRender->Get2DDimensions().y) > length(maxs-pObject->GetPosition())*0.1f)
					continue;

				// draw center handle for each object
				Vector3D center3D = (mins+maxs)*0.5f;
				Vector2D center2D;

				PointToScreen(center3D, center2D, viewProj, drawOptions->pViewRender->Get2DDimensions());

				CEditableEntity* pEntity = (CEditableEntity*)pObject;

				debugoverlay->GetFont()->DrawSetColor(1,1,1,1);
				debugoverlay->GetFont()->DrawTextEx(pEntity->GetClassname(), center2D.x,center2D.y,8,8,TEXT_ORIENT_RIGHT,false);
			}
		}
	}
	else
	{
		for(int i = 0; i < m_ObjectList.numElem(); i++)
		{
			CBaseEditableObject* pObject = (CBaseEditableObject*)m_ObjectList[i];

			if(g_editormainframe->GetSurfaceDialog()->IsSelectionMaskDisabled())
			{
				materials->SetAmbientColor(ColorRGBA(1,1,1,1));
			}
			else
			{
				if(pObject->GetFlags() & EDFL_SELECTED)
					materials->SetAmbientColor(ColorRGBA(1,0,0,1));
				else
					materials->SetAmbientColor(ColorRGBA(1,1,1,1));
			}

			g_pShaderAPI->SetDepthRange(0.0f, 1.0f);

			if(frustum_volume.IsBoxInside(	pObject->GetBBoxMins().x,
											pObject->GetBBoxMaxs().x,

											pObject->GetBBoxMins().y,
											pObject->GetBBoxMaxs().y,

											pObject->GetBBoxMins().z,
											pObject->GetBBoxMaxs().z))
				pObject->Render(nViewRenderFlags);
		}
	}
}

void CEditorLevel::HideSelectedObjects()
{
	for(int i = 0; i < m_pEditableList.numElem(); i++)
	{
		if(m_pEditableList[i]->GetFlags() & EDFL_SELECTED)
			m_pEditableList[i]->AddFlags(EDFL_HIDDEN);
	}

	((CSelectionBaseTool*)g_pSelectionTools[0])->CancelSelection();
}

void CEditorLevel::UnhideAll()
{
	for(int i = 0; i < m_pEditableList.numElem(); i++)
	{
		m_pEditableList[i]->RemoveFlags(EDFL_HIDDEN);
	}

	UpdateAllWindows();
}

/*
void AddSectorLump(int nLump, ubyte *pData, int nDataSize, eqworldhdr_t* pHdr, IFile* pFile)
{
	pHdr->lumps[nLump].data_offset = g_fileSystem->Tell(pFile);
	pHdr->lumps[nLump].data_size = nDataSize;

	g_fileSystem->Write(pData, 1, nDataSize, pFile);
}

// writes key-values section.
extern void	KVWriteSection_r(kvsection_t* sec, IFile* pFile, int depth, bool useDepthTabs = true);

void AddKeyValuesSectorLump(int nLump, KeyValues &kv, eqworldhdr_t* pHdr, DKFILE* pFile)
{
	int nDataSize = 0;

	pHdr->lumps[nLump].data_offset = g_fileSystem->Tell(pFile);

	// write, without tabs
	KVWriteSection_r(kv.GetRootSection(), pFile, 0, false);

	nDataSize = g_fileSystem->Tell(pFile) - pHdr->lumps[nLump].data_offset;
	pHdr->lumps[nLump].data_size = nDataSize;
}
*/

void CEditorLevel::BuildWorld()
{
	if(!g_editormainframe->SavePrompt())
		return;

	if(!IsSavedOnDisk())
	{
		MsgError("ERROR: World is not saved on disk.\n  Save it first\n");
		wxMessageBox(wxT("ERROR: World is not saved on disk.\n  Save it first\n"), wxT("ERROR"), wxOK | wxCENTRE | wxICON_ERROR);
		return;
	}

	int nResult = g_editormainframe->GetBuildOptionsDialog()->ShowModal();
	if(nResult != wxID_OK)
	{
		Msg("Building cancelled\n");
		return;
	}

	EqString leveldir(EqString("worlds/")+m_szLevelName);

	// make a file	
	EqString game_world_file(leveldir + "/" + m_szLevelName + ".world");

	// make level build data
	//level_build_data_t builddata;

	int nBrushCount = 0;
	for(int i = 0; i < m_pEditableList.numElem(); i++)
	{
		if(m_pEditableList[i]->GetType() == EDITABLE_BRUSH)
			nBrushCount++;
	}

	if(nBrushCount > 3000)
	{
		int result = wxMessageBox("Warning! There is too much brushes on your level\n and this can be result of freezing or crash of the editor.\n\nDo you want to continue?", "Warning", wxICON_WARNING | wxYES | wxNO, g_editormainframe);

		if(result == wxNO)
		{
			Msg("Building cancelled\n");
			return;
		}
	}

	Msg("Validating...\n");

	int nWorldInfoPresent = 0;
	bool isAnyRoomPresent = false;

	for(int i = 0; i < m_pEditableList.numElem(); i++)
	{
		if(m_pEditableList[i]->GetType() == EDITABLE_ENTITY)
		{
			CEditableEntity* pEntity = (CEditableEntity*)m_pEditableList[i];
			if(!stricmp(pEntity->GetClassname(), "worldinfo"))
				nWorldInfoPresent++;
		}

		for(int j = 0; j < m_pEditableList[i]->GetSurfaceTextureCount(); j++)
		{
			IMatVar* pVar = m_pEditableList[i]->GetSurfaceTexture(j)->pMaterial->FindMaterialVar("roomfiller");
			if(pVar && pVar->GetInt() > 0)
				isAnyRoomPresent = true;
		}
	}

	if(!isAnyRoomPresent)
	{
		MsgError("World validation failed! You must add brushes with material 'tools/room'\n");
		wxMessageBox(wxT("World validation failed! You must add brushes with material 'tools/room'"), wxT("ERROR"), wxOK | wxCENTRE | wxICON_ERROR);
		return;
	}

	if(nWorldInfoPresent == 0)
	{
		MsgError("World validation failed!\nCannot find entity with class 'worldinfo'\n");
		wxMessageBox(wxT("World validation failed!\n Cannot find entity with class 'worldinfo'"), wxT("ERROR"), wxOK | wxCENTRE | wxICON_ERROR);
		return;
	}
	else if(nWorldInfoPresent > 1)
	{
		MsgWarning("Warning: Too many worldinfos on the map\n");
	}

	// build EQWC arguments
	EqString args;

	args.Append(varargs("-game \"%s\" ", g_fileSystem->GetCurrentGameDirectory()));

	args.Append(varargs("-world \"%s\" ", g_pLevel->GetLevelName()));
	args.Append("-waitafterbuild ");
	
	if(!g_editormainframe->GetBuildOptionsDialog()->IsBrushCSGEnabled())
		args.Append("-nocsg ");

	if(g_editormainframe->GetBuildOptionsDialog()->IsOnlyEntitiesMode())
		args.Append("-onlyents ");

	if(g_editormainframe->GetBuildOptionsDialog()->IsPhysicsOnly())
		args.Append("-onlyphysics ");

	args.Append(varargs("-sectorsize %d ", g_editormainframe->GetBuildOptionsDialog()->GetSectorBoxSize()));

	if(g_editormainframe->GetBuildOptionsDialog()->IsLightmapsDisabled())
		args.Append("-nolightmap ");
	else
	{
		args.Append(varargs("-lightmapsize %d ", g_editormainframe->GetBuildOptionsDialog()->GetLightmapSize()));
		args.Append(varargs("-luxelspermeter %g ", g_editormainframe->GetBuildOptionsDialog()->GetLightmapLuxelsPerMeter()));
	}

	int result = system(varargs("bin32\\eqwc.exe %s", args.GetData()));

	if(result == 1)
	{
		wxMessageBox("Unable to start Equilibrium World Compiler:\n\n"
					"  - Make sure the SDK is correctly installed\n"
					"  - Make sure the eqwc.exe is in Bin32 directory", "Warning", 5L, g_editormainframe);
	}
	else
	{
		if(result != 0)
		{
			wxMessageBox("World Compiler error! Cannot continue world compilation!\n\n"
						 "Check eqwc_<username>.log for details.", "Warning", 5L, g_editormainframe);
			return;
		}

		if(!g_editormainframe->GetBuildOptionsDialog()->IsLightmapsDisabled() && !g_editormainframe->GetBuildOptionsDialog()->IsPhysicsOnly())
		{
			EqString lc_args;
			lc_args.Append(varargs("-game \"%s\" ", g_fileSystem->GetCurrentGameDirectory()));
			lc_args.Append(varargs("-world %s ", g_pLevel->GetLevelName()));

			system(varargs("bin32\\eqlc.exe %s", lc_args.GetData()));
		}
	}

	Msg("Compiler exit code: %d\n", result);
}

void CEditorLevel::Undo()
{
	if(m_pUndoActionList.numElem() == 0)
		return;

	int nLast = m_pUndoActionList.numElem() - 1;
	if(nLast < 0)
		return;

	((CVertexTool*)g_pSelectionTools[Tool_VertexManip])->ClearSelection();

	CObjectAction* pAction = m_pUndoActionList[nLast];

	if(pAction->IsObjectCreated())
	{
		// remove all objects
		for(int i = 0; i < pAction->m_objectIDS.numElem(); i++)
		{
			int nID = pAction->m_objectIDS[i];
			CBaseEditableObject* pObject = GetEditableByUniqueID(nID);
			RemoveEditable(pObject);
		}

		delete pAction;
		m_pUndoActionList.removeIndex(nLast);

		g_editormainframe->GetEntityListDialog()->RefreshObjectList();
		g_editormainframe->GetGroupListDialog()->RefreshObjectList();

		UpdateAllWindows();
		return;
	}

	DkList<CBaseEditableObject*> pObjects;

	LoadObjects(pAction->GetStream(), pObjects);

	for(int i = 0; i < pObjects.numElem(); i++)
	{
		int nID = pAction->m_objectIDS[i];

		CBaseEditableObject* pObject = GetEditableByUniqueID(nID);

		pObjects[i]->m_id = nID;

		bool bSelect = false;

		// do replacement function
		if(pObject)
		{
			if(pObject->GetFlags() & EDFL_SELECTED)
				bSelect = true;

			s_objectid_increment--;
			RemoveEditable(pObject);
		}

		//pNewAction->AddObject(pObjects[i]);
		m_pEditableList.append(pObjects[i]);

		if(bSelect)
		{
			((CSelectionBaseTool*)g_pSelectionTools[0])->SetSelectionState(SELECTION_PREPARATION);
			((CSelectionBaseTool*)g_pSelectionTools[0])->SelectObjects(true, &pObjects[i], 1);
		}
	}

	delete pAction;
	m_pUndoActionList.removeIndex(nLast);

	UpdateAllWindows();

	g_editormainframe->GetEntityListDialog()->RefreshObjectList();
	g_editormainframe->GetGroupListDialog()->RefreshObjectList();
}