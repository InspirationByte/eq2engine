#include <nvrhi/nvrhi.h>
#include "core/core_common.h"

#include "NVRHIBuffer.h"
#include "NVRHIStates.h"
#include "NVRHICommandRecorder.h"
#include "NVRHIRenderPassRecorder.h"
#include "NVRHIComputePassRecorder.h"
#include "NVRHIRenderDefs.h"
#include "NVRHITexture.h"

void CNVRHICommandRecorder::WriteBuffer(IGPUBuffer* buffer, const void* data, int64 size, int64 offset) const
{
	const int64 writeDataSize = (size + 3) & ~3;
	if (writeDataSize <= 0)
		return;

	CNVRHIBuffer* bufferImpl = static_cast<CNVRHIBuffer*>(buffer);
	if (!bufferImpl)
		return;

	ASSERT_MSG(bufferImpl->GetUsageFlags() & BUFFERUSAGE_COPY_DST, "buffer must have BUFFERUSAGE_COPY_DST usage bit");
	ASSERT_MSG(offset >= 0 && offset + writeDataSize <= bufferImpl->GetSize(), "Offset and/or Size outside buffer range");

	m_rhiCommandList->writeBuffer(bufferImpl->GetNVRHIBufferHandle(), data, writeDataSize, offset);
}

void CNVRHICommandRecorder::CopyBufferToBuffer(IGPUBuffer* source, int64 sourceOffset, IGPUBuffer* destination, int64 destinationOffset, int64 size) const
{
	const int64 copyDataSize = (size + 3) & ~3;
	if (copyDataSize <= 0)
		return;

	CNVRHIBuffer* sourceImpl = static_cast<CNVRHIBuffer*>(source);
	CNVRHIBuffer* destinationImpl = static_cast<CNVRHIBuffer*>(destination);

	if (!sourceImpl)
		return;

	if (!destinationImpl)
		return;

	ASSERT_MSG(sourceImpl->GetUsageFlags() & BUFFERUSAGE_COPY_SRC, "SRC buffer must have BUFFERUSAGE_COPY_SRC usage bit");
	ASSERT_MSG(destinationImpl->GetUsageFlags() & BUFFERUSAGE_COPY_DST, "DST buffer must have BUFFERUSAGE_COPY_DST usage bit");

	ASSERT_MSG(sourceOffset >= 0 && sourceOffset + copyDataSize <= sourceImpl->GetSize(), "Offset and/or Size outside source buffer range");
	ASSERT_MSG(destinationOffset >= 0 && destinationOffset + copyDataSize <= destinationImpl->GetSize(), "Offset and/or Size outside destination buffer range");

	m_rhiCommandList->copyBuffer(destinationImpl->GetNVRHIBufferHandle(), destinationOffset, sourceImpl->GetNVRHIBufferHandle(), sourceOffset, copyDataSize);
}

void CNVRHICommandRecorder::ClearBuffer(IGPUBuffer* buffer, int64 offset, int64 size) const
{
	const int64 clearDataSize = (size + 3) & ~3;
	if (clearDataSize <= 0)
		return;

	CNVRHIBuffer* bufferImpl = static_cast<CNVRHIBuffer*>(buffer);
	if (!bufferImpl)
		return;

	ASSERT_MSG(bufferImpl->GetUsageFlags() & BUFFERUSAGE_COPY_DST, "buffer must have BUFFERUSAGE_COPY_DST usage bit");
	ASSERT_MSG(offset >= 0 && offset + clearDataSize <= bufferImpl->GetSize(), "Offset and/or Size outside buffer range");

	if(offset > 0 || size < bufferImpl->GetSize())
	{
		static void* tmpClearMem = PPAlloc(size);

		memset(tmpClearMem, 0, size);
		m_rhiCommandList->writeBuffer(bufferImpl->GetNVRHIBufferHandle(), tmpClearMem, size, offset);

		PPFree(tmpClearMem);
	}
	else
	{
		m_rhiCommandList->clearBufferUInt(bufferImpl->GetNVRHIBufferHandle(), 0);
	}
}

void CNVRHICommandRecorder::CopyTextureToTexture(const TextureCopyInfo& source, const TextureCopyInfo& destination, const TextureExtent& copySize) const
{
	ASSERT(source.origin.x >= 0 && source.origin.y >= 0 && source.origin.arraySlice >= 0);
	ASSERT(copySize.width >= 0 && copySize.height >= 0 && copySize.arraySize >= 0);

	CNVRHITexture* srcTexture = static_cast<CNVRHITexture*>(source.texture);
	CNVRHITexture* dstTexture = static_cast<CNVRHITexture*>(destination.texture);

	if (!srcTexture)
		return;

	if (!dstTexture)
		return;

	ASSERT_MSG(srcTexture->GetFlags() & TEXFLAG_COPY_SRC, "%s don't have TEXFLAG_COPY_SRC flag", srcTexture->GetName());
	ASSERT_MSG(dstTexture->GetFlags() & TEXFLAG_COPY_DST, "%s don't have TEXFLAG_COPY_DST flag", dstTexture->GetName());

	ASSERT_MSG(source.origin.x + copySize.width <= srcTexture->GetWidth() && source.origin.y + copySize.height >= srcTexture->GetHeight() && source.origin.arraySlice + copySize.arraySize <= srcTexture->GetArrayLayersSize(),
		"source texture origin and size outside of source texture size range");
	ASSERT_MSG(destination.origin.x + copySize.width <= dstTexture->GetWidth() && destination.origin.y + copySize.height >= dstTexture->GetHeight() && destination.origin.arraySlice + copySize.arraySize <= dstTexture->GetArrayLayersSize(),
		"dest texture origin and size outside of dest texture size range");

	nvrhi::TextureSlice rhiImageSrc{};
	rhiImageSrc.width = copySize.width;
	rhiImageSrc.height = copySize.height;
	rhiImageSrc.mipLevel = source.origin.mipLevel;
	rhiImageSrc.x = source.origin.x;
	rhiImageSrc.y = source.origin.y;

	nvrhi::TextureSlice rhiImageDst{};
	rhiImageSrc.width = copySize.width;
	rhiImageSrc.height = copySize.height;
	rhiImageSrc.mipLevel = destination.origin.mipLevel;
	rhiImageSrc.x = destination.origin.x;
	rhiImageSrc.y = destination.origin.y;
	
	for(int i = 0; i < copySize.arraySize; ++i)
	{
		rhiImageSrc.arraySlice = source.origin.arraySlice + i;
		rhiImageDst.arraySlice = destination.origin.arraySlice + i;
		m_rhiCommandList->copyTexture(dstTexture->GetNVRHITextureHandle(), rhiImageDst, srcTexture->GetNVRHITextureHandle(), rhiImageSrc);
	}
}

void CNVRHICommandRecorder::DbgPopGroup() const
{
	m_rhiCommandList->endMarker();
}

void CNVRHICommandRecorder::DbgPushGroup(const char* groupLabel) const
{
	m_rhiCommandList->beginMarker(groupLabel);
}

void CNVRHICommandRecorder::DbgAddMarker(const char* label) const
{
	m_rhiCommandList->beginMarker(label);
	m_rhiCommandList->endMarker();
}

IGPUCommandBufferPtr CNVRHICommandRecorder::End()
{
	if (!m_rhiCommandList)
	{
		ASSERT_FAIL("Command recorder was already ended");
		return nullptr;
	}

	// simply transfer command list
	CRefPtr<CNVRHICommandBuffer> commandBuffer = CRefPtr_new(CNVRHICommandBuffer);
	commandBuffer->m_rhiCommandList = m_rhiCommandList;
	m_rhiCommandList = nullptr;

	return IGPUCommandBufferPtr(commandBuffer);
}

//---------------------------------------------------------------

IGPURenderPassRecorderPtr CNVRHICommandRecorder::BeginRenderPass(const RenderPassDesc& renderPassDesc, void* userData) const
{
	CRefPtr<CNVRHIRenderPassRecorder> renderPass = CRefPtr_new(CNVRHIRenderPassRecorder, m_rhiCommandList, userData);
	renderPass->InternalBeginRenderPass(renderPassDesc);

	return IGPURenderPassRecorderPtr(renderPass);
}

//---------------------------------------------------------------

IGPUComputePassRecorderPtr CNVRHICommandRecorder::BeginComputePass(const char* name, void* userData) const
{
	CRefPtr<CNVRHIComputePassRecorder> renderPass = CRefPtr_new(CNVRHIComputePassRecorder, m_rhiCommandList, userData, name);

	return IGPUComputePassRecorderPtr(renderPass);
}