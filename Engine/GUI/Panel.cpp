//******************* Copyright (C) Illusion Way, L.L.C 2010 *********************
//
// Description: Main panel class
// TODO: GUI Styles
//
//****************************************************************************

#include "Panel.h"

//TexID Panel::corner = TEXTURE_NONE;
//TexID Panel::check  = TEXTURE_NONE;

bool Panel::isInPanel(const int x, const int y) const {
	return ((x >= xPos) && (x <= xPos + width) && (y >= yPos) && (y <= yPos + height));
}

void Panel::setPosition(const float x, const float y){
	xPos = x;
	yPos = y;
}

void Panel::setSize(const float w, const float h){
	width  = w;
	height = h;
}

void Panel::drawShadedRectangle(Vector2D &mins,Vector2D &maxs,Vector4D &color1,Vector4D &color2)
{
	color1.w = 1;
	color2.w = 1;

	g_pShaderAPI->DrawSetColor(color1);
	g_pShaderAPI->DrawLine2D(mins,Vector2D(maxs.x,mins.y));
	g_pShaderAPI->DrawSetColor(color2);
	g_pShaderAPI->DrawLine2D(Vector2D(mins.x,maxs.y),maxs);

	g_pShaderAPI->DrawSetColor(color1);
	g_pShaderAPI->DrawLine2D(mins,Vector2D(mins.x,maxs.y));
	g_pShaderAPI->DrawSetColor(color2);
	g_pShaderAPI->DrawLine2D(Vector2D(maxs.x,mins.y),maxs);
}
/*
void Panel::drawSoftBorderQuad(IRenderer *renderer, const SamplerStateID linearClamp, const BlendStateID blendSrcAlpha, const DepthStateID depthState, const float x0, const float y0, const float x1, const float y1, const float borderWidth, const float colScale, const float transScale)
{
	if (corner == TEXTURE_NONE){
		ubyte pixels[32][32][4];

		for (int y = 0; y < 32; y++){
			for (int x = 0; x < 32; x++){
				int r = 255 - int(powf(sqrtf(float(x * x + y * y)) * (255.0f / 31.0f), 1.0f));
				if (r < 0) r = 0;
				pixels[y][x][0] = r;
				pixels[y][x][1] = r;
				pixels[y][x][2] = r;
				pixels[y][x][3] = r;
			}
		}

		Image img;
		img.loadFromMemory(pixels, FORMAT_RGBA8, 32, 32, 1, 1, false);
		corner = renderer->addTexture(img, false, linearClamp);
	}

	float x0bw = x0 + borderWidth;
	float y0bw = y0 + borderWidth;
	float x1bw = x1 - borderWidth;
	float y1bw = y1 - borderWidth;

	TexVertex border[] = {
		TexVertex(Vector2D(x0,   y0bw), Vector2D(1, 0)),
		TexVertex(Vector2D(x0,   y0  ), Vector2D(1, 1)),
		TexVertex(Vector2D(x0bw, y0bw), Vector2D(0, 0)),
		TexVertex(Vector2D(x0bw, y0  ), Vector2D(0, 1)),
		TexVertex(Vector2D(x1bw, y0bw), Vector2D(0, 0)),
		TexVertex(Vector2D(x1bw, y0  ), Vector2D(0, 1)),

		TexVertex(Vector2D(x1bw, y0  ), Vector2D(0, 1)),
		TexVertex(Vector2D(x1,   y0  ), Vector2D(1, 1)),
		TexVertex(Vector2D(x1bw, y0bw), Vector2D(0, 0)),
		TexVertex(Vector2D(x1,   y0bw), Vector2D(1, 0)),
		TexVertex(Vector2D(x1bw, y1bw), Vector2D(0, 0)),
		TexVertex(Vector2D(x1,   y1bw), Vector2D(1, 0)),

		TexVertex(Vector2D(x1,   y1bw), Vector2D(1, 0)),
		TexVertex(Vector2D(x1,   y1  ), Vector2D(1, 1)),
		TexVertex(Vector2D(x1bw, y1bw), Vector2D(0, 0)),
		TexVertex(Vector2D(x1bw, y1  ), Vector2D(0, 1)),
		TexVertex(Vector2D(x0bw, y1bw), Vector2D(0, 0)),
		TexVertex(Vector2D(x0bw, y1  ), Vector2D(0, 1)),

		TexVertex(Vector2D(x0bw, y1  ), Vector2D(0, 1)),
		TexVertex(Vector2D(x0,   y1  ), Vector2D(1, 1)),
		TexVertex(Vector2D(x0bw, y1bw), Vector2D(0, 0)),
		TexVertex(Vector2D(x0,   y1bw), Vector2D(1, 0)),
		TexVertex(Vector2D(x0bw, y0bw), Vector2D(0, 0)),
		TexVertex(Vector2D(x0,   y0bw), Vector2D(1, 0)),
	};
	Vector4D col = color * Vector4D(colScale, colScale, colScale, transScale);

	renderer->drawTextured(PRIM_TRIANGLE_STRIP, border, elementsOf(border), corner, linearClamp, blendSrcAlpha, depthState, &col);

	// Center
	Vector2D center[] = { Vector2D(x0bw, y0bw), Vector2D(x1bw, y0bw), Vector2D(x0bw, y1bw), Vector2D(x1bw, y1bw) };
	renderer->drawPlain(PRIM_TRIANGLE_STRIP, center, 4, blendSrcAlpha, depthState, &col);
}
*/