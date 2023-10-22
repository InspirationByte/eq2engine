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
	virtual const char* GetName() const override { return name; }\
	virtual int GetNameHash() const override { return NameHash; }

// hard-linked component instantiator
#define	ADD_COMPONENT_GETTER(type)	Type* Get<Type>() const { return &m_inst##Type; }
#define	ADD_COMPONENT_INST(type)	Type m_inst##Type(this);

template<typename TComponentBase>
using COMPONENT_MAP = Map<int, TComponentBase*>;

namespace ComponentHostImpl
{
	template<typename TComponentBase>
	inline TComponentBase* GetComponent(const COMPONENT_MAP<TComponentBase>& components, int hash)
	{
		auto it = components.find(hash);
		if (it == components.end())
			return nullptr;
		return *it;
	}

	template<typename TComponentBase>
	inline void AddComponent(COMPONENT_MAP<TComponentBase>& components, int hash, TComponentBase* component)
	{
		components[hash] = component;
		component->OnAdded();
	}

	template<typename TComponentBase>
	inline TComponentBase* GetComponent(const COMPONENT_MAP<TComponentBase>& components, const char* name)
	{
		const int hash = StringToHash(name);
		return GetComponent(components, hash);
	}

	template<typename TComponentBase>
	inline void ForEachComponent(COMPONENT_MAP<TComponentBase>& components, const EqFunction<void(TComponentBase* pComponent)>& componentWalkFn)
	{
		ASSERT(componentWalkFn);
		for (auto it = components.begin(); !it.atEnd(); ++it)
			componentWalkFn(*it);
	}

	template<typename TComponentBase>
	inline void RemoveComponent(COMPONENT_MAP<TComponentBase>& components, int hash)
	{
		auto it = components.find(hash);
		if (it == components.end())
			return;
		(*it)->OnRemoved();
		delete* it;
		components.remove(it);
	}

	template<typename TComponentBase>
	inline void RemoveComponent(COMPONENT_MAP<TComponentBase>& components, const char* name)
	{
		const int hash = StringToHash(name);
		RemoveComponent(components, hash);
	}

	template<typename TComponentBase>
	inline void RemoveAll(COMPONENT_MAP<TComponentBase>& components)
	{
		for (auto it = components.begin(); !it.atEnd(); ++it)
		{
			(*it)->OnRemoved();
			delete* it;
		}
		components.clear();
	}
}

//-------------------------------------------------

// Put this in class declaration
#define DECLARE_COMPONENT_HOST(hostType, componentBase) \
	private: \
		COMPONENT_MAP<componentBase>	m_components{ PP_SL }; \
	public: \
		inline componentBase*			GetComponent(int hash) const { return ComponentHostImpl::GetComponent(m_components, hash); } \
		inline void						AddComponent(int hash, componentBase* component) { return ComponentHostImpl::AddComponent(m_components, hash, component); } \
		inline componentBase*			Get(const char* name) const { return ComponentHostImpl::GetComponent(m_components, name); } \
		template<class CType> CType*	Get() const { return static_cast<CType*>(ComponentHostImpl::GetComponent<componentBase>(m_components, CType::NameHash)); } \
		template<class CType> void		Remove() { ComponentHostImpl::RemoveComponent<componentBase>(m_components, CType::NameHash); } \
		inline void						Remove(const char* name) { return ComponentHostImpl::RemoveComponent(m_components, name); } \
		inline void						RemoveAllComponents() { ComponentHostImpl::RemoveAll(m_components); } \
		template<class CType> CType*	Add() { \
			ASSERT(GetComponent(CType::NameHash) == nullptr); \
			CType* component = PPNew CType(this); \
			ComponentHostImpl::AddComponent<componentBase>(m_components, CType::NameHash, component); \
			return component; \
		}\
		void ForEachComponent(const EqFunction<void(componentBase* pComponent)>& componentWalkFn) {\
			ComponentHostImpl::ForEachComponent(m_components, componentWalkFn);\
		}
