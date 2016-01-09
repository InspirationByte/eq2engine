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
class CSingletonAbstract
{
public:
					CSingletonAbstract()							{ pInstance = NULL; }
					CSingletonAbstract(CSingletonAbstract const&)	{}

	virtual			~CSingletonAbstract()							{}
	void operator	=(CSingletonAbstract const&)					{}

	//----------------
	bool			IsInitialized()									{ return (pInstance != NULL); }

	T*				GetInstancePtr()								{ Initialize(); return pInstance; }

	// only instance pointer supported
	T*				operator->()									{ Initialize(); return pInstance; }

	operator const	T&() const										{ Initialize(); return *pInstance; }
	operator		T&()											{ Initialize(); return *pInstance; }

protected:
	// initialization function. Can be overrided
	virtual void	Initialize() = 0;

	// deletion function. Can be overrided
	virtual void	Destroy() = 0;

	static T*		pInstance;
};

template <typename T> T* CSingletonAbstract<T>::pInstance = 0;

//--------------------------------------------------------------------------

// This implements non-abstract singleton
template <class T>
class CSingleton : public CSingletonAbstract<T>
{
public:
	// initialization function. Can be overrided
	virtual void	Initialize()					{ if(!CSingletonAbstract<T>::pInstance) CSingletonAbstract<T>::pInstance = new T; }

	// deletion function. Can be overrided
	virtual void	Destroy()						{ if(CSingletonAbstract<T>::pInstance) delete CSingletonAbstract<T>::pInstance; CSingletonAbstract<T>::pInstance = NULL; }
};

//#ifdef __GNUC__
//template <typename T> T* CSingleton<T>::pInstance = 0;
//#endif // __GNUC__

#endif // SINGLETON_H
