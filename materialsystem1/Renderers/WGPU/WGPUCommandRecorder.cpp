#include <webgpu/webgpu.h>
#include "core/core_common.h"

#include "WGPUBuffer.h"
#include "WGPUStates.h"
#include "WGPUCommandRecorder.h"
#include "WGPURenderPassRecorder.h"
#include "WGPUComputePassRecorder.h"
#include "WGPURenderDefs.h"
#include "WGPUTexture.h"


CWGPUCommandRecorder::~CWGPUCommandRecorder()
{
	if(m_rhiCommandEncoder)
		wgpuCommandEncoderRelease(m_rhiCommandEncoder);
}

void CWGPUCommandRecorder::WriteBuffer(IGPUBuffer* buffer, const void* data, int64 size, int64 offset) const
{
	const int64 writeDataSize = (size + 3) & ~3;
	if (writeDataSize <= 0)
		return;

	CWGPUBuffer* bufferImpl = static_cast<CWGPUBuffer*>(buffer);
	if (!bufferImpl)
		return;

	wgpuCommandEncoderWriteBuffer(m_rhiCommandEncoder, bufferImpl->GetWGPUBuffer(), offset, reinterpret_cast<const uint8_t*>(data), writeDataSize);
	m_hasCommands = true;
}

void CWGPUCommandRecorder::CopyBufferToBuffer(IGPUBuffer* source, int64 sourceOffset, IGPUBuffer* destination, int64 destinationOffset, int64 size) const
{
	const int64 copyDataSize = (size + 3) & ~3;
	if (copyDataSize <= 0)
		return;

	CWGPUBuffer* sourceImpl = static_cast<CWGPUBuffer*>(source);
	if (!sourceImpl)
		return;

	CWGPUBuffer* destinationImpl = static_cast<CWGPUBuffer*>(destination);
	if (!destinationImpl)
		return;

	wgpuCommandEncoderCopyBufferToBuffer(m_rhiCommandEncoder, sourceImpl->GetWGPUBuffer(), sourceOffset, destinationImpl->GetWGPUBuffer(), destinationOffset, copyDataSize);
	m_hasCommands = true;
}

void CWGPUCommandRecorder::ClearBuffer(IGPUBuffer* buffer, int64 offset, int64 size) const
{
	const int64 clearDataSize = (size + 3) & ~3;
	if (clearDataSize <= 0)
		return;

	CWGPUBuffer* bufferImpl = static_cast<CWGPUBuffer*>(buffer);
	if (!bufferImpl)
		return;

	wgpuCommandEncoderClearBuffer(m_rhiCommandEncoder, bufferImpl->GetWGPUBuffer(), offset, clearDataSize);
	m_hasCommands = true;
}

IGPUCommandBufferPtr CWGPUCommandRecorder::End()
{
	if (!m_rhiCommandEncoder)
	{
		ASSERT_FAIL("Command recorder was already ended");
		return nullptr;
	}

	if (!m_hasCommands)
		return nullptr;

	CRefPtr<CWGPUCommandBuffer> commandBuffer = CRefPtr_new(CWGPUCommandBuffer);

	WGPUCommandBuffer rhiCommandBuffer = wgpuCommandEncoderFinish(m_rhiCommandEncoder, nullptr);
	wgpuCommandEncoderRelease(m_rhiCommandEncoder);
	m_rhiCommandEncoder = nullptr;

	commandBuffer->m_rhiCommandBuffer = rhiCommandBuffer;
	return IGPUCommandBufferPtr(commandBuffer);
}

IGPURenderPassRecorderPtr CWGPUCommandRecorder::BeginRenderPass(const RenderPassDesc& renderPassDesc, void* userData) const
{
	WGPURenderPassDescriptor rhiRenderPassDesc = {};
	FixedArray<WGPURenderPassColorAttachment, MAX_RENDERTARGETS> rhiColorAttachmentList;
	WGPURenderPassDepthStencilAttachment rhiDepthStencilAttachment = {};
	FillWGPURenderPassDescriptor(renderPassDesc, rhiRenderPassDesc, rhiColorAttachmentList, rhiDepthStencilAttachment);

	IVector2D renderTargetDims = 0;
	for (const RenderPassDesc::ColorTargetDesc& colorTarget : renderPassDesc.colorTargets)
		renderTargetDims = IVector2D(colorTarget.target->GetWidth(), colorTarget.target->GetHeight());

	WGPURenderPassEncoder rhiRenderPassEncoder = wgpuCommandEncoderBeginRenderPass(m_rhiCommandEncoder, &rhiRenderPassDesc);
	if (!rhiRenderPassEncoder)
		return nullptr;

	m_hasCommands = true;

	CRefPtr<CWGPURenderPassRecorder> renderPass = CRefPtr_new(CWGPURenderPassRecorder);
	for (int i = 0; i < renderPassDesc.colorTargets.numElem(); ++i)
	{
		const RenderPassDesc::ColorTargetDesc& colorTarget = renderPassDesc.colorTargets[i];
		renderPass->m_renderTargetsFormat[i] = colorTarget.target ? colorTarget.target->GetFormat() : FORMAT_NONE;
	}
	renderPass->m_depthTargetFormat = renderPassDesc.depthStencil ? renderPassDesc.depthStencil->GetFormat() : FORMAT_NONE;
	renderPass->m_depthReadOnly = renderPassDesc.depthReadOnly;
	renderPass->m_stencilReadOnly = renderPassDesc.stencilReadOnly;

	//renderPass->m_rhiCommandEncoder = m_rhiCommandEncoder;
	renderPass->m_rhiRenderPassEncoder = rhiRenderPassEncoder;
	renderPass->m_renderTargetDims = renderTargetDims;
	renderPass->m_userData = userData;

	return IGPURenderPassRecorderPtr(renderPass);
}

IGPUComputePassRecorderPtr CWGPUCommandRecorder::BeginComputePass(const char* name, void* userData) const
{
	WGPUComputePassDescriptor rhiComputePassDesc = {};
	rhiComputePassDesc.label = name;
	//rhiComputePassDesc.timestampWrites TODO
	WGPUComputePassEncoder rhiComputePassEncoder = wgpuCommandEncoderBeginComputePass(m_rhiCommandEncoder, &rhiComputePassDesc);
	if (!rhiComputePassEncoder)
		return nullptr;

	CRefPtr<CWGPUComputePassRecorder> renderPass = CRefPtr_new(CWGPUComputePassRecorder);

	//renderPass->m_rhiCommandEncoder = rhiCommandEncoder;
	renderPass->m_rhiComputePassEncoder = rhiComputePassEncoder;
	renderPass->m_userData = userData;

	return IGPUComputePassRecorderPtr(renderPass);
}