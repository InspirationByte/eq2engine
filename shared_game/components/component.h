//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Component System
//////////////////////////////////////////////////////////////////////////////////

#pragma once

template <typename HOST>
class ComponentBase
{
public:
	virtual ~ComponentBase() = default;
	ComponentBase() = default;
	ComponentBase(HOST* host) : m_host(host) {}

	virtual const char*	GetName() const = 0;
	virtual int			GetNameHash() const = 0;

	virtual void		OnAdded() {}
	virtual void		OnRemoved() {}

	HOST*				GetHost() const { return m_host; }
protected:
	HOST*	m_host{ nullptr };
};

#define DECLARE_COMPONENT(name) \
	static constexpr const char Name[] = name; \
	static constexpr int NameHash{ StringToHashConst(name) }; \
	virtual const char* GetName() const override { return name; } \
	virtual int GetNameHash() const override { return NameHash; }

#define DECLARE_COMPONENT_DERIVED(name, baseClass) \
	using BaseClass = baseClass; \
	static constexpr const char Name[] = name; \
	static constexpr int NameHash{ StringToHashConst(baseClass::Name) }; \
	virtual const char* GetName() const override { return name; } \
	virtual int GetNameHash() const override { return NameHash; }

// hard-linked component instantiator
#define	ADD_COMPONENT_GETTER(type)	Type* Get<Type>() const { return &m_inst##Type; }
#define	ADD_COMPONENT_INST(type)	Type m_inst##Type(this);

template<typename TComponentBase>
struct COMPONENT_STORAGE
{
	COMPONENT_STORAGE(PPSourceLine sl)
		: idToComponent(sl)
		, components(sl)
	{
	}

	Map<int, int>			idToComponent;
	Array<TComponentBase*>	components;
};

namespace ComponentHostImpl
{
	template<typename TComponentBase>
	inline TComponentBase* GetComponent(const COMPONENT_STORAGE<TComponentBase>& storage, int hash)
	{
		auto it = storage.idToComponent.find(hash);
		if (it.atEnd())
			return nullptr;
		return storage.components[*it];
	}

	template<typename TComponentBase>
	inline void AddComponent(COMPONENT_STORAGE<TComponentBase>& storage, int hash, TComponentBase* component)
	{
		auto existingIt = storage.idToComponent.find(hash);
		if (existingIt.atEnd())
		{
			const int newIndex = storage.components.append(component);
			storage.idToComponent.insert(hash, newIndex);
		}
		else
		{
			// add component back to slot
			storage.components[*existingIt] = component;
		}
		component->OnAdded();
	}

	template<typename TComponentBase>
	inline TComponentBase* GetComponent(const COMPONENT_STORAGE<TComponentBase>& storage, const char* name)
	{
		const int hash = StringToHash(name);
		return GetComponent(storage, hash);
	}

	template<typename TComponentBase>
	inline void ForEachComponent(COMPONENT_STORAGE<TComponentBase>& storage, const EqFunction<void(TComponentBase* pComponent)>& componentWalkFn)
	{
		ASSERT(componentWalkFn);
		for (TComponentBase* component : storage.components)
		{
			if (!component)
				continue;
			componentWalkFn(component);
		}
	}

	template<typename TComponentBase>
	inline void RemoveComponent(COMPONENT_STORAGE<TComponentBase>& storage, int hash)
	{
		auto it = storage.idToComponent.find(hash);
		if (it.atEnd())
			return;

		TComponentBase* component = storage.components[*it];
		if(component)
		{
			storage.components[*it] = nullptr;
			component->OnRemoved();
			delete component;
		}
	}

	template<typename TComponentBase>
	inline void RemoveComponent(COMPONENT_STORAGE<TComponentBase>& storage, const char* name)
	{
		const int hash = StringToHash(name);
		RemoveComponent(storage, hash);
	}

	template<typename TComponentBase>
	inline void RemoveAll(COMPONENT_STORAGE<TComponentBase>& storage)
	{
		for (TComponentBase* component : storage.components)
		{
			if (!component)
				continue;
			component->OnRemoved();
			delete component;
		}
		storage.components.clear(true);
		storage.idToComponent.clear(true);
	}
}

//-------------------------------------------------

// Put this in class declaration
#define DECLARE_COMPONENT_HOST(hostType, componentBase) \
	private: \
		COMPONENT_STORAGE<componentBase>	m_components{ PP_SL }; \
	public: \
		inline componentBase*			GetComponent(int hash) const { return ComponentHostImpl::GetComponent(m_components, hash); } \
		inline void						AddComponent(int hash, componentBase* component) { return ComponentHostImpl::AddComponent(m_components, hash, component); } \
		inline componentBase*			Get(const char* name) const { return ComponentHostImpl::GetComponent(m_components, name); } \
		template<class CType> CType*	Get() const { return static_cast<CType*>(ComponentHostImpl::GetComponent<componentBase>(m_components, CType::NameHash)); } \
		template<class CType> void		Remove() { ComponentHostImpl::RemoveComponent<componentBase>(m_components, CType::NameHash); } \
		inline void						Remove(const char* name) { return ComponentHostImpl::RemoveComponent(m_components, name); } \
		inline void						RemoveAllComponents() { ComponentHostImpl::RemoveAll(m_components); } \
		template<class CType> CType*	Add() { \
			ASSERT_MSG(GetComponent(CType::NameHash) == nullptr, "Component %s is already added", CType::Name); \
			CType* component = PPNewSL(PPSourceLine::Make(CType::Name, 0)) CType(this); \
			ComponentHostImpl::AddComponent<componentBase>(m_components, CType::NameHash, component); \
			return component; \
		}\
		void ForEachComponent(const EqFunction<void(componentBase* pComponent)>& componentWalkFn) {\
			ComponentHostImpl::ForEachComponent(m_components, componentWalkFn);\
		}
