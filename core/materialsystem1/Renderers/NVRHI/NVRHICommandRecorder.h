#pragma once
#include "renderers/IGPUCommandRecorder.h"

class CNVRHICommandRecorder : public IGPUCommandRecorder
{
public:
	~CNVRHICommandRecorder() = default;

	void*						GetUserData() const { return m_userData; }

	void						DbgPopGroup() const;
	void						DbgPushGroup(const char* groupLabel) const;
	void						DbgAddMarker(const char* label) const;

	void						WriteBuffer(IGPUBuffer* buffer, const void* data, int64 size, int64 offset) const;
	void						CopyBufferToBuffer(IGPUBuffer* source, int64 sourceOffset, IGPUBuffer* destination, int64 destinationOffset, int64 size) const;
	void						ClearBuffer(IGPUBuffer* buffer, int64 offset, int64 size) const;
	
	void						CopyTextureToTexture(const TextureCopyInfo& source, const TextureCopyInfo& destination, const TextureExtent& copySize) const;

	// ResolveQuerySet(WGPUQuerySet querySet, uint32_t firstQuery, uint32_t queryCount, WGPUBuffer destination, uint64_t destinationOffset);
	// WriteTimestamp(WGPUQuerySet querySet, uint32_t queryIndex);

	IGPURenderPassRecorderPtr	BeginRenderPass(const RenderPassDesc& renderPassDesc, void* userData = nullptr) const;
	IGPUComputePassRecorderPtr	BeginComputePass(const char* name, void* userData) const;

	IGPUCommandBufferPtr		End();

	EqString					m_dbgName;
	nvrhi::CommandListHandle	m_rhiCommandList{ nullptr };
	void*						m_userData{ nullptr };
	int							m_cmdListIdx{ -1 };
};
