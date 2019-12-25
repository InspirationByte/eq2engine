//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Region editor frame
//////////////////////////////////////////////////////////////////////////////////

#include "RegionEditFrame.h"
#include "EditorMain.h"
#include "materialsystem/MeshBuilder.h"

#include "world.h"
#include "imaging/ImageLoader.h"

enum ERegionEditMenuCommands
{
	REdit_MenuBegin = 1000,

	REdit_GenerateMap,

	REdit_MenuEnd,
};

regionMap_t::regionMap_t()
{
	region = nullptr;
	image = nullptr;
	aiMapImage = nullptr;

	selected = false;
	state = REGION_MOVE_NONE;

	swapTemp.m_level = &g_pGameWorld->m_level;
}

BEGIN_EVENT_TABLE(CRegionEditFrame, wxFrame)
	EVT_MENU_RANGE(REdit_MenuBegin, REdit_MenuEnd, ProcessAllMenuCommands)
END_EVENT_TABLE()

CRegionEditFrame::CRegionEditFrame( wxWindow* parent ) : 
	wxFrame( parent, wxID_ANY, "Region editor and level navigation", wxDefaultPosition, wxSize( 1090,653 ), wxDEFAULT_FRAME_STYLE|wxMAXIMIZE|wxTAB_TRAVERSAL)
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxVERTICAL );
	
	m_pRenderPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxWANTS_CHARS);
	m_pRenderPanel->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNTEXT ) );
	
	bSizer1->Add( m_pRenderPanel, 1, wxEXPAND, 5 );
	
	Maximize();
	
	this->SetSizer( bSizer1 );
	this->Layout();
	
	this->Centre( wxBOTH );

	m_pMenu = new wxMenuBar( 0 );

	wxMenu* menuFile = new wxMenu;
	
	menuFile->Append( REdit_GenerateMap, DKLOC("TOKEN_GENERATEMAP", L"Generate map image") );
	
	m_pMenu->Append( menuFile, DKLOC("TOKEN_FILE", L"File") );

	this->SetMenuBar( m_pMenu );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( CRegionEditFrame::OnClose ) );
	m_pRenderPanel->Connect( wxEVT_ERASE_BACKGROUND, wxEraseEventHandler( CRegionEditFrame::OnEraseBG ), NULL, this );
	m_pRenderPanel->Connect( wxEVT_PAINT, wxPaintEventHandler( CRegionEditFrame::OnPaint ), NULL, this );
	m_pRenderPanel->Connect( wxEVT_SIZE, wxSizeEventHandler( CRegionEditFrame::OnResizeWin ), NULL, this );
	m_pRenderPanel->Connect( wxEVT_IDLE, wxIdleEventHandler( CRegionEditFrame::OnIdle ), NULL, this );
	m_pRenderPanel->Connect(wxEVT_MOUSEWHEEL, (wxObjectEventFunction)&CRegionEditFrame::ProcessMouseEvents, NULL, this);
	m_pRenderPanel->Connect(wxEVT_LEFT_DCLICK, wxMouseEventHandler(CRegionEditFrame::ProcessMouseEvents), NULL, this);
	m_pRenderPanel->Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(CRegionEditFrame::ProcessMouseEvents), NULL, this);
	m_pRenderPanel->Connect(wxEVT_LEFT_UP, wxMouseEventHandler(CRegionEditFrame::ProcessMouseEvents), NULL, this);
	m_pRenderPanel->Connect(wxEVT_RIGHT_DCLICK, wxMouseEventHandler(CRegionEditFrame::ProcessMouseEvents), NULL, this);
	m_pRenderPanel->Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(CRegionEditFrame::ProcessMouseEvents), NULL, this);
	m_pRenderPanel->Connect(wxEVT_RIGHT_UP, wxMouseEventHandler(CRegionEditFrame::ProcessMouseEvents), NULL, this);
	m_pRenderPanel->Connect(wxEVT_MOTION, (wxObjectEventFunction)&CRegionEditFrame::ProcessMouseEvents, NULL, this);
	m_pRenderPanel->Connect(wxEVT_KEY_DOWN, (wxObjectEventFunction)&CRegionEditFrame::OnKey, NULL, this);
	m_pRenderPanel->Connect(wxEVT_KEY_UP, (wxObjectEventFunction)&CRegionEditFrame::OnKey, NULL, this);
	//m_pRenderPanel->Connect(wxEVT_NAVIGATION_KEY, (wxObjectEventFunction)&CRegionEditFrame::OnKey, NULL, this);

	m_swapChain = materials->CreateSwapChain(m_pRenderPanel->GetHandle());

	m_regionMap = NULL;
	m_regWide = 0;
	m_regTall = 0;
	m_regCells = 0;

	m_doRedraw = true;
	m_zoomLevel = 0.5;
	m_viewPos = vec2_zero;
	m_mouseoverRegion = -1;

	m_showsNavGrid = false;
}

CRegionEditFrame::~CRegionEditFrame()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( CRegionEditFrame::OnClose ) );
	m_pRenderPanel->Disconnect( wxEVT_ERASE_BACKGROUND, wxEraseEventHandler( CRegionEditFrame::OnEraseBG ), NULL, this );
	m_pRenderPanel->Disconnect( wxEVT_PAINT, wxPaintEventHandler( CRegionEditFrame::OnPaint ), NULL, this );
	m_pRenderPanel->Disconnect( wxEVT_SIZE, wxSizeEventHandler( CRegionEditFrame::OnResizeWin ), NULL, this );
	m_pRenderPanel->Disconnect( wxEVT_IDLE, wxIdleEventHandler( CRegionEditFrame::OnIdle ), NULL, this );
	m_pRenderPanel->Disconnect(wxEVT_MOUSEWHEEL, (wxObjectEventFunction)&CRegionEditFrame::ProcessMouseEvents, NULL, this);
	m_pRenderPanel->Disconnect(wxEVT_LEFT_DCLICK, wxMouseEventHandler(CRegionEditFrame::ProcessMouseEvents), NULL, this);
	m_pRenderPanel->Disconnect(wxEVT_LEFT_DOWN, wxMouseEventHandler(CRegionEditFrame::ProcessMouseEvents), NULL, this);
	m_pRenderPanel->Disconnect(wxEVT_LEFT_UP, wxMouseEventHandler(CRegionEditFrame::ProcessMouseEvents), NULL, this);
	m_pRenderPanel->Disconnect(wxEVT_RIGHT_DCLICK, wxMouseEventHandler(CRegionEditFrame::ProcessMouseEvents), NULL, this);
	m_pRenderPanel->Disconnect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(CRegionEditFrame::ProcessMouseEvents), NULL, this);
	m_pRenderPanel->Disconnect(wxEVT_RIGHT_UP, wxMouseEventHandler(CRegionEditFrame::ProcessMouseEvents), NULL, this);
	m_pRenderPanel->Disconnect(wxEVT_MOTION, (wxObjectEventFunction)&CRegionEditFrame::ProcessMouseEvents, NULL, this);
	m_pRenderPanel->Disconnect(wxEVT_KEY_DOWN, (wxObjectEventFunction)&CRegionEditFrame::OnKey, NULL, this);
	m_pRenderPanel->Disconnect(wxEVT_KEY_UP, (wxObjectEventFunction)&CRegionEditFrame::OnKey, NULL, this);
}

void CRegionEditFrame::ProcessAllMenuCommands(wxCommandEvent& event)
{
	switch(event.GetId())
	{
		case REdit_GenerateMap:
		{
			BuildAndSaveMapFromRegionImages();
			break;
		}
	}
}

void CRegionEditFrame::SelectAll()
{
	for (int y = 0; y < m_regTall; y++)
	{
		for (int x = 0; x < m_regWide; x++)
		{
			int regIdx = y * m_regWide + x;

			regionMap_t* rm = &m_regionMap[regIdx];

			if (!rm->region->m_isLoaded)
				continue;

			rm->selected = !rm->selected;
		}
	}
}

void CRegionEditFrame::CancelSelection()
{
	for (int y = 0; y < m_regTall; y++)
	{
		for (int x = 0; x < m_regWide; x++)
		{
			int regIdx = y * m_regWide + x;

			regionMap_t* rm = &m_regionMap[regIdx];
			rm->selected = false;
		}
	}
}

void CRegionEditFrame::MoveRegions(const IVector2D& delta)
{
	// First we have to validate boundaries
	for (int y = 0; y < m_regTall; y++)
	{
		for (int x = 0; x < m_regWide; x++)
		{
			int srcRegIdx = y * m_regWide + x;

			regionMap_t& srcReg = m_regionMap[srcRegIdx];

			// also reset states
			srcReg.state = REGION_MOVE_NONE;

			if (!srcReg.selected)
				continue;

			IVector2D targetPos(x + delta.x, y + delta.y);

			int targetRegIdx = targetPos.y * m_regWide + targetPos.x;
			regionMap_t& targetReg = m_regionMap[targetRegIdx];

			if (srcRegIdx == targetRegIdx)
				break;

			// cancel
			if (!(targetPos.x >= 0 && targetPos.x < m_regWide && targetPos.y >= 0 && targetPos.y < m_regTall))
				return;
		}
	}

	// second we're CUTTING
	for (int y = 0; y < m_regTall; y++)
	{
		for (int x = 0; x < m_regWide; x++)
		{
			int dx = delta.x < 0 ? x : ((m_regWide - 1) - x);
			int dy = delta.y < 0 ? y : ((m_regWide - 1) - y);

			int srcRegIdx = dy * m_regWide + dx;

			regionMap_t& srcReg = m_regionMap[srcRegIdx];

			// skip processed
			if (!srcReg.selected || srcReg.state > REGION_MOVE_NONE)
				continue;

			IVector2D targetPos(dx + delta.x, dy + delta.y);

			int targetRegIdx = targetPos.y * m_regWide + targetPos.x;
			regionMap_t& targetReg = m_regionMap[targetRegIdx];

			if (srcRegIdx == targetRegIdx)
				break;

			srcReg.swapTemp.Init(g_pGameWorld->m_level.m_cellsSize, 0, vec3_zero);

			// swap source to temp
			g_pGameWorld->m_level.Ed_SwapRegions(srcReg.swapTemp, *srcReg.region);

			srcReg.state = REGION_MOVE_MOVED;
			targetReg.state = REGION_MOVE_MOVED;

			QuickSwap(targetReg.image, srcReg.image);
			QuickSwap(targetReg.aiMapImage, srcReg.aiMapImage);
			QuickSwap(targetReg.selected, srcReg.selected);

			// swap temp to destination
			g_pGameWorld->m_level.Ed_SwapRegions(*srcReg.region, *targetReg.region);

			// swap source into destination
			g_pGameWorld->m_level.Ed_SwapRegions(srcReg.swapTemp, *targetReg.region);

			// destructor is called explicitly
			srcReg.swapTemp.~CEditorLevelRegion();

			srcReg.region->m_modified = true;
			targetReg.region->m_modified = true;

			g_pMainFrame->NotifyUpdate();
		}
	}

	ReDraw();
}

void CRegionEditFrame::ClearSelectedRegions()
{
	int result = wxMessageBox("Are you sure to clear selected regions? This can't be undone", "Question", wxYES_NO | wxCENTRE | wxICON_WARNING, this);

	if (result == wxCANCEL || result == wxNO)
		return;

	for (int y = 0; y < m_regTall; y++)
	{
		for (int x = 0; x < m_regWide; x++)
		{
			int srcRegIdx = y * m_regWide + x;

			regionMap_t& srcReg = m_regionMap[srcRegIdx];

			if (!srcReg.selected)
				continue;

			// clear out the region
			srcReg.region->Cleanup();
			srcReg.region->Init(g_pGameWorld->m_level.m_cellsSize, IVector2D(x,y), vec3_zero);
			srcReg.region->m_modified = true;

			srcReg.selected = false;

			g_pShaderAPI->FreeTexture(srcReg.aiMapImage);
			srcReg.aiMapImage = nullptr;

			g_pShaderAPI->FreeTexture(srcReg.image);
			srcReg.image = nullptr;

			g_pMainFrame->NotifyUpdate();
		}
	}

	ReDraw();
}

void CRegionEditFrame::OnKey(wxKeyEvent& event)
{
	if (event.GetEventType() == wxEVT_KEY_DOWN)
	{
		if (event.GetKeyCode() == wxKeyCode::WXK_ESCAPE)
		{
			CancelSelection();
		}
		else if (event.GetRawKeyCode() == 'N')
		{
			m_showsNavGrid = !m_showsNavGrid;
		}
		else if (event.GetKeyCode() == wxKeyCode::WXK_DELETE)
		{
			ClearSelectedRegions();
		}
		else if (event.ControlDown() && event.GetRawKeyCode() == 'A')
		{
			SelectAll();
		}
		else if (event.GetKeyCode() == wxKeyCode::WXK_UP)
		{
			MoveRegions(IVector2D(0,1));
		}
		else if (event.GetKeyCode() == wxKeyCode::WXK_DOWN)
		{
			MoveRegions(IVector2D(0, -1));
		}
		else if (event.GetKeyCode() == wxKeyCode::WXK_LEFT)
		{
			MoveRegions(IVector2D(-1, 0));
		}
		else if (event.GetKeyCode() == wxKeyCode::WXK_RIGHT)
		{
			MoveRegions(IVector2D(1, 0));
		}
	}
}

extern Vector3D g_camera_target;

void CRegionEditFrame::ProcessMouseEvents(wxMouseEvent& event)
{
	if(event.Dragging() && event.RightIsDown())
	{
		wxPoint delta = m_mouseOldPos - event.GetPosition();
		m_viewPos += Vector2D(-delta.x,delta.y)*m_zoomLevel;
	}

	int w, h;
	m_pRenderPanel->GetSize(&w, &h);

	Vector2D point(w-event.GetPosition().x, event.GetPosition().y);

	//float wh = (float)w/(float)h;

	Vector3D start, dir;
	ScreenToDirection(Vector3D(-m_viewPos.x,0,-m_viewPos.y), point, Vector2D(w,h), start, dir, m_viewProj, true);
		
	if(event.ButtonDClick(wxMOUSE_BTN_LEFT))
	{
		g_camera_target = Vector3D(start.x,g_camera_target.y,start.z);
	}

	if( event.Moving() )
	{
		m_mouseoverRegion = -1;

		float regSize = HFIELD_POINT_SIZE*m_regCells;

		for(int y = 0; y < m_regTall; y++)
		{
			for(int x = 0; x < m_regWide; x++)
			{
				int regIdx = y*m_regWide+x;

				regionMap_t* rm = &m_regionMap[regIdx];

				if(!rm->region->m_isLoaded)
					continue;

				CHeightTileField* hfield = rm->region->GetHField();

				if(!hfield)
					continue;

				Rectangle_t rect(hfield->m_position.xz(), hfield->m_position.xz()+regSize);

				if(rect.IsInRectangle(Vector2D(start.x,start.z)))
				{
					m_mouseoverRegion = regIdx;
					break;
				}
			}

			if(m_mouseoverRegion != -1)
				break;
		}
		
	}

	// handle selection
	if (!event.Dragging())
	{
		if (event.ControlDown() && event.LeftDown() && m_mouseoverRegion != -1)
		{
			m_regionMap[m_mouseoverRegion].selected = !m_regionMap[m_mouseoverRegion].selected;
		}
	}
	else
	{

	}

	m_mouseOldPos = event.GetPosition();

	m_zoomLevel -= (float)event.GetWheelRotation()*0.0015f*m_zoomLevel;

	if(m_zoomLevel < 0.1f )
		m_zoomLevel = 0.1f;

	if(m_zoomLevel > 5.0f )
		m_zoomLevel = 5.0f;
}

void CRegionEditFrame::OnClose( wxCloseEvent& event )
{
	Hide();
	event.Veto(); 
}

void CRegionEditFrame::OnLevelLoad()
{
	// draw regions
	m_regWide = g_pGameWorld->m_level.m_wide;
	m_regTall = g_pGameWorld->m_level.m_tall;
	m_regCells = g_pGameWorld->m_level.m_cellsSize;

	m_regionMap = new regionMap_t[m_regWide*m_regTall];

	for(int y = 0; y < m_regTall; y++)
	{
		for(int x = 0; x < m_regWide; x++)
		{
			int regIdx = y*m_regWide+x;

			CEditorLevelRegion* reg = (CEditorLevelRegion*)g_pGameWorld->m_level.GetRegionAt(IVector2D(x,y));
			m_regionMap[regIdx].region = reg;
		}
	}
}

void CRegionEditFrame::OnLevelUnload()
{
	for(int y = 0; y < m_regTall; y++)
	{
		for(int x = 0; x < m_regWide; x++)
		{
			int regIdx = y*m_regWide+x;
			g_pShaderAPI->FreeTexture(m_regionMap[regIdx].image);
			g_pShaderAPI->FreeTexture(m_regionMap[regIdx].aiMapImage);
		}
	}

	delete [] m_regionMap;
	m_regionMap = NULL;
	m_regWide = 0;
	m_regTall = 0;
	m_regCells = 0;
}

void CRegionEditFrame::RefreshRegionMapImages()
{
	for(int y = 0; y < m_regTall; y++)
	{
		for(int x = 0; x < m_regWide; x++)
		{
			int regIdx = y*m_regWide+x;

			//if( m_regionMap[regIdx].region->IsRegionEmpty() )
			//	continue;

			RegenerateRegionImage( &m_regionMap[regIdx] );
		}
	}
}

void CRegionEditFrame::RegenerateRegionImage(regionMap_t* regMap)
{
	if(!regMap->region->m_isLoaded)
		return;

	if(!regMap->region->m_roads)
		return;

	if(!regMap->image)
	{
		regMap->image = g_pShaderAPI->CreateProceduralTexture("regTex_%d", FORMAT_RGBA8, m_regCells, m_regCells, 1, 1,TEXFILTER_NEAREST, TEXADDRESS_CLAMP, TEXFLAG_NOQUALITYLOD);
		regMap->image->Ref_Grab();
	}

	texlockdata_t lockdata;

	regMap->image->Lock(&lockdata, NULL, true);
	if(lockdata.pData)
	{
		memset(lockdata.pData, 0, m_regCells*m_regCells*sizeof(TVec4D<ubyte>));

		TVec4D<ubyte>* imgData = (TVec4D<ubyte>*)lockdata.pData;

		for(int y = 0; y < m_regCells; y++)
		{
			for(int x = 0; x < m_regCells; x++)
			{
				int pixIdx = y*m_regCells+x;

				levroadcell_t& roadcell = regMap->region->m_roads[pixIdx];

				TVec4D<ubyte> color(0);
				color.x = roadcell.type == ROADTYPE_JUNCTION ? 128 : 0;
				color.z = roadcell.type == ROADTYPE_STRAIGHT ? 128 : 0;
				color.y = 32;
				color.w = 255;

				imgData[pixIdx] = color;
			}
		}

		regMap->image->Unlock();
		lockdata.pData = NULL;
	}

	navGrid_t& navGrid = regMap->region->m_navGrid[0];

	if(!regMap->aiMapImage)
	{
		regMap->aiMapImage = g_pShaderAPI->CreateProceduralTexture(varargs("navgrid_%d", regMap->region->m_regionIndex), FORMAT_RGBA8, navGrid.wide, navGrid.tall, 1, 1,TEXFILTER_NEAREST, TEXADDRESS_CLAMP, TEXFLAG_NOQUALITYLOD);
		regMap->aiMapImage->Ref_Grab();
	}

	regMap->aiMapImage->Lock(&lockdata, NULL, true);
	if(lockdata.pData)
	{
		memset(lockdata.pData, 0, navGrid.wide*navGrid.tall*sizeof(TVec4D<ubyte>));

		TVec4D<ubyte>* imgData = (TVec4D<ubyte>*)lockdata.pData;

		for(int y = 0; y < navGrid.tall; y++)
		{
			for(int x = 0; x < navGrid.wide; x++)
			{
				int pixIdx = y*navGrid.tall+x;

				TVec4D<ubyte> color(0);
				color.z = 255-navGrid.staticObst[pixIdx] * 32;

				imgData[pixIdx] = color;
			}
		}

		regMap->aiMapImage->Unlock();
	}

}

void CRegionEditFrame::BuildAndSaveMapFromRegionImages()
{
	int imgWide = m_regWide*m_regCells;
	int imgTall = m_regTall*m_regCells;

	CImage img;
	TVec3D<ubyte>* imgData = (TVec3D<ubyte>*)img.Create(FORMAT_RGB8, imgWide, imgTall, 1, 1, 1);

	int imgSize = m_regWide*m_regCells*m_regTall*m_regCells;

	memset(imgData, 0xFFFFFFFF, GetBytesPerPixel(FORMAT_RGB8)*imgSize);

	for(int ry = 0; ry < m_regTall; ry++)
	{
		for(int rx = 0; rx < m_regWide; rx++)
		{
			int regIdx = ry*m_regWide+rx;

			if( !m_regionMap[regIdx].region->m_isLoaded )
				continue;

			if( !m_regionMap[regIdx].region->m_roads )
				continue;

			// make region image
			for(int y = 0; y < m_regCells; y++)
			{
				for(int x = 0; x < m_regCells; x++)
				{
					int px = rx*m_regCells+x;
					int py = ry*m_regCells+y;

					int pixIdx = py*imgWide + (imgWide-px-1);

					int regPixIdx = y*m_regCells+x;

					regionMap_t* regMap = &m_regionMap[regIdx];

					TVec3D<ubyte> color(255);
					bool placeRoad = regMap->region->m_roads[regPixIdx].type > ROADTYPE_NOROAD && regMap->region->m_roads[regPixIdx].type < ROADTYPE_PARKINGLOT;

					color.x = placeRoad ? 255 : 255;
					color.y = placeRoad ? 80 : 255;
					color.z = placeRoad ? 80 : 255;

					imgData[pixIdx] = color;
				}
			}
		}
	}

	EqString mapTexName(g_fileSystem->GetCurrentGameDirectory() + _Es("/materials/levelmap/"));
	mapTexName.Append( g_pGameWorld->GetLevelName() );
	mapTexName.Append(".tga");

	wxMessageBox(varargs("Level map saved to '%s'", mapTexName.c_str()));

	img.SaveTGA( mapTexName.c_str() );
}

void CRegionEditFrame::OnEraseBG( wxEraseEvent& event )
{

}

void CRegionEditFrame::OnIdle(wxIdleEvent &event)
{
	ReDraw();

	event.RequestMore(true);
}

void CRegionEditFrame::ReDraw()
{
	if( !m_doRedraw )
		return;

	if(!IsShown() && !IsShownOnScreen())
		return;

	if(!m_regionMap)
		return;

	static IEqFont* font = g_fontCache->GetFont("default", 30);

	//m_doRedraw = false;

	int w, h;
	m_pRenderPanel->GetSize(&w, &h);

	g_pShaderAPI->SetViewport(0, 0, w,h);

	materials->GetConfiguration().wireframeMode = false;

	materials->SetAmbientColor(color4_white);

	if(materials->BeginFrame())
	{
		g_pShaderAPI->Clear(true,true,false, ColorRGBA(0,0,0, 1));
		
		// find aspect
		//float aspect = (float)w / (float)h;

		//float half_w = aspect*m_zoomLevel;
		//float half_h = m_zoomLevel;

		float half_w = (float)w*m_zoomLevel*0.5f;
		float half_h = (float)h*m_zoomLevel*0.5f;

		materials->SetupOrtho(-half_w, half_w, -half_h, half_h, -10.0f, 10.0f);

		Vector3D angl = g_pGameWorld->GetView()->GetAngles();

		Matrix4x4 orthoRotation = rotateZXY4(DEG2RAD(-90.0f),0.0f,0.0f);
		orthoRotation.translate(Vector3D(m_viewPos.x,0,m_viewPos.y));

		materials->SetMatrix(MATRIXMODE_VIEW,  orthoRotation);
		materials->SetMatrix(MATRIXMODE_WORLD, identity4());

		materials->GetWorldViewProjection(m_viewProj);

		float regSize = HFIELD_POINT_SIZE * m_regCells;

		BlendStateParam_t params;
		params.alphaTest = false;
		params.alphaTestRef = 1.0f;
		params.blendEnable = false;
		params.srcFactor = BLENDFACTOR_ONE;
		params.dstFactor = BLENDFACTOR_ONE;
		params.mask = COLORMASK_ALL;
		params.blendFunc = BLENDFUNC_ADD;

		DepthStencilStateParams_t depthparams;
		depthparams.depthWrite = false;
		depthparams.depthTest = false;

		g_pShaderAPI->Reset();

		int total = 0;
		int displayed = 0;
		int selected = 0;

		const int nStepSize = HFIELD_POINT_SIZE * g_pGameWorld->m_level.m_cellsSize;
		const Vector3D center = Vector3D(m_regWide*nStepSize, 0, m_regTall*nStepSize)*0.5f - Vector3D(HFIELD_POINT_SIZE, 0, HFIELD_POINT_SIZE)*0.5f;

		for(int y = 0; y < m_regTall; y++)
		{
			for(int x = 0; x < m_regWide; x++)
			{
				int regIdx = y*m_regWide+x;

				regionMap_t* rm = &m_regionMap[regIdx];

				const Vector3D regionPos = Vector3D(x*nStepSize, 0, y*nStepSize) - center;

				total++;

				if(rm->selected)
					selected++;

				Vector2D regBoxMins(regionPos.xz());
				Vector2D regBoxMaxs(regionPos.xz()+regSize);

				Vector3D v1(regBoxMins.x, 0, regBoxMins.y);
				Vector3D v2(regBoxMins.x, 0, regBoxMaxs.y);
				Vector3D v3(regBoxMaxs.x, 0, regBoxMins.y);
				Vector3D v4(regBoxMaxs.x, 0, regBoxMaxs.y);

				IDynamicMesh* mesh = materials->GetDynamicMesh();
				CMeshBuilder meshBuilder(mesh);

				materials->SetDepthStates(depthparams);
				materials->SetBlendingStates(params);
				
				if (!rm->region->IsRegionEmpty())
				{
					displayed++;

					ITexture* showTexture = m_showsNavGrid ? rm->aiMapImage : rm->image;

					g_pShaderAPI->SetTexture(showTexture, nullptr, 0);
					materials->SetAmbientColor(1.0f);

					materials->BindMaterial(materials->GetDefaultMaterial());

					meshBuilder.Begin(PRIM_TRIANGLE_STRIP);
						meshBuilder.TexturedQuad3(v1, v2, v3, v4, Vector2D(0, 0), Vector2D(0, 1), Vector2D(1, 0), Vector2D(1, 1));
					meshBuilder.End();

					if (m_mouseoverRegion == regIdx || rm->selected)
					{
						g_pShaderAPI->SetTexture(showTexture, nullptr, 0);
						materials->SetAmbientColor(rm->selected ? ColorRGBA(0.5f, 0.5f, 0.0f, 0.5f) : ColorRGBA(0.5f));
						materials->BindMaterial(materials->GetDefaultMaterial());

						// reuse
						mesh->Render();

						g_pShaderAPI->SetTexture(nullptr, nullptr, 0);
						materials->BindMaterial(materials->GetDefaultMaterial());

						// render the contour
						meshBuilder.Begin(PRIM_LINE_STRIP);
							meshBuilder.Position3fv(v1); meshBuilder.AdvanceVertex();
							meshBuilder.Position3fv(v2); meshBuilder.AdvanceVertex();
							meshBuilder.Position3fv(v4); meshBuilder.AdvanceVertex();
							meshBuilder.Position3fv(v3); meshBuilder.AdvanceVertex();
							meshBuilder.Position3fv(v1); meshBuilder.AdvanceVertex();
						meshBuilder.End();
					}
				}
				else
				{
					g_pShaderAPI->SetTexture(nullptr, nullptr, 0);
					materials->SetAmbientColor(ColorRGBA(0.15f));
					materials->BindMaterial(materials->GetDefaultMaterial());
					
					// render outline
					meshBuilder.Begin(PRIM_LINE_STRIP);
						meshBuilder.Position3fv(v1); meshBuilder.AdvanceVertex();
						meshBuilder.Position3fv(v2); meshBuilder.AdvanceVertex();
						meshBuilder.Position3fv(v4); meshBuilder.AdvanceVertex();
						meshBuilder.Position3fv(v3); meshBuilder.AdvanceVertex();
						meshBuilder.Position3fv(v1); meshBuilder.AdvanceVertex();
					meshBuilder.End();
				}
			}
		}

		materials->Setup2D(w,h);

		eqFontStyleParam_t fontParam;
		fontParam.styleFlag = TEXT_STYLE_SHADOW | TEXT_STYLE_FROM_CAP;
		fontParam.textColor = ColorRGBA(1,1,1,1);

		Vector2D baseTextPos(15);
		float lineHeight = font->GetLineHeight(fontParam);

		Vector2D lineOffset(0, lineHeight);

		font->RenderText(varargs("Regions: %d / %d", displayed, total), baseTextPos + lineOffset*0, fontParam);
		font->RenderText(varargs("Selected: %d", selected), baseTextPos + lineOffset*1, fontParam);

		font->RenderText("RMB Drag - navigate", baseTextPos + lineOffset * 3, fontParam);
		font->RenderText(varargs("Mouse Wheel - Zoom (%g)", m_zoomLevel), baseTextPos + lineOffset * 4, fontParam);
		font->RenderText("N - change display", baseTextPos + lineOffset * 5, fontParam);
		
		font->RenderText("CTRL + Left click - Toggle selection", baseTextPos + lineOffset * 7, fontParam);
		font->RenderText("CTRL + A - Select All", baseTextPos + lineOffset * 8, fontParam);
		font->RenderText("ARROWS - move regions", baseTextPos + lineOffset * 9, fontParam);
		font->RenderText("DELETE - remove regions", baseTextPos + lineOffset * 10, fontParam);

		materials->EndFrame(m_swapChain);
	}
}

void CRegionEditFrame::OnResizeWin( wxSizeEvent& event )
{
	m_doRedraw = true;
}