#pragma once
#include "renderers/IShaderAPI.h"

class CWGPUCommandRecorder : public IGPUCommandRecorder
{
public:
	~CWGPUCommandRecorder();
	void*						GetUserData() const { return m_userData; }

	void						WriteBuffer(IGPUBuffer* buffer, const void* data, int64 size, int64 offset) const;
	void						CopyBufferToBuffer(IGPUBuffer* source, int64 sourceOffset, IGPUBuffer* destination, int64 destinationOffset, int64 size) const;
	void						ClearBuffer(IGPUBuffer* buffer, int64 offset, int64 size) const;
	
	// TODO:

	// CopyBufferToTexture(WGPUImageCopyBuffer const* source, WGPUImageCopyTexture const* destination, WGPUExtent3D const* copySize);
	// CopyTextureToBuffer(WGPUImageCopyTexture const* source, WGPUImageCopyBuffer const* destination, WGPUExtent3D const* copySize);
	// CopyTextureToTexture(WGPUImageCopyTexture const* source, WGPUImageCopyTexture const* destination, WGPUExtent3D const* copySize);

	// PopDebugGroup(WGPUCommandEncoder commandEncoder);
	// PushDebugGroup(char const* groupLabel);

	// ResolveQuerySet(WGPUQuerySet querySet, uint32_t firstQuery, uint32_t queryCount, WGPUBuffer destination, uint64_t destinationOffset);
	// WriteTimestamp(WGPUQuerySet querySet, uint32_t queryIndex);

	IGPURenderPassRecorderPtr	BeginRenderPass(const RenderPassDesc& renderPassDesc, void* userData = nullptr) const;
	IGPUComputePassRecorderPtr	BeginComputePass(const char* name, void* userData) const;

	IGPUCommandBufferPtr		End();

	WGPUCommandEncoder			m_rhiCommandEncoder{ nullptr };
	void*						m_userData{ nullptr };
	mutable bool				m_hasCommands{ false };
};
