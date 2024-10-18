//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Thread-safe Promise and Future concept implementation
//////////////////////////////////////////////////////////////////////////////////

// TODO: add cancellation token when needed

#pragma once

enum EFutureStatus : int
{
	FUTURE_NONE = 0,
	FUTURE_PENDING,
	FUTURE_SUCCESS,
	FUTURE_FAILURE
};

template<typename T>
class Future;

template<typename T>
class Promise;

template<typename T>
class FutureResult
{
public:
	FutureResult(const T& value) 
		: m_value(&value)
	{
	}

	FutureResult(int errorCode, const char* message) 
		: m_errorCode(errorCode), m_errorMessage(message)
	{
	}

	bool			IsError() const { return m_value == nullptr; }
	int				GetErrorCode() const { return m_errorCode; }
	const char*		GetErrorMessage() const { return m_errorMessage; }

	T				Ptr() const			{ return m_value; }
	T&				Ref() const			{ return *m_value; }

	const T*		operator->() const	{ return m_value; }
	const T&		operator*() const	{ return *m_value; }

private:
	const char* m_errorMessage{ nullptr };
	int			m_errorCode{ 0 };
	const T*	m_value{ nullptr };
};

namespace FutureImpl
{
template<typename T>
using RawVal = RawItem<T, alignof(T)>;

template<typename T>
class FutureData : public RefCountedObject<T>
{
	friend class ::Promise<T>;
	friend class ::Future<T>;
public:
	
	using ResultCb = EqFunction<void(const FutureResult<T>& result)>;

	FutureData() {}
	~FutureData()
	{
		if (m_status == FUTURE_SUCCESS)
			(*m_value).~T();
		else if (m_status == FUTURE_FAILURE)
			(*m_errorInfo).~ErrorInfo();
	}
	bool			HasResult() const { return m_status == FUTURE_SUCCESS || m_status == FUTURE_FAILURE; }

protected:
	struct ErrorInfo
	{
		EqString	message;
		int			code;
	};

	union 
	{
		RawVal<T>			m_value;
		RawVal<ErrorInfo>	m_errorInfo;
	};

	Array<ResultCb>			m_resultCb{ PP_SL };
	EFutureStatus			m_status{ FUTURE_NONE };
	Threading::CEqMutex		m_condMutex;
	Threading::CEqSignal	m_waitSignal;
};
}

//-------------------------------------------------------

template<typename T>
class Promise
{
public:
	using Data = FutureImpl::FutureData<T>;

					Promise();
					Promise(const Promise& other);
					Promise(Data* refDataPtr);

	Future<T>		CreateFuture() const;

	void			SetResult(T&& value) const;
	void			SetError(int code, const char* message) const;

	EFutureStatus	GetStatus() const;

	// for use in C callbacks
	Data*			GrabDataPtr() { m_data.Ptr()->Ref_Grab(); return m_data.Ptr(); }
protected:
	CRefPtr<Data>	m_data;
};

//------------------------

template<typename T>
class Future
{
	using Data = FutureImpl::FutureData<T>;
	using FutureCb = typename FutureImpl::FutureData<T>::ResultCb;
	friend class Promise<T>;
public:
						Future() = default;

	bool				IsValid() const { return m_data != nullptr; }

	bool				HasResult() const;
	const T&			GetResult() const;

	bool				HasErrors() const;
	int					GetErrorCode() const;
	const EqString&		GetErrorMessage() const;

	void				Wait(int timeout = Threading::WAIT_INFINITE);
	void				AddCallback(FutureCb callback);

	void				operator=(std::nullptr_t) { m_data = nullptr; }
	operator const		bool() const { return IsValid(); }

	static Future<T>	Succeed(T&& value);
	static Future<T>	Failure(int code, const char* message);

protected:
	Future(CRefPtr<Data> data);

	CRefPtr<Data>		m_data;
};

//-------------------------------------------------------

template<typename T>
inline Promise<T>::Promise()
{
	m_data = CRefPtr_new(Data);
}

template<typename T>
inline Promise<T>::Promise(const Promise& other)
	: m_data(other.m_data)
{

}

template<typename T>
inline Promise<T>::Promise(Data* ref)
	: m_data(ref)
{
	const bool deleted = ref->Ref_Drop();
	ASSERT_MSG(deleted == false, "Future data was not acquired with GrabDataPtr");
}

template<typename T>
inline Future<T> Promise<T>::CreateFuture() const
{
	ASSERT(m_data);
	{
		Threading::CScopedMutex m(m_data->m_condMutex);
		if(m_data->m_status == FUTURE_NONE)
			m_data->m_status = FUTURE_PENDING;
	}
	return Future<T>(m_data);
}

template<typename T>
inline void Promise<T>::SetResult(T&& value) const
{
	ASSERT_MSG(m_data->HasResult() == false, "Promise already has been resolved");

	Threading::CScopedMutex m(m_data->m_condMutex);
	m_data->m_status = FUTURE_SUCCESS;

	new(&(*m_data->m_value)) T(value);
	FutureResult<T> result(*m_data->m_value);
	for (typename Data::ResultCb cb : m_data->m_resultCb)
		cb(result);

	m_data->m_resultCb.clear(true);
	m_data->m_waitSignal.Raise();
}

template<typename T>
inline void Promise<T>::SetError(int code, const char* message) const
{
	ASSERT_MSG(m_data->HasResult() == false, "Promise already has been resolved");

	Threading::CScopedMutex m(m_data->m_condMutex);
	m_data->m_status = FUTURE_FAILURE;

	new(&(*m_data->m_errorInfo)) typename Data::ErrorInfo{ message, code };

	FutureResult<T> result((*m_data->m_errorInfo).code, (*m_data->m_errorInfo).message);
	for (typename Data::ResultCb cb : m_data->m_resultCb)
		cb(result);

	m_data->m_resultCb.clear(true);
	m_data->m_waitSignal.Raise();
}

template<typename T>
inline EFutureStatus Promise<T>::GetStatus() const
{
	if (!m_data)
		return FUTURE_NONE;

	Threading::CScopedMutex m(m_data->m_condMutex);
	return m_data->m_status;
}

//-------------------------------------------------------

template<typename T>
inline Future<T>::Future(CRefPtr<Data> data) 
	: m_data(data)
{
}

template<typename T>
inline bool Future<T>::HasResult() const
{
	if (!m_data)
		return false;

	Threading::CScopedMutex m(m_data->m_condMutex);
	return m_data->HasResult();
}

template<typename T>
inline const T& Future<T>::GetResult() const
{
	ASSERT(m_data);

	Threading::CScopedMutex m(m_data->m_condMutex);
	ASSERT(m_data->m_status == FUTURE_SUCCESS);
	return *m_data->m_value;
}

template<typename T>
inline bool Future<T>::HasErrors() const
{
	if (!m_data)
		return false;

	Threading::CScopedMutex m(m_data->m_condMutex);
	return m_data->m_status == FUTURE_FAILURE;
}

template<typename T>
inline int Future<T>::GetErrorCode() const
{
	ASSERT(m_data);

	Threading::CScopedMutex m(m_data->m_condMutex);
	return (*m_data->m_errorInfo).code;
}

template<typename T>
inline const EqString& Future<T>::GetErrorMessage() const
{
	ASSERT(m_data);

	Threading::CScopedMutex m(m_data->m_condMutex);
	return (*m_data->m_errorInfo).message;
}

template<typename T>
inline void Future<T>::Wait(int timeout)
{
	ASSERT(m_data);
	m_data->m_waitSignal.Wait(timeout);
}

template<typename T>
inline void Future<T>::AddCallback(FutureCb callback)
{
	ASSERT(m_data);

	Threading::CScopedMutex m(m_data->m_condMutex);

	switch (m_data->m_status)
	{
		case FUTURE_SUCCESS:
		{
			FutureResult<T> result(*m_data->m_value);
			callback(result);
			break;
		}
		case FUTURE_FAILURE:
		{
			typename Data::ErrorInfo& error = *m_data->m_errorInfo;
			FutureResult<T> result(error.code, error.message);
			callback(result);
			break;
		}
		case FUTURE_PENDING:
		{
			m_data->m_resultCb.append(callback);
			break;
		}
		default:
		{
			ASSERT_FAIL("Invalid Future status");
		}
	}
}

template<typename T>
inline Future<T> Future<T>::Succeed(T&& value)
{
	Promise<T> promise;
	Future<T> future = promise.CreateFuture();
	promise.SetResult(std::forward<T>(value));
	return future;
}

template<typename T>
inline Future<T> Future<T>::Failure(int code, const char* message)
{
	Promise<T> promise;
	Future<T> future = promise.CreateFuture();
	promise.SetError(code, message);
	return future;
}