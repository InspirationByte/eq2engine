//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
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
	Threading::IncrementInterlocked(numRefs);
}

template< class TYPE>
inline bool	WeakRefBlock<TYPE>::Ref_Drop()
{
	if (Threading::DecrementInterlocked(numRefs) <= 0)
	{
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
public:
	using Block = WeakPtr::WeakRefBlock<TYPE>;
	virtual ~WeakRefObject()
	{
		if (m_block)
			m_block->ptr = nullptr;
	}

private:
	Block*			GetBlock() { return m_block ? m_block : (m_block = PPNew Block(this)); }
	mutable Block*	m_block{ nullptr };
};

//-----------------------------------------------------------------------------
// weak pointer for weak ref counted

template< class TYPE >
class CWeakPtr
{
public:
	using PTR_TYPE = TYPE*;

	CWeakPtr() = default;

	explicit CWeakPtr( PTR_TYPE pObject );
	CWeakPtr(std::nullptr_t);
	CWeakPtr(const CWeakPtr<TYPE>& other);
	CWeakPtr(CWeakPtr<TYPE>&& other);
	~CWeakPtr();

	void				Assign( PTR_TYPE obj);
	void				Release(bool deref = true);

	operator const		bool() const		{ return m_weakRefPtr && m_weakRefPtr->ptr; }
	operator			bool()				{ return m_weakRefPtr && m_weakRefPtr->ptr; }
	operator const		PTR_TYPE() const	{ return m_weakRefPtr ? static_cast<PTR_TYPE>(m_weakRefPtr->ptr) : nullptr; }
	operator			PTR_TYPE ()			{ return m_weakRefPtr ? static_cast<PTR_TYPE>(m_weakRefPtr->ptr) : nullptr; }
	PTR_TYPE			Ptr() const			{ return m_weakRefPtr ? static_cast<PTR_TYPE>(m_weakRefPtr->ptr) : nullptr; }
	TYPE&				Ref() const			{ return m_weakRefPtr ? *static_cast<PTR_TYPE>(m_weakRefPtr->ptr) : nullptr; }
	PTR_TYPE			operator->() const	{ return m_weakRefPtr ? static_cast<PTR_TYPE>(m_weakRefPtr->ptr) : nullptr; }

	void				operator=(std::nullptr_t);
	void				operator=(CWeakPtr<TYPE>&& other);
	void				operator=( const CWeakPtr<TYPE>& other);

	friend bool			operator==(const CWeakPtr<TYPE>& a, const CWeakPtr<TYPE>& b) { return a.Ptr() == b.Ptr(); }
	friend bool			operator==(const CWeakPtr<TYPE>& a, std::nullptr_t) { return a.Ptr() == nullptr; }
	friend bool			operator==(const CWeakPtr<TYPE>& a, PTR_TYPE b) { return a.Ptr() == b; }

private:
	using Block = WeakPtr::WeakRefBlock<TYPE>;
	Block*				m_weakRefPtr{ nullptr };
};

//---------------------------------------------------------

template< class TYPE >
inline CWeakPtr<TYPE>::CWeakPtr(std::nullptr_t)
	: m_ptrObj(nullptr)
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
	m_weakRefPtr = other.m_weakRefPtr;
	other.m_weakRefPtr = nullptr;
}

template< class TYPE >
inline CWeakPtr<TYPE>::~CWeakPtr()
{
	Block* block = m_weakRefPtr;
	if (block)
		block->Ref_Drop();
}

template< class TYPE >
inline void CWeakPtr<TYPE>::Assign(PTR_TYPE obj)
{
	if(m_weakRefPtr == obj->m_block)
		return;

	// del old ref
	Block* oldBlock = m_weakRefPtr;
	Block* block;
	m_weakRefPtr = block = obj->GetBlock();
	if(block)
		block->Ref_Grab();

	if(oldBlock)
		oldBlock->Ref_Drop();
}

template< class TYPE >
inline void CWeakPtr<TYPE>::Release(bool deref)
{
	Block* block = m_weakRefPtr;
	if (block && deref)
		block->Ref_Drop();
	m_weakRefPtr = nullptr;
}

template< class TYPE >
inline void CWeakPtr<TYPE>::operator=(std::nullptr_t)
{
	Release();
}

template< class TYPE >
inline void CWeakPtr<TYPE>::operator=(CWeakPtr<TYPE>&& other)
{
	Block* oldBlock = m_weakRefPtr;

	m_weakRefPtr = other.m_weakRefPtr;
	other.m_weakRefPtr = nullptr;

	if (oldBlock)
		oldBlock->Ref_Drop();
}

template< class TYPE >
inline void CWeakPtr<TYPE>::operator=(const CWeakPtr<TYPE>& other)
{
	Assign(other.m_weakRefPtr ? (TYPE*)other.m_weakRefPtr->ptr : nullptr);
}