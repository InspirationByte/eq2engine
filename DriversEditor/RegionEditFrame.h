//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Region editor frame
//////////////////////////////////////////////////////////////////////////////////

#ifndef REGIONEDITFRAME_H
#define REGIONEDITFRAME_H

#include "EditorHeader.h"

class CLevelRegion;

struct regionMap_t
{
	regionMap_t()
	{
		region = NULL;
		image = NULL;
		aiMapImage = NULL;
	}

	CLevelRegion*	region;
	ITexture*		image;	// image of region
	ITexture*		aiMapImage;
};

class CRegionEditFrame : public wxFrame 
{
private:
	
protected:
	wxPanel* m_pRenderPanel;
	
public:
		
	CRegionEditFrame( wxWindow* parent );
	~CRegionEditFrame();

	void ProcessAllMenuCommands(wxCommandEvent& event);

	void OnClose( wxCloseEvent& event );

	void OnEraseBG( wxEraseEvent& event );
	void OnPaint( wxPaintEvent& event ) {}
	void OnResizeWin( wxSizeEvent& event );
	void OnIdle(wxIdleEvent &event);
	void OnKey(wxKeyEvent& event);

	void ProcessMouseEvents(wxMouseEvent& event);

	void ReDraw();

	void OnLevelLoad();
	void OnLevelUnload();

	void RefreshRegionMapImages();
	void RegenerateRegionImage(regionMap_t* regMap);

	void BuildAndSaveMapFromRegionImages();

	DECLARE_EVENT_TABLE();

protected:
	wxMenuBar*		m_pMenu;

	IEqSwapChain*	m_swapChain;

	regionMap_t*	m_regionMap;
	int				m_regWide;
	int				m_regTall;
	int				m_regCells;
	bool			m_doRedraw;

	float			m_zoomLevel;

	Matrix4x4		m_viewProj;
	Vector2D		m_viewPos;
	wxPoint			m_mouseOldPos;

	int				m_mouseoverRegion;

	bool			m_showsNavGrid;
};

#endif // REGIONEDITFRAME_H