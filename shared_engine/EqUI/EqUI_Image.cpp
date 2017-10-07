//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
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

	const char* materialPath = KV_GetValueString(sec->FindKeyBase("path"), 0, "ui/default");

	SetMaterial( materialPath );
}

void Image::SetMaterial(const char* materialName)
{
	materials->FreeMaterial(m_material);

	m_material = materials->GetMaterial(materialName);
	m_material->Ref_Grab();

	materials->PutMaterialToLoadingQueue(m_material);
}

void Image::DrawSelf( const IRectangle& rect )
{
	materials->BindMaterial(m_material);
	
	// draw all rectangles with just single draw call
	CMeshBuilder meshBuilder(materials->GetDynamicMesh());
	meshBuilder.Begin(PRIM_TRIANGLE_STRIP);
		//meshBuilder.Color4fv(m_color);
		meshBuilder.TexturedQuad2(rect.GetLeftBottom(), rect.GetRightBottom(), rect.GetLeftTop(), rect.GetRightTop(), 
									Vector2D(0,1), Vector2D(1,1), Vector2D(0,0), Vector2D(1, 0));
	meshBuilder.End();
}

};

DECLARE_EQUI_CONTROL(image, Image)
