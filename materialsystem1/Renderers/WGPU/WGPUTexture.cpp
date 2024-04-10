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

	for (WGPUTexture texture : m_rhiTextures)
		wgpuTextureRelease(texture);

	m_rhiViews.clear();
	m_rhiTextures.clear();
}

void CWGPUTexture::Ref_DeleteObject()
{
	if (!(m_flags & TEXFLAG_TRANSIENT))
		CWGPURenderAPI::Instance.FreeTexture(this);

	RefCountedObject::Ref_DeleteObject();
}

bool CWGPUTexture::Init(const ArrayCRef<CImagePtr> images, const SamplerStateParams& sampler, int flags)
{
	// FIXME: only release if pool, flags, format and size is different
	Release();

	// since texture is initialized from image buffer, it neeeds copy destination flag
	flags |= TEXFLAG_COPY_DST;

	m_samplerState = sampler;
	m_samplerState.maxAnisotropy = max(CWGPURenderAPI::Instance.GetCaps().maxTextureAnisotropicLevel, sampler.maxAnisotropy);
	m_flags = flags;

	HOOK_TO_CVAR(r_loadmiplevel);
	const int quality = (m_flags & TEXFLAG_IGNORE_QUALITY) ? 0 : r_loadmiplevel->GetInt();

	for (CImage* image : images)
	{
		if (image->IsCube())
			m_flags |= TEXFLAG_CUBEMAP;
	}

	m_rhiTextures.reserve(images.numElem());
	m_rhiViews.reserve(images.numElem());

	WGPUTextureUsageFlags rhiUsageFlags = WGPUTextureUsage_TextureBinding;
	if (flags & TEXFLAG_STORAGE) rhiUsageFlags |= WGPUTextureUsage_StorageBinding;
	if (flags & TEXFLAG_COPY_SRC) rhiUsageFlags |= WGPUTextureUsage_CopySrc;
	if (flags & TEXFLAG_COPY_DST) rhiUsageFlags |= WGPUTextureUsage_CopyDst;

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

		int texFormat = imgFmt;
		if (m_flags & TEXFLAG_SRGB)
			texFormat |= TEXFORMAT_FLAG_SRGB;

		WGPUTextureDescriptor rhiTextureDesc{};
		rhiTextureDesc.label = img->GetName();
		rhiTextureDesc.mipLevelCount = mipCount;
		rhiTextureDesc.size = WGPUExtent3D{ (uint)texWidth, (uint)texHeight, (uint)texDepth };
		rhiTextureDesc.sampleCount = 1;
		rhiTextureDesc.usage = rhiUsageFlags;
		rhiTextureDesc.format = GetWGPUTextureFormat(static_cast<ETextureFormat>(texFormat));
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

				const WGPUExtent3D rhiTexSize{ (uint)mipWidth, (uint)mipHeight, (uint)mipDepth };

				WGPUImageCopyTexture rhImageCopy{};
				rhImageCopy.texture = rhiTexture;
				rhImageCopy.aspect = WGPUTextureAspect_All;
				rhImageCopy.mipLevel = lockBoxLevel;
				rhImageCopy.origin = WGPUOrigin3D{ 0, 0, 0 };

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

				const ubyte* src = img->GetPixels(mipMapLevel);
				const int size = img->GetMipMappedSize(mipMapLevel, 1);

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

		{
			WGPUTextureViewDescriptor rhiTexViewDesc = {};
			rhiTexViewDesc.format = rhiTextureDesc.format;
			rhiTexViewDesc.aspect = WGPUTextureAspect_All;
			rhiTexViewDesc.arrayLayerCount = texDepth;
			rhiTexViewDesc.baseArrayLayer = 0;
			rhiTexViewDesc.baseMipLevel = 0;
			rhiTexViewDesc.mipLevelCount = rhiTextureDesc.mipLevelCount;
			rhiTexViewDesc.dimension = rhiTexViewDimension;
			rhiTexViewDesc.label = rhiTextureDesc.label;
		
			WGPUTextureView rhiView = wgpuTextureCreateView(rhiTexture, &rhiTexViewDesc);
			m_rhiViews.append(rhiView);
		}

		// FIXME: check for differences?
		m_mipCount = max(m_mipCount, mipCount);
		m_width = max(m_width, texWidth);
		m_height = max(m_height, texHeight);
		m_arraySize = max(m_arraySize, texDepth);
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
	ASSERT_MSG(!m_lockData, "CWGPUTexture: already locked");

	if (m_lockData)
		return false;

	if (m_rhiTextures.numElem() > 1)
	{
		ASSERT_FAIL("Couldn't handle locking of animated texture! Please tell to programmer!");
		return false;
	}

	if (IsCompressedFormat(m_format))
	{
		ASSERT_FAIL("Compressed textures aren't lockable yet!");
		return false;
	}

	ASSERT(data.lockOrigin.x >= 0 && data.lockOrigin.y >= 0 && data.lockOrigin.arraySlice >= 0);
	ASSERT(data.lockSize.width >= 0 && data.lockSize.height >= 0 && data.lockSize.arraySize >= 0);

	const int lockPitch = data.lockSize.width;
	const int lockOffset = data.lockOrigin.x * data.lockOrigin.y * data.lockOrigin.arraySlice;
	const int sizeToLock = data.lockSize.width * data.lockSize.height * data.lockSize.arraySize;

	const int lockByteCount = GetBytesPerPixel(m_format) * sizeToLock;

	// allocate memory for lock data
	data.lockData = (ubyte*)PPAlloc(lockByteCount);
	data.lockPitch = lockPitch * GetBytesPerPixel(m_format);
	data.lockByteCount = lockByteCount;

	if (!(data.flags & TEXLOCK_DISCARD) && (m_flags & TEXFLAG_COPY_SRC))
	{
		CWGPUBuffer tmpBuffer(BufferInfo(1, data.lockByteCount), BUFFERUSAGE_READ | BUFFERUSAGE_COPY_DST, "TexLockReadBuffer");

		{
			IGPUCommandRecorderPtr cmdRecorder = g_renderAPI->CreateCommandRecorder("TexLockReadCmd");
			cmdRecorder->CopyTextureToBuffer(TextureCopyInfo{ this }, &tmpBuffer, data.lockSize);
			g_renderAPI->SubmitCommandBuffer(cmdRecorder->End());
		}

		IGPUBuffer::LockFuture future = tmpBuffer.Lock(0, tmpBuffer.GetSize(), 0);
		future.AddCallback([this, &data, lockByteCount](const FutureResult<BufferLockData>& result) {
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
		g_renderWorker.WaitForExecute("UnlockTex", [&]() {
			WGPUTextureDataLayout rhiTexLayout{};
			rhiTexLayout.offset = 0;
			rhiTexLayout.bytesPerRow = data.lockPitch;
			rhiTexLayout.rowsPerImage = data.lockSize.height;

			WGPUImageCopyTexture rhiTexDestination{};
			rhiTexDestination.texture = m_rhiTextures[0];
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
				wgpuQueueWriteTexture(CWGPURenderAPI::Instance.GetWGPUQueue(), &rhiTexDestination, data.lockData, data.lockByteCount, &rhiTexLayout, &rhiTexSize);
			}
		
			return 0;
		});
	}

	PPFree(data.lockData);
	data.lockData = nullptr;

	m_lockData = nullptr;
}
