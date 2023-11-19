#pragma once

static const char s_boilerPlateStrGLSL[] = R"(
// functions
#define     frac        fract
#define     lerp        mix
#define     saturate(x) clamp(x, 0.0, 1.0)
#define     mul(a,b)    ((a)*(b))

float fmod(float x, float y) { return x - y * floor(x / y); }

#ifdef VERTEX
#	define clip(x)	(x)
#	define ddx(x)	(x)
#	define ddy(x)	(x)
#endif

#ifdef FRAGMENT
#	define clip(x)	if((x) < 0.0) discard
#	define ddx    	dFdx
#	define ddy    	dFdy
#endif

// See BaseShader and shaders layouts
#define BIND_CONSTANT( N )		layout(set = 0, binding = N)
#define BIND_RENDERPASS( N )	layout(set = 1, binding = N)
#define BIND_TRANSIENT( N )		layout(set = 2, binding = N)

)";