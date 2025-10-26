#pragma once
#include <nvrhi/nvrhi.h>
#include "renderers/ITexture.h"

#if !defined(_RETAIL) && !defined(_PROFILE)
#define NVRHI_WITH_VALIDATION
#endif

enum ENVRHIBackendType : int
{
	NVRHI_BACKEND_D3D11 = 0,
	NVRHI_BACKEND_D3D12,
	NVRHI_BACKEND_VULKAN,
};

struct CNVRHIMessageCallback : public nvrhi::IMessageCallback
{
	static CNVRHIMessageCallback Instance;

	void message(nvrhi::MessageSeverity severity, const char* messageText) override;
};

bool nvrhiCaptureBackbufferImage(ITexturePtr srcTexture, CImage& dstImage);