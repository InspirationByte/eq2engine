#pragma once

template<int align>
struct RawAlignHelper;

#define MAKE_RAWITEM_ALIGN_T(N, T) \
	template<> struct RawAlignHelper<N> { \
		struct Type { \
			T value; \
		}; \
	};

#define MAKE_RAWITEM_ALIGN(N) \
	template<> struct RawAlignHelper<N> { \
		struct Type { \
			ALIGNED_DECL(int, N, value); \
		}; \
	};

MAKE_RAWITEM_ALIGN_T(1, char)
MAKE_RAWITEM_ALIGN_T(2, short)
MAKE_RAWITEM_ALIGN_T(4, int32)
MAKE_RAWITEM_ALIGN_T(8, int64)
MAKE_RAWITEM_ALIGN(16)
MAKE_RAWITEM_ALIGN(32)
MAKE_RAWITEM_ALIGN(64)

template<typename T, int ALIGN >
struct RawItem
{
	using DummyT = typename RawAlignHelper<ALIGN>::Type;
	union {
		ubyte	data[sizeof(T)];
		DummyT	_dummy;
	};
	T* operator->() { return reinterpret_cast<T*>(&data); }
	T& operator*() { return *reinterpret_cast<T*>(&data); }
	const T* operator->() const { return reinterpret_cast<const T*>(&data); }
	const T& operator*() const { return *reinterpret_cast<const T*>(&data); }
};