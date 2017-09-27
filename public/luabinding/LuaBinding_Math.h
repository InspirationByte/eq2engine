//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium core bindings
//
//				
//
//////////////////////////////////////////////////////////////////////////////////

// TODO
#ifndef LUABINDING_MATH
#define LUABINDING_MATH

#include "LuaBinding.h"

#include "math/DkMath.h"

#ifndef __INTELLISENSE__

//
// Vector2D
//
OOLUA_PROXY( IVector2D )
	OOLUA_TAGS(
        Equal_op
        , Less_op
        , Less_equal_op
        , Add_op
        , Sub_op
        , Mul_op
        , Div_op
		, No_shared
    )
	OOLUA_CTORS(
		OOLUA_CTOR(int)
		OOLUA_CTOR(int, int)
		OOLUA_CTOR( IVector2D )
	)
	OOLUA_MGET_MSET(x)
	OOLUA_MGET_MSET(y)
OOLUA_PROXY_END

//
// Vector2D
//
OOLUA_PROXY( Vector2D )
	OOLUA_TAGS(
        Equal_op
        , Less_op
        , Less_equal_op
        , Add_op
        , Sub_op
        , Mul_op
        , Div_op
		, No_shared
    )
	OOLUA_CTORS(
		OOLUA_CTOR(float)
		OOLUA_CTOR(float, float)
		OOLUA_CTOR( Vector2D )
	)
	OOLUA_MGET_MSET(x)
	OOLUA_MGET_MSET(y)
OOLUA_PROXY_END

//
// Vector3D
//
OOLUA_PROXY( Vector3D )
	OOLUA_TAGS(
        Equal_op
        , Less_op
        , Less_equal_op
        , Add_op
        , Sub_op
        , Mul_op
        , Div_op
		, No_shared
    )
	OOLUA_CTORS(
		OOLUA_CTOR(float)
		OOLUA_CTOR(float, float, float)
		OOLUA_CTOR( Vector2D, float )
		OOLUA_CTOR( float, Vector2D )
		OOLUA_CTOR( Vector3D )
	)

	OOLUA_MFUNC_CONST(xy)
	OOLUA_MFUNC_CONST(yz)
	OOLUA_MFUNC_CONST(xz)

	OOLUA_MGET_MSET(x)
	OOLUA_MGET_MSET(y)
	OOLUA_MGET_MSET(z)
OOLUA_PROXY_END

//
// Vector4D
//
OOLUA_PROXY( Vector4D )
	OOLUA_TAGS(
        Equal_op
        , Less_op
        , Less_equal_op
        , Add_op
        , Sub_op
        , Mul_op
        , Div_op
		, No_shared
    )
	OOLUA_CTORS(
		OOLUA_CTOR(float)
		OOLUA_CTOR(float, float, float, float)
		OOLUA_CTOR( Vector2D, float, float )
		OOLUA_CTOR( float, Vector2D, float )
		OOLUA_CTOR( float, float, Vector2D )
		OOLUA_CTOR( Vector2D, Vector2D )
		OOLUA_CTOR( Vector3D, float )
		OOLUA_CTOR( float, Vector3D )
	)

	OOLUA_MFUNC_CONST(xy)
	OOLUA_MFUNC_CONST(xz)
	OOLUA_MFUNC_CONST(xw)
	OOLUA_MFUNC_CONST(yz)
	OOLUA_MFUNC_CONST(yw)
	OOLUA_MFUNC_CONST(zw)

	OOLUA_MFUNC_CONST(xyz)
	OOLUA_MFUNC_CONST(yzw)

	OOLUA_MGET_MSET(x)
	OOLUA_MGET_MSET(y)
	OOLUA_MGET_MSET(z)
	OOLUA_MGET_MSET(w)
OOLUA_PROXY_END

//
// Plane
//

OOLUA_PROXY( Plane )
	OOLUA_CTORS(
		OOLUA_CTOR(float, float, float, float)
		OOLUA_CTOR( Vector3D, float )
		OOLUA_CTOR( Vector3D, Vector3D, Vector3D )
	)

	OOLUA_MFUNC_CONST( Distance )
	OOLUA_MFUNC_CONST( GetIntersectionWithRay )
	OOLUA_MFUNC_CONST( GetIntersectionLineFraction )

	OOLUA_MGET_MSET( normal )
	OOLUA_MGET_MSET( offset )
OOLUA_PROXY_END

#endif // __INTELLISENSE__


// HACK: get TVec*D values pushed using OOLua and not getting fucking frustrated with SharedPtr thingy
namespace OOLUA
{
	namespace INTERNAL
	{
		template<typename NumberType>
		struct push_basic_type<TVec2D<NumberType>, 0, 0>
		{
			static bool push(lua_State* const vm, TVec2D<NumberType> const& value)
			{
				return OOLUA::push(vm, new TVec2D<NumberType>(value), Lua);
			}
		};

		template<typename NumberType>
		struct push_basic_type<TVec3D<NumberType>, 0, 0>
		{
			static bool push(lua_State* const vm, TVec3D<NumberType> const& value)
			{
				return OOLUA::push(vm, new TVec3D<NumberType>(value), Lua);
			}
		};

		template<typename NumberType>
		struct push_basic_type<TVec4D<NumberType>, 0, 0>
		{
			static bool push(lua_State* const vm, TVec4D<NumberType> const& value)
			{
				return OOLUA::push(vm, new TVec4D<NumberType>(value), Lua);
			}
		};
	}
};

#endif // LUABINDING_MATH