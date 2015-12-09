//******************* Copyright (C) Illusion Way, L.L.C 2010 *********************
//
// Description: 
//
//****************************************************************************

#ifndef _CHECKBOX_H_
#define _CHECKBOX_H_

#include "Panel.h"

class CheckBox;
class CheckBoxListener {
public:
	virtual ~CheckBoxListener(){}

	virtual void onCheckBoxClicked(CheckBox *checkBox) = 0;
};


class CheckBox : public Panel {
public:
	CheckBox(const float x, const float y, const float w, const float h, const char *txt, const bool check = false);
	virtual ~CheckBox();

	void setListener(CheckBoxListener *listener){ checkBoxListener = listener; }
	
	void setChecked(const bool ch){ checked = ch; }
	bool isChecked() const { return checked; }

	bool onMouseButton(const int x, const int y, const MouseButton button, const bool pressed);
	bool onKey(const unsigned int key, const bool pressed);

	void draw(IRenderer *renderer, const FontID defaultFont, const SamplerStateID linearClamp, const BlendStateID blendSrcAlpha, const DepthStateID depthState);

protected:
	char *text;

	CheckBoxListener *checkBoxListener;

	bool checked;
};

#endif // _CHECKBOX_H_
