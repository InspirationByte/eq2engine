//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: EGUI label
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "utils/KeyValues.h"
#include "utils/TextureAtlas.h"
#include "EqUI_Image.h"

#include "EqUI_Manager.h"

#include "materialsystem1/IMaterialSystem.h"
#include "materialsystem1/MeshBuilder.h"

namespace equi
{

Image::Image() : IUIControl()
{
	m_color = color_white;
	m_imageFlags = 0;
}

Image::~Image()
{
}

void Image::Parse(const KVSection& sec )
{
	BaseClass::Parse(sec);

	m_color = KV_GetVector4D(sec.FindSection("color"), 0, m_color);

	const bool flipX = KV_GetValueBool(sec.FindSection("flipX"), 0, (m_imageFlags & FLIP_X) > 0);
	const bool flipY = KV_GetValueBool(sec.FindSection("flipY"), 0, (m_imageFlags & FLIP_Y) > 0);

	m_imageFlags = (flipX ? FLIP_X : 0) | (flipY ? FLIP_Y : 0);
	m_uvRegion = AARectangle(0.0f, 0.0f, 1.0f, 1.0f);

	const KVSection* pathBase = sec.FindSection("path");
	if (pathBase)
	{
		EqStringRef materialName = "ui/default";
		pathBase->GetValues(materialName);
		SetMaterial(materialName);

		sec.Get("uvLeftTop").GetValues(m_uvRegion.leftTop);
		sec.Get("uvRightBottom").GetValues(m_uvRegion.rightBottom);
		return;
	}

	pathBase = sec.FindSection("atlas");
	if (pathBase)
	{
		EqStringRef materialName, imageName;
		pathBase->GetValues(materialName, imageName);

		SetAtlasImage(materialName, imageName);
	}
	else
	{
		MsgError("EqUI error: image '%s' missing 'path' or 'atlas' property\n", m_name.ToCString());
	}
}

void Image::SetMaterial(const char* materialName)
{
	m_uvRegion = AARectangle(0.0f, 0.0f, 1.0f, 1.0f);
	SetMaterialInternal(g_matSystem->GetMaterial(materialName));
}

void Image::SetMaterialInternal(IMaterialPtr material)
{
	m_material = material;
	g_matSystem->QueueLoading(m_material);
}

void Image::SetAtlasImage(const char* materialAtlasName, const char* imageName)
{
	const EqStringRef atlasImg = imageName ? EqStringRef(imageName) : EqStringRef(m_atlasImageName);

	SetMaterial(materialAtlasName);
	if (!m_material->GetAtlas())
	{
		MsgError("EqUI error: material %s has no atlas\n", materialAtlasName);
		return;
	}

	const AtlasEntry* entry = m_material->GetAtlas()->FindEntry(atlasImg);
	if (entry)
		m_uvRegion = entry->rect;
	else
		MsgError("EqUI error: image %s - no atlas entry '%s' in '%s'\n", m_name.ToCString(), atlasImg.ToCString(), materialAtlasName);

	m_atlasImageName = atlasImg;
}

const char* Image::GetMaterialName() const
{
	return m_material->GetName();
}

const char* Image::GetAtlasImageName() const
{
	return m_atlasImageName;
}

void Image::SetColor(const ColorRGBA &color)
{
	m_color = color;
}

const ColorRGBA& Image::GetColor() const
{
	return m_color;
}

AARectangle Image::GetUVRegion() const
{
	const TextureExtent texSize = m_material->GetBaseTexture(0) ? m_material->GetBaseTexture(0)->GetSize() : TextureExtent{ 1, 1 };
	return AARectangle(m_uvRegion.leftTop * texSize.xy(), m_uvRegion.rightBottom * texSize.xy());
}

void Image::SetUVRegion(const AARectangle& rect)
{
	const TextureExtent texSize = m_material->GetBaseTexture(0) ? m_material->GetBaseTexture(0)->GetSize() : TextureExtent{ 1, 1 };
	const Vector2D invSize = 1.0f / texSize.xy();
	m_uvRegion = AARectangle(rect.leftTop * invSize, rect.rightBottom * invSize);
}

void Image::DrawSelf( const IAARectangle& rect, IGPURenderPassRecorder* rendPassRecorder)
{
	AARectangle uvRect = m_uvRegion;
	if (m_imageFlags & FLIP_X)
		uvRect.FlipX();

	if (m_imageFlags & FLIP_Y)
		uvRect.FlipY();

	// draw all rectangles with just single draw call
	CMeshBuilder meshBuilder(g_matSystem->GetDynamicMesh());

	RenderDrawCmd drawCmd;
	drawCmd.SetMaterial(m_material);

	MatSysDefaultRenderPass defaultRenderPass;
	defaultRenderPass.blendMode = SHADER_BLEND_TRANSLUCENT;
	defaultRenderPass.cullMode = CULL_NONE;

	meshBuilder.Begin(PRIM_TRIANGLE_STRIP);
		meshBuilder.Color4fv(m_color);
		meshBuilder.TexturedQuad2(rect.GetLeftBottom(), rect.GetLeftTop(), rect.GetRightBottom(), rect.GetRightTop(),
			uvRect.GetLeftBottom(), uvRect.GetLeftTop(), uvRect.GetRightBottom(), uvRect.GetRightTop());
	if(meshBuilder.End(drawCmd))
		g_matSystem->SetupDrawCommand(drawCmd, RenderPassContext(rendPassRecorder, &defaultRenderPass));
}

};

DECLARE_EQUI_CONTROL(image, Image)
