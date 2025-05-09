//////////////////////////////////////////////////////////////////////////////////
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
struct WeakRefHandle
{
	WeakRefHandle(WeakRefObject<TYPE>* obj) : ptr(obj) {}

	WeakRefObject<TYPE>*	ptr{ nullptr };
	mutable int				numRefs{ 0 };

	void	Ref_Grab();
	bool	Ref_Drop();
	int		Ref_Count() const { return numRefs; }
};

template< class TYPE>
inline void	WeakRefHandle<TYPE>::Ref_Grab()
{
	Atomic::Increment(numRefs);
}

template< class TYPE>
inline bool	WeakRefHandle<TYPE>::Ref_Drop()
{
	if (Atomic::Decrement(numRefs) == 0)
	{
		if(ptr)
			ptr->m_handle = nullptr;

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
	friend struct WeakPtr::WeakRefHandle<TYPE>;
public:

	using WeakHandle = WeakPtr::WeakRefHandle<TYPE>;
	virtual ~WeakRefObject()
	{
		if (m_handle)
			m_handle->ptr = nullptr;
	}

	WeakHandle*	GetWeakHandle() const
	{
		if (!Atomic::Load(m_handle))
			Atomic::Exchange(m_handle, PPNew WeakHandle(const_cast<WeakRefObject<TYPE>*>(this)));
		return (WeakHandle*)Atomic::Load(m_handle);
	}

private:
	mutable WeakHandle*	m_handle{ nullptr };
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
	operator const	PTR_TYPE() const	{ return GetHandle() ? static_cast<PTR_TYPE>(GetHandle()->ptr) : nullptr; }
	operator		PTR_TYPE ()			{ return GetHandle() ? static_cast<PTR_TYPE>(GetHandle()->ptr) : nullptr; }
	PTR_TYPE		Ptr() const			{ return GetHandle() ? static_cast<PTR_TYPE>(GetHandle()->ptr) : nullptr; }
	TYPE&			Ref() const			{ return *Ptr(); }
	PTR_TYPE		operator->() const	{ return GetHandle() ? static_cast<PTR_TYPE>(GetHandle()->ptr) : nullptr; }

	void			operator=(std::nullptr_t);
	void			operator=(CWeakPtr<TYPE>&& other);
	void			operator=( const CWeakPtr<TYPE>& other);

	friend bool		operator==(const CWeakPtr<TYPE>& a, const CWeakPtr<TYPE>& b) { return a.m_weakRefPtr == b.m_weakRefPtr; }
	friend bool		operator==(const CWeakPtr<TYPE>& a, std::nullptr_t) { return a.Ptr() == nullptr; }
	friend bool		operator==(const CWeakPtr<TYPE>& a, PTR_TYPE b) { return a.Ptr() == b; }

private:
	auto*		GetHandle() const { return (typename TYPE::WeakHandle*)(m_weakRefPtr); }

	using Handle = WeakPtr::WeakRefHandle<TYPE>;
	mutable Handle*			m_weakRefPtr{ nullptr };
};

//---------------------------------------------------------

template< class TYPE >
inline CWeakPtr<TYPE>::CWeakPtr(std::nullptr_t)
{
}

template< class TYPE >
inline CWeakPtr<TYPE>::CWeakPtr( PTR_TYPE pObject )
{
	Handle* handle = nullptr;
	if (pObject)
		m_weakRefPtr = handle = (Handle*)pObject->GetWeakHandle();
	
	if(handle)
		handle->Ref_Grab();
}

template< class TYPE >
inline CWeakPtr<TYPE>::CWeakPtr( const CWeakPtr<TYPE>& other )
{
	Handle* handle = other.m_weakRefPtr;
	m_weakRefPtr = handle;

	if (handle)
		handle->Ref_Grab();
}

template< class TYPE >
inline CWeakPtr<TYPE>::CWeakPtr(CWeakPtr<TYPE>&& other)
{
	Atomic::Exchange(m_weakRefPtr, Atomic::Exchange(other.m_weakRefPtr, (Handle*)nullptr));
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
	Handle* handle = (Handle*)obj->GetWeakHandle();
	Handle* oldHandle = (Handle*)Atomic::Exchange(m_weakRefPtr, (Handle*)handle);

	if(handle)
		handle->Ref_Grab();

	if(oldHandle)
		oldHandle->Ref_Drop();
}

template< class TYPE >
inline void CWeakPtr<TYPE>::Release()
{
	Handle* handle = (Handle*)Atomic::Exchange(m_weakRefPtr, (Handle*)nullptr);
	if (handle != nullptr)
		handle->Ref_Drop();
}

template< class TYPE >
inline void CWeakPtr<TYPE>::operator=(std::nullptr_t)
{
	Release();
}

template< class TYPE >
inline void CWeakPtr<TYPE>::operator=(CWeakPtr<TYPE>&& other)
{
	Handle* oldHandle = Atomic::Exchange(m_weakRefPtr, Atomic::Exchange(other.m_weakRefPtr, (Handle*)nullptr));

	if (oldHandle)
		oldHandle->Ref_Drop();
}

template< class TYPE >
inline void CWeakPtr<TYPE>::operator=(const CWeakPtr<TYPE>& other)
{
	if (other.m_weakRefPtr)
		Assign((TYPE*)other.m_weakRefPtr->ptr);
	else
		Release();
}