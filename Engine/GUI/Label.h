//******************* Copyright (C) Illusion Way, L.L.C 2010 *********************
//
// Description: 
//
//****************************************************************************

#ifndef _LABEL_H_
#define _LABEL_H_

#include "Panel.h"

class Label : public Panel {
public:
	Label(const float x, const float y, const float w, const float h, const char *txt);
	virtual ~Label();

	void draw(IFont* defaultFont);

protected:
	char *text;
};

#endif // _LABEL_H_
