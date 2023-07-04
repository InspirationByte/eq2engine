//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: Singleton
//////////////////////////////////////////////////////////////////////////////////

#pragma once

template <class T>
class CAutoPtr
{
public:
	T*				GetInstancePtr();
	T&				GetInstance();
	T*				operator->()	{ return GetInstancePtr(); }
	operator		T*()			{ return GetInstancePtr(); }
	operator const	T&() const		{ return GetInstance(); }
	operator		T&()			{ return GetInstance(); }
};

template <class T>
inline T* CAutoPtr<T>::GetInstancePtr()
{
	static T instance; 
	return &instance;
}

template <class T>
inline T& CAutoPtr<T>::GetInstance()
{
	return *GetInstancePtr();
}

template <class T>
class CSingletonAbstract
{
public:
	virtual T*		GetInstancePtr(){ Initialize(); return Instance; }

	// only instance pointer supported
	T*				operator->()	{ return GetInstancePtr(); }
	operator const	T&() const		{ return GetInstancePtr(); }
	operator		T&()			{ return GetInstancePtr(); }

protected:
	// initialization function. Can be overriden
	virtual void	Initialize() = 0;

	// deletion function. Can be overriden
	virtual void	Destroy() = 0;

	static T*		Instance;
};

template <typename T> T* CSingletonAbstract<T>::Instance = nullptr;