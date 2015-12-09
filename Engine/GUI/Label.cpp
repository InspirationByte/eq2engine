//******************* Copyright (C) Illusion Way, L.L.C 2010 *********************
//
// Description: 
//
//****************************************************************************

#include "Label.h"

Label::Label(const float x, const float y, const float w, const float h, const char *txt){
	setPosition(x, y);
	setSize(w, h);

	text = new char[strlen(txt) + 1];
	strcpy(text, txt);

	enabled = false;
}

Label::~Label(){
	delete text;
}

void Label::draw(IFont* defaultFont)
{
	float textWidth = 0.75f * height;

	float tw = defaultFont->GetStringLength(text,strlen(text));
	float maxW = width / tw;
	if (textWidth > maxW) textWidth = maxW;

	//g_pShaderAPI->DrawSetColor(color1);
	defaultFont->DrawText(text, xPos, yPos, textWidth, height);
}
