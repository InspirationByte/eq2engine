//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium OpenGL ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "imaging/textureformats.h"
#include "renderers/ShaderAPI_defs.h"

#ifdef USE_GLES2
#include <glad_es3.h>

#ifdef PLAT_WIN
#	include <glad_egl.h>
#else
#	include <EGL/egl.h>
#endif

#else
#	include <glad.h>
#endif

extern const GLuint g_gl_internalFormats[FORMAT_COUNT];
extern const GLuint g_gl_chanCountTypes[];
extern const GLuint g_gl_chanTypePerFormat[];
extern const GLenum g_gl_texAddrModes[];
extern const GLenum g_gl_blendingConsts[];
extern const GLenum g_gl_blendingModes[];
extern const GLenum g_gl_depthConst[];
extern const GLenum g_gl_stencilConst[];
extern const GLenum g_gl_cullConst[];

#ifndef USE_GLES2
extern const GLenum g_gl_fillConst[];
#endif // USE_GLES2

extern const GLenum g_gl_texMinFilters[];
extern const GLenum g_gl_primitiveType[];
extern const GLenum g_gl_bufferUsages[];
extern const GLenum g_gl_texTargetType[];	// map to EImageType

#ifdef USE_GLES2

#define glDrawElementsInstancedARB					glDrawElementsInstanced
#define glDrawArraysInstancedARB					glDrawArraysInstanced
#define glVertexAttribDivisorARB					glVertexAttribDivisor

#define GL_OBJECT_COMPILE_STATUS_ARB				GL_COMPILE_STATUS
#define GL_OBJECT_COMPILE_STATUS_ARB				GL_COMPILE_STATUS
#define GL_OBJECT_LINK_STATUS_ARB					GL_LINK_STATUS

#define GL_OBJECT_ACTIVE_UNIFORMS_ARB				GL_ACTIVE_UNIFORMS
#define GL_OBJECT_ACTIVE_UNIFORM_MAX_LENGTH_ARB		GL_ACTIVE_UNIFORM_MAX_LENGTH

#endif // USE_GLES2

#define MAX_GL_GENERIC_ATTRIB		16

static inline GLuint PickGLInternalFormat(ETextureFormat format)
{
	GLint internalFormat = g_gl_internalFormats[format];

	if (format >= FORMAT_I32F && format <= FORMAT_RGBA32F)
		internalFormat = g_gl_internalFormats[format - (FORMAT_I32F - FORMAT_I16F)];

	return internalFormat;
}