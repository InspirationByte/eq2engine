//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2019
//////////////////////////////////////////////////////////////////////////////////
// Description: EqUI timer
//////////////////////////////////////////////////////////////////////////////////

#ifndef EQUI_DRVSYNTIMER_H
#define EQUI_DRVSYNTIMER_H

#include "EqUI/equi_defs.h"
#include "EqUI/IEqUI_Control.h"

namespace equi
{
	enum ETimerType
	{
		TIMER_TYPE_DIGITS = 0,
		TIMER_TYPE_CLOCKFACE
	};

	class DrvSynTimerElement : public IUIControl
	{
	public:
		EQUI_CLASS(DrvSynTimerElement, IUIControl)

		DrvSynTimerElement();
		~DrvSynTimerElement() {}

		void			InitFromKeyValues(kvkeybase_t* sec, bool noClear);

		// events
		bool			ProcessMouseEvents(float x, float y, int nMouseButtons, int flags) { return true; }
		bool			ProcessKeyboardEvents(int nKeyButtons, int flags) { return true; }

		void			DrawSelf(const IRectangle& rect);

		void			SetType(ETimerType type) { m_type = type; }
		void			SetTimeValue(double time) { m_timeDisplayValue = time; }

		ETimerType		GetType() const { return m_type; }

	protected:

		ETimerType		m_type;
		double			m_timeDisplayValue;
	};
};

#endif // EQUI_DRVSYNTIMER_H