//******************* Copyright (C) Illusion Way, L.L.C 2010 *********************
//
// Description:
//
//****************************************************************************

#ifndef _BUTTON_H_
#define _BUTTON_H_

#include "Panel.h"

class PushButton;

class PushButtonListener {
public:
	virtual ~PushButtonListener(){}

	virtual void onButtonClicked(PushButton *button) = 0;
};


class PushButton : public Panel {
public:
	PushButton(const float x, const float y, const float w, const float h, const char *txt);
	virtual ~PushButton();

	void setListener(PushButtonListener *listener){ buttonListener = listener; }

	virtual bool onMouseButton(const int x, const int y, const MouseButton button, const bool pressed);
	virtual bool onKey(const unsigned int key, const bool pressed);

	virtual void draw(IFont* defaultFont);

protected:
	void drawButton(IFont* defaultFont, const char* text);

	char *text;

	PushButtonListener *buttonListener;

	bool pushed;
};

class KeyWaiterButton : public PushButton {
public:
	KeyWaiterButton(const float x, const float y, const float w, const float h, const char *txt, uint *key);
	virtual ~KeyWaiterButton();

	bool onMouseButton(const int x, const int y, const MouseButton button, const bool pressed);
	bool onKey(const unsigned int key, const bool pressed);

	void draw(IFont* defaultFont);
protected:
	uint *targetKey;

	bool waitingForKey;
};

class AxisWaiterButton : public PushButton {
public:
	AxisWaiterButton(const float x, const float y, const float w, const float h, const char *txt, int *axis, bool *axisInvert);
	virtual ~AxisWaiterButton();

	bool onMouseButton(const int x, const int y, const MouseButton button, const bool pressed);
	bool onKey(const unsigned int key, const bool pressed);
	bool onJoystickAxis(const int axis, const float value);

	void draw(IFont* defaultFont);
protected:
	int *targetAxis;
	bool *targetAxisInvert;

	bool waitingForAxis;
};

class ButtonWaiterButton : public PushButton {
public:
	ButtonWaiterButton(const float x, const float y, const float w, const float h, const char *txt, int *button);
	virtual ~ButtonWaiterButton();

	bool onMouseButton(const int x, const int y, const MouseButton button, const bool pressed);
	bool onKey(const unsigned int key, const bool pressed);
	bool onJoystickButton(const int button, const bool pressed);

	void draw(IFont* defaultFont);
protected:
	int *targetButton;

	bool waitingForButton;
};

#endif // _BUTTON_H_
