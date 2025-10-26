
#include <nvrhi/nvrhi.h>
#include "core/core_common.h"
#include "core/ConVar.h"

#include "imaging/ImageLoader.h"
#include "NVRHIBackend.h"
#include "NVRHIRenderAPI.h"
#include "../RenderWorker.h"

CNVRHIMessageCallback CNVRHIMessageCallback::Instance;

DECLARE_CVAR_G(nvrhi_validation, "0", nullptr, 0);
DECLARE_CVAR_G(nvrhi_breakOnError, "0", nullptr, 0);

void CNVRHIMessageCallback::message(nvrhi::MessageSeverity severity, const char* messageText)
{
	switch (severity)
	{
	case nvrhi::MessageSeverity::Info:
		Msg("[NVRHI] INFO: %s\n", messageText);
		break;
	case nvrhi::MessageSeverity::Warning:
		MsgWarning("[NVRHI] WARN: %s\n", messageText);
		break;
	case nvrhi::MessageSeverity::Error:
		if (nvrhi_breakOnError.GetBool())
		{
			ASSERT_FAIL("NVRHI ERROR: %s", messageText);
		}
		else
		{
			MsgError("[NVRHI] ERROR: %s\n", messageText);
		}
		break;
	case nvrhi::MessageSeverity::Fatal:
		if (nvrhi_breakOnError.GetBool())
		{
			ASSERT_FAIL("NVRHI FATAL: %s", messageText);
		}

		CrashMsg("NVRHI FATAL: %s", messageText);
		break;
	}
}

bool nvrhiCaptureBackbufferImage(ITexturePtr srcTexture, CImage& dstImage)
{
	CNVRHITexture* currentTextureImpl = static_cast<CNVRHITexture*>(srcTexture.Ptr());

	const TextureExtent size = currentTextureImpl->GetSize();
	const ETextureFormat textureFmt = currentTextureImpl->GetFormat();

	auto rhiTextureDesc = nvrhi::TextureDesc()
		.setWidth(size.width)
		.setHeight(size.height)
		.setFormat(GetNVRHITextureFormat(textureFmt))
		.setInitialState(nvrhi::ResourceStates::CopyDest)
		.setKeepInitialState(true);

	nvrhi::IDevice* rhiDevice = CNVRHIRenderAPI::Instance.GetNVRHIDevice();
	nvrhi::StagingTextureHandle rhiStagingTexture = rhiDevice->createStagingTexture(rhiTextureDesc, nvrhi::CpuAccessMode::Read);
	if (!rhiStagingTexture)
		return false;

	// copy backbuffer to readback staging texture
	int cmdListIdx = -1;
	nvrhi::CommandListHandle rhiCopyCommandList = CNVRHIRenderAPI::Instance.AcquireRHICommandList(cmdListIdx);
	rhiCopyCommandList->open();
	rhiCopyCommandList->copyTexture(rhiStagingTexture, {}, currentTextureImpl->GetNVRHITextureHandle(), {});
	rhiCopyCommandList->close();

	g_renderWorker.WaitForExecute(__func__, [&]() {
		uint64_t lastSubmitInstance = rhiDevice->executeCommandList(rhiCopyCommandList);
		rhiDevice->queueWaitForCommandList(nvrhi::CommandQueue::Graphics, nvrhi::CommandQueue::Graphics, lastSubmitInstance);
		CNVRHIRenderAPI::Instance.ReleaseCommandList(cmdListIdx);
		return 0;
		});

	size_t stagingRowPitch = 0;
	void* data = rhiDevice->mapStagingTexture(rhiStagingTexture, nvrhi::TextureSlice{}, nvrhi::CpuAccessMode::Read, &stagingRowPitch);
	if (!data)
		return false;

	const int bytesPerPixel = GetBytesPerPixel(GetTexFormat(textureFmt));
	const bool rbSwapped = HasTexFormatFlags(textureFmt, TEXFORMAT_FLAG_SWAP_RB);
	ubyte* dst = dstImage.Create(FORMAT_RGB8, size.width, size.height, 1, 1);
	for (int y = 0; y < size.height; y++)
	{
		const ubyte* src = (ubyte*)data + bytesPerPixel * y * size.width;
		for (int x = 0; x < size.width; ++x)
		{
			if (rbSwapped)
			{
				dst[0] = src[2];
				dst[1] = src[1];
				dst[2] = src[0];
			}
			else
			{
				dst[0] = src[0];
				dst[1] = src[1];
				dst[2] = src[2];
			}
			dst += 3;
			src += bytesPerPixel;
		}
	}

	rhiDevice->unmapStagingTexture(rhiStagingTexture);
	return true;
}