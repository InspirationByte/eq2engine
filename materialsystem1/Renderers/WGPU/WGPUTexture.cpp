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
		int texDepth = img->GetDepth(mipStart);

		if (imgType == IMAGE_TYPE_CUBE)
			texDepth = 6;

		WGPUTextureDescriptor rhiTextureDesc{};
		rhiTextureDesc.label = img->GetName();
		rhiTextureDesc.mipLevelCount = img->GetMipMapCount(); // TODO: compute as we used to
		rhiTextureDesc.size = WGPUExtent3D{ (uint)texWidth, (uint)texHeight, (uint)texDepth };
		rhiTextureDesc.sampleCount = 1;
		rhiTextureDesc.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
		rhiTextureDesc.format = GetWGPUTextureFormat(imgFmt);
		rhiTextureDesc.viewFormatCount = 0;
		rhiTextureDesc.viewFormats = nullptr;

		WGPUTextureViewDimension rhiTexViewDimension = WGPUTextureViewDimension_Undefined;
		switch (imgType)
		{
		case IMAGE_TYPE_1D:
			rhiTextureDesc.dimension = WGPUTextureDimension_1D;
			rhiTexViewDimension = WGPUTextureViewDimension_1D;
			break;
		case IMAGE_TYPE_2D:
			rhiTextureDesc.dimension = WGPUTextureDimension_2D;
			rhiTexViewDimension = WGPUTextureViewDimension_2D;
			break;
		case IMAGE_TYPE_3D:
			rhiTextureDesc.dimension = WGPUTextureDimension_3D;
			rhiTexViewDimension = WGPUTextureViewDimension_3D;
			break;
		case IMAGE_TYPE_CUBE:
			rhiTextureDesc.dimension = WGPUTextureDimension_2D;
			rhiTexViewDimension = WGPUTextureViewDimension_Cube;
			break;
		default:
			ASSERT_FAIL("Invalid image type of %s", img->GetName());
		}

		WGPUTexture rhiTexture = wgpuDeviceCreateTexture(CWGPURenderAPI::Instance.GetWGPUDevice(), &rhiTextureDesc);
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
			WGPUTextureViewDescriptor rhiTexViewDesc = {};
			rhiTexViewDesc.format = rhiTextureDesc.format;
			rhiTexViewDesc.aspect = WGPUTextureAspect_All;
			rhiTexViewDesc.arrayLayerCount = texDepth;
			rhiTexViewDesc.baseArrayLayer = 0;
			rhiTexViewDesc.baseMipLevel = 0;
			rhiTexViewDesc.mipLevelCount = rhiTextureDesc.mipLevelCount;
			rhiTexViewDesc.dimension = rhiTexViewDimension;
		
			WGPUTextureView rhiView = wgpuTextureCreateView(rhiTexture, &rhiTexViewDesc);
			m_rhiViews.append(rhiView);
		}

		// FIXME: check for differences?
		m_mipCount = max(m_mipCount, mipCount);
		m_width = max(m_width, texWidth);
		m_height = max(m_height, texHeight);
		m_depth = max(m_depth, texDepth);
		m_format = imgFmt;
		m_imgType = imgType;

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
	ASSERT_MSG(!m_lockData, "CGLTexture: already locked");

	if (m_lockData)
		return false;

	if (m_rhiTextures.numElem() > 1)
	{
		ASSERT_FAIL("Couldn't handle locking of animated texture! Please tell to programmer!");
		return false;
	}

	if (IsCompressedFormat(m_format))
	{
		ASSERT_FAIL("Compressed textures aren't lockable!");
		return false;
	}

	if (data.flags & TEXLOCK_REGION_BOX)
	{
		ASSERT_FAIL("does not support locking 3D texture yet");
		return false;
	}

	int sizeToLock = 0;
	int lockOffset = 0;
	int lockPitch = 0;
	switch (m_imgType)
	{
	case IMAGE_TYPE_3D:
	{
		// COULD BE INVALID! I'VE NEVER TESTED THAT

		IBoundingBox box = (data.flags & TEXLOCK_REGION_BOX) ? data.region.box : IBoundingBox(0, 0, 0, GetWidth(), GetHeight(), GetDepth());
		const IVector3D size = box.GetSize();

		sizeToLock = size.x * size.y * size.y;
		lockOffset = box.minPoint.x * box.minPoint.y * box.minPoint.z;
		lockPitch = size.x;

		break;
	}
	case IMAGE_TYPE_CUBE:
	case IMAGE_TYPE_2D:
	{
		const IAARectangle lockRect = (data.flags & TEXLOCK_REGION_RECT) ? data.region.rectangle : IAARectangle(0, 0, GetWidth(), GetHeight());
		const IVector2D size = lockRect.GetSize();
		sizeToLock = size.x * size.y;
		lockOffset = lockRect.leftTop.x * lockRect.leftTop.y;
		lockPitch = lockRect.GetSize().x;
		break;
	}
	default:
		ASSERT_FAIL("Invalid image type of %s", GetName());
	}

	const int lockByteCount = GetBytesPerPixel(m_format) * sizeToLock;

	// allocate memory for lock data
	data.lockData = (ubyte*)PPAlloc(lockByteCount);
	data.lockPitch = lockPitch * GetBytesPerPixel(m_format);
	data.lockByteCount = lockByteCount;

	if (!(data.flags & TEXLOCK_DISCARD))
	{
		// TODO: texture readback (possibly Async)
	}

	m_lockData = &data;
	return true;
}

// unlocks texture for modifications, etc
void CWGPUTexture::Unlock()
{
	if (!m_lockData)
		return;

	ASSERT(m_lockData->lockData != nullptr);

	LockInOutData& data = *m_lockData;

	if (!(data.flags & TEXLOCK_READONLY))
	{
		g_renderWorker.WaitForExecute("UnlockTex", [&]() {

			WGPUImageCopyTexture texImage{};
			texImage.texture = m_rhiTextures[0];
			texImage.aspect = WGPUTextureAspect_All;
			texImage.mipLevel = 0;

			WGPUTextureDataLayout texLayout{};
			texLayout.offset = 0;
			texLayout.bytesPerRow = data.lockPitch;
			texLayout.rowsPerImage = m_height;

			switch (m_imgType)
			{
			case IMAGE_TYPE_3D:
			{
				IBoundingBox box = (data.flags & TEXLOCK_REGION_BOX) ? data.region.box : IBoundingBox(0, 0, 0, GetWidth(), GetHeight(), GetDepth());
				const IVector3D boxSize = box.GetSize();

				texImage.origin = WGPUOrigin3D{ (uint)box.minPoint.x, (uint)box.minPoint.y, (uint)box.minPoint.z };
				const WGPUExtent3D texSize{ (uint)boxSize.x, (uint)boxSize.y, (uint)boxSize.z };
				wgpuQueueWriteTexture(CWGPURenderAPI::Instance.GetWGPUQueue(), &texImage, data.lockData, data.lockByteCount, &texLayout, &texSize);
				break;
			}
			case IMAGE_TYPE_CUBE:
			{
				const IAARectangle lockRect = (data.flags & TEXLOCK_REGION_RECT) ? data.region.rectangle : IAARectangle(0, 0, GetWidth(), GetHeight());
				const IVector2D size = lockRect.GetSize();

				texImage.origin = WGPUOrigin3D{ (uint)lockRect.leftTop.x, (uint)lockRect.leftTop.y, (uint)data.cubeFaceIdx };
				const WGPUExtent3D texSize{ (uint)size.x, (uint)size.y, 1u };
				wgpuQueueWriteTexture(CWGPURenderAPI::Instance.GetWGPUQueue(), &texImage, data.lockData, data.lockByteCount, &texLayout, &texSize);
			}
			case IMAGE_TYPE_2D:
			{
				const IAARectangle lockRect = (data.flags & TEXLOCK_REGION_RECT) ? data.region.rectangle : IAARectangle(0, 0, GetWidth(), GetHeight());
				const IVector2D size = lockRect.GetSize();

				texImage.origin = WGPUOrigin3D{ (uint)lockRect.leftTop.x, (uint)lockRect.leftTop.y, 0u };
				const WGPUExtent3D texSize{ (uint)size.x, (uint)size.y, 1u };
				wgpuQueueWriteTexture(CWGPURenderAPI::Instance.GetWGPUQueue(), &texImage, data.lockData, data.lockByteCount, &texLayout, &texSize);
				break;
			}
			}

			return 0;
		});
	}

	PPFree(data.lockData);
	data.lockData = nullptr;

	m_lockData = nullptr;
}
