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

void CWGPUCommandRecorder::CopyTextureToTexture(const TextureCopyInfo& source, const TextureCopyInfo& destination, const TextureExtent& copySize) const
{
	ASSERT(source.origin.x >= 0 && source.origin.y >= 0 && source.origin.arraySlice >= 0);
	ASSERT(copySize.width >= 0 && copySize.height >= 0 && copySize.arraySize >= 0);

	CWGPUTexture* srcTexture = static_cast<CWGPUTexture*>(source.texture);
	CWGPUTexture* dstTexture = static_cast<CWGPUTexture*>(destination.texture);

	WGPUImageCopyTexture rhiImageSrc{};
	rhiImageSrc.texture = srcTexture->GetWGPUTexture();
	rhiImageSrc.aspect = WGPUTextureAspect_All;			// TODO: Aspect specification
	rhiImageSrc.mipLevel = source.origin.mipLevel;
	rhiImageSrc.origin = WGPUOrigin3D{ (uint)source.origin.x, (uint)source.origin.y, (uint)source.origin.arraySlice };
	
	WGPUImageCopyTexture rhiImageDst{};
	rhiImageDst.texture = dstTexture->GetWGPUTexture();
	rhiImageDst.aspect = WGPUTextureAspect_All;			// TODO: Aspect specification
	rhiImageDst.mipLevel = destination.origin.mipLevel;
	rhiImageDst.origin = WGPUOrigin3D{ (uint)destination.origin.x, (uint)destination.origin.y, (uint)destination.origin.arraySlice };
	
	WGPUExtent3D rhiCopySize;
	rhiCopySize.depthOrArrayLayers = copySize.arraySize;
	rhiCopySize.width = copySize.width;
	rhiCopySize.height = copySize.height;
	
	wgpuCommandEncoderCopyTextureToTexture(m_rhiCommandEncoder, &rhiImageSrc, &rhiImageDst, &rhiCopySize);
	m_hasCommands = true;
}

void CWGPUCommandRecorder::CopyTextureToBuffer(const TextureCopyInfo& source, const IGPUBuffer* destination, const TextureExtent& copySize) const
{
	ASSERT(source.origin.x >= 0 && source.origin.y >= 0 && source.origin.arraySlice >= 0);
	CWGPUTexture* srcTexture = static_cast<CWGPUTexture*>(source.texture);
	const CWGPUBuffer* dstBuffer = static_cast<const CWGPUBuffer*>(destination);

	WGPUImageCopyTexture rhiImageSrc{};
	rhiImageSrc.texture = srcTexture->GetWGPUTexture();
	rhiImageSrc.aspect = WGPUTextureAspect_All;			// TODO: Aspect specification
	rhiImageSrc.mipLevel = source.origin.mipLevel;
	rhiImageSrc.origin = WGPUOrigin3D{ (uint)source.origin.x, (uint)source.origin.y, (uint)source.origin.arraySlice };

	WGPUExtent3D rhiCopySize;
	rhiCopySize.depthOrArrayLayers = copySize.arraySize;
	rhiCopySize.width = copySize.width;
	rhiCopySize.height = copySize.height;

	// TODO: account arraySize in bytesPerRow
	ASSERT_MSG(copySize.arraySize == 1, "array size > 1 is unsupported yet");
	WGPUImageCopyBuffer rhiBufferDst;
	rhiBufferDst.buffer = dstBuffer->GetWGPUBuffer();
	rhiBufferDst.layout.offset = 0;
	rhiBufferDst.layout.bytesPerRow = copySize.width * GetBytesPerPixel(GetTexFormat(srcTexture->GetFormat()));
	rhiBufferDst.layout.rowsPerImage = copySize.height;

	wgpuCommandEncoderCopyTextureToBuffer(m_rhiCommandEncoder, &rhiImageSrc, &rhiBufferDst, &rhiCopySize);
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

//---------------------------------------------------------------

IGPURenderPassRecorderPtr CWGPUCommandRecorder::BeginRenderPass(const RenderPassDesc& renderPassDesc, void* userData) const
{
	WGPURenderPassDescriptor rhiRenderPassDesc = {};
	FixedArray<WGPURenderPassColorAttachment, MAX_RENDERTARGETS> rhiColorAttachmentList;
	WGPURenderPassDepthStencilAttachment rhiDepthStencilAttachment = {};
	FillWGPURenderPassDescriptor(renderPassDesc, rhiRenderPassDesc, rhiColorAttachmentList, rhiDepthStencilAttachment);

	WGPURenderPassEncoder rhiRenderPassEncoder = wgpuCommandEncoderBeginRenderPass(m_rhiCommandEncoder, &rhiRenderPassDesc);
	if (!rhiRenderPassEncoder)
		return nullptr;

	m_hasCommands = true;

	IVector2D renderTargetDims = 0;
	CRefPtr<CWGPURenderPassRecorder> renderPass = CRefPtr_new(CWGPURenderPassRecorder);
	for (int i = 0; i < renderPassDesc.colorTargets.numElem(); ++i)
	{
		const RenderPassDesc::ColorTargetDesc& colorTarget = renderPassDesc.colorTargets[i];
		if (colorTarget.target.texture)
		{
			renderTargetDims = IVector2D(colorTarget.target.texture->GetWidth(), colorTarget.target.texture->GetHeight());
			renderPass->m_renderTargetsFormat[i] = colorTarget.target ? colorTarget.target.texture->GetFormat() : FORMAT_NONE;
		}
	}

	if (renderPassDesc.depthStencil)
	{
		renderTargetDims = IVector2D(renderPassDesc.depthStencil.texture->GetWidth(), renderPassDesc.depthStencil.texture->GetHeight());
		renderPass->m_depthTargetFormat = renderPassDesc.depthStencil.texture->GetFormat();
	}
	else
		renderPass->m_depthTargetFormat = FORMAT_NONE;

	renderPass->m_depthReadOnly = renderPassDesc.depthReadOnly;
	renderPass->m_stencilReadOnly = renderPassDesc.stencilReadOnly;

	//renderPass->m_rhiCommandEncoder = m_rhiCommandEncoder;
	renderPass->m_rhiRenderPassEncoder = rhiRenderPassEncoder;
	renderPass->m_renderTargetDims = renderTargetDims;
	renderPass->m_userData = userData;

	return IGPURenderPassRecorderPtr(renderPass);
}

//---------------------------------------------------------------

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