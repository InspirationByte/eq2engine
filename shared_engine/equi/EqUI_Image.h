//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: EGUI image
//////////////////////////////////////////////////////////////////////////////////

#ifndef EQUI_IMAGE_H
#define EQUI_IMAGE_H

#include "equi_defs.h"
#include "IEqUI_Control.h"

class IMaterial;

namespace equi
{

	enum EImageFlags
	{
		FLIP_X = (1 << 0),
		FLIP_Y = (1 << 1),
	};

// eq label class
class Image : public IUIControl
{
public:
	EQUI_CLASS(Image, IUIControl)

	Image();
	virtual ~Image();

	void				InitFromKeyValues( kvkeybase_t* sec, bool noClear );

	void				SetMaterial(const char* materialName);

	// apperance
	void				SetColor(const ColorRGBA &color);
	const ColorRGBA& 	GetColor() const;

	// events
	bool				ProcessMouseEvents(float x, float y, int nMouseButtons, int flags) {return true;}
	bool				ProcessKeyboardEvents(int nKeyButtons, int flags) {return true;}

	void				DrawSelf( const IRectangle& rect);

public:

	IMaterial*			m_material;
	Rectangle_t			m_atlasRegion;

	ColorRGBA			m_color;
	int					m_imageFlags;
};

};


#endif // EQUI_IMAGE_H