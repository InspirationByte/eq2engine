//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Texture view
//////////////////////////////////////////////////////////////////////////////////

#include "TextureView.h"

BEGIN_EVENT_TABLE(CTextureView, wxPanel)
    EVT_ERASE_BACKGROUND(CTextureView::OnEraseBackground)
    EVT_IDLE(CTextureView::OnIdle)
END_EVENT_TABLE()

CTextureView::CTextureView(wxWindow* parent, const wxPoint& pos, const wxSize& size) : wxPanel( parent, -1, pos, size, wxTAB_TRAVERSAL)
{
	m_pTexture = NULL;
	m_pSwapChain = materials->CreateSwapChain(GetHandle());
}

// render material in this tiny preview window
void CTextureView::Redraw()
{
	if(!materials)
		return;

	int w, h;
	GetSize(&w, &h);

	g_pShaderAPI->SetViewport(0, 0, w,h);


	materials->GetConfiguration().wireframeMode = false;

	materials->SetAmbientColor(color4_white);

	if(materials->BeginFrame())
	{
		g_pShaderAPI->Clear(true,true,false, ColorRGBA(0,0,0, 1));
		materials->Setup2D(w,h);

		if(m_pTexture != NULL)
		{
			Vertex2D_t verts[] = {MAKETEXQUAD(0, 0, w, h, 0)};

			BlendStateParam_t params;
			params.alphaTest = false;
			params.alphaTestRef = 1.0f;
			params.blendEnable = false;
			params.srcFactor = BLENDFACTOR_ONE;
			params.dstFactor = BLENDFACTOR_ZERO;
			params.mask = COLORMASK_ALL;
			params.blendFunc = BLENDFUNC_ADD;

			materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, verts, 4, m_pTexture, color4_white, &params);
		}

		materials->EndFrame(m_pSwapChain);
	}
}