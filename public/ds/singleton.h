//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: Singleton
//////////////////////////////////////////////////////////////////////////////////

#pragma once

template <class T>
class CStaticAutoPtr
{
public:
	~CStaticAutoPtr();
	T*				GetInstancePtr();
	T&				GetInstance();
	T*				operator->()	{ return GetInstancePtr(); }
	operator		T*()			{ return GetInstancePtr(); }
	operator const	T&() const		{ return GetInstance(); }
	operator		T&()			{ return GetInstance(); }

private:
	struct Instantiator
	{
		Instantiator(T* _this, bool& initialized) 
		{
			if(!initialized) new (_this) T();
			initialized = true;
		}
	};

	static ubyte	m_data[sizeof(T)];
	static bool		m_initialized;
};

template <class T>
inline CStaticAutoPtr<T>::~CStaticAutoPtr()
{
	if(m_initialized) ((T*)&m_data)->~T();
	m_initialized = false;
}

template <class T>
inline T* CStaticAutoPtr<T>::GetInstancePtr()
{
	static Instantiator inst((T*)&m_data, m_initialized);
	return (T*)&m_data;
}

template <class T>
inline T& CStaticAutoPtr<T>::GetInstance()
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

template <typename T> ubyte CStaticAutoPtr<T>::m_data[sizeof(T)] = {};
template <typename T> bool CStaticAutoPtr<T>::m_initialized = false;
template <typename T> T* CSingletonAbstract<T>::Instance = nullptr;