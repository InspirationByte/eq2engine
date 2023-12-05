#pragma once
#include "renderers/IGPUCommandRecorder.h"

class CWGPUCommandRecorder : public IGPUCommandRecorder
{
public:
	~CWGPUCommandRecorder();
	void*						GetUserData() const { return m_userData; }

	void						WriteBuffer(IGPUBuffer* buffer, const void* data, int64 size, int64 offset) const;
	void						CopyBufferToBuffer(IGPUBuffer* source, int64 sourceOffset, IGPUBuffer* destination, int64 destinationOffset, int64 size) const;
	void						ClearBuffer(IGPUBuffer* buffer, int64 offset, int64 size) const;
	
	void						CopyTextureToTexture(const TextureCopyInfo& source, const TextureCopyInfo& destination, const TextureExtent& copySize) const;

	// TODO:
	// CopyBufferToTexture(const WGPUImageCopyBuffer& source, const WGPUImageCopyTexture& destination, const WGPUExtent3D& copySize) const;
	// CopyTextureToBuffer(const WGPUImageCopyTexture& source, const WGPUImageCopyBuffer& destination, const WGPUExtent3D& copySize) const;
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
