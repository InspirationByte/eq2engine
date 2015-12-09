//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2014
//////////////////////////////////////////////////////////////////////////////////
// Description: Reference counted object.
//////////////////////////////////////////////////////////////////////////////////

#ifndef REFCOUNTED_H
#define REFCOUNTED_H

/*
	usage:

	1. Inherit it in your class
	------------------------------------------------------------------------------

	class CBlaBlaObject : public RefCountedObject

	2. Make it's removal function
	------------------------------------------------------------------------------

	void CBlaBlaObject::Ref_DeleteObject()
	{
		assignedFactory->Free( this );
	}

	3. Creating object. First you need to grab it
	------------------------------------------------------------------------------
	CBlaBlaObject* pObject = assignedFactory->CreateNew();

	pObject->Ref_Grab();

	4. Removal. Object will be removed automatically as Ref_DeleteObject provides
	------------------------------------------------------------------------------
	pObject->Ref_Drop();

	Don't do Ref_Drop inside assignedFactory->Free();
*/


class RefCountedObject
{
public:
			RefCountedObject()	: m_numRefs(0) {}
	virtual ~RefCountedObject() {}

	void	Ref_Grab()			{m_numRefs++;}
	void	Ref_Drop()			{m_numRefs--; Ref_CheckRemove();}

	int		Ref_Count() const	{return m_numRefs;}

private:
	void	Ref_CheckRemove()
	{
		// ASSERT(m_numRefs < 0);

		if(m_numRefs <= 0)
			Ref_DeleteObject();
	}

	// deletes object when no references
	// example of usage: 
	// void Ref_DeleteObject()
	// {
	//		assignedRemover->Free(this);
	// }

	virtual void Ref_DeleteObject() = 0;

protected:
	int		m_numRefs;
};

#endif // REFCOUNTED_H