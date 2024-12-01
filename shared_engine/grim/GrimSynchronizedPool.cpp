//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: Synchronized slotted buffer
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "materialsystem1/renderers/IShaderAPI.h"
#include "GrimSynchronizedPool.h"


GRIMResource::GRIMResource(Type type)
	: type(type)
	, buffer(nullptr)
{
}

GRIMResource::GRIMResource(const GRIMResource& other)
	: type(other.type)
	, buffer(nullptr)
{
	if (type == BUFFER)
		buffer = other.buffer;
	else if (type == TEXTURE)
		texture = other.texture;
}

GRIMResource::GRIMResource(ITexture* texture)
	: type(TEXTURE)
	, texture(texture)
{
}

GRIMResource::GRIMResource(IGPUBuffer* buffer)
	: type(BUFFER)
	, buffer(buffer)
{
}

GRIMResource::~GRIMResource()
{
	Reset();
}

void GRIMResource::Reset()
{
	if (type == BUFFER)
		buffer = nullptr;
	else if (type == TEXTURE)
		texture = nullptr;
}

int	GRIMResource::GetSize() const
{
	if (type == BUFFER)
		return buffer->GetSize();
	else if (type == TEXTURE)
		return texture->GetArraySize();

	ASSERT_FAIL("Invalid type %d", type);
	return 0;
}

GRIMResource::operator bool() const
{
	if (type == BUFFER)
		return buffer;
	else if (type == TEXTURE)
		return texture;
	ASSERT_FAIL("Invalid type %d", type);
	return false;
}

void GRIMBaseSyncrhronizedPool::RunUpdatePipeline(IGPUCommandRecorder* cmdRecorder, IGPUComputePipeline* updatePipeline, IGPUBuffer* idxsBuffer, int idxsCount, IGPUBuffer* dataBuffer, const GRIMResource& targetData)
{
	IGPUComputePassRecorderPtr computePass = cmdRecorder->BeginComputePass("UpdateInstances");

	IGPUBindGroupPtr sourceIdxsAndDataGroup = g_renderAPI->CreateBindGroup(updatePipeline,
		Builder<BindGroupDesc>().GroupIndex(0)
		.Buffer(0, idxsBuffer)
		.Buffer(1, dataBuffer)
		.End()
	);
	Builder<BindGroupDesc> targetBindGroupBuilder;
	if (targetData.GetType() == GRIMResource::BUFFER)
		targetBindGroupBuilder.Buffer(0, targetData.Get<IGPUBuffer>());
	else if (targetData.GetType() == GRIMResource::TEXTURE)
		targetBindGroupBuilder.StorageTexture(0, targetData.Get<ITexture>());

	IGPUBindGroupPtr destPoolDataGroup = g_renderAPI->CreateBindGroup(updatePipeline, targetBindGroupBuilder.GroupIndex(1).End());

	computePass->SetPipeline(updatePipeline);
	computePass->SetBindGroup(0, sourceIdxsAndDataGroup);
	computePass->SetBindGroup(1, destPoolDataGroup);

	IVector2D workGroups = CalcWorkSize(idxsCount);
	computePass->DispatchWorkgroups(workGroups.x, workGroups.y);
	computePass->Complete();
}

// prepares data buffer
void GRIMBaseSyncrhronizedPool::PrepareDataBuffer(IGPUCommandRecorder* cmdRecorder, ArrayCRef<int> elementIds, const ubyte* sourceData, int sourceStride, int elemSize, IGPUBufferPtr& destDataBuffer)
{
	ArrayCRef<int> elementIdArray = ArrayCRef(elementIds.ptr()+1, elementIds.numElem()-1);
	const int updateBufferSize = elementIdArray.numElem() * elemSize;

	ubyte* updateData = reinterpret_cast<ubyte*>(PPAlloc(updateBufferSize));
	ubyte* updateDataStart = updateData;
	defer{
		PPFree(updateDataStart);
	};

	// as GPU does not like unaligned access, we put updated elements in separate buffer
	for (const int elemIdx : elementIdArray)
	{
		const ubyte* updInstPtr = sourceData + sourceStride * elemIdx;
		memcpy(updateData, updInstPtr, elemSize);
		updateData += elemSize;
	}

	destDataBuffer = g_renderAPI->CreateBuffer(BufferInfo(1, updateBufferSize), BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "InstUpdData");
	cmdRecorder->WriteBuffer(destDataBuffer, updateDataStart, updateBufferSize, 0);
}

// prepare data and indices
void GRIMBaseSyncrhronizedPool::PrepareBuffers(IGPUCommandRecorder* cmdRecorder, const Set<int>& updated, Array<int>& elementIds, const ubyte* sourceData, int sourceStride, int elemSize, IGPUBufferPtr& destIdxsBuffer, IGPUBufferPtr& destDataBuffer)
{
	ASSERT(elemSize <= sourceStride);

	const int updatedCount = updated.size();
	elementIds.reserve(updatedCount + 1);
	elementIds.clear();

	// always insert count as first element (sourceCount)
	elementIds.append(updatedCount);

	for (auto it = updated.begin(); !it.atEnd(); ++it)
		elementIds.append(it.key());

	const int updateBufferSize = elementIds.numElem() * elemSize;

	ubyte* updateData = reinterpret_cast<ubyte*>(PPAlloc(updateBufferSize));
	ubyte* updateDataStart = updateData;
	defer{
		PPFree(updateDataStart);
	};

	// as GPU does not like unaligned access, we put updated elements in separate buffer
	for (const int elemIdx : ArrayCRef(elementIds.ptr() + 1, elementIds.numElem() - 1))
	{
		const ubyte* updInstPtr = sourceData + sourceStride * elemIdx;
		memcpy(updateData, updInstPtr, elemSize);
		updateData += elemSize;
	}

	destIdxsBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(elementIds[0]), elementIds.numElem()), BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "InstUpdIdxs");
	destDataBuffer = g_renderAPI->CreateBuffer(BufferInfo(1, updateBufferSize), BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "InstUpdData");

	cmdRecorder->WriteBuffer(destIdxsBuffer, elementIds.ptr(), sizeof(elementIds[0]) * elementIds.numElem(), 0);
	cmdRecorder->WriteBuffer(destDataBuffer, updateDataStart, updateBufferSize, 0);
}

int GRIMBaseSyncrhronizedPool::GetGranulatedCapacity(int capacity, int granularity)
{
	return (capacity + granularity - 1) / granularity * granularity;
}

IVector2D GRIMBaseSyncrhronizedPool::CalcWorkSize(int length)
{
	constexpr int GPUSYNC_GROUP_SIZE = 256;
	constexpr int GPUSYNC_MAX_DIM_GROUPS = 1024;
	constexpr int GPUSYNC_MAX_DIM_THREADS = (GPUSYNC_GROUP_SIZE * GPUSYNC_MAX_DIM_GROUPS);

	IVector2D result;
	if (length <= GPUSYNC_MAX_DIM_THREADS)
	{
		result.x = (length - 1) / GPUSYNC_GROUP_SIZE + 1;
		result.y = 1;
	}
	else
	{
		result.x = GPUSYNC_MAX_DIM_GROUPS;
		result.y = (length - 1) / GPUSYNC_MAX_DIM_THREADS + 1;
	}

	return result;
}

GRIMBaseSyncrhronizedPool::GRIMBaseSyncrhronizedPool(GRIMResource::Type type, const char* name)
	: m_name(name)
	, m_gpuData(type)
{
}

void GRIMBaseSyncrhronizedPool::InitBuffer(int extraBufferFlags, int initialSize, int gpuItemsGranularity)
{
	ASSERT_MSG(GetType() == GRIMResource::BUFFER, "Trying to inititialize BUFFER resource while it's not");

	m_extraResourceFlags = extraBufferFlags;
	m_initialSize = initialSize;
	m_gpuItemsGranularity = gpuItemsGranularity;
}

void GRIMBaseSyncrhronizedPool::InitTexture(ETextureFormat format, IVector2D textureSize, int extraTextureFlags, int initialArraySize, int gpuItemsGranularity)
{
	ASSERT_MSG(GetType() == GRIMResource::TEXTURE, "Trying to inititialize TEXTURE resource while it's not");

	m_texFormat = format;
	m_texSize = textureSize;

	m_extraResourceFlags = extraTextureFlags;
	m_initialSize = initialArraySize;
	m_gpuItemsGranularity = gpuItemsGranularity;
}

void GRIMBaseSyncrhronizedPool::SetPipeline(IGPUComputePipelinePtr updatePipeline)
{
	m_updatePipeline = updatePipeline;
}

bool GRIMBaseSyncrhronizedPool::SyncImpl(IGPUCommandRecorder* cmdRecorder, const void* dataPtr, int stride)
{
	if (!m_updatePipeline)
	{
		ASSERT_FAIL("GRIMSyncrhronizedPool <%s> is not initialized", m_name.ToCString());
		return false;
	}

	const int currentNumSlots = NumSlots();
	const int oldInstElems = m_gpuData ? m_gpuData.GetSize() / stride : 0;

	bool buffersUpdated = false;

	// update instance root buffer
	// TODO: instead of re-creating buffer, create new separate buffer with it's instance pools
	if (!m_gpuData || currentNumSlots > oldInstElems)
	{
		// alloc (or re-create) new buffer and upload entire data
		const int allocInstElems = max(GetGranulatedCapacity(currentNumSlots, m_gpuItemsGranularity), m_initialSize);

		if (m_gpuData.GetType() == GRIMResource::BUFFER)
		{
			IGPUBufferPtr oldBuffer(m_gpuData.Get<IGPUBuffer>());

			const int bufferFlags = m_extraResourceFlags | BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST | BUFFERUSAGE_COPY_SRC;
			IGPUBufferPtr newBuffer = g_renderAPI->CreateBuffer(BufferInfo(stride, allocInstElems), bufferFlags, m_name);
			m_gpuData.Set(newBuffer.Ptr());

			if (oldInstElems > 0 && oldBuffer)
			{
				// copy old buffer data to new one, and still run update pipeline later below
				// effectively updating old data and adding new data
				cmdRecorder->CopyBufferToBuffer(oldBuffer, 0, newBuffer, 0, oldInstElems * stride);
			}
			else
			{
				// don't waste time on running pipeline and upload everything directly to GPU
				// since buffer is brand new
				cmdRecorder->WriteBuffer(newBuffer, dataPtr, currentNumSlots * stride, 0);
				{
					Threading::CScopedMutex m(m_mutex);
					m_updated.clear();
				}
				return true;
			}
		}
		else if (m_gpuData.GetType() == GRIMResource::TEXTURE)
		{
			const int textureFlags = m_extraResourceFlags | TEXFLAG_STORAGE | TEXFLAG_COPY_DST | TEXFLAG_COPY_DST;
			ITexturePtr newTexture = g_renderAPI->CreateRenderTarget(TextureDesc(m_name, textureFlags, m_texFormat, m_texSize.x, m_texSize.y, allocInstElems));
			m_gpuData.Set(newTexture.Ptr());

			ASSERT_FAIL("Texture Resize: Not implemented yet!");
		}
		
		buffersUpdated = true;
	}

	{
		Threading::CScopedMutex m(m_mutex);
		if (m_updated.size())
		{
			Array<int> elementIds(PP_SL);

			IGPUBufferPtr idxsBuffer;
			IGPUBufferPtr dataBuffer;
			PrepareBuffers(cmdRecorder, m_updated, elementIds, reinterpret_cast<const ubyte*>(dataPtr), stride, stride, idxsBuffer, dataBuffer);
			RunUpdatePipeline(cmdRecorder, m_updatePipeline, idxsBuffer, m_updated.size(), dataBuffer, m_gpuData);
		}
		m_updated.clear();
	}

	return buffersUpdated;
}

void GRIMBaseSyncrhronizedPool::SetUpdated(int idx)
{
	m_updated.insert(idx);
}