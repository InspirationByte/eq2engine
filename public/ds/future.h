//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Promise and future concept implementation
//////////////////////////////////////////////////////////////////////////////////

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
	FutureResult(const T& value) : m_value(&value)
	{
	}

	FutureResult(int errorCode, const char* message) : m_errorCode(errorCode), m_errorMessage(message)
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
struct RawVal
{
	T& getData()
	{
		return *(T*)(&data);
	}

	const T& getData() const
	{
		return *(T*)(&data);
	}

	mutable std::aligned_storage_t<sizeof(T), alignof(size_t)> data{ 0 };
};

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
			m_value.getData().~T();
		else if (m_status == FUTURE_FAILURE)
			m_errorInfo.getData().~ErrorInfo();
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
	using Data = FutureImpl::FutureData<T>;
public:
					Promise();
	Future<T>		CreateFuture();

	void			SetResult(T&& value);
	void			SetError(int code, const char* message);

	EFutureStatus	GetStatus() const;

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

	bool				IsValid() const { return m_data != nullptr; }

	bool				HasResult() const;
	const T&			GetResult() const;

	bool				HasErrors() const;
	int					GetErrorCode() const;
	const EqString&		GetErrorMessage() const;

	void				Wait(int timeout = Threading::CEqSignal::WAIT_INFINITE);
	void				AddCallback(FutureCb callback);

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
inline Future<T> Promise<T>::CreateFuture()
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
inline void Promise<T>::SetResult(T&& value)
{
	ASSERT_MSG(m_data->HasResult() == false, "Promise already has been resolved");

	Threading::CScopedMutex m(m_data->m_condMutex);
	m_data->m_status = FUTURE_SUCCESS;

	new(&m_data->m_value.getData()) T(value);

	FutureResult<T> result(m_data->m_value.getData());
	for (int i = 0; i < m_data->m_resultCb.numElem(); ++i)
		m_data->m_resultCb[i](result);

	m_data->m_waitSignal.Raise();
}

template<typename T>
inline void Promise<T>::SetError(int code, const char* message)
{
	ASSERT_MSG(m_data->HasResult() == false, "Promise already has been resolved");

	Threading::CScopedMutex m(m_data->m_condMutex);
	m_data->m_status = FUTURE_FAILURE;

	new(&m_data->m_errorInfo.getData()) Data::ErrorInfo{ message, code };

	FutureResult<T> result(m_data->m_errorInfo.getData().code, m_data->m_errorInfo.getData().message);
	for (int i = 0; i < m_data->m_resultCb.numElem(); ++i)
		m_data->m_resultCb[i](result);

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
	return m_data->m_value.getData();
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
	return m_data->m_errorCode;
}

template<typename T>
inline const EqString& Future<T>::GetErrorMessage() const
{
	ASSERT(m_data);

	Threading::CScopedMutex m(m_data->m_condMutex);
	return m_data->m_errorMessage;
}

template<typename T>
inline void Future<T>::Wait(int timeout)
{
	ASSERT(m_data)
	m_data->m_waitSignal.Wait(timeout);
}

template<typename T>
inline void Future<T>::AddCallback(FutureCb callback)
{
	ASSERT(m_data);

	Threading::CScopedMutex m(m_data->m_condMutex);
	if (m_data->HasResult())
	{
		if (m_data->m_status == FUTURE_SUCCESS)
		{
			FutureResult<T> result(m_data->m_value.getData());
			callback(result);
		}
		else
		{
			Data::ErrorInfo& error = m_data->m_errorInfo.getData();
			FutureResult<T> result(error.code, error.message);
			callback(result);
		}
		return;
	}
	m_data->m_resultCb.append(callback);
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