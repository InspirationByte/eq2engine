//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Component System
//////////////////////////////////////////////////////////////////////////////////

#pragma once

template <typename HOST>
class ComponentBase
{
public:
	ComponentBase(HOST* host) : m_host(host) {}
	virtual ~ComponentBase() = default;

protected:
	HOST*	m_host;
};

#define DECLARE_COMPONENT(name) \
	static constexpr const char* GetName() { return name; } \
	static constexpr int NameHash{ StringToHashConst(name) };

// hard-linked component instantiator
#define	ADD_COMPONENT_GETTER(type)	Type* Get<Type>() const { return &m_inst##Type; }
#define	ADD_COMPONENT_INST(type)	Type m_inst##Type(this);
