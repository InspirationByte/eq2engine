#pragma once
#include "GrimSynchronizedPool.h"

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
	virtual int			GetPoolSize() const = 0;

	virtual bool		Sync(IGPUCommandRecorder* cmdRecorder) = 0;

	virtual void		Init() = 0;
	virtual void		Term() = 0;

	virtual void		InitEmptyItem() = 0;
	virtual int			IsValid() const = 0;
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
		: DataPool(GRIMResource::BUFFER, T::NAME, PP_SL, T::POOL_SIZE_EXTEND)
	{
	}

	int				Add(const T& item) { return DataPool::Add(item); }
	const T&		Get(int idx) const { return DataPool::GetData()[idx]; }
	void			Update(int idx, const T& data) { return DataPool::Update(idx, data); }

	DataPool&		GetDataPool() { return *this; }
	IGPUBufferPtr	GetBuffer() const { return DataPool::GetGPUData().Get<IGPUBuffer>(); }

	EqStringRef		GetName() const override { return T::NAME; }

	void			Clear(bool dealloc) override { DataPool::Clear(dealloc); }
	void			SetUpdated(int idx) override { DataPool::SetUpdated(idx); }
	void			Remove(int idx) override { DataPool::Remove(idx); }

	int				NumSlots() const override { return DataPool::NumSlots(); }
	int				NumElem() const override { return DataPool::NumElem(); }
	int				GetItemSize() const override { return sizeof(T); }
	int				GetPoolSize() const override { return DataPool::GetGPUData().GetSize() / sizeof(T); }

	bool			Sync(IGPUCommandRecorder* cmdRecorder) override { return DataPool::Sync(cmdRecorder); }

	void			Init() override { DataPool::InitBuffer(0, T::INITIAL_POOL_SIZE, T::POOL_SIZE_EXTEND); T::InitPipeline(*this);}
	void			Term() override { DataPool::SetPipeline(nullptr); }

	void			InitEmptyItem() override { DataPool::Add(T{}); }

	int				IsValid() const override { return DataPool::IsValid(); }
};

// Instance component data storage
// Represented as texture (used as storage or sampled)
template<typename T>
class GRIMTextureComponentPool
	: public GRIMBaseComponentPool
	, GRIMSyncrhronizedPool<T>
{
	using DataPool = GRIMSyncrhronizedPool<T>;
public:
	using TYPE = T;
	GRIMTextureComponentPool()
		: DataPool(GRIMResource::TEXTURE, T::NAME, PP_SL, T::POOL_SIZE_EXTEND)
	{
	}

	int				Add(const T& item) { return DataPool::Add(item); }
	const T&		Get(int idx) const { return DataPool::GetData()[idx]; }
	void			Update(int idx, const T& data) { return DataPool::Update(idx, data); }

	DataPool&		GetDataPool() { return *this; }
	ITexturePtr		GetTexture() const { return DataPool::GetGPUData().Get<ITexture>(); }

	EqStringRef		GetName() const override { return T::NAME; }

	void			Clear(bool dealloc) override { DataPool::Clear(dealloc); }
	void			SetUpdated(int idx) override { DataPool::SetUpdated(idx); }
	void			Remove(int idx) override { DataPool::Remove(idx); }

	int				NumSlots() const override { return DataPool::NumSlots(); }
	int				NumElem() const override { return DataPool::NumElem(); }
	int				GetItemSize() const override { return sizeof(T); }
	int				GetPoolSize() const override { return DataPool::GetGPUData().GetSize(); }

	bool			Sync(IGPUCommandRecorder* cmdRecorder) override { return DataPool::Sync(cmdRecorder); }

	void			Init() override { DataPool::InitTexture(T::POOL_TEXTURE_FORMAT, T::POOL_TEXTURE_SIZE, 0, T::INITIAL_POOL_SIZE, T::POOL_SIZE_EXTEND); T::InitPipeline(*this); }
	void			Term() override { DataPool::SetPipeline(nullptr); }

	void			InitEmptyItem() override { DataPool::Add(T{}); }

	int				IsValid() const override { return DataPool::IsValid(); }
};

// Instance component data storage
// Represented as storage buffer
template<typename T>
class GRIMComponentPool
	: public GRIMBaseComponentPool
	, SlottedArray<T>
{
	using DataPool = SlottedArray<T>;
public:
	using TYPE = T;
	GRIMComponentPool()
		: DataPool(PP_SL, T::POOL_SIZE_EXTEND)
	{
	}

	int				Add(const T& item) { return DataPool::add(item); }
	const T&		Get(int idx) const {return DataPool::ptr()[idx]; }
	void			Update(int idx, const T& data) { static_cast<DataPool&>(*this)[idx] = data; }

	DataPool&		GetDataPool() { return *this; }

	EqStringRef		GetName() const override { return T::NAME; }

	void			Clear(bool dealloc) override { DataPool::clear(dealloc); }
	void			SetUpdated(int idx) override {}
	void			Remove(int idx) override { DataPool::remove(idx); }

	int				NumSlots() const override { return DataPool::numSlots(); }
	int				NumElem() const override { return DataPool::numElem(); }
	int				GetItemSize() const override { return sizeof(T); }
	int				GetPoolSize() const override { return DataPool::numSlots(); }

	bool			Sync(IGPUCommandRecorder* cmdRecorder) override { return true; }

	void			Init() override {}
	void			Term() override {}

	void			InitEmptyItem() override { DataPool::add(T{}); }
	int				IsValid() const override { return true; }
};

#define DEFINE_INSTANCE_COMPONENT_GUTS(PoolType, ID, Name) \
	using POOL_T = PoolType<Name>; \
	static constexpr const char NAME[] = #Name; \
	static constexpr int COMPONENT_ID = ID; \
	static int INITIAL_POOL_SIZE; \
	static int POOL_SIZE_EXTEND;

// defines instance component
#define DEFINE_INSTANCE_COMPONENT(ID, Name) \
	DEFINE_INSTANCE_COMPONENT_GUTS(GRIMComponentPool, ID, Name) \
	static void InitPipeline(GRIMBaseSyncrhronizedPool& pool)

// defines buffer instance component that is synced with GPU
#define DEFINE_GPU_BUFFER_INSTANCE_COMPONENT(ID, Name) \
	DEFINE_INSTANCE_COMPONENT_GUTS(GRIMBufferComponentPool, ID, Name) \
	static void InitPipeline(GRIMBaseSyncrhronizedPool& pool)

// defines texture instance component that is synced with GPU
#define DEFINE_GPU_TEXTURE_INSTANCE_COMPONENT(ID, Name, Format, SizeX, SizeY) \
	static constexpr ETextureFormat POOL_TEXTURE_FORMAT = Format; \
	static constexpr IVector2D POOL_TEXTURE_SIZE = IVector2D(SizeX, SizeY); \
	DEFINE_INSTANCE_COMPONENT_GUTS(GRIMTextureComponentPool, ID, Name) \
	static void InitPipeline(GRIMBaseSyncrhronizedPool& pool)

// initializes instance component
#define INIT_INSTANCE_COMPONENT_EX(Name, InitialPoolSize, PoolSizeExtend) \
	int Name::INITIAL_POOL_SIZE = InitialPoolSize; \
	int Name::POOL_SIZE_EXTEND = PoolSizeExtend;

#define INIT_INSTANCE_COMPONENT(Name) \
	INIT_INSTANCE_COMPONENT_EX(Name, GRIM_DEFAULT_INST_INITIAL_POOL_SIZE, GRIM_DEFAULT_INST_POOL_SIZE_EXTEND)

// initializes GPU instance component with pool settings
#define INIT_GPU_INSTANCE_COMPONENT_EX(Name, UpdateShaderName, InitialPoolSize, PoolSizeExtend ) \
	INIT_INSTANCE_COMPONENT_EX(Name, InitialPoolSize, PoolSizeExtend) \
	void Name::InitPipeline(GRIMBaseSyncrhronizedPool& pool) { \
		pool.SetPipeline(g_renderAPI->CreateComputePipeline(Builder<ComputePipelineDesc>() \
			.ShaderName(UpdateShaderName).ShaderLayoutId(StringIdConst24(NAME)).End())); \
	}

// initializes GPU instance component
#define INIT_GPU_INSTANCE_COMPONENT(Name, UpdateShaderName) \
	INIT_GPU_INSTANCE_COMPONENT_EX(Name, UpdateShaderName, GRIM_DEFAULT_INST_INITIAL_POOL_SIZE, GRIM_DEFAULT_INST_POOL_SIZE_EXTEND)

