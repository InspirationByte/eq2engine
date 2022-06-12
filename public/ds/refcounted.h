//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: EqEngine mutex storage
//////////////////////////////////////////////////////////////////////////////////

#ifndef REFCOUNTED_H
#define REFCOUNTED_H

template< class TYPE >
class CRefPtr;

template< class TYPE >
class RefCountedObject
{
public:
	using PTR_T = CRefPtr<TYPE>;

	RefCountedObject() = default;
	virtual ~RefCountedObject() {}

	void	Ref_Grab();
	bool	Ref_Drop();

	int		Ref_Count() const { return m_numRefs; }

private:
	void	Ref_CheckRemove()
	{
		// ASSERT(m_numRefs < 0);

		if (m_numRefs <= 0)
			Ref_DeleteObject();
	}

	// deletes object when no references
	// example of usage: 
	// void Ref_DeleteObject()
	// {
	//		assignedRemover->Free(this);
	// }

	virtual void Ref_DeleteObject() {}

private:
	mutable int	m_numRefs{ 0 };
};

template< class TYPE >
inline void	RefCountedObject<TYPE>::Ref_Grab()
{
	Threading::IncrementInterlocked(m_numRefs);
}

template< class TYPE >
inline bool	RefCountedObject<TYPE>::Ref_Drop()
{
	int refCount = Threading::DecrementInterlocked(m_numRefs);
	if (refCount <= 0)
	{
		Ref_DeleteObject();
		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
// smart pointer for ref counted

template< class TYPE >
class CRefPtr
{
public:
	using PTR_TYPE = TYPE*;
	using REF_TYPE = RefCountedObject<TYPE>;

	CRefPtr() = default;
	CRefPtr( PTR_TYPE pObject );
	CRefPtr( const CRefPtr<TYPE>& refptr );
	virtual ~CRefPtr();

	// frees object (before scope, if you econom-guy)
	void				Assign( PTR_TYPE obj);
	void				Release(bool deref = true);

	operator const		PTR_TYPE() const	{ return m_ptrObj; }
	operator			PTR_TYPE ()			{ return m_ptrObj; }
	PTR_TYPE			p() const			{ return m_ptrObj; }
	PTR_TYPE			operator->() const	{ return m_ptrObj; }

	void				operator=( const PTR_TYPE obj );
	void				operator=( const CRefPtr<TYPE>& refptr );

protected:
	PTR_TYPE			m_ptrObj{ nullptr };
};

//------------------------------------------------------------

template< class TYPE >
inline CRefPtr<TYPE>::CRefPtr( PTR_TYPE pObject ) : m_ptrObj(pObject)
{
	if (pObject)
		pObject->Ref_Grab();
}

template< class TYPE >
inline CRefPtr<TYPE>::CRefPtr( const CRefPtr<TYPE>& refptr ) : m_ptrObj(refptr.p())
{
	if (m_ptrObj)
		m_ptrObj->Ref_Grab();
}

template< class TYPE >
inline CRefPtr<TYPE>::~CRefPtr()
{
	Release();
}

template< class TYPE >
inline void CRefPtr<TYPE>::Assign( PTR_TYPE obj)
{
	if(m_ptrObj == obj)
		return;

	// del old ref
	REF_TYPE* oldObj = (REF_TYPE*)m_ptrObj;
	
	if(m_ptrObj = obj)
		((REF_TYPE*)obj)->Ref_Grab();

	if(oldObj != nullptr)
		oldObj->Ref_Drop();
}

template< class TYPE >
inline void CRefPtr<TYPE>::Release(bool deref)
{
	// del old ref
	REF_TYPE* oldObj = (REF_TYPE*)m_ptrObj;
	m_ptrObj = nullptr;
	if(oldObj != nullptr && deref)
		oldObj->Ref_Drop();
}

template< class TYPE >
inline void CRefPtr<TYPE>::operator=( const PTR_TYPE obj )
{
	Assign( obj );
}

template< class TYPE >
inline void CRefPtr<TYPE>::operator=( const CRefPtr<TYPE>& refptr )
{
	Assign( refptr.m_ptrObj );
}


#endif // REFCOUNTED_H