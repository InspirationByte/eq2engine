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
#include "WGPUCommandRecorder.h"
#include "imaging/ImageLoader.h"

CWGPUTexture::~CWGPUTexture()
{
	Release();
}

void CWGPUTexture::Release()
{
	for (WGPUTextureView view : m_rhiViews)
		wgpuTextureViewRelease(view);
	m_rhiViews.clear();

	if (m_rhiTexture)
		wgpuTextureRelease(m_rhiTexture);
	m_rhiTexture = nullptr;
}

void CWGPUTexture::Ref_DeleteObject()
{
	if (!(m_flags & TEXFLAG_TRANSIENT))
		CWGPURenderAPI::Instance.FreeTexture(this);

	RefCountedObject::Ref_DeleteObject();
}

bool CWGPUTexture::Init(const CRefPtr<CImage> image, const SamplerStateParams& sampler, int flags)
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
		MsgError("Error: Compressed texture %s size %dx%d (mipStart %d) is not a multiple of 4", image->GetName(), texWidth, texHeight, mipStart);
		return false;
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
	
	WGPUTextureUsage rhiUsageFlags = WGPUTextureUsage_TextureBinding;
	if (flags & TEXFLAG_STORAGE) rhiUsageFlags |= WGPUTextureUsage_StorageBinding;
	if (flags & TEXFLAG_COPY_SRC) rhiUsageFlags |= WGPUTextureUsage_CopySrc;
	if (flags & TEXFLAG_COPY_DST) rhiUsageFlags |= WGPUTextureUsage_CopyDst;
	rhiUsageFlags |= WGPUTextureUsage_CopyDst;

	m_flags = flags;
	if (image->IsCube())
		m_flags |= TEXFLAG_CUBEMAP;

	WGPUTextureDescriptor rhiTextureDesc{};
	rhiTextureDesc.label = _WSTR(m_name);
	rhiTextureDesc.mipLevelCount = mipCount;
	rhiTextureDesc.size = WGPUExtent3D{ (uint)texWidth, (uint)texHeight, (uint)arrayLayerCount };
	rhiTextureDesc.sampleCount = 1;
	rhiTextureDesc.usage = rhiUsageFlags;
	rhiTextureDesc.format = GetWGPUTextureFormat(imgFmt);
	rhiTextureDesc.viewFormatCount = 0;
	rhiTextureDesc.viewFormats = nullptr;

	WGPUTextureViewDimension rhiTexViewDimension = WGPUTextureViewDimension_Undefined;
	WGPUTextureViewDimension rhiTexViewDimensionMain = WGPUTextureViewDimension_Undefined;
	switch (imgType)
	{
	case IMAGE_TYPE_1D:
		rhiTextureDesc.dimension = WGPUTextureDimension_1D;
		rhiTexViewDimension = WGPUTextureViewDimension_1D;
		rhiTexViewDimensionMain = WGPUTextureViewDimension_1D; // TODO
		break;
	case IMAGE_TYPE_2D:
		rhiTextureDesc.dimension = WGPUTextureDimension_2D;
		rhiTexViewDimension = WGPUTextureViewDimension_2D;
		rhiTexViewDimensionMain = (arraySize > 1) ? WGPUTextureViewDimension_2DArray : rhiTexViewDimension;
		break;
	case IMAGE_TYPE_3D:
		rhiTextureDesc.dimension = WGPUTextureDimension_3D;
		rhiTexViewDimension = WGPUTextureViewDimension_3D;
		rhiTexViewDimensionMain = (arraySize > 1) ? WGPUTextureViewDimension_2DArray : rhiTexViewDimension; // is that correct?
		break;
	case IMAGE_TYPE_CUBE:
		rhiTextureDesc.dimension = WGPUTextureDimension_2D;
		rhiTexViewDimension = WGPUTextureViewDimension_Cube;
		rhiTexViewDimensionMain = (arraySize > 1) ? WGPUTextureViewDimension_CubeArray : rhiTexViewDimension;
		break;
	default:
		ASSERT_FAIL("Invalid image type of %s", image->GetName());
	}

	WGPUTexture rhiTexture = nullptr;
	g_renderWorker.WaitForExecute("CreateTexture", [&]() {
		rhiTexture = wgpuDeviceCreateTexture(CWGPURenderAPI::Instance.GetWGPUDevice(), &rhiTextureDesc);
		return 0;
	});

	if (!rhiTexture)
	{
		ErrorMsg("Failed to create texture from image %s\n", image->GetName());
		return false;
	}

	wgpuTextureAddRef(rhiTexture);
	m_rhiTexture = rhiTexture;

	// create main texture view
	WGPUTextureViewDescriptor rhiTexViewDesc = {};
	rhiTexViewDesc.format = GetWGPUTextureFormat(imgFmt);
	rhiTexViewDesc.aspect = WGPUTextureAspect_All;
	rhiTexViewDesc.arrayLayerCount = arrayLayerCount;
	rhiTexViewDesc.baseArrayLayer = 0;
	rhiTexViewDesc.baseMipLevel = 0;
	rhiTexViewDesc.mipLevelCount = rhiTextureDesc.mipLevelCount;
	rhiTexViewDesc.dimension = rhiTexViewDimensionMain;
	rhiTexViewDesc.label = rhiTextureDesc.label;

	WGPUTextureView rhiView = wgpuTextureCreateView(rhiTexture, &rhiTexViewDesc);
	m_rhiViews.append(rhiView);

	// TODO: create individual array views

	for (int arrIdx = 0; arrIdx < arraySize; ++arrIdx)
	{
		int mipMapLevel = image->GetMipMapCount() - 1;
		while (mipMapLevel >= mipStart)
		{
			g_renderWorker.Execute("UploadMipLevel", [=, captureTex = ITexturePtr(this)]() {

				int mipWidth = image->GetWidth(mipMapLevel);
				int mipHeight = image->GetHeight(mipMapLevel);
				const int mipDepth = image->GetDepth(mipMapLevel);
				const int lockBoxLevel = mipMapLevel - mipStart;

				if (IsCompressedFormat(imgFmt))
				{
					mipWidth = max(4, mipWidth & ~3);
					mipHeight = max(4, mipHeight & ~3);
				}

				const WGPUExtent3D rhiTexSize{ (uint)mipWidth, (uint)mipHeight, (uint)mipDepth };

				WGPUImageCopyTexture rhImageCopy{};
				rhImageCopy.texture = rhiTexture;
				rhImageCopy.aspect = WGPUTextureAspect_All;
				rhImageCopy.mipLevel = lockBoxLevel;
				rhImageCopy.origin = WGPUOrigin3D{ 0, 0, (uint)arrIdx };

				WGPUTextureDataLayout rhiTexDataLayout{};
				rhiTexDataLayout.offset = 0;
				if (IsCompressedFormat(imgFmt))
				{
					rhiTexDataLayout.bytesPerRow = ((mipWidth + 3) >> 2) * GetBytesPerBlock(imgFmt);
					rhiTexDataLayout.rowsPerImage = ((mipHeight + 3) >> 2);
				}
				else
				{
					rhiTexDataLayout.bytesPerRow = mipWidth * GetBytesPerPixel(imgFmt);
					rhiTexDataLayout.rowsPerImage = mipHeight;
				}

				const ubyte* src = image->GetPixels(mipMapLevel, arrIdx);
				const int size = image->GetMipMappedSize(mipMapLevel, 1);

				if (imgType == IMAGE_TYPE_CUBE)
				{
					const int cubeFaceSize = size / 6;
					for (int i = 0; i < CUBESIDE_COUNT; ++i)
					{
						rhImageCopy.origin.z = (uint32)i;
						wgpuQueueWriteTexture(CWGPURenderAPI::Instance.GetWGPUQueue(), &rhImageCopy, src, size, &rhiTexDataLayout, &rhiTexSize);
						src += cubeFaceSize;
					}
				}
				else
				{
					wgpuQueueWriteTexture(CWGPURenderAPI::Instance.GetWGPUQueue(), &rhImageCopy, src, size, &rhiTexDataLayout, &rhiTexSize);
				}

				return 0;
			});
			--mipMapLevel;
		}
	}

	g_renderWorker.Execute("TextureUnref", [=]() {
		wgpuTextureRelease(rhiTexture);
		return 0;
	});

	return true;
}

// locks texture for modifications, etc
bool CWGPUTexture::Lock(LockInOutData& data)
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
void CWGPUTexture::Unlock(IGPUCommandRecorder* writeCmdRecorder)
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
