//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2025
//////////////////////////////////////////////////////////////////////////////////
// Description: NVRHI buffer
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "../RenderWorker.h"
#include "NVRHIBuffer.h"
#include "NVRHIRenderAPI.h"

CNVRHIBuffer::~CNVRHIBuffer()
{
	m_rhiBuffer = nullptr;
}

CNVRHIBuffer::CNVRHIBuffer(const BufferInfo& bufferInfo, int bufferUsageFlags, const char* label)
{
	m_dbgName = label;

	const bool isConstantOrUAV = (bufferUsageFlags & (BUFFERUSAGE_UNIFORM | BUFFERUSAGE_STORAGE));
	int64 minAligment = 4;
	if (bufferUsageFlags & BUFFERUSAGE_UNIFORM)
		minAligment = max(minAligment, CNVRHIRenderAPI::Instance.GetCaps().minStorageBufferOffsetAlignment);
	if (bufferUsageFlags & BUFFERUSAGE_STORAGE)
		minAligment = max(minAligment, CNVRHIRenderAPI::Instance.GetCaps().minStorageBufferOffsetAlignment);
	const int64 minAlignmentMask = minAligment - 1;

	const int64 sizeInBytes = bufferInfo.elementSize * bufferInfo.elementCapacity;
	const int64 writeDataSize = (bufferInfo.dataSize + 3) & ~3;
	const bool hasData = bufferInfo.data && bufferInfo.dataSize;

	m_bufSize = (sizeInBytes + minAlignmentMask) & ~minAlignmentMask;
	m_usageFlags = bufferUsageFlags;

	nvrhi::CpuAccessMode cpuAccessMode = nvrhi::CpuAccessMode::None;
	if ((bufferUsageFlags & BUFFERUSAGE_READ))
	{
		//ASSERT_MSG(hasData, "Buffer can't have READ usage when data is specified");
		//ASSERT_MSG(bufferUsageFlags & BUFFERUSAGE_WRITE, "Buffer can't have both WRITE and READ usages");
		cpuAccessMode = nvrhi::CpuAccessMode::Read;
	}

	if ((bufferUsageFlags & BUFFERUSAGE_WRITE) || hasData)
	{
		cpuAccessMode = nvrhi::CpuAccessMode::Write;
	}

	auto rhiBufferDesc = nvrhi::BufferDesc()
		.setCpuAccess(cpuAccessMode)
		.setByteSize(m_bufSize)
		.setDebugName(label);

	rhiBufferDesc.setInitialState(nvrhi::ResourceStates::Common);
	if(bufferUsageFlags & BUFFERUSAGE_COPY_DST)
	{
		rhiBufferDesc.setInitialState(nvrhi::ResourceStates::CopyDest);
		rhiBufferDesc.keepInitialState = true;
	}

	m_needsTrackingState = rhiBufferDesc.keepInitialState == false;

	if (bufferUsageFlags & BUFFERUSAGE_VERTEX)
		rhiBufferDesc.setIsVertexBuffer(true);

	if (bufferUsageFlags & BUFFERUSAGE_INDEX) 
	{
		rhiBufferDesc.setFormat(bufferInfo.elementSize == 2 ? nvrhi::Format::R16_UINT : nvrhi::Format::R32_UINT);
		rhiBufferDesc.setIsIndexBuffer(true);
		rhiBufferDesc.setCanHaveRawViews(true);
		rhiBufferDesc.setCanHaveTypedViews(true);
	}
	if (bufferUsageFlags & BUFFERUSAGE_INDIRECT)
		rhiBufferDesc.setIsDrawIndirectArgs(true);

	if (bufferUsageFlags & BUFFERUSAGE_UNIFORM)	
		rhiBufferDesc.setIsConstantBuffer(true);

	if (bufferUsageFlags & BUFFERUSAGE_STORAGE)
	{
		//if (bufferUsageFlags & BUFFERUSAGE_COPY_DST)
		//	rhiBufferDesc.setInitialState(nvrhi::ResourceStates::CopyDest | nvrhi::ResourceStates::UnorderedAccess);
		//else
		//	rhiBufferDesc.setInitialState(nvrhi::ResourceStates::UnorderedAccess);

		//if (bufferUsageFlags & BUFFERUSAGE_COPY_SRC)
		//	rhiBufferDesc.setCanHaveUAVs(true);

		//rhiBufferDesc.setCanHaveUAVs(true);
		rhiBufferDesc.setCanHaveRawViews(true);
		rhiBufferDesc.setCanHaveTypedViews(true);
	}

	rhiBufferDesc.debugName = label;

	nvrhi::IDevice* rhiDevice = CNVRHIRenderAPI::Instance.GetNVRHIDevice();

	m_rhiBuffer = rhiDevice->createBuffer(rhiBufferDesc);
	ASSERT_MSG(m_rhiBuffer, "Failed to create buffer %s", label);

	//MsgInfo("NVRHI: created buffer %s - %lld bytes\n", GetDbgName(), sizeInBytes);

	if (!m_rhiBuffer)
		return;

	if (hasData)
	{
		//MsgInfo("NVRHI: map and update buffer %s with %lld bytes (CNVRHIBuffer ctor)\n", GetDbgName(), writeDataSize);

		void* outData = rhiDevice->mapBuffer(m_rhiBuffer, nvrhi::CpuAccessMode::Write);
		ASSERT_MSG(outData, "Buffer mapped range is NULL");
		memcpy(outData, bufferInfo.data, writeDataSize);
		rhiDevice->unmapBuffer(m_rhiBuffer);
	}
}

void CNVRHIBuffer::OnUpdated()
{
	if (!(m_usageFlags & BUFFERUSAGE_COPY_DST))
		m_needsTrackingState = false; 
}

nvrhi::ResourceStates CNVRHIBuffer::GetNVRHIResourceStates() const
{
	// TODO: figure out resource states, D3D12 validation fails with RESOURCE_MANIPULATION ERROR #526: RESOURCE_BARRIER_INVALID_COMBINATION
	const int bufferUsageFlags = m_usageFlags;
	nvrhi::ResourceStates resStates = nvrhi::ResourceStates::Unknown;
	if (bufferUsageFlags & BUFFERUSAGE_COPY_DST) resStates = resStates | nvrhi::ResourceStates::CopyDest;
	if (bufferUsageFlags & BUFFERUSAGE_COPY_SRC) resStates = resStates | nvrhi::ResourceStates::CopySource;
	if (bufferUsageFlags & BUFFERUSAGE_VERTEX)	resStates = resStates | nvrhi::ResourceStates::VertexBuffer;
	if (bufferUsageFlags & BUFFERUSAGE_INDEX)	resStates = resStates | nvrhi::ResourceStates::IndexBuffer;
	if (bufferUsageFlags & BUFFERUSAGE_INDIRECT)resStates = resStates | nvrhi::ResourceStates::IndirectArgument;
	if (bufferUsageFlags & BUFFERUSAGE_UNIFORM)	resStates = resStates | nvrhi::ResourceStates::ConstantBuffer;
	if (bufferUsageFlags & BUFFERUSAGE_STORAGE)
	{
		resStates = resStates | nvrhi::ResourceStates::UnorderedAccess;// | nvrhi::ResourceStates::ShaderResource;
	}

	return resStates;
}

void CNVRHIBuffer::Update(const void* data, int64 size, int64 offset)
{
	if (!m_rhiBuffer)
		return;

	if (!size)
		return;

	const int64 writeDataSize = (size + 3) & ~3;
	nvrhi::IDevice* rhiDevice = CNVRHIRenderAPI::Instance.GetNVRHIDevice();

	nvrhi::CommandListParameters rhiCmdListParams = {};
	rhiCmdListParams.enableImmediateExecution = false;

	nvrhi::CommandListHandle writeCmd = rhiDevice->createCommandList(rhiCmdListParams);
	writeCmd->open();

	if (m_needsTrackingState)
	{
		//MsgInfo("NVRHI: tracked update buffer %s with %lld bytes (CNVRHIBuffer::Update)\n", GetDbgName(), writeDataSize);
		writeCmd->beginTrackingBufferState(m_rhiBuffer, nvrhi::ResourceStates::Common);
		writeCmd->writeBuffer(m_rhiBuffer, data, writeDataSize, offset);
		writeCmd->setPermanentBufferState(m_rhiBuffer, GetNVRHIResourceStates());
		OnUpdated();
	}
	else
	{
		//MsgInfo("NVRHI: un-tracked update buffer %s with %lld bytes (CNVRHIBuffer::Update)\n", GetDbgName(), writeDataSize);
		writeCmd->writeBuffer(m_rhiBuffer, data, writeDataSize, offset);
	}

	writeCmd->close();
	rhiDevice->executeCommandList(writeCmd);
}

Future<BufferMapData> CNVRHIBuffer::Lock(int lockOfs, int sizeToLock, int flags)
{
	if (m_isLocked)
		return Future<BufferMapData>::Failure(-1, "Buffer is already locked");

	if (!m_rhiBuffer || lockOfs < 0 || lockOfs + sizeToLock > m_bufSize)
	{
		ASSERT_FAIL("Locking outside range");
		Promise<BufferMapData> errorPromise;
		errorPromise.SetError(-1, "Lock failure");
		return errorPromise.CreateFuture();
	}

	nvrhi::IDevice* rhiDevice = CNVRHIRenderAPI::Instance.GetNVRHIDevice();

	Promise<BufferMapData> promise;

	BufferMapData lockData;
	lockData.flags = flags;
	lockData.size = sizeToLock;
	lockData.offset = lockOfs;

	// mapBuffer is blocking
	lockData.data = rhiDevice->mapBuffer(m_rhiBuffer, (m_usageFlags & BUFFERUSAGE_READ) ? nvrhi::CpuAccessMode::Read : nvrhi::CpuAccessMode::Write);
	if(!lockData.data)
		promise.SetError(-1, "Failed to lock buffer, wrong usage?");
	else
		promise.SetResult(std::move(lockData));

	m_isLocked = true;

	return promise.CreateFuture();
}

void CNVRHIBuffer::Unlock()
{
	if (!m_isLocked)
		return;

	nvrhi::IDevice* rhiDevice = CNVRHIRenderAPI::Instance.GetNVRHIDevice();
	rhiDevice->unmapBuffer(m_rhiBuffer);
	m_isLocked = false;
}
