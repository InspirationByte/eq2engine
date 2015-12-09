//******************* Copyright (C) Illusion Way, L.L.C 2010 *********************
//
// Description: 
//
//****************************************************************************

#ifndef _Panel_H_
#define _Panel_H_

#include "Renderer/IShaderAPI.h"
#include "DebugInterface.h"
#include "IEngine.h"
#include "in_keys_ident.h"

enum MouseButton {
	MOUSE_LEFT   = 0,
	MOUSE_MIDDLE = 1,
	MOUSE_RIGHT  = 2,
};


class Panel {
public:
	Panel(){
		visible = true;
		capture = false;
		dead = false;
		enabled = true;
	}
	virtual ~Panel(){}

	virtual bool isInPanel(const int x, const int y) const;

	virtual bool onMouseMove(const int x, const int y){ return false; }
	virtual bool onMouseButton(const int x, const int y, const MouseButton button, const bool pressed){ return false; }
	virtual bool onMouseWheel(const int x, const int y, const int scroll){ return false; }
	virtual bool onKey(const unsigned int key, const bool pressed){ return false; }
	virtual bool onJoystickAxis(const int axis, const float value){ return false; }
	virtual bool onJoystickButton(const int button, const bool pressed){ return false; }
	virtual void onSize(const int w, const int h){}
	virtual void onFocus(const bool focus){}

	virtual void draw(IFont* defaultFont) = 0;

	void setPosition(const float x, const float y);
	void setSize(const float w, const float h);
	float getX() const { return xPos; }
	float getY() const { return yPos; }
	float getWidth()  const { return width;  }
	float getHeight() const { return height; }
	void setColor(const Vector4D &col){ color = col; }
//	void setCapturing(const bool capturing){ capture = capturing; }
	void setVisible(const bool isVisible){ visible = isVisible; }
	void setEnabled(const bool isEnabled){ enabled = isEnabled; }

	bool isVisible() const { return visible; }
	bool isCapturing() const { return capture; }
	bool isDead() const { return dead; }
	bool isEnabled() const { return enabled; }

	//static void clean(){corner = check = NULL; }
protected:

	//static ITexture* corner, *check;

	//void drawSoftBorderQuad(IRenderer *renderer, const SamplerStateID linearClamp, const BlendStateID blendSrcAlpha, const DepthStateID depthState, const float x0, const float y0, const float x1, const float y1, const float borderWidth, const float colScale = 1, const float transScale = 1);
	void drawShadedRectangle(Vector2D &mins,Vector2D &maxs,Vector4D &color1,Vector4D &color2);

	Vector4D color;
	Vector4D rendercolor;
	float xPos, yPos, width, height;

	bool visible;
	bool capture;
	bool dead;
	bool enabled;
};

#endif // _Panel_H_
