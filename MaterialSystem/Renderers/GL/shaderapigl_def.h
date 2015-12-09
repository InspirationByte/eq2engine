//**************Copyright (C) Two Dark Interactive Software 2009**************
//
// Description: Constant types for DarkTech renderer
//
//****************************************************************************

#ifndef GLRENDERER_CONSTANTS_H
#define GLRENDERER_CONSTANTS_H

static const int internalFormats[] = {
	0,
	// Unsigned formats
	GL_INTENSITY8,
	GL_LUMINANCE8_ALPHA8,
	GL_RGB8,
	GL_RGBA8,

	GL_INTENSITY16,
	GL_LUMINANCE16_ALPHA16,
	GL_RGB16,
	GL_RGBA16,

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
	GL_INTENSITY_FLOAT16_ATI,
	GL_LUMINANCE_ALPHA_FLOAT16_ATI,
	GL_RGB_FLOAT16_ATI,
	GL_RGBA_FLOAT16_ATI,

	GL_INTENSITY_FLOAT32_ATI,
	GL_LUMINANCE_ALPHA_FLOAT32_ATI,
	GL_RGB_FLOAT32_ATI,
	GL_RGBA_FLOAT32_ATI,

	// Signed integer formats
	0, // GL_INTENSITY16I_EXT,
	0, // GL_LUMINANCE_ALPHA16I_EXT,
	0, // GL_RGB16I_EXT,
	0, // GL_RGBA16I_EXT,

	0, // GL_INTENSITY32I_EXT,
	0, // GL_LUMINANCE_ALPHA32I_EXT,
	0, // GL_RGB32I_EXT,
	0, // GL_RGBA32I_EXT,

	// Unsigned integer formats
	0, // GL_INTENSITY16UI_EXT,
	0, // GL_LUMINANCE_ALPHA16UI_EXT,
	0, // GL_RGB16UI_EXT,
	0, // GL_RGBA16UI_EXT,

	0, // GL_INTENSITY32UI_EXT,
	0, // GL_LUMINANCE_ALPHA32UI_EXT,
	0, // GL_RGB32UI_EXT,
	0, // GL_RGBA32UI_EXT,

	// Packed formats
	0, // RGBE8 not directly supported
	0, // GL_RGB9_E5,
	0, // GL_R11F_G11F_B10F,
	GL_RGB5,
	GL_RGBA4,
	GL_RGB10_A2,

	// Depth formats
	GL_DEPTH_COMPONENT16,
	GL_DEPTH_COMPONENT24,
	GL_DEPTH24_STENCIL8_EXT,
	0, // GL_DEPTH_COMPONENT32F,

	// Compressed formats
	GL_COMPRESSED_RGB_S3TC_DXT1_EXT,
	GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,
	GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,
	0, // ATI1N not yet supported
	GL_COMPRESSED_LUMINANCE_ALPHA_3DC_ATI,
};

static const int srcFormats[] = { 0, GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_RGB, GL_RGBA };

static const int srcTypes[] = {
	0,

	// Unsigned formats
	GL_UNSIGNED_BYTE,
	GL_UNSIGNED_BYTE,
	GL_UNSIGNED_BYTE,
	GL_UNSIGNED_BYTE,

	GL_UNSIGNED_SHORT,
	GL_UNSIGNED_SHORT,
	GL_UNSIGNED_SHORT,
	GL_UNSIGNED_SHORT,

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
	GL_HALF_FLOAT,
	GL_HALF_FLOAT,
	GL_HALF_FLOAT,
	GL_HALF_FLOAT,

	GL_FLOAT,
	GL_FLOAT,
	GL_FLOAT,
	GL_FLOAT,

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
	GL_UNSIGNED_SHORT_5_6_5,
	GL_UNSIGNED_SHORT_4_4_4_4_REV,
	GL_UNSIGNED_INT_2_10_10_10_REV,

	// Depth formats
	GL_UNSIGNED_SHORT,
	GL_UNSIGNED_INT,
	GL_UNSIGNED_INT_24_8_EXT,
	0, // D32F not supported

	// Compressed formats
	GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,
	GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,
	GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,
	0,//(D3DFORMAT) '1ITA', // 3Dc 1 channel
	0,//(D3DFORMAT) '2ITA', // 3Dc 2 channels
};

static const int blendingConsts[] = {
	GL_ZERO,
	GL_ONE,
	GL_SRC_COLOR,
	GL_ONE_MINUS_SRC_COLOR,
	GL_DST_COLOR,
	GL_ONE_MINUS_DST_COLOR,
	GL_SRC_ALPHA,
	GL_ONE_MINUS_SRC_ALPHA,
	GL_DST_ALPHA,
	GL_ONE_MINUS_DST_ALPHA,
	GL_SRC_ALPHA_SATURATE,
};

static const int blendingModes[] = {
	GL_FUNC_ADD_EXT,
	GL_FUNC_SUBTRACT_EXT,
	GL_FUNC_REVERSE_SUBTRACT_EXT,
	GL_MIN_EXT,
	GL_MAX_EXT,
};

static const int depthConst[] = {
	GL_NEVER,
	GL_LESS,
	GL_EQUAL,
	GL_LEQUAL,
	GL_GREATER,
	GL_NOTEQUAL,
	GL_GEQUAL,
	GL_ALWAYS,
};

static const int stencilConst[] = {
	GL_KEEP,
	GL_ZERO,
	GL_REPLACE,
	GL_INVERT,
	GL_INCR_WRAP,
	GL_DECR_WRAP,
	GL_MAX_EXT,
	GL_INCR,
	GL_DECR,
	GL_ALWAYS,
};

static const int cullConst[] = {
	GL_NONE,
	GL_BACK,
	GL_FRONT,
};

static const int fillConst[] = {
	GL_FILL,
	GL_LINE,
	GL_POINT,
};

static const int minFilters[] = {
	GL_NEAREST, 
	GL_LINEAR, 
	GL_LINEAR_MIPMAP_NEAREST,
	GL_LINEAR_MIPMAP_LINEAR,
	GL_LINEAR_MIPMAP_NEAREST,
	GL_LINEAR_MIPMAP_LINEAR,
};

static const int matrixModeConst[] = {
	GL_MODELVIEW,
	GL_PROJECTION,
	GL_MODELVIEW,
	GL_TEXTURE,
};

// for some speedup
static const int mbTypeConst[] = {
	GL_POINTS,
	GL_LINES,
	GL_TRIANGLES,
	GL_TRIANGLE_STRIP,
	GL_LINE_STRIP,
	GL_LINE_LOOP,
	GL_POLYGON,
	GL_QUADS,
};

static const int glPrimitiveType[] = {
	GL_TRIANGLES,
	GL_TRIANGLE_FAN,
	GL_TRIANGLE_STRIP,
	GL_QUADS,
	GL_LINES,
	GL_LINE_STRIP,
	GL_LINE_LOOP,
	GL_POINTS,
};

static const int glBufferUsages[] = {
	GL_STREAM_DRAW,
	GL_STATIC_DRAW,
	GL_DYNAMIC_DRAW,
};

#endif //GLRENDERER_CONSTANTS_H