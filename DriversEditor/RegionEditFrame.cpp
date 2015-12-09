//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Region editor frame
//////////////////////////////////////////////////////////////////////////////////

#include "RegionEditFrame.h"

#include "world.h"
#include "imaging/ImageLoader.h"

CRegionEditFrame::CRegionEditFrame( wxWindow* parent ) : 
	wxFrame( parent, wxID_ANY, "Region editor and level navigation", wxDefaultPosition, wxSize( 1090,653 ), wxDEFAULT_FRAME_STYLE|wxMAXIMIZE|wxTAB_TRAVERSAL|wxSTAY_ON_TOP )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxVERTICAL );
	
	m_pRenderPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_pRenderPanel->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNTEXT ) );
	
	bSizer1->Add( m_pRenderPanel, 1, wxEXPAND, 5 );
	
	Maximize();
	
	this->SetSizer( bSizer1 );
	this->Layout();
	
	this->Centre( wxBOTH );

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
	m_pRenderPanel->Connect(wxEVT_MOTION, (wxObjectEventFunction)&CRegionEditFrame::ProcessMouseEvents, NULL, this);

	m_swapChain = materials->CreateSwapChain(m_pRenderPanel->GetHandle());

	m_regionMap = NULL;
	m_regWide = 0;
	m_regTall = 0;
	m_regCells = 0;

	m_doRedraw = true;
	m_zoomLevel = 0.5;
	m_viewPos = vec2_zero;
	m_mouseoverRegion = -1;
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
	m_pRenderPanel->Disconnect(wxEVT_MOTION, (wxObjectEventFunction)&CRegionEditFrame::ProcessMouseEvents, NULL, this);
}

extern Vector3D g_camera_target;

void CRegionEditFrame::ProcessMouseEvents(wxMouseEvent& event)
{
	if(event.Dragging())
	{
		wxPoint delta = m_mouseOldPos - event.GetPosition();

		m_viewPos += Vector2D(-delta.x,delta.y)*m_zoomLevel;
	}

	int w, h;
	m_pRenderPanel->GetSize(&w, &h);

	Vector2D point(w-event.GetPosition().x, event.GetPosition().y);

	float wh = (float)w/(float)h;

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

			CLevelRegion* reg = g_pGameWorld->m_level.GetRegionAt(IVector2D(x,y));
			m_regionMap[regIdx].region = reg;
		}
	}

	RefreshRegionMapImages();
}

void CRegionEditFrame::OnLevelUnload()
{
	for(int y = 0; y < m_regTall; y++)
	{
		for(int x = 0; x < m_regWide; x++)
		{
			int regIdx = y*m_regWide+x;
			g_pShaderAPI->FreeTexture(m_regionMap[regIdx].image);
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
		regMap->image = g_pShaderAPI->CreateProceduralTexture("regTex_%d", FORMAT_RGBA8, m_regCells, m_regCells, 1, 1,TEXFILTER_NEAREST, ADDRESSMODE_CLAMP, TEXFLAG_NOQUALITYLOD);
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

				TVec4D<ubyte> color(0);
				color.x = regMap->region->m_roads[pixIdx].type == ROADTYPE_JUNCTION ? 128 : 0;
				color.z = regMap->region->m_roads[pixIdx].type == ROADTYPE_STRAIGHT ? 128 : 0;
				color.y = 32;
				color.w = 255;

				imgData[pixIdx] = color;
			}
		}

		regMap->image->Unlock();
	}

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
		float aspect = (float)w / (float)h;

		//float half_w = aspect*m_zoomLevel;
		//float half_h = m_zoomLevel;

		float half_w = (float)w*m_zoomLevel*0.5f;
		float half_h = (float)h*m_zoomLevel*0.5f;

		materials->SetupOrtho(-half_w, half_w, -half_h, half_h, -10.0f, 10.0f);

		Vector3D angl = g_pGameWorld->GetCameraParams()->GetAngles();

		Matrix4x4 orthoRotation = rotateZXY4(DEG2RAD(-90.0f),0.0f,0.0f);
		orthoRotation.translate(Vector3D(m_viewPos.x,0,m_viewPos.y));

		materials->SetMatrix(MATRIXMODE_VIEW,  orthoRotation);
		materials->SetMatrix(MATRIXMODE_WORLD, identity4());

		materials->GetWorldViewProjection(m_viewProj);

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

		float regSize = HFIELD_POINT_SIZE*m_regCells;

		int displayed = 0;
		int enabled = 0;

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

				displayed++;

				enabled++;

				Vector2D regBoxMins(hfield->m_position.xz());
				Vector2D regBoxMaxs(hfield->m_position.xz()+regSize);

				Vector3D v1(regBoxMins.x, 0, regBoxMins.y);
				Vector3D v2(regBoxMins.x, 0, regBoxMaxs.y);
				Vector3D v3(regBoxMaxs.x, 0, regBoxMins.y);
				Vector3D v4(regBoxMaxs.x, 0, regBoxMaxs.y);
	
				Vertex3D_t verts[] = {
					Vertex3D_t(v1, Vector2D(0,0), color4_white),
					Vertex3D_t(v2, Vector2D(0,1), color4_white),
					Vertex3D_t(v3, Vector2D(1,0), color4_white),
					Vertex3D_t(v4, Vector2D(1,1), color4_white),
				};

				materials->DrawPrimitivesFFP(PRIM_TRIANGLE_STRIP, verts, elementsOf(verts), rm->image, color4_white, &params, &depthparams);

				if(m_mouseoverRegion == regIdx)
				{
					Vertex3D_t lverts[] = {
						Vertex3D_t(v1, Vector2D(0,0), color4_white),
						Vertex3D_t(v2, Vector2D(0,1), color4_white),
						Vertex3D_t(v4, Vector2D(1,0), color4_white),
						Vertex3D_t(v3, Vector2D(1,1), color4_white),
						Vertex3D_t(v1, Vector2D(1,1), color4_white),
					};

					materials->DrawPrimitivesFFP(PRIM_LINE_STRIP, lverts, elementsOf(lverts), NULL, ColorRGBA(0,1,0,1), &params, &depthparams);
					materials->DrawPrimitivesFFP(PRIM_TRIANGLE_STRIP, verts, elementsOf(verts), NULL, ColorRGBA(0.15f), &params, &depthparams);
				}

				//
				// set the texture 
				// make the rectangle that fits on screen
				// select mouseover
				// render


			}
		}

		materials->Setup2D(w,h);

		debugoverlay->GetFont()->DrawText(varargs("ZOOM: %g", m_zoomLevel), 15, 15, 8,8,false);
		debugoverlay->GetFont()->DrawText(varargs("displayed regions: %d", displayed), 15, 25, 8,8,false);
		debugoverlay->GetFont()->DrawText(varargs("enabled regions: %d", enabled), 15, 35, 8,8,false);

		materials->EndFrame(m_swapChain);
	}
}

void CRegionEditFrame::OnResizeWin( wxSizeEvent& event )
{
	m_doRedraw = true;
}