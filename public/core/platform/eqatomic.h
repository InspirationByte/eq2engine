//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Atomic functions
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "core/common_types.h"

class Atomic
{
public:
	static int32	Increment(volatile int32& var);
	static uint32	Increment(volatile uint32& var);
	static int64	Increment(volatile int64& var);
	static uint64	Increment(volatile uint64& var);

	static int32	Decrement(volatile int32& var);
	static uint32	Decrement(volatile uint32& var);
	static int64	Decrement(volatile int64& var);
	static uint64	Decrement(volatile uint64& var);

	static int32	Add(int32 volatile& var, int32 val);
	static uint32	Add(uint32 volatile& var, uint32 val);
	static int64	Add(int64 volatile& var, int64 val);
	static uint64	Add(uint64 volatile& var, uint64 val);

	static int32	CompareExchange(int32 volatile& var, int32 oldVal, int32 newVal);
	static uint32	CompareExchange(uint32 volatile& var, uint32 oldVal, uint32 newVal);
	static int64	CompareExchange(int64 volatile& var, int64 oldVal, int64 newVal);
	static uint64	CompareExchange(uint64 volatile& var, uint64 oldVal, uint64 newVal);
	template <typename T>
	static T*		CompareExchange(T* volatile& ptr, T* oldVal, T* newVal);

	static int32	Exchange(int32 volatile& var, int32 val);
	static uint32	Exchange(uint32 volatile& var, uint32 val);
	static int64	Exchange(int64 volatile& var, int64 val);
	static uint64	Exchange(uint64 volatile& var, uint64 val);
	template <typename T>
	static T*		Exchange(T* volatile& ptr, T* val);

	static int32	Load(const int32 volatile& var);
	static uint32	Load(const uint32 volatile& var);
	static int64	Load(const int64 volatile& var);
	static uint64	Load(const uint64 volatile& var);
	template <typename T> 
	static T*		Load(T* const volatile& ptr);

	static void		Store(int32 volatile& var, int32 value);
	static void		Store(uint32 volatile& var, uint32 value);
	static void		Store(int64 volatile& var, int64 value);
	static void		Store(uint64 volatile& var, uint64 value);
	template <typename T>
	static void		Store(T* volatile& ptr, T* value);
};

#include "eqatomic.inl"