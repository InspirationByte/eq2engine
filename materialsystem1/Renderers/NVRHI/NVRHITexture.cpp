//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: NVRHI texture
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/ConVar.h"
#include "core/IConsoleCommands.h"

#include "../RenderWorker.h"

#include "NVRHITexture.h"
#include "NVRHIRenderAPI.h"
#include "NVRHIRenderDefs.h"
#include "NVRHICommandRecorder.h"
#include "imaging/ImageLoader.h"

CNVRHITexture::~CNVRHITexture()
{
	Release();
}

void CNVRHITexture::Release()
{
	m_rhiViews.clear();
	m_rhiTexture = nullptr;
}

void CNVRHITexture::Ref_DeleteObject()
{
	if (!(m_flags & TEXFLAG_TRANSIENT))
		CNVRHIRenderAPI::Instance.FreeTexture(this);

	RefCountedObject::Ref_DeleteObject();
}

bool CNVRHITexture::Init(const CRefPtr<CImage> image, const SamplerStateParams& sampler, int flags)
{
	HOOK_TO_CVAR(r_loadmiplevel);

	DevMsg(DEVMSG_RENDER, "Creating texture from image %s\n", image->GetName());

	Release();

	const int quality = (m_flags & TEXFLAG_IGNORE_QUALITY) ? 0 : r_loadmiplevel->GetInt();

	const EImageType imgType = image->GetImageType();
	const ETextureFormat imgFmt = image->GetFormat();
	const int imgMipCount = image->GetMipMapCount();
	const bool imgHasMipMaps = (imgMipCount > 1);
	const int mipStart = imgHasMipMaps ? min(quality, imgMipCount - 1) : 0;
	const int mipCount = max(imgMipCount - quality, 1);
	const int texWidth = image->GetWidth(mipStart);
	const int texHeight = image->GetHeight(mipStart);

	if (IsCompressedFormat(imgFmt) && !((texWidth % 4) == 0 && (texHeight % 4) == 0))
	{
		MsgWarning("Error: Compressed texture %s size %dx%d (mipStart %d) is not a multiple of 4\n", image->GetName(), texWidth, texHeight, mipStart);
	}

	const int arraySize = image->GetArraySize();
	const int arrayLayerCount = (imgType == IMAGE_TYPE_CUBE) ? ITexture::CubeArraySlice(0, arraySize) : arraySize;

	//m_rhiViews.reserve(arraySize);

	m_texSize = image->GetMipMappedSize(mipStart);
	m_arraySize = arraySize;
	m_mipCount = mipCount;
	m_width = texWidth;
	m_height = texHeight;
	m_arraySize = arraySize;
	m_format = imgFmt;
	m_imgType = imgType;

	// since texture is initialized from image buffer, it neeeds copy destination flag
	m_samplerState = sampler;
	m_samplerState.maxAnisotropy = max(CNVRHIRenderAPI::Instance.GetCaps().maxTextureAnisotropicLevel, sampler.maxAnisotropy);
	
	m_flags = flags;
	if (image->IsCube())
		m_flags |= TEXFLAG_CUBEMAP;

	auto rhiTextureDesc = nvrhi::TextureDesc()
		.setMipLevels(mipCount)
		.setIsUAV((flags & TEXFLAG_STORAGE) != 0)
		.setFormat(GetNVRHITextureFormat(imgFmt));

	if (IsCompressedFormat(imgFmt))
	{
		rhiTextureDesc
			.setWidth((texWidth + 3u) & ~3u)
			.setHeight((texHeight + 3u) & ~3u)
			.setArraySize((uint)arraySize);
	}
	else
	{
		rhiTextureDesc
			.setWidth((uint)texWidth)
			.setHeight((uint)texHeight)
			.setArraySize((uint)arraySize);
	}

	switch (imgType)
	{
	case IMAGE_TYPE_1D:
		rhiTextureDesc.dimension = nvrhi::TextureDimension::Texture1D;
		break;
	case IMAGE_TYPE_2D:
		rhiTextureDesc.dimension = (arraySize > 1) ? nvrhi::TextureDimension::Texture2DArray : nvrhi::TextureDimension::Texture2D;
		break;
	case IMAGE_TYPE_3D:
		rhiTextureDesc.dimension = (arraySize > 1) ? nvrhi::TextureDimension::Texture2DArray : nvrhi::TextureDimension::Texture3D; // is that correct?
		break;
	case IMAGE_TYPE_CUBE:
		rhiTextureDesc.dimension = (arraySize > 1) ? nvrhi::TextureDimension::TextureCubeArray : nvrhi::TextureDimension::TextureCube;
		break;
	default:
		ASSERT_FAIL("Invalid image type of %s", image->GetName());
	}

	nvrhi::IDevice* rhiDevice = CNVRHIRenderAPI::Instance.GetNVRHIDevice();

	rhiTextureDesc.debugName = m_name.ToCString();
	nvrhi::TextureHandle rhiTexture = rhiDevice->createTexture(rhiTextureDesc);
	if (!rhiTexture)
	{
		ErrorMsg("Failed to create texture for %s\n", image->GetName());
		return false;
	}
	m_rhiTexture = rhiTexture;

	// create default texture view
	{
		auto rhiDefaultTexViewDesc = nvrhi::TextureSubresourceSet()
			.setNumMipLevels(nvrhi::TextureSubresourceSet::AllMipLevels)
			.setNumArraySlices(nvrhi::TextureSubresourceSet::AllArraySlices);
		m_rhiViews.append(rhiDefaultTexViewDesc);
	}

	// TODO: create individual array views

	nvrhi::CommandListHandle writeCmd = rhiDevice->createCommandList();
	for (int arrIdx = 0; arrIdx < arraySize; ++arrIdx)
	{
		int mipMapLevel = image->GetMipMapCount() - 1;
		while (mipMapLevel >= mipStart)
		{
			int mipWidth = image->GetWidth(mipMapLevel);
			int mipHeight = image->GetHeight(mipMapLevel);
			const int mipDepth = image->GetDepth(mipMapLevel);
			const int lockBoxLevel = mipMapLevel - mipStart;

			if (IsCompressedFormat(imgFmt))
			{
				mipWidth = max(4, mipWidth & ~3);
				mipHeight = max(4, mipHeight & ~3);
			}

			nvrhi::TextureSlice rhiTexSlice = {};
			rhiTexSlice.mipLevel = lockBoxLevel;
			rhiTexSlice.arraySlice = arrIdx;

			uint bytesPerRow;
			if (IsCompressedFormat(imgFmt))
				bytesPerRow = ((mipWidth + 3) >> 2) * GetBytesPerBlock(imgFmt);
			else
				bytesPerRow = mipWidth * GetBytesPerPixel(imgFmt);

			const ubyte* src = image->GetPixels(mipMapLevel, arrIdx);
			const int size = image->GetMipMappedSize(mipMapLevel, 1);

			writeCmd->writeTexture(rhiTexture, arrIdx, lockBoxLevel, src, bytesPerRow);

			--mipMapLevel;
		}
	}
	rhiDevice->executeCommandList(writeCmd);

	return true;
}

// locks texture for modifications, etc
bool CNVRHITexture::Lock(LockInOutData& data)
{
	ASSERT_MSG(!m_lockData, "CWGPUTexture: already locked");

	if (m_lockData)
		return false;

	if (IsCompressedFormat(m_format))
	{
		ASSERT_FAIL("Compressed textures aren't lockable yet!");
		return false;
	}

	ASSERT(data.lockOrigin.x >= 0 && data.lockOrigin.y >= 0 && data.lockOrigin.arraySlice >= 0);
	ASSERT(data.lockSize.width >= 0 && data.lockSize.height >= 0 && data.lockSize.arraySize >= 0);

	const int lockOffset = data.lockOrigin.x * data.lockOrigin.y * data.lockOrigin.arraySlice;
	const int sizeToLock = data.lockSize.width * data.lockSize.height * data.lockSize.arraySize;

	const int lockPitch = data.lockSize.width * GetBytesPerPixel(m_format);
	const int lockByteCount = GetBytesPerPixel(m_format) * sizeToLock;

	// allocate memory for lock data
	data.lockData = (ubyte*)PPAlloc(lockByteCount);
	data.lockPitch = lockPitch;
	data.lockByteCount = lockByteCount;

	if (!(data.flags & TEXLOCK_DISCARD) && (m_flags & TEXFLAG_COPY_SRC))
	{
		nvrhi::IDevice* rhiDevice = CNVRHIRenderAPI::Instance.GetNVRHIDevice();

		// create staging texture
		auto rhiTextureDesc = nvrhi::TextureDesc()
			.setMipLevels(1)
			.setSampleCount(1)
			.setArraySize(1)
			.setFormat(GetNVRHITextureFormat(m_format));

		if (IsCompressedFormat(m_format))
		{
			rhiTextureDesc
				.setWidth((data.lockSize.width + 3u) & ~3u)
				.setHeight((data.lockSize.height + 3u) & ~3u);
		}
		else
		{
			rhiTextureDesc.setWidth((uint)data.lockSize.width);
			rhiTextureDesc.setHeight((uint)data.lockSize.height);
		}

		switch (m_imgType)
		{
		case IMAGE_TYPE_1D:
			rhiTextureDesc.dimension = nvrhi::TextureDimension::Texture1D;
			break;
		case IMAGE_TYPE_2D:
			rhiTextureDesc.dimension = nvrhi::TextureDimension::Texture2D;
			break;
		case IMAGE_TYPE_3D:
			rhiTextureDesc.dimension = nvrhi::TextureDimension::Texture3D;
			break;
		case IMAGE_TYPE_CUBE:
			rhiTextureDesc.dimension = nvrhi::TextureDimension::TextureCube;
			break;
		default:
			ASSERT_FAIL("Invalid image type of %s", m_name.ToCString());
		}

		// Need Staging texture to read into CPU memory
		rhiTextureDesc.debugName = m_name.ToCString();
		nvrhi::StagingTextureHandle rhiStagingTexture = rhiDevice->createStagingTexture(rhiTextureDesc, nvrhi::CpuAccessMode::Write);
		if (!rhiStagingTexture)
		{
			ErrorMsg("Failed to create staging texture when locking %s\n", m_name.ToCString());
			PPFree(data.lockData);
			data.lockData = nullptr;
			return false;
		}

		nvrhi::TextureSlice rhiSrcSlice = {};
		rhiSrcSlice.arraySlice = data.lockOrigin.arraySlice;
		rhiSrcSlice.mipLevel = data.lockOrigin.mipLevel;
		//rhiSrcSlice.depth = data.lockSize.depth;
		rhiSrcSlice.width = data.lockSize.width;
		rhiSrcSlice.height = data.lockSize.height;

		nvrhi::TextureSlice rhiDstSlice = {};
		rhiSrcSlice.width = data.lockSize.width;
		rhiSrcSlice.height = data.lockSize.height;

		nvrhi::CommandListHandle copyCmd = rhiDevice->createCommandList();
		copyCmd->copyTexture(rhiStagingTexture, rhiDstSlice, m_rhiTexture, rhiSrcSlice);
		rhiDevice->executeCommandList(copyCmd);

		size_t rowPitch = 0;
		void* mapData = rhiDevice->mapStagingTexture(rhiStagingTexture, rhiDstSlice, nvrhi::CpuAccessMode::Read, &rowPitch);
		ASSERT_MSG(mapData, "Failed to lock staging texture for reading %s\n", m_name.ToCString());

		memcpy(data.lockData, mapData, lockByteCount);
		rhiDevice->unmapStagingTexture(rhiStagingTexture);
	}

	m_lockData = &data;
	return true;
}

// unlocks texture for modifications, etc
void CNVRHITexture::Unlock(IGPUCommandRecorder* writeCmdRecorder)
{
	if (!m_lockData)
		return;

	ASSERT(m_lockData->lockData != nullptr);

	LockInOutData& data = *m_lockData;

	if (!(data.flags & TEXLOCK_READONLY))
	{
		nvrhi::IDevice* rhiDevice = CNVRHIRenderAPI::Instance.GetNVRHIDevice();
		if (writeCmdRecorder)
		{
			CNVRHICommandRecorder* recorder = static_cast<CNVRHICommandRecorder*>(writeCmdRecorder);
			recorder->m_rhiCommandList->writeTexture(m_rhiTexture, data.lockOrigin.arraySlice, data.lockOrigin.mipLevel, data.lockData, data.lockPitch);
		}
		else
		{
			nvrhi::CommandListHandle writeCmd = rhiDevice->createCommandList();
			writeCmd->writeTexture(m_rhiTexture, data.lockOrigin.arraySlice, data.lockOrigin.mipLevel, data.lockData, data.lockPitch);
			rhiDevice->executeCommandList(writeCmd);
		}
	}

	PPFree(data.lockData);
	data.lockData = nullptr;

	m_lockData = nullptr;
}
