//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: EGUI panel
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "IEqUI_Control.h"

namespace equi
{

class Button;
class Label;

// eq panel class
class Panel : public IUIControl
{
	friend class CUIManager;

public:
	EQUI_CLASS(Panel, IUIControl)

	Panel();
	~Panel();

	virtual void			InitFromKeyValues(const KVSection* sec, bool noClear );

	virtual void			Hide();

	// apperance
	virtual void			SetColor(const ColorRGBA &color);
	virtual void			GetColor(ColorRGBA &color) const;

	virtual void			SetSelectionColor(const ColorRGBA &color);
	virtual void			GetSelectionColor(ColorRGBA &color) const;

	void					CenterOnScreen();

	// rendering
	virtual void			Render(int depth, IGPURenderPassRecorder* rendPassRecorder);
protected:

	virtual void			DrawSelf(const IAARectangle& rect, bool scissorOn, IGPURenderPassRecorder* rendPassRecorder);

	bool					ProcessMouseEvents(const IVector2D& mousePos, const IVector2D& mouseDelta, int nMouseButtons, int flags);

	ColorRGBA				m_color;
	ColorRGBA				m_selColor;

	bool					m_windowControls;
	bool					m_grabbed;
	bool					m_screenOverlay;

	equi::Label*			m_labelCtrl;
	equi::Button*			m_closeButton;
};

};
