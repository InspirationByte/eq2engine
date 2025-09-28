#pragma once

static const char s_boilerPlateStrGLSL[] = R"(
#ifndef _GLSL_BOILERPLATE_INCLUDE
#define _GLSL_BOILERPLATE_INCLUDE

// functions
#define     frac        fract
#define     lerp        mix
#define     saturate(x) clamp(x, 0.0, 1.0)

float fmod(float x, float y) { return x - y * floor(x / y); }

#ifdef VERTEX
#	define clip(x)	(x)
#	define ddx(x)	(x)
#	define ddy(x)	(x)
#endif

#ifdef FRAGMENT
#	define clip(x)				if((x) < 0.0) discard
#	define ddx    				dFdx
#	define ddy    				dFdy
#	define gl_VertexIndex		0
#	define gl_InstanceIndex		0
#endif

// WebGPU Dawn hacks
#define atomicLoad( x )			atomicAdd(x, 0)
#define atomicStore( x, value )	atomicExchange(x, value)

#ifndef BINDGROUP_CONSTANT
#define BINDGROUP_CONSTANT		0
#endif

#ifndef BINDGROUP_RENDERPASS
#define BINDGROUP_RENDERPASS	1
#endif

#ifndef BINDGROUP_TRANSIENT
#define BINDGROUP_TRANSIENT		2
#endif

#ifndef BINDGROUP_INSTANCES
#define BINDGROUP_INSTANCES		3
#endif

// See BaseShader and shaders layouts
#define BIND_E( S, N, E )			layout(set = S, binding = N, E) /*__sc_bind__(S,N)*/
#define BIND_CONSTANT_E( N, E )		BIND_E( BINDGROUP_CONSTANT, N, E)
#define BIND_RENDERPASS_E( N, E )	BIND_E( BINDGROUP_RENDERPASS, N, E)
#define BIND_TRANSIENT_E( N, E )	BIND_E( BINDGROUP_TRANSIENT, N, E)
#define BIND_INSTANCES_E( N, E )	BIND_E( BINDGROUP_INSTANCES, N, E)

#define BIND( S, N )				layout(set = S, binding = N ) /*__sc_bind__(S,N)*/
#define BIND_CONSTANT( N )			BIND( BINDGROUP_CONSTANT, N )
#define BIND_RENDERPASS( N )		BIND( BINDGROUP_RENDERPASS, N )
#define BIND_TRANSIENT( N )			BIND( BINDGROUP_TRANSIENT, N )
#define BIND_INSTANCES( N )			BIND( BINDGROUP_INSTANCES, N )

#define VERTEX_ID( name )			(VID_ ## name)

#endif // _GLSL_BOILERPLATE_INCLUDE
)";