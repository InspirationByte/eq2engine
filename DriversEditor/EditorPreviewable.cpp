//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Editor previewable object
//////////////////////////////////////////////////////////////////////////////////

#include "EditorPreviewable.h"
#include "materialsystem/renderers/IShaderAPI.h"
#include "imaging/ImageLoader.h"

CEditorPreviewable::CEditorPreviewable() :
	m_preview(nullptr),
	m_dirtyPreview(true)
{
}

CEditorPreviewable::~CEditorPreviewable()
{
	if(m_preview)
		g_pShaderAPI->FreeTexture(m_preview);

	m_preview = NULL;
	m_dirtyPreview = true;
}

void CEditorPreviewable::SetDirtyPreview()
{
	m_dirtyPreview = true;
}

bool CEditorPreviewable::IsDirtyPreview() const
{
	return m_dirtyPreview;
}

ITexture* CEditorPreviewable::GetPreview() const
{
	return m_preview;
}

bool CEditorPreviewable::CreatePreview(int texSize)
{
	if(m_preview)
		return true;

	CImage emptyImage;
	emptyImage.Create(FORMAT_RGBA8, texSize, texSize, 1, 1);

	DkList<CImage*> images;
	images.append(&emptyImage);

	SamplerStateParam_t sampler = g_pShaderAPI->MakeSamplerState(TEXFILTER_TRILINEAR, ADDRESSMODE_CLAMP, ADDRESSMODE_CLAMP, ADDRESSMODE_CLAMP);

	m_preview = g_pShaderAPI->CreateTexture(images, sampler);

	if(!m_preview)
		return false;

	m_preview->Ref_Grab();

	return true;
}
