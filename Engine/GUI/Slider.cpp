//******************* Copyright (C) Illusion Way, L.L.C 2010 *********************
//
// Description: 
//
//****************************************************************************

#include "Slider.h"

Slider::Slider(const float x, const float y, const float w, const float h, const float minVal, const float maxVal, const float val){
	setPosition(x, y);
	setSize(w, h);

	sliderListener = NULL;

	color = Vector4D(1, 0.2f, 0.2f, 0.65f);

	minValue = minVal;
	maxValue = maxVal;
	setValue(val);
}

Slider::~Slider(){
}

void Slider::setValue(const float val){
	value = clamp(val, minValue, maxValue);
}

void Slider::setRange(const float minVal, const float maxVal){
	minValue = minVal;
	maxValue = maxVal;
	value = clamp(value, minVal, maxVal);
}

bool Slider::onMouseMove(const int x, const int y){
	if (capture){
		updateValue(x);
		return true;
	}
	return false;
}

bool Slider::onMouseButton(const int x, const int y, const MouseButton button, const bool pressed){
	if (button == MOUSE_LEFT){
		if (pressed){
			updateValue(x);
		}
		capture = pressed;
	}

	return true;
}

void Slider::draw(IRenderer *renderer, const FontID defaultFont, const SamplerStateID linearClamp, const BlendStateID blendSrcAlpha, const DepthStateID depthState){
	Vector4D black(0, 0, 0, 1);

	Vector2D quad[] = { MAKEQUAD(xPos, yPos, xPos + width, yPos + height, 2) };
	renderer->drawPlain(PRIM_TRIANGLE_STRIP, quad, elementsOf(quad), blendSrcAlpha, depthState, &color);

	Vector2D rect[] = { MAKERECT(xPos, yPos, xPos + width, yPos + height, 2) };
	renderer->drawPlain(PRIM_TRIANGLE_STRIP, rect, elementsOf(rect), BS_NONE, depthState, &black);

	Vector2D line[] = { MAKEQUAD(xPos + 0.5f * height, yPos + 0.5f * height - 1, xPos + width - 0.5f * height, yPos + 0.5f * height + 1, 0) };
	renderer->drawPlain(PRIM_TRIANGLE_STRIP, line, elementsOf(line), BS_NONE, depthState, &black);

	float x = lerp(xPos + 0.5f * height, xPos + width - 0.5f * height, (value - minValue) / (maxValue - minValue));
	Vector2D marker[] = { MAKEQUAD(x - 0.2f * height, yPos + 0.2f * height, x + 0.2f * height, yPos + 0.8f * height, 0) };
	renderer->drawPlain(PRIM_TRIANGLE_STRIP, marker, elementsOf(marker), BS_NONE, depthState, &black);
}

void Slider::updateValue(const int x){
	float t = saturate((x - (xPos + 0.5f * height)) / (width - height));
	value = lerp(minValue, maxValue, t);

	if (sliderListener) sliderListener->onSliderChanged(this);
}
