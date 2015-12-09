//******************* Copyright (C) Illusion Way, L.L.C 2010 *********************
//
// Description: 
//
//****************************************************************************

#ifndef _SLIDER_H_
#define _SLIDER_H_

#include "Panel.h"

class Slider;
class SliderListener {
public:
	virtual ~SliderListener(){}

	virtual void onSliderChanged(Slider *Slider) = 0;
};


class Slider : public Panel {
public:
	Slider(const float x, const float y, const float w, const float h, const float minVal = 0, const float maxVal = 1, const float val = 0);
	virtual ~Slider();

	float getValue() const { return value; }
	void setValue(const float val);
	void setRange(const float minVal, const float maxVal);

	void setListener(SliderListener *listener){ sliderListener = listener; }

	bool onMouseMove(const int x, const int y);
	bool onMouseButton(const int x, const int y, const MouseButton button, const bool pressed);

	void draw(IRenderer *renderer, const FontID defaultFont, const SamplerStateID linearClamp, const BlendStateID blendSrcAlpha, const DepthStateID depthState);

protected:
	void updateValue(const int x);

	char *text;

	SliderListener *sliderListener;

	float minValue, maxValue, value;
};

#endif // _SLIDER_H_
