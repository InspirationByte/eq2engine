//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: EGUI label
//////////////////////////////////////////////////////////////////////////////////

#include "EqUI_Image.h"

#include "EqUI_Manager.h"
#include "IFont.h"

#include "materialsystem/MeshBuilder.h"

namespace equi
{

Image::Image() : IUIControl(), m_material(nullptr)
{

}

Image::~Image()
{
	materials->FreeMaterial(m_material);
}

void Image::InitFromKeyValues( kvkeybase_t* sec, bool noClear )
{
	BaseClass::InitFromKeyValues(sec, noClear);

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
			MsgError("EqUI error: image '%s' missing 'path' or 'atlas' property", m_name.c_str());
	}
}

void Image::SetMaterial(const char* materialName)
{
	materials->FreeMaterial(m_material);

	m_material = materials->GetMaterial(materialName);
	m_material->Ref_Grab();

	m_material->LoadShaderAndTextures();
	//materials->PutMaterialToLoadingQueue(m_material);
}

void Image::DrawSelf( const IRectangle& rect )
{
	materials->BindMaterial(m_material);
	
	// draw all rectangles with just single draw call
	CMeshBuilder meshBuilder(materials->GetDynamicMesh());
	meshBuilder.Begin(PRIM_TRIANGLE_STRIP);
		//meshBuilder.Color4fv(m_color);
		meshBuilder.TexturedQuad2(rect.GetLeftBottom(), rect.GetRightBottom(), rect.GetLeftTop(), rect.GetRightTop(), 
			m_atlasRegion.GetLeftBottom(), m_atlasRegion.GetRightBottom(), m_atlasRegion.GetLeftTop(), m_atlasRegion.GetRightTop());
	meshBuilder.End();
}

};

DECLARE_EQUI_CONTROL(image, Image)
