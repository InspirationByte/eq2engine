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
class IGPUBindGroup;
struct RenderPassDesc;
enum EIndexFormat : int;


//---------------------------------
// The command buffer ready to be passed into queue for execution
class IGPUCommandBuffer : public RefCountedObject<IGPUCommandBuffer> {};
using IGPUCommandBufferPtr = CRefPtr<IGPUCommandBuffer>;

//---------------------------------
// Render pass recorder
class IGPURenderPassRecorder : public RefCountedObject<IGPURenderPassRecorder>
{
public:
	virtual IVector2D				GetRenderTargetDimensions() const = 0;
	virtual ETextureFormat			GetRenderTargetFormat(int idx) const = 0;
	virtual ETextureFormat			GetDepthTargetFormat() const = 0;

	virtual bool					IsDepthReadOnly() const = 0;
	virtual bool					IsStencilReadOnly() const = 0;

	virtual void					SetPipeline(IGPURenderPipeline* pipeline) = 0;
	virtual void					SetBindGroup(int groupIndex, IGPUBindGroup* bindGroup, ArrayCRef<uint32> dynamicOffsets) = 0;

	// TODO: use IGPUBuffer instead of IVertexBuffer and IIndexBuffer
	virtual void					SetVertexBuffer(int slot, IGPUBuffer* vertexBuffer, int64 offset = 0, int64 size = -1) = 0;
	virtual void					SetIndexBuffer(IGPUBuffer* indexBuf, EIndexFormat indexFormat, int64 offset = 0, int64 size = -1) = 0;

	virtual void					SetViewport(const AARectangle& rectangle, float minDepth, float maxDepth) = 0;
	virtual void					SetScissorRectangle(const IAARectangle& rectangle) = 0;

	virtual void					Draw(int vertexCount, int firstVertex, int instanceCount, int firstInstance = 0) = 0;
	virtual void					DrawIndexed(int indexCount, int firstIndex, int instanceCount, int baseVertex = 0, int firstInstance = 0) = 0;
	virtual void					DrawIndexedIndirect(IGPUBuffer* indirectBuffer, int indirectOffset) = 0;
	virtual void					DrawIndirect(IGPUBuffer* indirectBuffer, int indirectOffset) = 0;

	virtual void*					GetUserData() const = 0;
	virtual IGPUCommandBufferPtr	End() = 0;
};
using IGPURenderPassRecorderPtr = CRefPtr<IGPURenderPassRecorder>;

//---------------------------------
// Command recorded. Used for render passes and compute passes
class IGPUCommandRecorder : public RefCountedObject<IGPUCommandRecorder>
{
public:
	virtual ~IGPUCommandRecorder() {}

	virtual IGPURenderPassRecorderPtr	BeginRenderPass(const RenderPassDesc& renderPassDesc, void* userData = nullptr) const = 0;

	virtual void						WriteBuffer(IGPUBuffer* buffer, const void* data, int64 size, int64 offset) const = 0;
	void								WriteBufferView(const GPUBufferPtrView& bufferView, const void* data, int64 size = -1, int64 offset = 0) const;

	virtual void*						GetUserData() const = 0;
	virtual IGPUCommandBufferPtr		End() = 0;
};
using IGPUCommandRecorderPtr = CRefPtr<IGPUCommandRecorder>;

inline void IGPUCommandRecorder::WriteBufferView(const GPUBufferPtrView& bufferView, const void* data, int64 size, int64 offset) const
{
	if (size < 0) // write whole view
		size = bufferView.size - offset;

	ASSERT(offset + size <= bufferView.size);

	WriteBuffer(bufferView.buffer.Ptr(), data, size, bufferView.offset + offset);
}