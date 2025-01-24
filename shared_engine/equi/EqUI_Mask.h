//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: EGUI image mask
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "EqUI_Image.h"

class ITexture;
using ITexturePtr = CRefPtr<ITexture>;

namespace equi
{
// eq mask class
class Mask : public IUIControl
{
public:
	EQUI_CLASS(Mask, IUIControl)

	Mask();
	virtual ~Mask();

	void				InitFromKeyValues(const KVSection* sec, bool noClear ) override;
	void				DrawSelf(const IAARectangle& rect, IGPURenderPassRecorder* rendPassRecorder) override;
	void				RenderChilds(int depth, RenderContextAbstract& context) override;

	// UV rectangle
	AARectangle			GetUVRegion() const;
	void				SetUVRegion(const AARectangle& rect);

private:
	void				InitMask(const char* fileName);

	IAARectangle		m_renderRect;
	ITexturePtr			m_maskedChilds;
	IMaterialPtr		m_maskMaterial;
	AARectangle			m_uvRegion;
	ColorRGBA			m_color{ color_white };
};

};
