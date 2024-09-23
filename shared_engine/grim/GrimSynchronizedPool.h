//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: Synchronized slotted buffer
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "ds/SlottedArray.h"

class IGPUBuffer;
using IGPUBufferPtr = CRefPtr<IGPUBuffer>;

class IGPUComputePipeline;
using IGPUComputePipelinePtr = CRefPtr<IGPUComputePipeline>;

class IGPUCommandRecorder;

class GRIMBaseSyncrhronizedPool
{
public:
	virtual ~GRIMBaseSyncrhronizedPool() = default;
	GRIMBaseSyncrhronizedPool(const char* name);

	const char*				GetName() const { return m_name; }
	bool					IsValid() const { return m_updatePipeline != nullptr; }

	void					Init(int extraBufferFlags = 0, int initialSize = 3072, int gpuItemsGranularity = 1024);
	void					SetPipeline(IGPUComputePipelinePtr updatePipeline);
	virtual void			Clear(bool deallocate = false) = 0;

	virtual int				NumElem() const = 0;
	virtual int				NumSlots() const = 0;

	virtual void			Reserve(int count) = 0;
	virtual void			Remove(const int idx) = 0;

	virtual bool			Sync(IGPUCommandRecorder* cmdRecorder) = 0;
	IGPUBufferPtr			GetBuffer() const { return m_buffer; }

	static void				RunUpdatePipeline(IGPUCommandRecorder* cmdRecorder, IGPUComputePipeline* updatePipeline, IGPUBuffer* idxsBuffer, int idxsCount, IGPUBuffer* dataBuffer, IGPUBuffer* targetBuffer);
	static void				PrepareDataBuffer(IGPUCommandRecorder* cmdRecorder, ArrayCRef<int> elementIds, const ubyte* sourceData, int sourceStride, int elemSize, IGPUBufferPtr& destDataBuffer);
	static void				PrepareBuffers(IGPUCommandRecorder* cmdRecorder, const Set<int>& updated, Array<int>& elementIds, const ubyte* sourceData, int sourceStride, int elemSize, IGPUBufferPtr& destIdxsBuffer, IGPUBufferPtr& destDataBuffer);
	static int				GetGranulatedCapacity(int capacity, int granularity);

protected:
	bool					SyncImpl(IGPUCommandRecorder* cmdRecorder, const void* dataPtr, int stride);

	Threading::CEqMutex		m_mutex;
	Set<int>				m_updated{ PP_SL };
	IGPUBufferPtr			m_buffer;
	IGPUComputePipelinePtr	m_updatePipeline;
	EqString				m_name;

	int						m_extraBufferFlags{ 0 };
	int						m_initialSize{ 3072 };
	int						m_gpuItemsGranularity{ 1024 };
};

template<typename T>
class GRIMSyncrhronizedPool
	: public GRIMBaseSyncrhronizedPool
	, SlottedArray<T>
{
	using DATA = SlottedArray<T>;
public:

	GRIMSyncrhronizedPool(const char* name, PPSourceLine sl)
		: SlottedArray<T>(sl)
		, GRIMBaseSyncrhronizedPool(name)
	{
	}

	const T*		GetData() const { return DATA::ptr(); }
	int				Add(T& item);
	void			Remove(const int idx);
	void			Update(int idx, const T& item);

	void			Clear(bool deallocate = false) override;

	int				NumElem() const override { return DATA::numElem(); }
	int				NumSlots() const override { return DATA::numSlots(); }

	void			Reserve(int count) override { DATA::reserve(count); }

	// syncrhonizes data with GPU buffer
	// returns true if buffer has been changed
	bool			Sync(IGPUCommandRecorder* cmdRecorder) override;
};


//-------------------------------------------------------------------

template<typename T>
void GRIMSyncrhronizedPool<T>::Clear(bool deallocate)
{
	m_updated.clear(deallocate);
	DATA::clear(deallocate);

	if(deallocate)
		m_buffer = nullptr;
}

template<typename T>
int	GRIMSyncrhronizedPool<T>::Add(T& item)
{
	const int idx = DATA::add(item);
	m_updated.insert(idx);
	return idx;
}

template<typename T>
void GRIMSyncrhronizedPool<T>::Remove(const int idx)
{
	DATA::remove(idx);
}

template<typename T>
void GRIMSyncrhronizedPool<T>::Update(int idx, const T& item)
{
	(*this)[idx] = item;
	m_updated.insert(idx);
}

template<typename T>
bool GRIMSyncrhronizedPool<T>::Sync(IGPUCommandRecorder* cmdRecorder)
{
	return SyncImpl(cmdRecorder, GetData(), sizeof(T));
}
