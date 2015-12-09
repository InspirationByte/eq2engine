//-=-=-=-=-=-=-=-=-=-= Copyright (C) Damage Byte L.L.C 2009-2012 =-=-=-=-=-=-=-=-=-=-=-
//
// Description: Equilibrium Engine Editor main cycle
//
//-=-=-=-D-=-A-=-M-=-A-=-G-=-E-=-=-B-=-Y-=-T-=-E-=-=-S-=-T-=-U-=-D-=-I-=-O-=-S-=-=-=-=-

#include <windows.h>
#include <commdlg.h>

#include "EditorHeader.h"
#include "Sys_Radiant.h"
#include "..\Renderers\IRenderLibrary.h"
#include "EngineModelLoader.h"
#include "DebugOverlay.h"
#include "math/math_util.h"

#include "scriptcompile.h"

DECLARE_CVAR_NONSTATIC(r_wireframe,0,Wireframe,0);
DECLARE_CVAR_NONSTATIC(__cheats,1,Wireframe,0);

// The main engine class pointers
IShaderAPI *g_pShaderAPI = NULL;
IMaterialSystem *materials = NULL;

static CDebugOverlay g_DebugOverlays;
IDebugOverlay *debugoverlay = ( IDebugOverlay * )&g_DebugOverlays;

bool g_bInitialized = false;

mxWindow* main_window = NULL;

void Rad_FirstRunModal()
{
	const char* fRunLoc = GetLocalizer()->GetTokenString("EDIT_FIRSTRUNQESTION", "This is your first run. Do you want to configure the editor settings?");
	if(mxMessageBox(NULL, fRunLoc, "First run question", MX_MB_YESNO) == 0)
	{
		const char* fRun2Loc = GetLocalizer()->GetTokenString("EDIT_OPTIMAL_SETUP", "Editor uses default settings for first run.");
		mxMessageBox(NULL, fRun2Loc, "First run info", MX_MB_OK);
	}
}

IEngineModel* pLoadedModel = NULL;
sceneinfo_t scinfo;
CViewParams pParams(Vector3D(0,0,-100), vec3_zero, 90);

Matrix4x4 bonematrices[128];

void AddModel(IEngineModel* model)
{
	// TODO: animation support
	for(int i = 0; i < model->GetHWData()->pStudioHdr->numbones; i++)
	{
		bonematrices[i] = model->GetHWData()->joints[i].absTrans;
	}

	// setup each bone's transformation
	for(int i = 0; i < model->GetHWData()->pStudioHdr->numbones; i++)
	{
		if(model->GetHWData()->joints[i].parentbone != -1)
		{
			debugoverlay->Line3D(bonematrices[i].rows[3].xyz(),bonematrices[model->GetHWData()->joints[i].parentbone].rows[3].xyz(), color4_white, color4_white);
		}
	}
}

float g_realtime = 0;
float g_oldrealtime = 0;
float g_frametime = 0;

#define ACTION_FILE_LOADMODEL	1101
#define ACTION_FILE_LOADSCRIPT	1102
#define ACTION_FILE_SAVE		1103
#define ACTION_FILE_EXIT		1104

#define ACTION_VIEW_RESET		1111
#define ACTION_VIEW_AXISTOGGLE	1112
#define ACTION_VIEW_PHYSTOGGLE	1113
#define ACTION_VIEW_TBNTOGGLE	1114

Vector3D camera_rotation(25,225,0);
Vector3D camera_target(0);
float vdistance = 100;
bool draw_axis = true;
bool draw_physmodel = false;
bool move_cam_forward = false;
bool tbn_mode = false;
bool normals_only = true;

bool savemodel = false;

float old_x = 0, old_y = 0;

void OpenModel();
void OpenScript();
void SaveModel();

void FlushCache()
{
	g_pModelCache->ReleaseCache();

	materials->FreeMaterials(true);

	g_pModelCache->PrecacheModel("models/error.egf");
}

void ResetView()
{
	camera_rotation = Vector3D(25,225,0);
	camera_target = (pLoadedModel->GetBBoxMins() + pLoadedModel->GetBBoxMaxs()) * 0.5f;

	vdistance = length(pLoadedModel->GetBBoxMaxs())*1.25f;
}

int window_size_x = 1024;
int window_size_y = 768;

void RenderPhysModel();
void RenderTBN();

typedef void (*MOD_MENU_CALL)( mxMenu* pMenu, int nAction );

enum MENUMODIFIER_TYPE_E
{
	MENUMOD_TOGGLE = 0,	// toggle modifier
	MENUMOD_CALLBACK		// custom callback.
};

struct menumodifier_t
{
	MENUMODIFIER_TYPE_E type;

	mxMenu* menu;

	int action_id;

	union
	{
		bool*			boolValue;
		MOD_MENU_CALL	callback;
	};
};

void ShowFPS()
{
	// FPS status
	static float accTime = 0.1f;
	static int fps = 0;
	static int nFrames = 0;

	if (accTime > 0.1f)
	{
		fps = (int) (nFrames / accTime + 0.5f);
		nFrames = 0;
		accTime = 0;
	}

	accTime += g_frametime;
	nFrames++;

	ColorRGBA col(1,0,0,1);

	if(fps > 25)
	{
		col = ColorRGBA(1,1,0,1);
		if(fps > 45)
			col = ColorRGBA(0,1,0,1);
	}

	debugoverlay->Text(col, "FPS: %d", fps);
}

class CCustomEventWindow : public mxWindow
{
protected:
	mxMenuBar*					m_pMainMenu;

	DkList<mxMenu*>				m_MenuList;
	DkList<menumodifier_t>		m_menuModifiers;

	int CreateMenu(const char* pszName)
	{
		mxMenu* new_menu = new mxMenu;
		m_pMainMenu->addMenu(pszName, new_menu);

		return m_MenuList.append(new_menu);
	}

	void AddMenuItemWithModifier(const char* pszName, int menu_id, MENUMODIFIER_TYPE_E type, void* data)
	{
		int modifier_event_id = 1001 + m_menuModifiers.numElem();

		// add menu item
		GetMenuById(menu_id)->add(pszName, modifier_event_id);

		menumodifier_t mod;
		mod.type = type;
		mod.menu = GetMenuById(menu_id);
		mod.action_id = modifier_event_id;
		
		switch(mod.type)
		{
			case MENUMOD_TOGGLE:
				mod.boolValue = (bool*)data;

				GetMenuById(menu_id)->setChecked(modifier_event_id, *mod.boolValue);

				break;
			case MENUMOD_CALLBACK:
				mod.callback = static_cast<MOD_MENU_CALL>(data);
				break;
		}

		m_menuModifiers.append(mod);
	}

	int FindModifierByAction(int action_id)
	{
		for(int i = 0; i < m_menuModifiers.numElem(); i++)
		{
			if(m_menuModifiers[i].action_id == action_id)
				return i;
		}

		return -1;
	}

	void HandleModifier(int action_id)
	{
		int mod_id = FindModifierByAction(action_id);

		if(mod_id == -1)
			return;

		menumodifier_t *mod = &m_menuModifiers[mod_id];

		switch(mod->type)
		{
			case MENUMOD_TOGGLE:
			{
				bool value = *mod->boolValue;

				value = !value;

				mod->menu->setChecked(action_id, value);

				*mod->boolValue = value;

				break;
			}
			case MENUMOD_CALLBACK:
			{
				MOD_MENU_CALL callback = mod->callback;
				(callback)(mod->menu, action_id);
				break;
			}
		}
	}

public:

	mxMenu* GetMenuById(int id)
	{
		return m_MenuList[id];
	}

	CCustomEventWindow(mxWindow *parent, int x, int y, int w, int h, const char *label = 0, int style = 0) : mxWindow(parent,x,y,w,h,label,style)
	{
		m_pMainMenu = new mxMenuBar(this);

		InitializeControls();
	}

	void InitializeControls()
	{
		int fileMenu = CreateMenu(GetLocalizer()->GetTokenString("TOKEN_FILE", "File"));
		int viewMenu = CreateMenu(GetLocalizer()->GetTokenString("TOKEN_VIEW", "View"));

		GetMenuById(fileMenu)->add(GetLocalizer()->GetTokenString("TOKEN_OPENESC", "Compile .ESC/.ASC script"), ACTION_FILE_LOADSCRIPT);
		GetMenuById(fileMenu)->addSeparator();
		GetMenuById(fileMenu)->add(GetLocalizer()->GetTokenString("TOKEN_OPENEGF", "Load EGF"), ACTION_FILE_LOADMODEL);
		GetMenuById(fileMenu)->addSeparator();
		GetMenuById(fileMenu)->add(GetLocalizer()->GetTokenString("TOKEN_SAVEMODEL", "Save EGF"), ACTION_FILE_SAVE);
		GetMenuById(fileMenu)->addSeparator();
		GetMenuById(fileMenu)->add(GetLocalizer()->GetTokenString("TOKEN_EXIT", "Exit"), ACTION_FILE_EXIT);

		GetMenuById(viewMenu)->add(GetLocalizer()->GetTokenString("TOKEN_RESETCAM", "Reset camera"), ACTION_VIEW_RESET);

		AddMenuItemWithModifier(GetLocalizer()->GetTokenString("TOKEN_TOGGLEWORLDCENTER", "Draw world center"), viewMenu, MENUMOD_TOGGLE, &draw_axis);
		AddMenuItemWithModifier(GetLocalizer()->GetTokenString("TOKEN_TOGGLEPHYSMODEL", "Draw physics model"), viewMenu, MENUMOD_TOGGLE, &draw_physmodel);
		AddMenuItemWithModifier(GetLocalizer()->GetTokenString("TOKEN_TOGGLECAMMOVE", "Forward camera movement by RMB"), viewMenu, MENUMOD_TOGGLE, &move_cam_forward);
		AddMenuItemWithModifier(GetLocalizer()->GetTokenString("TOKEN_TOGGLETBN", "Toggle TBN"), viewMenu, MENUMOD_TOGGLE, &tbn_mode);
		AddMenuItemWithModifier(GetLocalizer()->GetTokenString("TOKEN_NORMALSONLY", "Normals only"), viewMenu, MENUMOD_TOGGLE, &normals_only);
		
	}

	int handleEvent(mxEvent *event)
	{
		switch (event->event)
		{
			case mxEvent::Action:
			{
				switch(event->action)
				{
					case ACTION_FILE_LOADMODEL:
					{
						OpenModel();
						break;
					}
					case ACTION_FILE_LOADSCRIPT:
					{
						OpenScript();
						break;
					}
					case ACTION_FILE_SAVE:
					{
						savemodel = true;
						SaveModel();
						savemodel = false;
						break;
					}
					case ACTION_VIEW_RESET:
					{
						ResetView();
						break;
					}
					case ACTION_FILE_EXIT:
					{
						mx::quit();
						break;
					}
					default:
					{
						HandleModifier(event->action);
					}
				}
				return 1;
			}
			break;

			case mxEvent::MouseDown:
			{
				old_x = event->x;
				old_y = event->y;

				return 1;
			}
			break;

			case mxEvent::MouseDrag:
			{
				if (event->buttons & mxEvent::MouseLeftButton)
				{
					camera_rotation.y += (old_x - event->x) * 0.5f;
					camera_rotation.x += (event->y - old_y) * 0.5f;

					old_x = event->x;
					old_y = event->y;
				}

				if (event->buttons & mxEvent::MouseRightButton)
				{
					if(move_cam_forward)
					{
						Vector3D forward;
						AngleVectors(camera_rotation, &forward);

						camera_target += forward * (old_y - event->y) * 0.5f;
					}
					else
						vdistance += (event->y - old_y) * 0.5f;

					//camera_rotation.z += (old_x - event->x) * 0.5f;

					old_x = event->x;
					old_y = event->y;
				}

				if (event->buttons & mxEvent::MouseMiddleButton)
				{
					Vector3D right, up;
					AngleVectors(camera_rotation, NULL, &right, &up);

					camera_target += right * (old_x - event->x) * 0.5f;
					camera_target += up * (event->y - old_y) * 0.5f;

					old_x = event->x;
					old_y = event->y;
				}

				redraw();
				break;
			}

			case mxEvent::Idle:
			{
				redraw();
				break;
			}
		}

		return 1;
	}

	void redraw()
	{
		if(!g_bInitialized)
			return;

		HWND handle = (HWND)getHandle();

		RECT rectangle;
		GetClientRect(handle, &rectangle);
		int winWide = rectangle.right;
		int winTall = rectangle.bottom;

		if(winWide != window_size_x || winTall != window_size_y)
		{
			materials->SetDeviceBackbufferSize(winWide, winTall);

			window_size_x = winWide;
			window_size_y = winTall;
		}

		// compute time
		g_realtime = Platform_GetCurrentTime();
		g_frametime = g_realtime - g_oldrealtime;
		g_oldrealtime = g_realtime;

		PAINTSTRUCT paint;
		BeginPaint(handle, &paint);
		EndPaint(handle, &paint);

		if(draw_axis)
		{
			// draw axe
			debugoverlay->Line3D(vec3_zero, Vector3D(10,0,0), ColorRGBA(0,0,0,1), ColorRGBA(1,0,0,1));
			debugoverlay->Line3D(vec3_zero, Vector3D(0,10,0), ColorRGBA(0,0,0,1), ColorRGBA(0,1,0,1));
			debugoverlay->Line3D(vec3_zero, Vector3D(0,0,10), ColorRGBA(0,0,0,1), ColorRGBA(0,0,1,1));
		}

		// draw axe
		debugoverlay->Line3D(Vector3D(-10,0,0) + camera_target, Vector3D(10,0,0) + camera_target, ColorRGBA(1,1,0,1), ColorRGBA(1,1,0,1));
		debugoverlay->Line3D(Vector3D(0,-10,0) + camera_target, Vector3D(0,10,0) + camera_target, ColorRGBA(1,1,0,1), ColorRGBA(1,1,0,1));
		debugoverlay->Line3D(Vector3D(0,0,-10) + camera_target, Vector3D(0,0,10) + camera_target, ColorRGBA(1,1,0,1), ColorRGBA(1,1,0,1));

		//debugoverlay->Box3D(camera_target - Vector3D(10,10,10), camera_target + Vector3D(10,10,10),  ColorRGBA(0,1,0,1));

		// reset scene renderer
		g_pShaderAPI->Reset();

		// update material system and proxies
		materials->Update(g_frametime);

		// setup scene info
		scinfo.m_fZNear = 1;
		scinfo.m_fZFar = 1000;
		scinfo.m_cAmbientColor = Vector3D(0.25f);

		// add model to scene
		AddModel(pLoadedModel);

		if(materials->BeginFrame())
		{
			g_pShaderAPI->Clear( true, true, false, ColorRGBA(0.25));

			Vector3D forward, right;
			AngleVectors(camera_rotation, &forward, &right);

			pParams.SetAngles(camera_rotation);
			pParams.SetOrigin(camera_target + forward*-vdistance);

			debugoverlay->Text(color4_white, "Camera position: %g %g %g\n", camera_target.x,camera_target.y,camera_target.z);


			FogInfo_t fog;
			materials->GetFogInfo(fog);

			fog.viewPos = pParams.GetOrigin();

			materials->SetFogInfo(fog);

			g_pShaderAPI->SetupFog(&fog);

			// setup perspective
			materials->SetupProjection(window_size_x, window_size_y, pParams.GetFOV(), 1, 5000);

			Matrix4x4 viewMatrix = rotateZXY(DEG2RAD(-pParams.GetAngles().x),DEG2RAD(-pParams.GetAngles().y),DEG2RAD(-pParams.GetAngles().z));
			viewMatrix.translate(-pParams.GetOrigin());
			materials->SetMatrix(MATRIXMODE_VIEW, viewMatrix);

			materials->SetMatrix(MATRIXMODE_WORLD, identity4());

			/*
			// Draw model here
			for(int i = 0; i < pLoadedModel->GetHWData()->pStudioHdr->pModelDesc(0)->numgroups; i++)
			{
				IMaterial* pMaterial = pLoadedModel->GetMaterial(0, i);
				materials->BindMaterial(pMaterial, false);

				pLoadedModel->DrawGroup(0, i, 0);
			}
			*/

			studiohdr_t* pHdr = pLoadedModel->GetHWData()->pStudioHdr;

			int nActualLOD = pLoadedModel->SelectLod(length(pParams.GetOrigin()));

			for(int i = 0; i < pHdr->numbodygroups; i++)
			{
				int nLod = nActualLOD;

				int nLodableModelIndex = pHdr->pBodyGroups(i)->lodmodel_index;
				int nModDescId = pHdr->pLodModel(nLodableModelIndex)->lodmodels[nLod];

				while(nLod > 0 && nModDescId != -1)
				{
					nLod--;
					nModDescId = pHdr->pLodModel(nLodableModelIndex)->lodmodels[nLod];
				}

				if(nModDescId == 1)
					continue;

				for(int j = 0; j < pHdr->pModelDesc(nModDescId)->numgroups; j++)
				{
					IMaterial* pMaterial = pLoadedModel->GetMaterial(nModDescId, j);
					materials->BindMaterial(pMaterial, false);

					pLoadedModel->PrepareForSkinning(bonematrices);

					pLoadedModel->DrawGroup(nModDescId, j);
				}
			}

			// draw physics model
			RenderPhysModel();

			// draw TBN
			RenderTBN();

			debugoverlay->Text(color4_white, "Primitives: %d\n", g_pShaderAPI->GetTrianglesCount());
			debugoverlay->Text(color4_white, "DIPs: %d\n", g_pShaderAPI->GetDrawIndexedPrimitiveCallsCount());

			ShowFPS();

			Matrix4x4 proj;
			materials->GetMatrix(MATRIXMODE_PROJECTION, proj);

			debugoverlay->Draw(proj, viewMatrix, window_size_x,window_size_y);

			materials->EndFrame();

			g_pShaderAPI->ResetCounters();
		}
	}
};

void RenderPhysModel()
{
	if(!draw_physmodel || pLoadedModel->GetHWData()->m_physmodel.numobjects == 0)
		return;

	g_pShaderAPI->Reset();
	materials->SetDepthStates(true,false);
	materials->SetRasterizerStates(CULL_BACK,FILL_WIREFRAME);
	g_pShaderAPI->Apply();

	IMeshBuilder* mesh = g_pShaderAPI->CreateMeshBuilder();

	physmodeldata_t* phys_data = &pLoadedModel->GetHWData()->m_physmodel;

	for(int i = 0; i < phys_data->numobjects; i++)
	{
		for(int j = 0; j < phys_data->objects[i].numShapes; j++)
		{
			int nShape = phys_data->objects[i].shape_indexes[j];

			if(nShape < 0 || nShape > phys_data->numshapes)
				Msg("MEMFAULT!\n");

			int startIndex = phys_data->shapes[nShape].shape_info.startIndices;
			int moveToIndex = startIndex + phys_data->shapes[nShape].shape_info.numIndices;

			mesh->Begin(PRIM_TRIANGLES);
			for(int k = startIndex; k < moveToIndex; k++)
			{
				mesh->Color4f(0,1,1,1);
				mesh->Position3fv(phys_data->vertices[phys_data->indices[k]] + phys_data->objects[i].offset);

				mesh->AdvanceVertex();
			}
			mesh->End();
		}
	}

	g_pShaderAPI->DestroyMeshBuilder(mesh);
}

void RenderTBN()
{
	if(!tbn_mode)
		return;
	
	studiohdr_t* pHdr = pLoadedModel->GetHWData()->pStudioHdr;

	int nActualLOD = pLoadedModel->SelectLod(length(pParams.GetOrigin()));

	for(int i = 0; i < pHdr->numbodygroups; i++)
	{
		int nLod = nActualLOD;

		int nLodableModelIndex = pHdr->pBodyGroups(i)->lodmodel_index;
		int nModDescId = pHdr->pLodModel(nLodableModelIndex)->lodmodels[nLod];

		while(nLod > 0 && nModDescId != -1)
		{
			nLod--;
			nModDescId = pHdr->pLodModel(nLodableModelIndex)->lodmodels[nLod];
		}

		if(nModDescId == -1)
			continue;

		for(int j = 0; j < pHdr->pModelDesc(nModDescId)->numgroups; j++)
		{
			modelgroupdesc_t* pGroup = pHdr->pModelDesc(nModDescId)->pGroup(j);

			// render tbn
			for(int k = 0; k < pGroup->numvertices; k++)
			{
				Vector3D pos = pGroup->pVertex(k)->point;

				if(!normals_only)
				{
					debugoverlay->Line3D(pos, pos + pGroup->pVertex(k)->tangent, ColorRGBA(1,0,0,1), ColorRGBA(1,0,0,1), 0);
					debugoverlay->Line3D(pos, pos + pGroup->pVertex(k)->binormal, ColorRGBA(0,1,0,1), ColorRGBA(0,1,0,1), 0);
				}

				debugoverlay->Line3D(pos, pos + pGroup->pVertex(k)->normal, ColorRGBA(0,0,1,1), ColorRGBA(0,0,1,1), 0);
			}
		}
	}
}

void SaveModel()
{
	if(savemodel)
	{
		int result = mxMessageBox(NULL, GetLocalizer()->GetTokenString("TOKEN_ON_SAVE", "Are you sure?"), "Question", MX_MB_YESNO | MX_MB_QUESTION);
		if(result == 0)
		{
			DKFILE *file = GetFileSystem()->Open(pLoadedModel->GetHWData()->pStudioHdr->modelname, "wb", SP_MOD);
			if(file)
			{
				// write model
				GetFileSystem()->Write(pLoadedModel->GetHWData()->pStudioHdr,pLoadedModel->GetHWData()->pStudioHdr->length,1,file);

				GetFileSystem()->Close(file);
			}
			savemodel = false;
		}
	}
}

void OpenModel()
{
	SaveModel();

	//
	// choose filename
	//
	OPENFILENAME ofn;
	memset (&ofn, 0, sizeof (OPENFILENAME));
    
	char szFile[MAX_PATH];
	char szFileTitle[MAX_PATH];
	char szDefExt[32] = "egf";
	char szFilter[80] = "Equilibrium Model Format File (*.EGF)\0*.egf\0All Files (*.*)\0*.*\0\0";
	szFile[0] = '\0';
	szFileTitle[0] = '\0';

	ofn.lStructSize = sizeof (OPENFILENAME);
	ofn.lpstrDefExt = szDefExt;
	ofn.lpstrFilter = szFilter;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFileTitle = szFileTitle;
	ofn.nMaxFileTitle = MAX_PATH;
	ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
	ofn.lpstrTitle = "Open Equilibrium Model Format file";

	if (!::GetOpenFileName(&ofn))
		return;

	FlushCache();

	int cache_index = g_pModelCache->PrecacheModel(szFile);
	if(cache_index == 0)
	{
		ErrorMsg(varargs("Can't open %s\n", szFile));
	}

	pLoadedModel = g_pModelCache->GetModel(cache_index);

	materials->PreloadNewMaterials();

	savemodel = false;

	ResetView();
}

EqString g_model_aftercomp_name;

void OpenScript()
{
	SaveModel();

	//
	// choose filename
	//
	OPENFILENAME ofn;
	memset (&ofn, 0, sizeof (OPENFILENAME));
    
	char szFile[MAX_PATH];
	char szFileTitle[MAX_PATH];
	char szDefExt[32] = "esc";
	char szFilter[80] = "Script (*.ESC, *.ASC)\0*.esc;*.asc\0All Files (*.*)\0*.*\0\0";
	szFile[0] = '\0';
	szFileTitle[0] = '\0';

	ofn.lStructSize = sizeof (OPENFILENAME);
	ofn.lpstrDefExt = szDefExt;
	ofn.lpstrFilter = szFilter;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFileTitle = szFileTitle;
	ofn.nMaxFileTitle = MAX_PATH;
	ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
	ofn.lpstrTitle = "Open Equilibrium Script file";

	if (!::GetOpenFileName(&ofn))
		return;

	KeyValues script;

	EqString fname(szFile);
	EqString ext = fname.ExtractFileExtension();

	if(!stricmp(ext.GetData(), "asc"))
	{
		EqString cmdLine(varargs("bin32\\animca.exe +filename \"%s\"", szFile));
		Msg("***Command line: %s\n", cmdLine.GetData());
		system( cmdLine.GetData() );
	}
	else
	{
		if(script.LoadFromFile(szFile))
		{
			// load all script data
			kvkeybase_t* mainsection = script.GetRootSection();
			if(mainsection)
			{
				kvkeybase_t* pPair = mainsection->FindKeyBase("modelfilename");

				if(pPair)
				{
					g_model_aftercomp_name = pPair->values[0];
					Msg("***Starting egfca for %s\n", szFile);

					EqString cmdLine(varargs("egfca.exe +filename \"%s\"", szFile));

					if( GetFileSystem()->FileExist("bin32\\egfca.exe") )
					{
						cmdLine = "bin32\\" + cmdLine;
					}

					Msg("***Command line: %s\n", cmdLine.GetData());
					system( cmdLine.GetData() );
				}
				else
				{
					ErrorMsg(varargs("ERROR! 'modelfilename' isn't specified in the script!!!\n"));
				}
			}
		}

	

		FlushCache();

		int cache_index = g_pModelCache->PrecacheModel(g_model_aftercomp_name.GetData());
		if(cache_index == 0)
		{
			ErrorMsg(varargs("Can't open %s\n", g_model_aftercomp_name.GetData()));
		}

		pLoadedModel = g_pModelCache->GetModel(cache_index);

		materials->PreloadNewMaterials();

		camera_target = (pLoadedModel->GetBBoxMins() + pLoadedModel->GetBBoxMaxs()) * 0.5f;

		savemodel = false;

		ResetView();
	}
}

void Rad_CreateWindow()
{
	main_window = new CCustomEventWindow(NULL, 0, 0, 0, 0, GetLocalizer()->GetTokenString("TOKEN_TITLE", "Equilibrium Model Viewer/Compiler"), mxWindow::Normal);
	main_window->setEnabled(true);
	main_window->setVisible(true);

	main_window->setBounds(45,45, window_size_x,window_size_y);

	HWND handle = (HWND)main_window->getHandle();

	UpdateWindow(handle);

	materials = (IMaterialSystem*)GetCore()->GetInterface(MATSYSTEM_INTERFACE_VERSION);
	if(!materials)
	{
		ErrorMsg("ERROR! Couldn't get interface of EqMatSystem!");
		exit(0);
	}
	else
	{
		int bpp = 32;

		FORMAT format = FORMAT_RGB8;

		// Figure display format to use
		if(bpp == 32) 
		{
			format = FORMAT_RGB8;
		}
		else if(bpp == 24) 
		{
			format = FORMAT_RGB8;
		}
		else if(bpp == 16) 
		{
			format = FORMAT_RGB565;
		}

		matsystem_render_config_t materials_config;

		materials_config.enableBumpmapping = false;
		materials_config.enableSpecular = true;
		materials_config.enableShadows = false;
		materials_config.wireframeMode = false;
		materials_config.threadedloader = false;

		materials_config.ffp_mode = false;
		materials_config.lighting_model = MATERIAL_LIGHT_FORWARD;
		materials_config.stubMode = false;
		materials_config.editormode = true;

		DefaultShaderAPIParameters(&materials_config.shaderapi_params);
		materials_config.shaderapi_params.bIsWindowed = true;
		materials_config.shaderapi_params.hWindow = handle;
		materials_config.shaderapi_params.nScreenFormat = format;
		materials_config.shaderapi_params.bEnableVerticalSync = true;

		bool materialSystemStatus = materials->Init("materials/", "EqShaderAPID3DX9", materials_config);

		g_pShaderAPI = materials->GetShaderAPI();

		if(!materialSystemStatus)
			return;
	}

	materials->SetLightingModel(MATERIAL_LIGHT_UNLIT);
	materials->LoadShaderLibrary("Shaders_Engine.dll");

	g_pModelCache->PrecacheModel("models/error.egf");
	pLoadedModel = g_pModelCache->GetModel(0);

	ResetView();

	mx::setIdleWindow(main_window);
}

void Sys_InitRadiant()
{
	GetLocalizer()->AddTokensFile("egf_view");
	GetCmdLine()->ExecuteCommandLine(true,true);

	Platform_InitTime();

	Rad_CreateWindow();

	debugoverlay->Init();

	g_bInitialized = true;
}