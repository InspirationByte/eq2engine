#pragma once

struct CNVRHIMessageCallback : public nvrhi::IMessageCallback
{
	static CNVRHIMessageCallback Instance;

	void message(nvrhi::MessageSeverity severity, const char* messageText) override;
};

