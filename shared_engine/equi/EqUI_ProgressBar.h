//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: EqUI percentage bar
//////////////////////////////////////////////////////////////////////////////////

#ifndef EQUI_PROGRESSBAR_H
#define EQUI_PROGRESSBAR_H

#include "EqUI/equi_defs.h"
#include "EqUI/IEqUI_Control.h"

namespace equi
{
	class ProgressBar : public IUIControl
	{
	public:
		EQUI_CLASS(ProgressBar, IUIControl)

		ProgressBar();
		~ProgressBar() {}

		void				InitFromKeyValues(kvkeybase_t* sec, bool noClear);

		// events
		bool				ProcessMouseEvents(float x, float y, int nMouseButtons, int flags) { return true; }
		bool				ProcessKeyboardEvents(int nKeyButtons, int flags) { return true; }

		void				DrawSelf(const IRectangle& rect);

		void				SetValue(float value)				{ m_value = value; }
		void				SetColor(const ColorRGBA& color)	{ m_color = color; }
		const ColorRGBA&	GetColor() const					{ return m_color; }

	protected:
		ColorRGBA			m_color;
		float				m_value;
	};
};

#endif // EQUI_PERCENTAGEBAR_H