//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
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

		void				Parse(const KVSection& sec) override;
		void				DrawSelf(const IAARectangle& rect, IGPURenderPassRecorder* rendPassRecorder) override;

		void				SetValue(float value)				{ m_value = value; }
		float				GetValue() const					{ return m_value; }

		void				SetColor(const ColorRGBA& color)	{ m_color = color; }
		const ColorRGBA&	GetColor() const					{ return m_color; }

	protected:
		ColorRGBA			m_color;
		float				m_value;
	};
};
