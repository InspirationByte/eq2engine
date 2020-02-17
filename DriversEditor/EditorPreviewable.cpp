//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
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

	SamplerStateParam_t sampler = g_pShaderAPI->MakeSamplerState(TEXFILTER_TRILINEAR, TEXADDRESS_CLAMP, TEXADDRESS_CLAMP, TEXADDRESS_CLAMP);

	m_preview = g_pShaderAPI->CreateTexture(images, sampler);

	if(!m_preview)
		return false;

	m_preview->Ref_Grab();

	return true;
}

void UTIL_CopyRendertargetToTexture(ITexture* rt, ITexture* dest)
{
	ASSERT(rt->GetWidth() == dest->GetWidth() && rt->GetHeight() == dest->GetHeight());

	texlockdata_t readFrom;
	texlockdata_t writeTo;
	memset(&readFrom, 0, sizeof(readFrom));

	rt->Lock(&readFrom, NULL, false, true);

	if(readFrom.pData)
	{
		dest->Lock(&writeTo, NULL, true, false);

		if(writeTo.pData)
			memcpy(writeTo.pData, readFrom.pData, writeTo.nPitch*rt->GetHeight());
	}

	dest->Unlock();
	rt->Unlock();
}