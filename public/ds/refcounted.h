//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: RefCounted object with policies support
//////////////////////////////////////////////////////////////////////////////////

#pragma once

struct RefCountedDeletePolicy {
	enum { SHOULD_DELETE = 1 };
};

struct RefCountedKeepPolicy {
	enum { SHOULD_DELETE = 0 };
};

template< typename TYPE>
class CRefPtr;

template< typename TYPE, typename POLICY = RefCountedDeletePolicy>
class RefCountedObject
{
public:
	using REF_POLICY = POLICY;
	using PTR_T = CRefPtr<TYPE>;

	RefCountedObject() = default;
	virtual ~RefCountedObject() {}

	void	Ref_Grab();
	bool	Ref_Drop();

	int		Ref_Count() const { return m_numRefs; }

private:

	// deletes object when no references
	// example of usage: 
	// void Ref_DeleteObject()
	// {
	//		assignedRemover->Free(this);
	// }

	virtual void Ref_DeleteObject() {}	// could be useful with RefCountedKeepPolicy and virtual objects

private:
	mutable int	m_numRefs{ 0 };
};

template< class TYPE, class POLICY >
inline void	RefCountedObject<TYPE, POLICY>::Ref_Grab()
{
	Threading::IncrementInterlocked(m_numRefs);
}

template< class TYPE, class POLICY >
inline bool	RefCountedObject<TYPE, POLICY>::Ref_Drop()
{
	if (Threading::DecrementInterlocked(m_numRefs) <= 0)
	{
		Ref_DeleteObject();
		if (POLICY::SHOULD_DELETE) { delete this; }
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// smart pointer for ref counted

#define CRefPtr_new(TYPE, ...) CRefPtr<TYPE>(new(PP_SL) TYPE(__VA_ARGS__))

template< class TYPE >
class CRefPtr
{
public:
	using PTR_TYPE = TYPE*;

	CRefPtr() = default;

	explicit CRefPtr( PTR_TYPE pObject );
	CRefPtr(std::nullptr_t);
	CRefPtr(const CRefPtr<TYPE>& refptr);
	CRefPtr(CRefPtr<TYPE>&& refptr);
	~CRefPtr();

	void				Assign( PTR_TYPE obj);
	void				Release(bool deref = true);

	operator const		bool() const		{ return m_ptrObj != nullptr; }
	operator			bool()				{ return m_ptrObj != nullptr; }
	operator const		PTR_TYPE() const	{ return m_ptrObj; }
	operator			PTR_TYPE ()			{ return m_ptrObj; }
	PTR_TYPE			Ptr() const			{ return m_ptrObj; }
	TYPE&				Ref() const			{ return *m_ptrObj; }
	PTR_TYPE			operator->() const	{ return m_ptrObj; }

	void				operator=(std::nullptr_t);
	void				operator=(CRefPtr<TYPE>&& refptr);
	void				operator=( const CRefPtr<TYPE>& refptr );

	friend bool			operator==(const CRefPtr<TYPE>& a, const CRefPtr<TYPE>& b) { return a.Ptr() == b.Ptr(); }
	friend bool			operator==(const CRefPtr<TYPE>& a, std::nullptr_t) { return a.Ptr() == nullptr; }
	friend bool			operator==(const CRefPtr<TYPE>& a, PTR_TYPE b) { return a.Ptr() == b; }

private:
	PTR_TYPE			m_ptrObj{ nullptr };
};

//------------------------------------------------------------

template< class TYPE >
inline CRefPtr<TYPE>::CRefPtr(std::nullptr_t)
	: m_ptrObj(nullptr)
{
}

template< class TYPE >
inline CRefPtr<TYPE>::CRefPtr( PTR_TYPE pObject )
{
	if (pObject)
		pObject->Ref_Grab();
	m_ptrObj = pObject;
}

template< class TYPE >
inline CRefPtr<TYPE>::CRefPtr( const CRefPtr<TYPE>& refptr )
{
	if (refptr)
		refptr->Ref_Grab();
	m_ptrObj = refptr;
}

template< class TYPE >
inline CRefPtr<TYPE>::CRefPtr(CRefPtr<TYPE>&& refptr)
{
	m_ptrObj = refptr.m_ptrObj;
	refptr.m_ptrObj = nullptr;
}

template< class TYPE >
inline CRefPtr<TYPE>::~CRefPtr()
{
	using REF_TYPE = RefCountedObject<TYPE, typename TYPE::REF_POLICY>;

	REF_TYPE* oldObj = (REF_TYPE*)m_ptrObj;
	if (oldObj != nullptr)
		oldObj->Ref_Drop();
}

template< class TYPE >
inline void CRefPtr<TYPE>::Assign(PTR_TYPE obj)
{
	using REF_TYPE = RefCountedObject<TYPE, typename TYPE::REF_POLICY>;

	if(m_ptrObj == obj)
		return;

	// del old ref
	REF_TYPE* oldObj = (REF_TYPE*)m_ptrObj;
	m_ptrObj = obj;
	if(obj)
		((REF_TYPE*)obj)->Ref_Grab();

	if(oldObj != nullptr)
		oldObj->Ref_Drop();
}

template< class TYPE >
inline void CRefPtr<TYPE>::Release(bool deref)
{
	using REF_TYPE = RefCountedObject<TYPE, typename TYPE::REF_POLICY>;

	// del old ref
	REF_TYPE* oldObj = (REF_TYPE*)m_ptrObj;
	m_ptrObj = nullptr;

	if(oldObj != nullptr && deref)
		oldObj->Ref_Drop();
}

template< class TYPE >
inline void CRefPtr<TYPE>::operator=(std::nullptr_t)
{
	Release();
}

template< class TYPE >
inline void CRefPtr<TYPE>::operator=(CRefPtr<TYPE>&& refptr)
{
	using REF_TYPE = RefCountedObject<TYPE, typename TYPE::REF_POLICY>;
	REF_TYPE* oldObj = (REF_TYPE*)m_ptrObj;

	m_ptrObj = refptr.m_ptrObj;
	refptr.m_ptrObj = nullptr;

	if (oldObj)
		oldObj->Ref_Drop();
}

template< class TYPE >
inline void CRefPtr<TYPE>::operator=( const CRefPtr<TYPE>& refptr )
{
	Assign( refptr.m_ptrObj );
}