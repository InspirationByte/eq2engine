//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Component System
//////////////////////////////////////////////////////////////////////////////////

#ifndef COMPONENTS_H
#define COMPONENTS_H

#include "core/dktypes.h"
#include "utils/CRC32.h"

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
	static constexpr int NameHash{ CRC32_StringConst(name) };

// hard-linked component instantiator
#define	ADD_COMPONENT_GETTER(type)	Type* Get<Type>() const { return &m_inst##Type; }
#define	ADD_COMPONENT_INST(type)	Type m_inst##Type(this);

#endif // COMPONENTS_H
