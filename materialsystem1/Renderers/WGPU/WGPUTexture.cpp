//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: WebGPU texture
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/ConVar.h"
#include "core/IConsoleCommands.h"

#include "../RenderWorker.h"

#include "WGPUTexture.h"
#include "WGPURenderAPI.h"
#include "WGPURenderDefs.h"
#include "imaging/ImageLoader.h"


CWGPUTexture::~CWGPUTexture()
{
	Release();
}

void CWGPUTexture::Release()
{
	for (WGPUTextureView view : m_rhiViews)
		wgpuTextureViewRelease(view);

	for (WGPUTexture texture : m_rhiTextures)
		wgpuTextureRelease(texture);

	m_rhiViews.clear();
	m_rhiTextures.clear();
}

void CWGPUTexture::Ref_DeleteObject()
{
	CWGPURenderAPI::Instance.FreeTexture(this);
	RefCountedObject::Ref_DeleteObject();
}

bool CWGPUTexture::Init(const SamplerStateParams& sampler, const ArrayCRef<CImagePtr> images, int flags)
{
	// FIXME: only release if pool, flags, format and size is different
	Release();

	HOOK_TO_CVAR(r_loadmiplevel);
	const int quality = (m_flags & TEXFLAG_NOQUALITYLOD) ? 0 : r_loadmiplevel->GetInt();

	m_samplerState = sampler;
	m_samplerState.maxAnisotropy = max(CWGPURenderAPI::Instance.GetCaps().maxTextureAnisotropicLevel, sampler.maxAnisotropy);
	m_flags = flags;

	for (CImage* image : images)
	{
		if (image->IsCube())
			m_flags |= TEXFLAG_CUBEMAP;
	}

	m_rhiTextures.reserve(images.numElem());
	m_rhiViews.reserve(images.numElem());

	for (CImagePtr img : images)
	{
		const EImageType imgType = img->GetImageType();
		const ETextureFormat imgFmt = img->GetFormat();
		const int imgMipCount = img->GetMipMapCount();
		const bool imgHasMipMaps = (imgMipCount > 1);

		const int mipStart = imgHasMipMaps ? min(quality, imgMipCount - 1) : 0;
		const int mipCount = max(imgMipCount - quality, 1);

		const int texWidth = img->GetWidth(mipStart);
		const int texHeight = img->GetHeight(mipStart);
		const int texDepth = img->GetDepth(mipStart);

		WGPUTextureDescriptor textureDesc{};
		textureDesc.label = img->GetName();
		textureDesc.mipLevelCount = img->GetMipMapCount(); // TODO: compute as we used to
		textureDesc.size = WGPUExtent3D{ (uint)texWidth, (uint)texHeight, (uint)texDepth };
		textureDesc.sampleCount = 1;
		textureDesc.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
		textureDesc.format = g_wgpuTexFormats[imgFmt];
		textureDesc.viewFormatCount = 0;
		textureDesc.viewFormats = nullptr;

		WGPUTextureViewDimension texViewDimension = WGPUTextureViewDimension_Undefined;
		switch (imgType)
		{
		case IMAGE_TYPE_1D:
			textureDesc.dimension = WGPUTextureDimension_1D;
			texViewDimension = WGPUTextureViewDimension_1D;
			break;
		case IMAGE_TYPE_2D:
			textureDesc.dimension = WGPUTextureDimension_2D;
			texViewDimension = WGPUTextureViewDimension_2D;
			break;
		case IMAGE_TYPE_3D:
			textureDesc.dimension = WGPUTextureDimension_3D;
			texViewDimension = WGPUTextureViewDimension_3D;
			break;
		case IMAGE_TYPE_CUBE:
			textureDesc.dimension = WGPUTextureDimension_2D;
			texViewDimension = WGPUTextureViewDimension_Cube;
			break;
		default:
			ASSERT_FAIL("Invalid image type of %s", img->GetName());
		}

		WGPUTexture rhiTexture = wgpuDeviceCreateTexture(CWGPURenderAPI::Instance.GetWGPUDevice(), &textureDesc);
		if (!rhiTexture)
		{
			MsgError("ERROR: failed to create texture for image %s\n", img->GetName());
			continue;
		}

		m_rhiTextures.append(rhiTexture);

		// TODO: progressive mip levels loading
		int mipMapLevel = img->GetMipMapCount() - 1;
		while (mipMapLevel >= mipStart)
		{
			g_renderWorker.Execute("UploadTexture", [=]() {
				int mipWidth = img->GetWidth(mipMapLevel);
				int mipHeight = img->GetHeight(mipMapLevel);
				const int mipDepth = img->GetDepth(mipMapLevel);
				const int lockBoxLevel = mipMapLevel - mipStart;

				if (IsCompressedFormat(imgFmt))
				{
					mipWidth = max(4, mipWidth & ~3);
					mipHeight = max(4, mipHeight & ~3);
				}

				const WGPUExtent3D texSize{ (uint)mipWidth, (uint)mipHeight, (uint)mipDepth };

				WGPUImageCopyTexture texImage{};
				texImage.texture = rhiTexture;
				texImage.aspect = WGPUTextureAspect_All;
				texImage.mipLevel = lockBoxLevel;
				texImage.origin = WGPUOrigin3D{ 0, 0, 0 };

				WGPUTextureDataLayout texLayout{};
				texLayout.offset = 0;
				if (IsCompressedFormat(imgFmt))
				{
					texLayout.bytesPerRow = ((mipWidth + 3) >> 2) * GetBytesPerBlock(imgFmt);
					texLayout.rowsPerImage = ((mipHeight + 3) >> 2);
				}
				else
				{
					texLayout.bytesPerRow = mipWidth * GetBytesPerPixel(imgFmt);
					texLayout.rowsPerImage = mipHeight;
				}

				const ubyte* src = img->GetPixels(mipMapLevel);
				const int size = img->GetMipMappedSize(mipMapLevel, 1);

				// TODO: make a temp buffer and make a command to copy it to texture
				//{
				//	WGPUCommandEncoder asyncCmdsEncoder = wgpuDeviceCreateCommandEncoder(CWGPURenderAPI::Instance.GetWGPUDevice(), nullptr);
				//
				//	//wgpuCommandEncoderCopyBufferToTexture(asyncCmdsEncoder, );
				//
				//	WGPUCommandBuffer asyncCmdBuffer = wgpuCommandEncoderFinish(asyncCmdsEncoder, nullptr);
				//	wgpuCommandEncoderRelease(asyncCmdsEncoder);
				//
				//
				//	// TODO: put asyncCmdBuffer into queue in our main thread
				//	//wgpuQueueSubmit(m_deviceQueue, 1, &asyncCmdBuffer);
				//	//wgpuCommandBufferRelease(asyncCmdBuffer);
				//}

				wgpuQueueWriteTexture(CWGPURenderAPI::Instance.GetWGPUQueue(), &texImage, src, size, &texLayout, &texSize);
				return 0;
			});
			--mipMapLevel;
		}

		{
			WGPUTextureViewDescriptor texViewDesc = {};
			texViewDesc.format = textureDesc.format;
			texViewDesc.aspect = WGPUTextureAspect_All;
			texViewDesc.arrayLayerCount = 1;
			texViewDesc.baseArrayLayer = 0;
			texViewDesc.baseMipLevel = 0;
			texViewDesc.mipLevelCount = textureDesc.mipLevelCount;
			texViewDesc.dimension = texViewDimension;
		
			WGPUTextureView rhiView = wgpuTextureCreateView(rhiTexture, &texViewDesc);
			m_rhiViews.append(rhiView);
		}

		// FIXME: check for differences?
		m_mipCount = max(m_mipCount, mipCount);
		m_width = max(m_width, texWidth);
		m_height = max(m_height, texHeight);
		m_depth = max(m_depth, texDepth);
		m_format = imgFmt;

		m_texSize += img->GetMipMappedSize(mipStart);
	}

	// hey you have concurrency errors if this assert hits!
	ASSERT_MSG(images.numElem() == m_rhiTextures.numElem(), "%s - %d images at input while %d textures created", m_name.ToCString(), images.numElem(), m_rhiTextures.numElem());

	m_animFrameCount = m_rhiTextures.numElem();
	
	return true;
}

// locks texture for modifications, etc
bool CWGPUTexture::Lock(LockInOutData& data)
{
	m_lockData = &data;

	data.lockData = PPAlloc(16 * 1024 * 1024);
	data.lockPitch = 16;
	return true;
}

// unlocks texture for modifications, etc
void CWGPUTexture::Unlock()
{
	if (!m_lockData)
		return;

	PPFree(m_lockData->lockData);
	m_lockData->lockData = nullptr;

	m_lockData = nullptr;
}
