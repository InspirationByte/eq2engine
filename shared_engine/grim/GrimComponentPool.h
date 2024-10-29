#pragma once

class IGPUCommandRecorder;

class GRIMBaseComponentPool
{
public:
	virtual EqStringRef	GetName() const = 0;

	virtual void		Clear(bool dealloc) = 0;
	virtual void		SetUpdated(int idx) = 0;
	virtual void		Remove(int idx) = 0;

	virtual int			NumSlots() const = 0;
	virtual int			NumElem() const = 0;
	virtual int			GetItemSize() const = 0;

	virtual bool		Sync(IGPUCommandRecorder* cmdRecorder) = 0;

	virtual void		Init(int extraBufferFlags = 0, int initialSize = 3072, int gpuItemsGranularity = 1024) = 0;
	virtual void		InitEmptyItem() = 0;

	virtual void		InitPipeline() = 0;
	virtual void		TermPipeline() = 0;
	virtual int			IsValid() const = 0;

	virtual int			GetInitialSize() const = 0;
	virtual int			GetSizeGranularity() const = 0;
};

// Instance component data storage
// Represented as storage buffer
template<typename T>
class GRIMBufferComponentPool 
	: public GRIMBaseComponentPool
	, GRIMSyncrhronizedPool<T>
{
	using DataPool = GRIMSyncrhronizedPool<T>;
public:
	using TYPE = T;
	GRIMBufferComponentPool()
		: DataPool(T::NAME, PP_SL)
	{
	}

	DataPool&		GetDataPool() { return *this; }
	IGPUBufferPtr	GetBuffer() const { return DataPool::GetBuffer(); }

	EqStringRef		GetName() const override { return DataPool::GetName(); }

	void			Clear(bool dealloc) override { DataPool::Clear(dealloc); }
	void			SetUpdated(int idx) override { DataPool::SetUpdated(idx); }
	void			Remove(int idx) override { DataPool::Remove(idx); }

	int				NumSlots() const override { return DataPool::NumSlots(); }
	int				NumElem() const override { return DataPool::NumElem(); }

	bool			Sync(IGPUCommandRecorder* cmdRecorder) override { return DataPool::Sync(cmdRecorder); }

	void			Init(int extraBufferFlags = 0, int initialSize = 3072, int gpuItemsGranularity = 1024) override { DataPool::Init(extraBufferFlags, initialSize, gpuItemsGranularity); }
	void			InitEmptyItem() override { DataPool::Add(T{}); }
	void			InitPipeline() override { T::InitPipeline(*this); }

	void			TermPipeline() override { DataPool::SetPipeline(nullptr); }
	int				GetItemSize() const override { return sizeof(T); }
	int				IsValid() const override { return DataPool::IsValid(); }

	int				GetInitialSize() const override { return T::INITIAL_POOL_SIZE; }
	int				GetSizeGranularity() const override { return T::POOL_SIZE_EXTEND; }
};