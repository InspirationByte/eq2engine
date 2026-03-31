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
	ShaderAPIStats& stats = CNVRHIRenderAPI::Instance.GetStatsMutable();
	Atomic::Add(stats.bufferMem, -m_bufSize);

	CNVRHIRenderAPI::Instance.ReleaseRHITransientBufferHeap(m_transientHeapIdx);
	m_transientHeapIdx = -1;
	m_rhiBuffer = nullptr;
}

CNVRHIBuffer::CNVRHIBuffer(const BufferInfo& bufferInfo, int bufferUsageFlags, const char* label)
{
#ifdef RENDER_DEBUG_RESOURCE_NAMES
	m_dbgName = label;
#endif

	const int64 sizeInBytes = bufferInfo.elementSize * bufferInfo.elementCapacity;
	const int64 writeDataSize = ALIGN(bufferInfo.dataSize, 4);
	const bool hasData = bufferInfo.data && bufferInfo.dataSize;

	m_bufSize = ALIGN(sizeInBytes, 4);
	m_usageFlags = bufferUsageFlags;

	nvrhi::CpuAccessMode cpuAccessMode = nvrhi::CpuAccessMode::None;
	if (bufferUsageFlags & BUFFERUSAGE_READ)
	{
		//ASSERT_MSG(hasData, "Buffer can't have READ usage when data is specified");
		ASSERT_MSG((bufferUsageFlags & BUFFERUSAGE_WRITE) == 0, "Buffer can't have both WRITE and READ usages");
		cpuAccessMode = nvrhi::CpuAccessMode::Read;
	}

	if (bufferUsageFlags & BUFFERUSAGE_WRITE)
	{
		cpuAccessMode = nvrhi::CpuAccessMode::Write;
	}

	auto rhiBufferDesc = nvrhi::BufferDesc()
		.setCpuAccess(cpuAccessMode)
		.setByteSize(m_bufSize)
		.setDebugName(label)
		.setIsVirtual(bufferUsageFlags & BUFFERUSAGE_TRANSIENT);

	rhiBufferDesc.setInitialState(hasData ? nvrhi::ResourceStates::CopyDest : nvrhi::ResourceStates::Common);
	if(bufferUsageFlags & BUFFERUSAGE_COPY_DST)
	{
		rhiBufferDesc.setInitialState(nvrhi::ResourceStates::CopyDest);
		rhiBufferDesc.keepInitialState = true;
	}

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
		if (!(bufferUsageFlags & BUFFERUSAGE_COPY_DST))
		{
			if (bufferUsageFlags & BUFFERUSAGE_INDIRECT)
				rhiBufferDesc.setInitialState(nvrhi::ResourceStates::IndirectArgument);
			else
				rhiBufferDesc.setInitialState(nvrhi::ResourceStates::UnorderedAccess);

			rhiBufferDesc.keepInitialState = true;
		}

		rhiBufferDesc.setCanHaveUAVs(true);
		rhiBufferDesc.setCanHaveRawViews(true);
		rhiBufferDesc.setCanHaveTypedViews(true);
	}

	m_needsTrackingState = rhiBufferDesc.keepInitialState == false;
	rhiBufferDesc.debugName = label;

	nvrhi::IDevice* rhiDevice = CNVRHIRenderAPI::Instance.GetNVRHIDevice();

	m_rhiBuffer = rhiDevice->createBuffer(rhiBufferDesc);
	ASSERT_MSG(m_rhiBuffer, "Failed to create buffer %s", label);

	if (!m_rhiBuffer)
		return;

	ShaderAPIStats& stats = CNVRHIRenderAPI::Instance.GetStatsMutable();
	Atomic::Add(stats.bufferMem, m_bufSize);

	//MsgInfo("NVRHI: created buffer %s - %lld bytes\n", GetDbgName(), sizeInBytes);
	if(bufferUsageFlags & BUFFERUSAGE_TRANSIENT)
	{
		const int heapIdx = CNVRHIRenderAPI::Instance.AcquireRHITransientBufferHeap();
		m_transientHeapIdx = heapIdx;
		rhiDevice->bindBufferMemory(m_rhiBuffer, CNVRHIRenderAPI::Instance.GetRHIBufferHeap(heapIdx), 0);
	}

	if (hasData)
	{
		if (cpuAccessMode == nvrhi::CpuAccessMode::Write)
		{
			//MsgInfo("NVRHI: map and update buffer %s with %lld bytes (CNVRHIBuffer ctor)\n", GetDbgName(), writeDataSize);

			void* outData = rhiDevice->mapBuffer(m_rhiBuffer, nvrhi::CpuAccessMode::Write);
			ASSERT_MSG(outData, "Buffer mapped range is NULL");
			memcpy(outData, bufferInfo.data, writeDataSize);
			rhiDevice->unmapBuffer(m_rhiBuffer);
		}
		else
		{
			Update(bufferInfo.data, writeDataSize, 0);
		}
	}
}

void CNVRHIBuffer::OnUpdated()
{
	if (!(m_usageFlags & BUFFERUSAGE_COPY_DST))
		m_needsTrackingState = false; 
}

nvrhi::ResourceStates CNVRHIBuffer::GetNVRHIResourceStates(int usageFlags)
{
	// TODO: figure out resource states, D3D12 validation fails with RESOURCE_MANIPULATION ERROR #526: RESOURCE_BARRIER_INVALID_COMBINATION

	nvrhi::ResourceStates resStates = nvrhi::ResourceStates::Unknown;
	if (usageFlags & BUFFERUSAGE_COPY_DST) resStates = resStates | nvrhi::ResourceStates::CopyDest;
	if (usageFlags & BUFFERUSAGE_COPY_SRC) resStates = resStates | nvrhi::ResourceStates::CopySource;
	if (usageFlags & BUFFERUSAGE_VERTEX) resStates = resStates | nvrhi::ResourceStates::VertexBuffer;
	if (usageFlags & BUFFERUSAGE_INDEX)	resStates = resStates | nvrhi::ResourceStates::IndexBuffer;
	if (usageFlags & BUFFERUSAGE_INDIRECT) resStates = resStates | nvrhi::ResourceStates::IndirectArgument;
	if (usageFlags & BUFFERUSAGE_UNIFORM) resStates = resStates | nvrhi::ResourceStates::ConstantBuffer;
	if (usageFlags & BUFFERUSAGE_STORAGE)
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

	int cmdListIdx = -1;
	nvrhi::CommandListHandle writeCmd = CNVRHIRenderAPI::Instance.AcquireRHICommandList(cmdListIdx);
	writeCmd->open();

	if (m_needsTrackingState)
	{
		const nvrhi::ResourceStates rhiResStates = GetNVRHIResourceStates(m_usageFlags);
		nvrhi::ResourceStates rhiTrackingState = nvrhi::ResourceStates::Common;

		//MsgInfo("NVRHI: tracked update buffer %s with %lld bytes (CNVRHIBuffer::Update)\n", GetDbgName(), writeDataSize);
		writeCmd->beginTrackingBufferState(m_rhiBuffer, rhiTrackingState);
		writeCmd->writeBuffer(m_rhiBuffer, data, writeDataSize, offset);
		writeCmd->setPermanentBufferState(m_rhiBuffer, rhiResStates);
		OnUpdated();
	}
	else
	{
		//MsgInfo("NVRHI: un-tracked update buffer %s with %lld bytes (CNVRHIBuffer::Update)\n", GetDbgName(), writeDataSize);
		writeCmd->writeBuffer(m_rhiBuffer, data, writeDataSize, offset);
	}

	writeCmd->close();

	g_renderWorker.Execute("UpdateBuffer", [rhiDevice, writeCmd, cmdListIdx]() {
		rhiDevice->executeCommandList(writeCmd);
		CNVRHIRenderAPI::Instance.ReleaseCommandList(cmdListIdx);
		return 0;
	});
}

Future<BufferMapData> CNVRHIBuffer::Lock(int lockOfs, int sizeToLock, int flags)
{
	ASSERT_MSG(m_usageFlags & (BUFFERUSAGE_READ | BUFFERUSAGE_WRITE), "Buffer must have READ or WRITE usage flags");

	if (m_isLocked)
		return Future<BufferMapData>::Failure(-1, "Buffer is already locked");

	if (!m_rhiBuffer || lockOfs < 0 || lockOfs + sizeToLock > m_bufSize)
	{
		ASSERT_FAIL("Locking outside range");
		Promise<BufferMapData> errorPromise;
		errorPromise.SetError(-1, "Lock failure");
		return errorPromise.CreateFuture();
	}

	Promise<BufferMapData> promise;

	g_renderWorker.Execute(__func__, [this, promise, flags, sizeToLock, lockOfs]() {
		nvrhi::IDevice* rhiDevice = CNVRHIRenderAPI::Instance.GetNVRHIDevice();

		BufferMapData lockData;
		lockData.flags = flags;
		lockData.size = sizeToLock;
		lockData.offset = lockOfs;

		lockData.data = rhiDevice->mapBuffer(m_rhiBuffer, (m_usageFlags & BUFFERUSAGE_READ) ? nvrhi::CpuAccessMode::Read : nvrhi::CpuAccessMode::Write);
		if (!lockData.data)
			promise.SetError(-1, "Failed to lock buffer, wrong usage?");
		else
			promise.SetResult(std::move(lockData));
		return 0;
	});

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
