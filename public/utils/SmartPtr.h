//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Smart scoped pointer is a great way to control deallocation of memory 
//				whithin the scope (epilog)
//////////////////////////////////////////////////////////////////////////////////

#ifndef SCOPEDPTR_H
#define SCOPEDPTR_H

#include <malloc.h>
#include "core/ppmem.h"

enum PtrAllocType_e
{
	SPTR_NEW = 0,
	SPTR_NEW_ARRAY,
	SPTR_MALLOC,
	SPTR_PPALLOC,
};

template< class TYPE >
class CScopedPointer
{
public:
	// don't have an default constructor

	CScopedPointer( TYPE* pObject, PtrAllocType_e declType);
	~CScopedPointer<TYPE>();

	// frees object (before scope, if you econom-guy)
	void				Free();
	void				Disable() { m_pTypedObject = NULL; }

	TYPE*				p() const { return m_pTypedObject; }
	operator TYPE		*() const { return m_pTypedObject; }

protected:
	TYPE*				m_pTypedObject;
	PtrAllocType_e		m_nAllocType;
};

template< class TYPE >
CScopedPointer<TYPE>::CScopedPointer( TYPE* pObject, PtrAllocType_e declType)
{
	m_pTypedObject	= pObject;
	m_nAllocType	= declType;
}

template< class TYPE >
CScopedPointer<TYPE>::~CScopedPointer()
{
	Free();
}

template< class TYPE >
void CScopedPointer<TYPE>::Free()
{
	if(!m_pTypedObject)
		return;

	// select alloc type
	if(m_nAllocType == SPTR_NEW)
	{
		delete m_pTypedObject;
	}
	else if(m_nAllocType == SPTR_NEW_ARRAY)
	{
		delete [] m_pTypedObject;
	}
	else if(m_nAllocType == SPTR_MALLOC)
	{
		free( m_pTypedObject );
	}
	else if(m_nAllocType == SPTR_PPALLOC)
	{
		PPFree( m_pTypedObject );
	}

	m_pTypedObject = NULL;
}

// parody on RAiI
#define S_MALLOC(name, size)		CScopedPointer<char> name((char*)malloc(size), SPTR_MALLOC)
#define S_PPALLOC(name, size)		CScopedPointer<char> name((char*)PPAlloc(size), SPTR_PPALLOC)
#define S_NEW(name, type)			CScopedPointer<type> name(new type, SPTR_NEW)
#define S_NEWA(name, type, count)	CScopedPointer<type> name(new type[count], SPTR_NEW_ARRAY)

#endif // SCOPEDPTR_H