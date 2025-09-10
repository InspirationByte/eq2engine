#pragma once

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

