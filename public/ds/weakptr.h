﻿//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: RefCounted object with policies support
//////////////////////////////////////////////////////////////////////////////////

#pragma once

template< class TYPE >
class WeakRefObject;

namespace WeakPtr {

template< class TYPE >
struct WeakRefBlock
{
	WeakRefBlock(WeakRefObject<TYPE>* obj) : ptr(obj) {}

	WeakRefObject<TYPE>*	ptr{ nullptr };
	mutable int				numRefs{ 0 };

	void	Ref_Grab();
	bool	Ref_Drop();
	int		Ref_Count() const { return numRefs; }
};

template< class TYPE>
inline void	WeakRefBlock<TYPE>::Ref_Grab()
{
	Atomic::Increment(numRefs);
}

template< class TYPE>
inline bool	WeakRefBlock<TYPE>::Ref_Drop()
{
	if (Atomic::Decrement(numRefs) == 0)
	{
		if(ptr)
			ptr->m_block = nullptr;

		delete this;
		return true;
	}

	return false;
}

}

//-----------------------------------------------------------------------------
//

template< class TYPE >
class CWeakPtr;

template< class TYPE >
class WeakRefObject
{
	friend class CWeakPtr<TYPE>;
	friend struct WeakPtr::WeakRefBlock<TYPE>;
public:
	using Block = WeakPtr::WeakRefBlock<TYPE>;
	virtual ~WeakRefObject()
	{
		if (m_block)
			m_block->ptr = nullptr;
	}

private:
	Block*	GetBlock()
	{
		if (!Atomic::Load(m_block))
			Atomic::Exchange(m_block, PPNew Block(this));
		return (Block*)Atomic::Load(m_block);
	}
	mutable Block*	m_block{ nullptr };
};

//-----------------------------------------------------------------------------
// weak pointer for weak ref counted

template< class _TYPE >
class CWeakPtr
{
public:
	using TYPE = _TYPE;
	using PTR_TYPE = TYPE*;

	CWeakPtr() = default;

	explicit CWeakPtr( PTR_TYPE pObject );
	CWeakPtr(std::nullptr_t);
	CWeakPtr(const CWeakPtr<TYPE>& other);
	CWeakPtr(CWeakPtr<TYPE>&& other);
	~CWeakPtr();

	void			Assign( PTR_TYPE obj);
	void			Release();

	bool			IsSet() const		{ return m_weakRefPtr; }
	operator const	bool() const		{ return Ptr(); }
	operator		bool()				{ return Ptr(); }
	operator const	PTR_TYPE() const	{ return m_weakRefPtr ? static_cast<PTR_TYPE>(m_weakRefPtr->ptr) : nullptr; }
	operator		PTR_TYPE ()			{ return m_weakRefPtr ? static_cast<PTR_TYPE>(m_weakRefPtr->ptr) : nullptr; }
	PTR_TYPE		Ptr() const			{ return m_weakRefPtr ? static_cast<PTR_TYPE>(m_weakRefPtr->ptr) : nullptr; }
	TYPE&			Ref() const			{ return m_weakRefPtr ? *static_cast<PTR_TYPE>(m_weakRefPtr->ptr) : nullptr; }
	PTR_TYPE		operator->() const	{ return m_weakRefPtr ? static_cast<PTR_TYPE>(m_weakRefPtr->ptr) : nullptr; }

	void			operator=(std::nullptr_t);
	void			operator=(CWeakPtr<TYPE>&& other);
	void			operator=( const CWeakPtr<TYPE>& other);

	friend bool		operator==(const CWeakPtr<TYPE>& a, const CWeakPtr<TYPE>& b) { return a.m_weakRefPtr == b.m_weakRefPtr; }
	friend bool		operator==(const CWeakPtr<TYPE>& a, std::nullptr_t) { return a.Ptr() == nullptr; }
	friend bool		operator==(const CWeakPtr<TYPE>& a, PTR_TYPE b) { return a.Ptr() == b; }

private:
	using Block = WeakPtr::WeakRefBlock<TYPE>;
	Block*			m_weakRefPtr{ nullptr };
};

//---------------------------------------------------------

template< class TYPE >
inline CWeakPtr<TYPE>::CWeakPtr(std::nullptr_t)
{
}

template< class TYPE >
inline CWeakPtr<TYPE>::CWeakPtr( PTR_TYPE pObject )
{
	Block* block = nullptr;
	if (pObject)
		m_weakRefPtr = block = pObject->GetBlock();
	
	if(block)
		block->Ref_Grab();
}

template< class TYPE >
inline CWeakPtr<TYPE>::CWeakPtr( const CWeakPtr<TYPE>& other )
{
	Block* block = other.m_weakRefPtr;
	m_weakRefPtr = block;

	if (block)
		block->Ref_Grab();
}

template< class TYPE >
inline CWeakPtr<TYPE>::CWeakPtr(CWeakPtr<TYPE>&& other)
{
	Atomic::Exchange(m_weakRefPtr, Atomic::Exchange(other.m_weakRefPtr, (Block*)nullptr));
}

template< class TYPE >
inline CWeakPtr<TYPE>::~CWeakPtr()
{
	Release();
}

template< class TYPE >
inline void CWeakPtr<TYPE>::Assign(PTR_TYPE obj)
{
	if (!obj) {
		Release();
		return;
	}
	Block* block = obj->GetBlock();
	Block* oldBlock = (Block*)Atomic::Exchange(m_weakRefPtr, (Block*)block);

	if(block)
		block->Ref_Grab();

	if(oldBlock)
		oldBlock->Ref_Drop();
}

template< class TYPE >
inline void CWeakPtr<TYPE>::Release()
{
	Block* block = (Block*)Atomic::Exchange(m_weakRefPtr, (Block*)nullptr);
	if (block != nullptr)
		block->Ref_Drop();
}

template< class TYPE >
inline void CWeakPtr<TYPE>::operator=(std::nullptr_t)
{
	Release();
}

template< class TYPE >
inline void CWeakPtr<TYPE>::operator=(CWeakPtr<TYPE>&& other)
{
	Block* oldBlock = Atomic::Exchange(m_weakRefPtr, Atomic::Exchange(other.m_weakRefPtr, (Block*)nullptr));

	if (oldBlock)
		oldBlock->Ref_Drop();
}

template< class TYPE >
inline void CWeakPtr<TYPE>::operator=(const CWeakPtr<TYPE>& other)
{
	if (other.m_weakRefPtr)
		Assign((TYPE*)other.m_weakRefPtr->ptr);
	else
		Release();
}