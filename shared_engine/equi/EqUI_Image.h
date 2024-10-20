//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: EGUI image
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "IEqUI_Control.h"

class IMaterial;
using IMaterialPtr = CRefPtr<IMaterial>;

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

	void				InitFromKeyValues(const KVSection* sec, bool noClear );

	void				SetMaterial(const char* materialName);

	// apperance
	void				SetColor(const ColorRGBA &color);
	const ColorRGBA& 	GetColor() const;

	// UV rectangle
	AARectangle			GetUVRegion() const;
	void				SetUVRegion(const AARectangle& rect);

	// events
	bool				ProcessMouseEvents(float x, float y, int nMouseButtons, int flags) {return true;}
	bool				ProcessKeyboardEvents(int nKeyButtons, int flags) {return true;}

	void				DrawSelf( const IAARectangle& rect, bool scissorOn, IGPURenderPassRecorder* rendPassRecorder);

public:

	IMaterialPtr		m_material;
	AARectangle			m_uvRegion;

	ColorRGBA			m_color;
	int					m_imageFlags;
};

};
