//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2016
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech OpenGL ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#ifndef GLRENDERER_CONSTANTS_H
#define GLRENDERER_CONSTANTS_H

static const int internalFormats[] = {
	0,
	// Unsigned formats
	gl::INTENSITY8,
	gl::LUMINANCE8_ALPHA8,
	gl::RGB8,
	gl::RGBA8,

	gl::INTENSITY16,
	gl::LUMINANCE16_ALPHA16,
	gl::RGB16,
	gl::RGBA16,

	// Signed formats
	0,
	0,
	0,
	0,

	0,
	0,
	0,
	0,

	// Float formats
	gl::INTENSITY16F_ARB,
	gl::LUMINANCE_ALPHA16F_ARB,
	gl::RGB16F_ARB,
	gl::RGBA16F_ARB,

	gl::INTENSITY32F_ARB,
	gl::LUMINANCE_ALPHA32F_ARB,
	gl::RGB32F_ARB,
	gl::RGBA32F_ARB,

	// Signed integer formats
	0, // gl::INTENSITY16I_EXT,
	0, // gl::LUMINANCE_ALPHA16I_EXT,
	0, // gl::RGB16I_EXT,
	0, // gl::RGBA16I_EXT,

	0, // gl::INTENSITY32I_EXT,
	0, // gl::LUMINANCE_ALPHA32I_EXT,
	0, // gl::RGB32I_EXT,
	0, // gl::RGBA32I_EXT,

	// Unsigned integer formats
	0, // gl::INTENSITY16UI_EXT,
	0, // gl::LUMINANCE_ALPHA16UI_EXT,
	0, // gl::RGB16UI_EXT,
	0, // gl::RGBA16UI_EXT,

	0, // gl::INTENSITY32UI_EXT,
	0, // gl::LUMINANCE_ALPHA32UI_EXT,
	0, // gl::RGB32UI_EXT,
	0, // gl::RGBA32UI_EXT,

	// Packed formats
	0, // RGBE8 not directly supported
	0, // gl::RGB9_E5,
	0, // gl::R11F_G11F_B10F,
	gl::RGB5,
	gl::RGBA4,
	gl::RGB10_A2,

	// Depth formats
	gl::DEPTH_COMPONENT16,
	gl::DEPTH_COMPONENT24,
	gl::DEPTH24_STENCIL8_EXT,
	0, // gl::DEPTH_COMPONENT32F,

	// Compressed formats
	gl::COMPRESSED_RGB_S3TC_DXT1_EXT,
	gl::COMPRESSED_RGBA_S3TC_DXT3_EXT,
	gl::COMPRESSED_RGBA_S3TC_DXT5_EXT,
	0, // ATI1N not yet supported
	gl::COMPRESSED_LUMINANCE_ALPHA,			//COMPRESSED_LUMINANCE_ALPHA_3DC_ATI,
};

static const int srcFormats[] = { 0, gl::LUMINANCE, gl::LUMINANCE_ALPHA, gl::RGB, gl::RGBA };

static const int srcTypes[] = {
	0,

	// Unsigned formats
	gl::UNSIGNED_BYTE,
	gl::UNSIGNED_BYTE,
	gl::UNSIGNED_BYTE,
	gl::UNSIGNED_BYTE,

	gl::UNSIGNED_SHORT,
	gl::UNSIGNED_SHORT,
	gl::UNSIGNED_SHORT,
	gl::UNSIGNED_SHORT,

	// Signed formats
	0,
	0,
	0,
	0,

	0,
	0,
	0,
	0,

	// Float formats
	gl::HALF_FLOAT_ARB,
	gl::HALF_FLOAT_ARB,
	gl::HALF_FLOAT_ARB,
	gl::HALF_FLOAT_ARB,

	gl::FLOAT,
	gl::FLOAT,
	gl::FLOAT,
	gl::FLOAT,

	// Signed integer formats
	0,
	0,
	0,
	0,

	0,
	0,
	0,
	0,

	// Unsigned integer formats
	0,
	0,
	0,
	0,

	0,
	0,
	0,
	0,

	// Packed formats
	0, // RGBE8 not directly supported
	0, // RGBE9E5 not supported
	0, // RG11B10F not supported
	gl::UNSIGNED_SHORT_5_6_5,
	gl::UNSIGNED_SHORT_4_4_4_4_REV,
	gl::UNSIGNED_INT_2_10_10_10_REV,

	// Depth formats
	gl::UNSIGNED_SHORT,
	gl::UNSIGNED_INT,
	gl::UNSIGNED_INT_24_8_EXT,
	0, // D32F not supported

	// Compressed formats
	gl::COMPRESSED_RGBA_S3TC_DXT1_EXT,
	gl::COMPRESSED_RGBA_S3TC_DXT3_EXT,
	gl::COMPRESSED_RGBA_S3TC_DXT5_EXT,
	0,//(D3DFORMAT) '1ITA', // 3Dc 1 channel
	0,//(D3DFORMAT) '2ITA', // 3Dc 2 channels
};

static const int blendingConsts[] = {
	gl::ZERO,
	gl::ONE,
	gl::SRC_COLOR,
	gl::ONE_MINUS_SRC_COLOR,
	gl::DST_COLOR,
	gl::ONE_MINUS_DST_COLOR,
	gl::SRC_ALPHA,
	gl::ONE_MINUS_SRC_ALPHA,
	gl::DST_ALPHA,
	gl::ONE_MINUS_DST_ALPHA,
	gl::SRC_ALPHA_SATURATE,
};

static const int blendingModes[] = {
	gl::FUNC_ADD,
	gl::FUNC_SUBTRACT,
	gl::FUNC_REVERSE_SUBTRACT,
	gl::MIN,
	gl::MAX,
};

static const int depthConst[] = {
	gl::NEVER,
	gl::LESS,
	gl::EQUAL,
	gl::LEQUAL,
	gl::GREATER,
	gl::NOTEQUAL,
	gl::GEQUAL,
	gl::ALWAYS,
};

static const int stencilConst[] = {
	gl::KEEP,
	gl::ZERO,
	gl::REPLACE,
	gl::INVERT,
	gl::INCR_WRAP,
	gl::DECR_WRAP,
	gl::MAX,
	gl::INCR,
	gl::DECR,
	gl::ALWAYS,
};

static const int cullConst[] = {
	gl::NONE,
	gl::BACK,
	gl::FRONT,
};

static const int fillConst[] = {
	gl::FILL,
	gl::LINE,
	gl::POINT,
};

static const int minFilters[] = {
	gl::NEAREST, 
	gl::LINEAR, 
	gl::LINEAR_MIPMAP_NEAREST,
	gl::LINEAR_MIPMAP_LINEAR,
	gl::LINEAR_MIPMAP_NEAREST,
	gl::LINEAR_MIPMAP_LINEAR,
};

static const int matrixModeConst[] = {
	gl::MODELVIEW,
	gl::PROJECTION,
	gl::MODELVIEW,
	gl::TEXTURE,
};

// for some speedup
static const int mbTypeConst[] = {
	gl::POINTS,
	gl::LINES,
	gl::TRIANGLES,
	gl::TRIANGLE_STRIP,
	gl::LINE_STRIP,
	gl::LINE_LOOP,
	gl::POLYGON,
	gl::QUADS,
};

static const int glPrimitiveType[] = {
	gl::TRIANGLES,
	gl::TRIANGLE_FAN,
	gl::TRIANGLE_STRIP,
	gl::QUADS,
	gl::LINES,
	gl::LINE_STRIP,
	gl::LINE_LOOP,
	gl::POINTS,
};

static const int glBufferUsages[] = {
	gl::STREAM_DRAW,
	gl::STATIC_DRAW,
	gl::DYNAMIC_DRAW,
};

#endif //GLRENDERER_CONSTANTS_H