//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: EGUI label
//////////////////////////////////////////////////////////////////////////////////

#include "EqUI_Image.h"

#include "EqUI_Manager.h"
#include "font/IFont.h"

#include "utils/KeyValues.h"

#include "materialsystem1/IMaterialSystem.h"
#include "materialsystem1/MeshBuilder.h"
#include "materialsystem1/renderers/IShaderAPI.h"

namespace equi
{

Image::Image() : IUIControl(), m_material(nullptr)
{
	m_color = color4_white;
	m_imageFlags = 0;
}

Image::~Image()
{
	materials->FreeMaterial(m_material);
}

void Image::InitFromKeyValues( kvkeybase_t* sec, bool noClear )
{
	BaseClass::InitFromKeyValues(sec, noClear);

	m_color = KV_GetVector4D(sec->FindKeyBase("color"), 0, m_color);

	bool flipX = KV_GetValueBool(sec->FindKeyBase("flipx"), 0, (m_imageFlags & FLIP_X) > 0);
	bool flipY = KV_GetValueBool(sec->FindKeyBase("flipy"), 0, (m_imageFlags & FLIP_Y) > 0);

	m_imageFlags = (flipX ? FLIP_X : 0) | (flipY ? FLIP_Y : 0);

	kvkeybase_t* pathBase = sec->FindKeyBase("path");
	if (pathBase != nullptr)
	{
		const char* materialPath = KV_GetValueString(pathBase, 0, "ui/default");
		SetMaterial(materialPath);
		m_atlasRegion.vleftTop = Vector2D(0.0f);
		m_atlasRegion.vrightBottom = Vector2D(1.0f);
	}
	else
	{
		pathBase = sec->FindKeyBase("atlas");

		if (pathBase)
		{
			const char* atlasPath = KV_GetValueString(pathBase, 0, "");

			SetMaterial(atlasPath);

			if (m_material->GetAtlas())
			{
				TexAtlasEntry_t* entry = m_material->GetAtlas()->FindEntry(KV_GetValueString(pathBase, 1));
				if (entry)
					m_atlasRegion = entry->rect;
			}
		}
		else
			MsgError("EqUI error: image '%s' missing 'path' or 'atlas' property\n", m_name.ToCString());
	}
}

void Image::SetMaterial(const char* materialName)
{
	materials->FreeMaterial(m_material);

	m_material = materials->GetMaterial(materialName);
	m_material->Ref_Grab();

	m_material->LoadShaderAndTextures();
}

void Image::SetColor(const ColorRGBA &color)
{
	m_color = color;
}

const ColorRGBA& Image::GetColor() const
{
	return m_color;
}

void Image::DrawSelf( const IRectangle& rect, bool scissorOn)
{
	materials->SetAmbientColor(m_color);
	materials->BindMaterial(m_material);

	Rectangle_t atlasRect = m_atlasRegion;
	if (m_imageFlags & FLIP_X)
		atlasRect.FlipX();

	if (m_imageFlags & FLIP_Y)
		atlasRect.FlipY();

	// draw all rectangles with just single draw call
	CMeshBuilder meshBuilder(materials->GetDynamicMesh());
	meshBuilder.Begin(PRIM_TRIANGLE_STRIP);
		//meshBuilder.Color4fv(m_color);
		meshBuilder.TexturedQuad2(rect.GetLeftBottom(), rect.GetRightBottom(), rect.GetLeftTop(), rect.GetRightTop(), 
			atlasRect.GetLeftBottom(), atlasRect.GetRightBottom(), atlasRect.GetLeftTop(), atlasRect.GetRightTop());
	meshBuilder.End();
}

};

DECLARE_EQUI_CONTROL(image, Image)
