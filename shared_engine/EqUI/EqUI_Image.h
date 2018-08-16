//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: EGUI label
//////////////////////////////////////////////////////////////////////////////////

#ifndef EQUI_LABEL_H
#define EQUI_LABEL_H

#include "equi_defs.h"
#include "IEqUI_Control.h"

class IMaterial;

namespace equi
{

// eq label class
class Image : public IUIControl
{
public:
	EQUI_CLASS(Image, IUIControl)

	Image();
	virtual ~Image();

	void			InitFromKeyValues( kvkeybase_t* sec, bool noClear );

	void			SetMaterial(const char* materialName);

	// events
	bool			ProcessMouseEvents(float x, float y, int nMouseButtons, int flags) {return true;}
	bool			ProcessKeyboardEvents(int nKeyButtons, int flags) {return true;}

	void			DrawSelf( const IRectangle& rect);

public:

	IMaterial*		m_material;
	Rectangle_t		m_atlasRegion;
};

};


#endif // EQUI_LABEL_H