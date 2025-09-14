#pragma once

static const char s_boilerPlateStrHLSL[] = R"(
#ifndef _HLSL_BOILERPLATE_INCLUDE
#define _HLSL_BOILERPLATE_INCLUDE

#define vec2 float2
#define vec3 float3
#define vec4 float4

#define ivec2 int2
#define ivec3 int3
#define ivec4 int4

#define ivec2 int2
#define ivec3 int3
#define ivec4 int4

#define bvec2 bool2
#define bvec3 bool3
#define bvec4 bool4

#define mat2 row_major float2x2
#define mat3 row_major float3x3
#define mat4 row_major float4x4

vec4 unpackUnorm4x8(uint p)
{
	return vec4(
		float(p & 255) / 255.0, 
		float(p >> 8 & 255) / 255.0, 
		float(p >> 16 & 255) / 255.0,
		float(p >> 24 & 255) / 255.0
	);
}

//// functions
#define     fract        frac
#define     mix			lerp        
//#define     saturate(x) clamp(x, 0.0, 1.0)

//float fmod(float x, float y) { return x - y * floor(x / y); }

#ifdef VERTEX
//#	define clip(x)	(x)
//#	define ddx(x)	(x)
//#	define ddy(x)	(x)
#endif

#ifdef FRAGMENT
//#	define clip(x)				if((x) < 0.0) discard
//#	define ddx    				dFdx
//#	define ddy    				dFdy
//#	define gl_VertexIndex		0
//#	define gl_InstanceIndex		0
#endif

// WebGPU Dawn hacks
//#define atomicLoad( x )			atomicAdd(x, 0)
//#define atomicStore( x, value )	atomicExchange(x, value)

#ifndef BINDGROUP_CONSTANT
#define BINDGROUP_CONSTANT		space0
#endif

#ifndef BINDGROUP_RENDERPASS
#define BINDGROUP_RENDERPASS	space1
#endif

#ifndef BINDGROUP_TRANSIENT
#define BINDGROUP_TRANSIENT		space2
#endif

#ifndef BINDGROUP_INSTANCES
#define BINDGROUP_INSTANCES		space3
#endif

// See BaseShader and shaders layouts
#define BIND_E( S, N, E )			: register(N, S)
#define BIND_CONSTANT_E( N, E )		BIND_E( BINDGROUP_CONSTANT, N, E)
#define BIND_RENDERPASS_E( N, E )	BIND_E( BINDGROUP_RENDERPASS, N, E)
#define BIND_TRANSIENT_E( N, E )	BIND_E( BINDGROUP_TRANSIENT, N, E)
#define BIND_INSTANCES_E( N, E )	BIND_E( BINDGROUP_INSTANCES, N, E)

#define BIND( S, N )				: register(N, S)
#define BIND_CONSTANT( N )			BIND( BINDGROUP_CONSTANT, N )
#define BIND_RENDERPASS( N )		BIND( BINDGROUP_RENDERPASS, N )
#define BIND_TRANSIENT( N )			BIND( BINDGROUP_TRANSIENT, N )
#define BIND_INSTANCES( N )			BIND( BINDGROUP_INSTANCES, N )

#define VERTEX_ID( name ) (VID_ ## name)

#endif // _HLSL_BOILERPLATE_INCLUDE
)";