//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Texture view
//////////////////////////////////////////////////////////////////////////////////

#ifndef TEXTUREVIEW_H
#define TEXTUREVIEW_H

#include "EditorHeader.h"

class CTextureView: public wxPanel
{
public:
    CTextureView(wxWindow* parent, const wxPoint& pos, const wxSize& size);

	void OnIdle(wxIdleEvent &event)
	{
		Redraw();
	}

	void OnEraseBackground(wxEraseEvent& event)
	{

	}

	// render material in this tiny preview window
	void Redraw();

	void SetTexture(ITexture* pTex)
	{
		m_pTexture = pTex;
	}

	DECLARE_EVENT_TABLE()

protected:
	ITexture*		m_pTexture;
	IEqSwapChain*	m_pSwapChain;
};

#endif // TEXTUREVIEW_H