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
	//m_rhiViews.clear();
	m_rhiTexture = nullptr;
}

void CNVRHITexture::Ref_DeleteObject()
{
	if (!(m_flags & TEXFLAG_TRANSIENT))
		CWGPURenderAPI::Instance.FreeTexture(this);

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
	m_samplerState.maxAnisotropy = max(CWGPURenderAPI::Instance.GetCaps().maxTextureAnisotropicLevel, sampler.maxAnisotropy);
	
	m_flags = flags;
	if (image->IsCube())
		m_flags |= TEXFLAG_CUBEMAP;

	nvrhi::IDevice* rhiDevice = CWGPURenderAPI::Instance.GetNVRHIDevice();
	nvrhi::TextureDesc rhiTextureDesc{};
	rhiTextureDesc.debugName = m_name;
	rhiTextureDesc.mipLevels = mipCount;
	rhiTextureDesc.sampleCount = 1;
	rhiTextureDesc.isShaderResource = true;

	if (flags & TEXFLAG_STORAGE)
		rhiTextureDesc.isUAV = true;

	rhiTextureDesc.format = GetNVRHITextureFormat(imgFmt);

	if (IsCompressedFormat(imgFmt))
	{
		rhiTextureDesc.width = (texWidth + 3u) & ~3u;
		rhiTextureDesc.height = (texHeight + 3u) & ~3u;
		rhiTextureDesc.arraySize = (uint)arraySize;
	}
	else
	{
		rhiTextureDesc.width = (uint)texWidth;
		rhiTextureDesc.height = (uint)texHeight;
		rhiTextureDesc.arraySize = (uint)arraySize;
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

	nvrhi::TextureHandle rhiTexture = rhiDevice->createTexture(rhiTextureDesc);
	if (!rhiTexture)
	{
		ErrorMsg("Failed to create texture for %s\n", image->GetName());
		return false;
	}
	m_rhiTexture = rhiTexture;

	// create main texture view
	nvrhi::TextureSubresourceSet rhiTexViewDesc = {};
	rhiTexViewDesc.numArraySlices = nvrhi::TextureSubresourceSet::AllArraySlices;
	rhiTexViewDesc.numMipLevels = nvrhi::TextureSubresourceSet::AllMipLevels;
	m_rhiViews.append(rhiTexViewDesc);

	// TODO: create individual array views

	nvrhi::CommandListHandle writeCmd = rhiDevice->createCommandList();

	// DO WE NEED THIS? create staging texture for uploading data
	nvrhi::StagingTextureHandle rhiStagingTexture = rhiDevice->createStagingTexture(rhiTextureDesc, nvrhi::CpuAccessMode::Write);
	if (!rhiStagingTexture)
	{
		ErrorMsg("Failed to create staging texture for %s\n", image->GetName());
		return false;
	}

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
		CWGPUBuffer tmpBuffer(BufferInfo(1, data.lockByteCount), BUFFERUSAGE_READ | BUFFERUSAGE_COPY_DST, "TexLockReadBuffer");

		{
			IGPUCommandRecorderPtr cmdRecorder = g_renderAPI->CreateCommandRecorder("TexLockReadCmd");
			cmdRecorder->CopyTextureToBuffer(TextureCopyInfo{ this }, &tmpBuffer, data.lockSize);
			g_renderAPI->SubmitCommandBuffer(cmdRecorder->End());
		}

		IGPUBuffer::MapFuture future = tmpBuffer.Lock(0, tmpBuffer.GetSize(), 0);
		future.AddCallback([this, &data, lockByteCount](const FutureResult<BufferMapData>& result) {
			memcpy(data.lockData, result->data, lockByteCount);
		});

		// force WebGPU to process everything it has queued
		while (!future.HasResult()) {
			WGPU_INSTANCE_SPIN;
		}
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
		WGPUTextureDataLayout rhiTexLayout{};
		rhiTexLayout.offset = 0;
		rhiTexLayout.bytesPerRow = data.lockPitch;
		rhiTexLayout.rowsPerImage = data.lockSize.height;

		WGPUImageCopyTexture rhiTexDestination{};
		rhiTexDestination.texture = m_rhiTexture;
		rhiTexDestination.aspect = WGPUTextureAspect_All;
		rhiTexDestination.mipLevel = data.lockOrigin.mipLevel;
		rhiTexDestination.origin = WGPUOrigin3D{ (uint)data.lockOrigin.x, (uint)data.lockOrigin.y, (uint)data.lockOrigin.arraySlice };

		const WGPUExtent3D rhiTexSize{ (uint)data.lockSize.width, (uint)data.lockSize.height, (uint)data.lockSize.arraySize };

		if (writeCmdRecorder)
		{
			CWGPUCommandRecorder* recorder = static_cast<CWGPUCommandRecorder*>(writeCmdRecorder);
			// TODO: all of this must be CWGPUCommandRecorder::WriteTexture();

			CWGPUBuffer tmpBuffer(BufferInfo(1, data.lockByteCount), BUFFERUSAGE_COPY_SRC | BUFFERUSAGE_COPY_DST, "TexLockWriteBuffer");
			writeCmdRecorder->WriteBuffer(&tmpBuffer, data.lockData, data.lockByteCount, 0);

			WGPUImageCopyBuffer rhiTexBuffer{};
			rhiTexBuffer.layout = rhiTexLayout;
			rhiTexBuffer.buffer = tmpBuffer.GetWGPUBuffer();

			wgpuCommandEncoderCopyBufferToTexture(recorder->m_rhiCommandEncoder, &rhiTexBuffer, &rhiTexDestination, &rhiTexSize);
		}
		else
		{
			g_renderWorker.WaitForExecute("UnlockTex", [&]() {
				wgpuQueueWriteTexture(CWGPURenderAPI::Instance.GetWGPUQueue(), &rhiTexDestination, data.lockData, data.lockByteCount, &rhiTexLayout, &rhiTexSize);
				return 0;
			});
		}
	}

	PPFree(data.lockData);
	data.lockData = nullptr;

	m_lockData = nullptr;
}
