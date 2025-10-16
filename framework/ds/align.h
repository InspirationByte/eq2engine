//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: value structure alignment
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#define ALIGN( x, a )		(((x) + ((a)-1) ) & ~((a)-1))

#define ALIGN4( a ) a		((a) + 3) & ~ 3)
#define ALIGN16( a ) a		((a) + 15) & ~ 15)
#define ALIGN32( a ) a		((a) + 31) & ~ 31)
#define ALIGN64( a ) a		((a) + 63) & ~ 63)

#ifdef _MSC_VER

#define _ALIGNED(x)					__declspec(align(x))
#define ALIGNED_TYPE(s, a)			typedef s _ALIGNED(a)
#define ALIGNED_DECL(t, a, name)	_ALIGNED(a) t name

#else

#define _ALIGNED(x)			__attribute__ ((aligned(x)))
#define ALIGNED_TYPE(s, a)	typedef struct s _ALIGNED(a)
#define ALIGNED_DECL(t, a, name)	t name _ALIGNED(a)

#endif