//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: EqUI percentage bar
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "IEqUI_Control.h"

namespace equi
{
	class ProgressBar : public IUIControl
	{
	public:
		EQUI_CLASS(ProgressBar, IUIControl)

		ProgressBar();
		~ProgressBar() {}

		void				InitFromKeyValues(KVSection* sec, bool noClear);

		// events
		bool				ProcessMouseEvents(float x, float y, int nMouseButtons, int flags) { return true; }
		bool				ProcessKeyboardEvents(int nKeyButtons, int flags) { return true; }

		void				DrawSelf(const IRectangle& rect, bool scissorOn);

		void				SetValue(float value)				{ m_value = value; }
		void				SetColor(const ColorRGBA& color)	{ m_color = color; }
		const ColorRGBA&	GetColor() const					{ return m_color; }

	protected:
		ColorRGBA			m_color;
		float				m_value;
	};
};
