#pragma once

static const char s_boilerPlateStrGLSL[] = R"(
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
#define BIND_CONSTANT( N )		layout(set = BINDGROUP_CONSTANT, binding = N)
#define BIND_RENDERPASS( N )	layout(set = BINDGROUP_RENDERPASS, binding = N)
#define BIND_TRANSIENT( N )		layout(set = BINDGROUP_TRANSIENT, binding = N)
#define BIND_INSTANCES( N )		layout(set = BINDGROUP_INSTANCES, binding = N)

#define VERTEX_ID( name ) (VID_ ## name)

)";