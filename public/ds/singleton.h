//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: Singleton
//////////////////////////////////////////////////////////////////////////////////

#ifndef SINGLETON_H
#define SINGLETON_H

// This implements RAII non-lazy singleton for abstract classes
template <class T>
class CSingleton
{
public:
	T*				GetInstancePtr();
	T*				operator->()						{ return GetInstancePtr(); }
	operator const	T&() const							{ return GetInstancePtr(); }
	operator		T&()								{ return GetInstancePtr(); }
};

template <class T>
inline T* CSingleton<T>::GetInstancePtr() 
{
	static T instance; 
	return &instance;
}

// This implements RAII non-lazy singleton for abstract classes
template <class T>
class CSingletonAbstract
{
public:
	virtual T*		GetInstancePtr()								{ Initialize(); return Instance; }

	// only instance pointer supported
	T*				operator->()									{ return GetInstancePtr(); }
	operator const	T&() const										{ return GetInstancePtr(); }
	operator		T&()											{ return GetInstancePtr(); }

protected:
	// initialization function. Can be overriden
	virtual void	Initialize() = 0;

	// deletion function. Can be overriden
	virtual void	Destroy() = 0;

	static T*		Instance;
};

template <typename T> T* CSingletonAbstract<T>::Instance = 0;

#endif // SINGLETON_H
