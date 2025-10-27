//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: Synchronized slotted buffer
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "ds/SlottedArray.h"

enum ETextureFormat : uint16;

class IGPUBuffer;
using IGPUBufferPtr = CRefPtr<IGPUBuffer>;

class ITexture;
using ITexturePtr = CRefPtr<ITexture>;

class IGPUComputePipeline;
using IGPUComputePipelinePtr = CRefPtr<IGPUComputePipeline>;

class IGPUCommandRecorder;

struct GRIMLock
{
	virtual void LockRead() {}
	virtual void LockWrite() {}
	virtual void UnlockRead() {}
	virtual void UnlockWrite() {}
	static GRIMLock EmptyLock;
};

struct GRIMResource
{
	enum Type
	{
		BUFFER,
		TEXTURE
	};

	GRIMResource(Type type);
	GRIMResource(const GRIMResource& other) = delete;
	~GRIMResource();

	operator			bool() const;
	Type				GetType() const { return type; }
	int					GetSize() const;
	void				Reset();

	template<typename T>
	CRefPtr<T>			Get() const;

	template<typename T>
	void				Set(T* ptr);

private:
	union {
		IGPUBufferPtr	buffer;
		ITexturePtr		texture;
	};
	Type	type;
};

template<typename T>
CRefPtr<T> GRIMResource::Get() const
{
	static_assert(std::is_same_v<T, IGPUBuffer> || std::is_same_v<T, ITexture>, "Get<T> - T is not Buffer or Texture");
	if constexpr (std::is_same_v<T, IGPUBuffer>)
	{
		ASSERT(type == BUFFER);
		return buffer;
	}
	else if constexpr (std::is_same_v<T, ITexture>)
	{
		ASSERT(type == TEXTURE);
		return texture;
	}
}

template<typename T>
void GRIMResource::Set(T* ptr)
{
	static_assert(std::is_same_v<T, IGPUBuffer> || std::is_same_v<T, ITexture>, "Set<T> - T is not Buffer or Texture");
	if constexpr (std::is_same_v<T, IGPUBuffer>)
	{
		ASSERT(type == BUFFER);
		buffer.Assign(ptr);
	}
	else if constexpr (std::is_same_v<T, ITexture>)
	{
		ASSERT(type == TEXTURE);
		texture.Assign(ptr);
	}
}

//-----------------------------------------------------

class GRIMBaseSyncrhronizedPool
{
public:
	virtual ~GRIMBaseSyncrhronizedPool() = default;
	GRIMBaseSyncrhronizedPool(GRIMResource::Type type, const char* name);

	const char*				GetName() const { return m_name; }
	bool					IsValid() const { return m_updatePipeline != nullptr; }

	void					InitBuffer(int extraBufferFlags = 0, int initialSize = 3072, int gpuItemsGranularity = 1024);
	void					InitTexture(ETextureFormat format, IVector2D textureSize, int extraTextureFlags = 0, int initialArraySize = 3072, int gpuItemsGranularity = 1024);

	void					SetPipeline(IGPUComputePipelinePtr updatePipeline);
	virtual void			Clear(bool deallocate = false) = 0;

	virtual int				NumElem() const = 0;
	virtual int				NumSlots() const = 0;

	virtual void			Reserve(int count) = 0;
	virtual void			Remove(const int idx) = 0;
	void					SetUpdated(int idx);

	virtual bool			Sync(IGPUCommandRecorder* cmdRecorder, GRIMLock& lock, IGPUBufferPtr& updDataBuffer, IGPUBufferPtr& updIdxsBuffer) = 0;
	const GRIMResource&		GetGPUData() const { return m_gpuData; }
	GRIMResource::Type		GetType() const { return m_gpuData.GetType(); }

	static void				RunUpdatePipeline(IGPUCommandRecorder* cmdRecorder, IGPUComputePipeline* updatePipeline, IGPUBuffer* idxsBuffer, int idxsCount, IGPUBuffer* dataBuffer, const GRIMResource& targetData);
	static void				PrepareDataBuffer(IGPUCommandRecorder* cmdRecorder, ArrayCRef<int> elementIds, const ubyte* sourceData, int sourceStride, int elemSize, IGPUBufferPtr& destDataBuffer);
	static void				PrepareBuffers(IGPUCommandRecorder* cmdRecorder, const Set<int>& updated, Array<int>& elementIds, const ubyte* sourceData, int sourceStride, int elemSize, IGPUBufferPtr& destIdxsBuffer, IGPUBufferPtr& destDataBuffer);
	static int				GetGranulatedCapacity(int capacity, int granularity);

	static IVector2D		CalcWorkSize(int length);

protected:
	bool					SyncImpl(IGPUCommandRecorder* cmdRecorder, const void* dataPtr, int stride, GRIMLock& lock, IGPUBufferPtr& updDataBuffer, IGPUBufferPtr& updIdxsBuffer);

	Set<int>				m_updated{ PP_SL };
	Array<int>				m_syncElementIds{ PP_SL };
	GRIMResource			m_gpuData;
	IGPUComputePipelinePtr	m_updatePipeline;
	EqString				m_name;

	ETextureFormat			m_texFormat{ (ETextureFormat)0 };
	IVector2D				m_texSize{ 0 };

	int						m_extraResourceFlags{ 0 };
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

	GRIMSyncrhronizedPool(GRIMResource::Type type, const char* name, PPSourceLine sl, int granularity = 16)
		: SlottedArray<T>(sl, granularity)
		, GRIMBaseSyncrhronizedPool(type, name)
	{
	}

	T&				operator[](const int idx) { return static_cast<DATA&>(*this)[idx]; }
	const T&		operator[](const int idx) const { return static_cast<const DATA&>(*this)[idx]; }
	bool			operator()(const int idx) const { return static_cast<const DATA&>(*this)(idx); }

	const T*		GetData() const { return DATA::ptr(); }
	int				Add(const T& item);
	void			Remove(const int idx);
	void			Update(int idx, const T& newData);

	void			Clear(bool deallocate = false) override;

	int				NumElem() const override { return DATA::numElem(); }
	int				NumSlots() const override { return DATA::numSlots(); }
	void			Reserve(int count) override { DATA::reserve(count); }

	// syncrhonizes data with GPU buffer
	// returns true if buffer has been changed
	bool			Sync(IGPUCommandRecorder* cmdRecorder, GRIMLock& lock, IGPUBufferPtr& updDataBuffer, IGPUBufferPtr& updIdxsBuffer) override;
};


//-------------------------------------------------------------------

template<typename T>
void GRIMSyncrhronizedPool<T>::Clear(bool deallocate)
{
	m_updated.clear(deallocate);
	DATA::clear(deallocate);

	if(deallocate)
		m_gpuData.Reset();
}

template<typename T>
int	GRIMSyncrhronizedPool<T>::Add(const T& item)
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
	SetUpdated(idx);
}

template<typename T>
bool GRIMSyncrhronizedPool<T>::Sync(IGPUCommandRecorder* cmdRecorder, GRIMLock& lock, IGPUBufferPtr& updDataBuffer, IGPUBufferPtr& updIdxsBuffer)
{
	return SyncImpl(cmdRecorder, GetData(), sizeof(T), lock, updDataBuffer, updIdxsBuffer);
}
