//******************* Copyright (C) Illusion Way, L.L.C 2010 *********************
//
// Description: 
//
//****************************************************************************

#ifndef _DIALOG_H_
#define _DIALOG_H_

#include "Button.h"
#include "utils/DkLinkedList.h"

struct WInfo 
{
	Panel *Panel;
	float x, y;
};

struct DialogTab 
{
	DkLinkedList <WInfo> Panels;
	char *caption;
	float rightX;
};

class Dialog : public Panel, public PushButtonListener {
public:
	Dialog(const float x, const float y, const float w, const float h, const bool modal, const bool hideOnClose);
	virtual ~Dialog();

	int addTab(const char *caption);
	void addPanel(const int tab, Panel *panel, const uint flags = 0);

	void setCurrentTab(const int tab){ currTab = tab; }

	bool onMouseMove(const int x, const int y);
	bool onMouseButton(const int x, const int y, const MouseButton button, const bool pressed);
	bool onMouseWheel(const int x, const int y, const int scroll);
	bool onKey(const unsigned int key, const bool pressed);
	bool onJoystickAxis(const int axis, const float value);
	bool onJoystickButton(const int button, const bool pressed);
	void onButtonClicked(PushButton *button);

	void draw(IFont* defaultFont);

protected:
	void close();

	float tabHeight;
	float borderWidth;

	DkList <DialogTab *> tabs;
	uint currTab;

	PushButton *closeButton;

	int sx, sy;
	bool draging;
	bool closeModeHide;
	bool showSelection;
	bool isModal;
};

#endif // _DIALOG_H_
