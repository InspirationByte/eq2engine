#pragma once

static const char s_boilerPlateStrHLSL[] = R"(
#ifndef _HLSL_BOILERPLATE_INCLUDE
#define _HLSL_BOILERPLATE_INCLUDE

//// functions
//#define     frac        fract
//#define     lerp        mix
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
#define BIND_CONSTANT( N )		register(N, BINDGROUP_CONSTANT)
#define BIND_RENDERPASS( N )	register(N, BINDGROUP_RENDERPASS)
#define BIND_TRANSIENT( N )		register(N, BINDGROUP_TRANSIENT)
#define BIND_INSTANCES( N )		register(N, BINDGROUP_INSTANCES)

#define VERTEX_ID( name ) (VID_ ## name)

#endif // _HLSL_BOILERPLATE_INCLUDE
)";