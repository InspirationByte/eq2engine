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
	static constexpr int NameHash{ baseClass::NameHash }; \
	virtual const char* GetName() const override { return name; } \
	virtual int GetNameHash() const override { return NameHash; }

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


//-------------------------------------------------
// Put this in class declaration

template<typename TComponentBase, typename THostType>
class ComponentContainer : COMPONENT_STORAGE<TComponentBase>
{
public:
	using WalkFunc = EqFunction<void(TComponentBase* pComponent)>;

	using STORAGE = COMPONENT_STORAGE<TComponentBase>;
	ComponentContainer() : STORAGE(PP_SL) {}
	~ComponentContainer() = default;

	void			RemoveAllComponents();

	TComponentBase*	GetComponent(int hash) const;
	void			AddComponent(int hash, TComponentBase* component);
	void			RemoveComponent(int hash);

	template<class CType>
	CType*			Get() const;

	template<class CType>
	void			Remove();
	
	template<class CType>
	CType*			Add();

	void			ForEachComponent(const WalkFunc& componentWalkFn);
};

template<typename TComponentBase, typename THostType>
TComponentBase* ComponentContainer<TComponentBase, THostType>::GetComponent(int hash) const
{
	auto it = STORAGE::idToComponent.find(hash);
	if (it.atEnd())
		return nullptr;
	return STORAGE::components[*it];
}

template<typename TComponentBase, typename THostType>
void ComponentContainer<TComponentBase, THostType>::AddComponent(int hash, TComponentBase* component)
{
	auto existingIt = STORAGE::idToComponent.find(hash);
	if (existingIt.atEnd())
	{
		const int newIndex = STORAGE::components.append(component);
		STORAGE::idToComponent.insert(hash, newIndex);
	}
	else
	{
		// add component back to slot
		STORAGE::components[*existingIt] = component;
	}
	component->OnAdded();
}

template<typename TComponentBase, typename THostType>
void ComponentContainer<TComponentBase, THostType>::RemoveComponent(int hash)
{
	auto it = STORAGE::idToComponent.find(hash);
	if (it.atEnd())
		return;

	TComponentBase* component = STORAGE::components[*it];
	if (component)
	{
		STORAGE::components[*it] = nullptr;
		component->OnRemoved();
		delete component;
	}
}

template<typename TComponentBase, typename THostType>
template<class CType>
CType* ComponentContainer<TComponentBase, THostType>::Get() const
{
	return static_cast<CType*>(GetComponent(CType::NameHash));
}

template<typename TComponentBase, typename THostType>
template<class CType>
void ComponentContainer<TComponentBase, THostType>::Remove()
{
	RemoveComponent(CType::NameHash);
}

template<typename TComponentBase, typename THostType>
void ComponentContainer<TComponentBase, THostType>::RemoveAllComponents()
{
	for (TComponentBase* component : STORAGE::components)
	{
		if (!component)
			continue;
		component->OnRemoved();
		delete component;
	}
	STORAGE::components.clear(true);
	STORAGE::idToComponent.clear(true);
}

template<typename TComponentBase, typename THostType>
template<class CType> 
CType* ComponentContainer<TComponentBase, THostType>::Add()
{
	ASSERT_MSG(GetComponent(CType::NameHash) == nullptr, "Component %s is already added", CType::Name);
	CType* component = PPNewSL(PPSourceLine::Make(CType::Name, 0)) CType(static_cast<THostType*>(this));
	AddComponent(CType::NameHash, component);
	return component;
}

template<typename TComponentBase, typename THostType>
void ComponentContainer<TComponentBase, THostType>::ForEachComponent(const WalkFunc& componentWalkFn)
{
	ASSERT(componentWalkFn);
	for (TComponentBase* component : STORAGE::components)
	{
		if (!component)
			continue;
		componentWalkFn(component);
	}
}
