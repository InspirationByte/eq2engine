#pragma once

// Fluent interface builder pattern

template<typename T>
struct Builder;

#define FLUENT_BEGIN_TYPE(Type) \
	template<> struct Builder<Type> : private Type { \
		using ThisType = Builder<Type>; \
		using RetType = Type; \
		Builder<Type>() : ref(*this) {} \
		Builder<Type>(Type& ref) : ref(ref) {}

#define FLUENT_END_TYPE \
		const RetType& End() const { return *this; } \
		RetType&& End() { return std::move(*this); } \
		private: RetType& ref; \
	};

#define FLUENT_SET(VarName, SetterName, Value) \
	ThisType& SetterName() { ref.VarName = Value; return *this; }

#define FLUENT_SET_VALUE(VarName, SetterName) \
	ThisType& SetterName(decltype(RetType::VarName)&& x) { ref.VarName = std::move(x); return *this; } \
	ThisType& SetterName(const decltype(RetType::VarName)& x) { ref.VarName = x; return *this; }