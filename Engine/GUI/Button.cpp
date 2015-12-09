//******************* Copyright (C) Illusion Way, L.L.C 2010 *********************
//
// Description: 
//
//****************************************************************************

#include "Button.h"

PushButton::PushButton(const float x, const float y, const float w, const float h, const char *txt){
	setPosition(x, y);
	setSize(w, h);

	buttonListener = NULL;

	text = new char[strlen(txt) + 1];
	strcpy(text, txt);

	color = Vector4D(0.5f, 0.5f, 0.5f, 1);
	rendercolor = color;

	pushed = false;
}

PushButton::~PushButton(){
	delete text;
}

bool PushButton::onMouseButton(const int x, const int y, const MouseButton button, const bool pressed){
	if (pressed == pushed) return false;

	if (button == MOUSE_LEFT){
		if (pressed){
			pushed  = true;
			capture = true;
		} else {
			if (buttonListener && isInPanel(x, y)) buttonListener->onButtonClicked(this);
			pushed  = false;
			capture = false;
		}
	}

	return true;
}

bool PushButton::onKey(const unsigned int key, const bool pressed){
	if (key == KEY_ENTER || key == KEY_SPACE){
		if (buttonListener && pushed && !pressed) buttonListener->onButtonClicked(this);
		pushed = pressed;
		return true;
	}

	return false;
}

void PushButton::draw(IFont* defaultFont)
{
	drawButton(defaultFont, text);
}

void PushButton::drawButton(IFont* defaultFont, const char *text)
{
	Vector4D black(0, 0, 0, 1);
	Vector4D col = color;
	if (pushed) col *= Vector4D(0.5f, 0.5f, 0.5f, 1);

	rendercolor = lerp(rendercolor,col,8 * eng->GetFrameTime());
	col = rendercolor;
/*
	Vector2D quad[] = { MAKEQUAD(xPos, yPos, xPos + width, yPos + height, 2) };
	renderer->drawPlain(PRIM_TRIANGLE_STRIP, quad, elementsOf(quad), blendSrcAlpha, depthState, &col);

	Vector2D rect[] = { MAKERECT(xPos, yPos, xPos + width, yPos + height, 2) };
	renderer->drawPlain(PRIM_TRIANGLE_STRIP, rect, elementsOf(rect), BS_NONE, depthState, &black);
*/
	/*
	renderer->reset();
	renderer->setBlendState(blendSrcAlpha);
	renderer->setDepthState(depthState);
	renderer->apply();
	*/

	Vector4D col_shade1 = color;
	Vector4D col_shade2 = color;
	if (pushed)
	{
		col_shade2 *= 2;
		col_shade1 *= 0.2f;
	}
	else
	{
		col_shade1 *= 2;
		col_shade2 *= 0.2f;
	}

	renderer->drawFilledRectangle(Vector2D(xPos,yPos),Vector2D(xPos + width, yPos + height),col);
	drawShadedRectangle(renderer,Vector2D(xPos,yPos),Vector2D(xPos + width, yPos + height),col_shade1,col_shade2);

	float textWidth = 0.75f * height;

	float tw = renderer->getTextWidth(defaultFont, text);
	float maxW = width / tw;
	if (textWidth > maxW) textWidth = maxW;

	float x = 0.5f * (width - textWidth * tw);

	renderer->drawText(text, xPos + x, yPos, textWidth, height, defaultFont, linearClamp, blendSrcAlpha, depthState);
}

/***********************************************************************************************************/

KeyWaiterButton::KeyWaiterButton(const float x, const float y, const float w, const float h, const char *txt, uint *key) : PushButton(x, y, w, h, txt){
	targetKey = key;
	waitingForKey = false;
	if (*key == 0)
	{
		// Disabled
		color = Vector4D(0.1f, 0.1f, 0.1f, 1);
	}
}

KeyWaiterButton::~KeyWaiterButton(){

}

bool KeyWaiterButton::onMouseButton(const int x, const int y, const MouseButton button, const bool pressed){
	if (pressed == pushed) return false;

	if (button == MOUSE_LEFT){
		if (pressed){
			pushed  = true;
			capture = true;
		} else {
			if (isInPanel(x, y)){
				if (buttonListener) buttonListener->onButtonClicked(this);

				waitingForKey = true;
			}
			pushed = false;
		}
	}

	return true;
}

bool KeyWaiterButton::onKey(const unsigned int key, const bool pressed){
	if (waitingForKey)
	{
		if (key != KEY_ESCAPE)
		{
			if (key == KEY_DELETE && *targetKey != 0)
			{
				*targetKey = 0;
				color = Vector4D(0.1f, 0.1f, 0.1f, 1);
			} else {
				*targetKey = key;
				color = Vector4D(0.5f, 0.1f, 1, 1);
			}
		}
		waitingForKey = false;
		capture = false;
		return true;
	}
	else if (key == KEY_SPACE || key == KEY_ENTER){
		if (pushed && !pressed)
		{
			if (buttonListener) buttonListener->onButtonClicked(this);
			waitingForKey = true;
		}
		pushed = pressed;
		return true;
	}

	return false;
}

void KeyWaiterButton::draw(IRenderer *renderer, const FontID defaultFont, const SamplerStateID linearClamp, const BlendStateID blendSrcAlpha, const DepthStateID depthState){
	if (waitingForKey){
		drawButton(renderer, "<Press key>", defaultFont, linearClamp, blendSrcAlpha, depthState);
	} else {
		drawButton(renderer, text, defaultFont, linearClamp, blendSrcAlpha, depthState);
	}
}

/***********************************************************************************************************/

AxisWaiterButton::AxisWaiterButton(const float x, const float y, const float w, const float h, const char *txt, int *axis, bool *axisInvert) : PushButton(x, y, w, h, txt){
	targetAxis = axis;
	targetAxisInvert = axisInvert;
	waitingForAxis = false;
	if (*axis < 0){
		// Disabled
		color = Vector4D(0.1f, 0.1f, 0.1f, 1);
	}
}

AxisWaiterButton::~AxisWaiterButton(){

}

bool AxisWaiterButton::onMouseButton(const int x, const int y, const MouseButton button, const bool pressed){
	if (pressed == pushed) return false;

	if (button == MOUSE_LEFT){
		if (pressed){
			pushed  = true;
			capture = true;
		} else {
			if (isInPanel(x, y)){
				if (buttonListener) buttonListener->onButtonClicked(this);

				waitingForAxis = true;
			}
			pushed = false;
		}
	}

	return true;
}

bool AxisWaiterButton::onKey(const unsigned int key, const bool pressed){
	if (waitingForAxis){
		if (pressed && key == KEY_DELETE){
			*targetAxis = -1;
			*targetAxisInvert = false;
			color = Vector4D(0.1f, 0.1f, 0.1f, 1);

			waitingForAxis = false;
			capture = false;
			return true;
		}
	} else if (key == KEY_SPACE || key == KEY_ENTER){
		if (pushed && !pressed){
			if (buttonListener) buttonListener->onButtonClicked(this);
			waitingForAxis = true;
		}
		pushed = pressed;
		return true;
	}

	return false;
}

bool AxisWaiterButton::onJoystickAxis(const int axis, const float value){
	if (waitingForAxis && fabsf(value) > 0.5f){
		*targetAxis = axis;
		*targetAxisInvert = (value < 0.0f);
		color = Vector4D(0.5f, 0.1f, 1, 1);

		waitingForAxis = false;
		capture = false;
		return true;
	}

	return false;
}

void AxisWaiterButton::draw(IRenderer *renderer, const FontID defaultFont, const SamplerStateID linearClamp, const BlendStateID blendSrcAlpha, const DepthStateID depthState){
	if (waitingForAxis){
		drawButton(renderer, "<Push stick>", defaultFont, linearClamp, blendSrcAlpha, depthState);
	} else {
		drawButton(renderer, text, defaultFont, linearClamp, blendSrcAlpha, depthState);
	}
}

/***********************************************************************************************************/

ButtonWaiterButton::ButtonWaiterButton(const float x, const float y, const float w, const float h, const char *txt, int *button) : PushButton(x, y, w, h, txt){
	targetButton = button;
	waitingForButton = false;
	if (button < 0){
		// Disabled
		color = Vector4D(0.1f, 0.1f, 0.1f, 1);
	}
}

ButtonWaiterButton::~ButtonWaiterButton(){

}

bool ButtonWaiterButton::onMouseButton(const int x, const int y, const MouseButton button, const bool pressed){
	if (pressed == pushed) return false;

	if (button == MOUSE_LEFT){
		if (pressed){
			pushed  = true;
			capture = true;
		} else {
			if (isInPanel(x, y)){
				if (buttonListener) buttonListener->onButtonClicked(this);

				waitingForButton = true;
			}
			pushed = false;
		}
	}

	return true;
}

bool ButtonWaiterButton::onKey(const unsigned int key, const bool pressed){
	if (waitingForButton){
		if (pressed && key == KEY_DELETE){
			*targetButton = -1;
			color = Vector4D(0.1f, 0.1f, 0.1f, 1);

			waitingForButton = false;
			capture = false;
			return true;
		}
	} else if (key == KEY_SPACE || key == KEY_ENTER){
		if (pushed && !pressed){
			if (buttonListener) buttonListener->onButtonClicked(this);
			waitingForButton = true;
		}
		pushed = pressed;
		return true;
	}

	return false;
}

bool ButtonWaiterButton::onJoystickButton(const int button, const bool pressed){
	if (waitingForButton){
		*targetButton = button;
		color = Vector4D(0.5f, 0.1f, 1, 1);

		waitingForButton = false;
		capture = false;
		return true;
	}

	return false;
}

void ButtonWaiterButton::draw(IRenderer *renderer, const FontID defaultFont, const SamplerStateID linearClamp, const BlendStateID blendSrcAlpha, const DepthStateID depthState){
	if (waitingForButton){
		drawButton(renderer, "<Press button>", defaultFont, linearClamp, blendSrcAlpha, depthState);
	} else {
		drawButton(renderer, text, defaultFont, linearClamp, blendSrcAlpha, depthState);
	}
}
