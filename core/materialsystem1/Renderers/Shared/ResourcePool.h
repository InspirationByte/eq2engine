#pragma once

struct RenderResources
{
	struct IResourcePool
	{
		virtual void Clear() = 0;
		bool isInit = false;
	};

	template<typename ResourceType>
	struct ResourcePool : public IResourcePool
	{
		void Clear() override
		{
			memPool.clear();
			isInit = false;
		}
		Threading::CEqMutex		mutex;
		MemoryPool<ResourceType>	memPool{ PP_SL };
	};

	static Array<IResourcePool*>		s_resourcePools;

	template<typename ResourceType>
	static ResourcePool<ResourceType>& GetResourcePool()
	{
		static ResourcePool<ResourceType>	s_resPool;
		if (!s_resPool.isInit)
		{
			Threading::CScopedMutex m(s_resPool.mutex);
			s_resourcePools.append(&s_resPool);
			s_resPool.isInit = true;
		}

		return s_resPool;
	}

	template<typename ResourceType>
	static ResourceType* Alloc()
	{
		ResourcePool<ResourceType>& resPool = GetResourcePool<ResourceType>();
		Threading::CScopedMutex m(resPool.mutex);
		return resPool.memPool.allocate();
	}

	template<typename ResourceType>
	static void Free(ResourceType* res)
	{
		ResourcePool<ResourceType>& effPool = GetResourcePool<ResourceType>();
		Threading::CScopedMutex m(effPool.mutex);
		res->~ResourceType();
		effPool.memPool.deallocate(res);
	}

	static void Terminate();
};

#define DECLARE_RENDER_RESOURCE(TYPE) \
	template<typename ...Args> \
	inline static CRefPtr<TYPE> Create(Args&&...args) { return CRefPtr<TYPE>(new(RenderResources::Alloc<TYPE>()) TYPE(std::forward<Args>(args)...)); } \
	void Ref_DeleteObject() override { RenderResources::Free(this); }

