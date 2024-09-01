//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Command recorders
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ShaderAPI_defs.h"

class IGPUBuffer;

class IGPURenderPipeline;
using IGPURenderPipelinePtr = CRefPtr<IGPURenderPipeline>;

class IGPUComputePipeline;
using IGPUComputePipelinePtr = CRefPtr<IGPUComputePipeline>;

class IGPUBindGroup;
struct RenderPassDesc;
enum EIndexFormat : int;

struct TextureCopyInfo
{
	ITexture*		texture{ nullptr };
	TextureOrigin	origin;
};

// helper structure used for filling buffer data
// for Draw[Indexed]Indirect

struct GPUDrawIndirectCmd
{
	uint	vertexCount{ 0 };
	uint	instanceCount{ 0 };
	uint	firstVertex{ 0 };
	uint	firstInstance{ 0 };
};

struct GPUDrawIndexedIndirectCmd
{
	uint	indexCount{ 0 };
	uint	instanceCount{ 0 };
	uint	firstIndex{ 0 };
	uint	baseVertex{ 0 };
	uint	firstInstance{ 0 };
};

//---------------------------------
// The command buffer ready to be passed into queue for execution
class IGPUCommandBuffer : public RefCountedObject<IGPUCommandBuffer> {};
using IGPUCommandBufferPtr = CRefPtr<IGPUCommandBuffer>;

class IGPURenderCommandsRecorder : public RefCountedObject<IGPURenderCommandsRecorder>
{
public:
	enum EType
	{
		PASS_RECORDER,
		BUNDLE_RECORDER,
	};

	virtual EType					GetType() const = 0;

	virtual IVector2D					GetRenderTargetDimensions() const = 0;
	virtual ArrayCRef<ETextureFormat>	GetRenderTargetFormats() const = 0;
	virtual ETextureFormat			GetDepthTargetFormat() const = 0;

	virtual bool					IsDepthReadOnly() const = 0;
	virtual bool					IsStencilReadOnly() const = 0;

	virtual int						GetTargetMultiSamples() const = 0;

	/* TODO: multi - threaded recorder
		like:
			GPUCommandBlock* block = cmdRec->BeginCommandBlock()
				block.SetPipeline
				block.SetBindGroup(n...)
				block.SetVertexBuffer(...)
				block.SetIndexBuffer(...)
				block.Draw(...)
			EndBlock(block)
	*/

	virtual void					DbgPopGroup() const = 0;
	virtual void					DbgPushGroup(const char* groupLabel) const = 0;
	virtual void					DbgAddMarker(const char* label) const = 0;

	virtual void					SetPipeline(IGPURenderPipeline* pipeline) = 0;
	virtual IGPURenderPipelinePtr	GetPipeline() const = 0;
	virtual void					SetBindGroup(int groupIndex, IGPUBindGroup* bindGroup, ArrayCRef<uint32> dynamicOffsets = nullptr) = 0;

	virtual void					SetVertexBuffer(int slot, IGPUBuffer* vertexBuffer, int64 offset = 0, int64 size = -1) = 0;
	virtual void					SetIndexBuffer(IGPUBuffer* indexBuffer, EIndexFormat indexFormat, int64 offset = 0, int64 size = -1) = 0;

	void							SetVertexBufferView(int slot, const GPUBufferView& vertexBuffer);
	void							SetIndexBufferView(const GPUBufferView& indexBuffer, EIndexFormat indexFormat);

	virtual void					SetViewport(const AARectangle& rectangle, float minDepth, float maxDepth) = 0;
	virtual void					SetScissorRectangle(const IAARectangle& rectangle) = 0;

	virtual void					Draw(int vertexCount, int firstVertex, int instanceCount, int firstInstance = 0) = 0;
	virtual void					DrawIndexed(int indexCount, int firstIndex, int instanceCount, int baseVertex = 0, int firstInstance = 0) = 0;

	virtual void					DrawIndirect(IGPUBuffer* indirectBuffer, int indirectOffset) = 0;
	virtual void					DrawIndexedIndirect(IGPUBuffer* indirectBuffer, int indirectOffset) = 0;

	virtual void*					GetUserData() const = 0;

};

inline void IGPURenderCommandsRecorder::SetVertexBufferView(int slot, const GPUBufferView& vertexBuffer)
{
	SetVertexBuffer(slot, vertexBuffer.buffer, vertexBuffer.offset, vertexBuffer.size);
}

inline void IGPURenderCommandsRecorder::SetIndexBufferView(const GPUBufferView& indexBuffer, EIndexFormat indexFormat)
{
	SetIndexBuffer(indexBuffer.buffer, indexFormat, indexBuffer.offset, indexBuffer.size);
}

//---------------------------------
// Render bundle recorder
class IGPURenderBundleRecorder : public IGPURenderCommandsRecorder
{
public:
	EType							GetType() const { return PASS_RECORDER; }
};

//---------------------------------
// Render pass recorder
class IGPURenderPassRecorder : public IGPURenderCommandsRecorder
{
public:
	EType							GetType() const { return PASS_RECORDER; }

	// adds render bundle
	virtual void					AddBundle(IGPURenderBundleRecorder* bundle) = 0;

	// completes pass recording. After this recorder can be disposed
	virtual void					Complete() = 0;

	// completes pass recording and command recording.
	virtual IGPUCommandBufferPtr	End() = 0;
};
using IGPURenderPassRecorderPtr = CRefPtr<IGPURenderPassRecorder>;


//---------------------------------
// Compute pass recorder
class IGPUComputePassRecorder : public RefCountedObject<IGPUComputePassRecorder>
{
public:
	virtual void					DbgPopGroup() const = 0;
	virtual void					DbgPushGroup(const char* groupLabel) const = 0;
	virtual void					DbgAddMarker(const char* label) const = 0;

	virtual void					SetPipeline(IGPUComputePipeline* pipeline) = 0;
	virtual IGPUComputePipelinePtr	GetPipeline() const = 0;

	virtual void					SetBindGroup(int groupIndex, IGPUBindGroup* bindGroup, ArrayCRef<uint32> dynamicOffsets = nullptr) = 0;

	virtual void					DispatchWorkgroups(int32 workgroupCountX, int32 workgroupCountY = 1, int32 workgroupCountZ = 1) = 0;
	virtual void					DispatchWorkgroupsIndirect(IGPUBuffer* indirectBuffer, int64 indirectOffset) = 0;

	virtual void*					GetUserData() const = 0;

	// completes pass recording. After this recorder can be disposed
	virtual void					Complete() = 0;

	// completes pass recording and command recording.
	virtual IGPUCommandBufferPtr	End() = 0;
};
using IGPUComputePassRecorderPtr = CRefPtr<IGPUComputePassRecorder>;

//---------------------------------
// Command recorded. Used for render passes and compute passes
class IGPUCommandRecorder : public RefCountedObject<IGPUCommandRecorder>
{
public:
	virtual ~IGPUCommandRecorder() {}

	virtual IGPURenderPassRecorderPtr	BeginRenderPass(const RenderPassDesc& renderPassDesc, void* userData = nullptr) const = 0;
	virtual IGPUComputePassRecorderPtr	BeginComputePass(const char* name, void* userData = nullptr) const = 0;

	virtual void						DbgPopGroup() const = 0;
	virtual void						DbgPushGroup(const char* groupLabel) const = 0;
	virtual void						DbgAddMarker(const char* label) const = 0;

	virtual void						WriteBuffer(IGPUBuffer* buffer, const void* data, int64 size, int64 offset) const = 0;
	void								WriteBufferView(const GPUBufferView& bufferView, const void* data, int64 size = -1, int64 offset = 0) const;
	virtual void						CopyBufferToBuffer(IGPUBuffer* source, int64 sourceOffset, IGPUBuffer* destination, int64 destinationOffset, int64 size) const = 0;
	virtual void						ClearBuffer(IGPUBuffer* buffer, int64 offset, int64 size) const = 0;

	virtual void						CopyTextureToTexture(const TextureCopyInfo& source, const TextureCopyInfo& destination, const TextureExtent& copySize) const = 0;
	virtual void						CopyTextureToBuffer(const TextureCopyInfo& source, const IGPUBuffer* destination, const TextureExtent& copySize) const = 0;

	virtual void*						GetUserData() const = 0;
	virtual IGPUCommandBufferPtr		End() = 0;
};
using IGPUCommandRecorderPtr = CRefPtr<IGPUCommandRecorder>;

inline void IGPUCommandRecorder::WriteBufferView(const GPUBufferView& bufferView, const void* data, int64 size, int64 offset) const
{
	if (size < 0) // write whole view
		size = bufferView.size - offset;

	ASSERT(offset + size <= bufferView.size);

	WriteBuffer(bufferView.buffer.Ptr(), data, size, bufferView.offset + offset);
}