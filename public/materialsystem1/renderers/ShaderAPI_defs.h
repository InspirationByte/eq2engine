//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Constant types for Equilibrium renderer
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "imaging/textureformats.h"

//---------------------------------------
//        HIGH LEVEL CONSTANTS
//---------------------------------------

// Reserved, used for shaders only
enum ER_ConstantType : int
{
    CONSTANT_FLOAT,
	CONSTANT_VECTOR2D,
    CONSTANT_VECTOR3D,
    CONSTANT_VECTOR4D,
    CONSTANT_INT,
    CONSTANT_IVECTOR2D,
    CONSTANT_IVECTOR3D,
    CONSTANT_IVECTOR4D,
    CONSTANT_BOOL,
    CONSTANT_BVECTOR2D,
    CONSTANT_BVECTOR3D,
    CONSTANT_BVECTOR4D,
    CONSTANT_MATRIX2x2,
    CONSTANT_MATRIX3x3,
    CONSTANT_MATRIX4x4,

    CONSTANT_TYPE_COUNT
};

static int s_constantTypeSizes[CONSTANT_TYPE_COUNT] = {
	sizeof(float),
	sizeof(Vector2D),
	sizeof(Vector3D),
	sizeof(Vector4D),
	sizeof(int),
	sizeof(int) * 2,
	sizeof(int) * 3,
	sizeof(int) * 4,
	sizeof(int),
	sizeof(int) * 2,
	sizeof(int) * 3,
	sizeof(int) * 4,
	sizeof(Matrix2x2),
	sizeof(Matrix3x3),
	sizeof(Matrix4x4),
};

// Texture filtering type for SetTextureFilteringMode()
enum ER_TextureFilterMode : int
{
	TEXFILTER_NEAREST	= 0,
	TEXFILTER_LINEAR,
	TEXFILTER_BILINEAR,
	TEXFILTER_TRILINEAR,
	TEXFILTER_BILINEAR_ANISO,
	TEXFILTER_TRILINEAR_ANISO,
};

// for SetMatrixMode()
enum ER_MatrixMode : int
{
	MATRIXMODE_VIEW	= 0,			// view tranformation matrix
	MATRIXMODE_PROJECTION,			// projection mode matrix

	MATRIXMODE_WORLD,				// world transformation matrix
	MATRIXMODE_WORLD2,				// world transformation - used as offset for MATRIXMODE_WORLD

	MATRIXMODE_TEXTURE,
};

// for SetTextureClamp()
enum ER_TextureAddressMode : int
{
	TEXADDRESS_WRAP	= 0,
	TEXADDRESS_CLAMP,
	TEXADDRESS_MIRROR
};

// for mesh builder and type of drawing the world model
enum ER_PrimitiveType : int
{
	PRIM_TRIANGLES      = 0,
	PRIM_TRIANGLE_FAN,
	PRIM_TRIANGLE_STRIP,
	PRIM_QUADS,
	PRIM_LINES,
	PRIM_LINE_STRIP,
	PRIM_LINE_LOOP,
	PRIM_POINTS,
};

typedef int (*PRIMCOUNTER)(int numPrimitives);

static int PrimCount_TriangleList( int numPrimitives )
{
	return numPrimitives / 3;
}

static int PrimCount_QuadList( int numPrimitives )
{
	return numPrimitives / 4;
}

static int PrimCount_TriangleFanStrip( int numPrimitives )
{
	return numPrimitives - 2;
}

static int PrimCount_ListList( int numPrimitives )
{
	return numPrimitives / 2;
}

static int PrimCount_ListStrip( int numPrimitives )
{
	return numPrimitives - 1;
}

static int PrimCount_Points( int numPrimitives )
{
	return numPrimitives;
}

static int PrimCount_None( int )
{
	return 0;
}

// Vertex type
enum ER_VertexAttribType : int
{
	VERTEXATTRIB_UNUSED		= 0,	// or unused

	VERTEXATTRIB_COLOR,
	VERTEXATTRIB_POSITION,
	VERTEXATTRIB_TEXCOORD,
	VERTEXATTRIB_NORMAL,
	VERTEXATTRIB_TANGENT,
	VERTEXATTRIB_BINORMAL,
};

// Attribute format
enum ER_AttributeFormat : int
{
	ATTRIBUTEFORMAT_FLOAT = 0,
	ATTRIBUTEFORMAT_HALF,
	ATTRIBUTEFORMAT_UBYTE,
};

static int s_attributeSize[] =
{
	sizeof(float),
	sizeof(half),
	sizeof(ubyte)
};

// Buffer access type (for VBO)
enum ER_BufferAccess : int
{
	BUFFER_STREAM		= 0,
	BUFFER_STATIC,
	BUFFER_DYNAMIC,
};

typedef struct VertexFormatDesc_s
{
	int					streamId{ 0 };
	int					elemCount{ 0 };

	ER_VertexAttribType	attribType{ VERTEXATTRIB_UNUSED };
	ER_AttributeFormat	attribFormat{ ATTRIBUTEFORMAT_FLOAT };

	const char*			name{ nullptr };

}VertexFormatDesc_t;

// comparison functions
enum ER_CompareFunc : int
{
	COMPFUNC_NEVER = 0,

	COMPFUNC_LESS,			// 1
	COMPFUNC_EQUAL,			// 2
	COMPFUNC_LEQUAL,		// 3
	COMPFUNC_GREATER,		// 4
	COMPFUNC_NOTEQUAL,		// 5
	COMPFUNC_GEQUAL,		// 6
	COMPFUNC_ALWAYS,		// 7
};

typedef struct SamplerStateParam_s
{
	ER_TextureFilterMode		minFilter{ TEXFILTER_NEAREST };
	ER_TextureFilterMode		magFilter{ TEXFILTER_NEAREST };

	ER_CompareFunc				compareFunc{ COMPFUNC_LESS };

	ER_TextureAddressMode		wrapS{ TEXADDRESS_WRAP };
	ER_TextureAddressMode		wrapT{ TEXADDRESS_WRAP };
	ER_TextureAddressMode		wrapR{ TEXADDRESS_WRAP };

	int							aniso{ 4 };
	float						lod{ 1.0f };
	void*						userData{ nullptr };
}SamplerStateParam_t;

//---------------------------------------
//        LOWER LEVEL CONSTANTS
//---------------------------------------

// Mask constants
#define COLORMASK_RED   0x1
#define COLORMASK_GREEN 0x2
#define COLORMASK_BLUE  0x4
#define COLORMASK_ALPHA 0x8

#define COLORMASK_ALL (COLORMASK_RED | COLORMASK_GREEN | COLORMASK_BLUE | COLORMASK_ALPHA)

// Blending factors

// HOW BLENDING WORKS:
// (TextureRGBA * SRCBlendingFactor) (Blending Function) (FrameBuffer * DSTBlendingFactor)

enum ER_BlendFactor : int
{
	BLENDFACTOR_ZERO					= 0,
	BLENDFACTOR_ONE,					//	1
	BLENDFACTOR_SRC_COLOR,				//	2
	BLENDFACTOR_ONE_MINUS_SRC_COLOR,	//	3
	BLENDFACTOR_DST_COLOR,				//	4
	BLENDFACTOR_ONE_MINUS_DST_COLOR,	//	5
	BLENDFACTOR_SRC_ALPHA,				//	6
	BLENDFACTOR_ONE_MINUS_SRC_ALPHA,	//	7
	BLENDFACTOR_DST_ALPHA,				//	8
	BLENDFACTOR_ONE_MINUS_DST_ALPHA,	//	9
	BLENDFACTOR_SRC_ALPHA_SATURATE,		//	10
};

// Function of blending
enum ER_BlendFunction : int
{
	// Function of blending
	BLENDFUNC_ADD				= 0,
	BLENDFUNC_SUBTRACT,			// 1
	BLENDFUNC_REVERSE_SUBTRACT,	// 2
	BLENDFUNC_MIN,				// 3
	BLENDFUNC_MAX,				// 4
};

//-----------------------------------------------------------------------------
// Texture flags
//-----------------------------------------------------------------------------

enum ER_TextureFlags : int
{
	// texture creating flags
	TEXFLAG_PROGRESSIVE_LODS		= (1 << 0),		// progressive LOD uploading, used to avoid driver hangs during uploads
	TEXFLAG_NULL_ON_ERROR			= (1 << 1),
	TEXFLAG_CUBEMAP					= (1 << 2),		// should create cubemap
	TEXFLAG_NOQUALITYLOD			= (1 << 3),		// not affected by texture quality Cvar, always load all mip levels

	// texture identification flags
	TEXFLAG_RENDERTARGET			= (1 << 5),		// this is a rendertarget texture
	TEXFLAG_RENDERDEPTH				= (1 << 6),		// rendertarget with depth texture
	TEXFLAG_FOREIGN					= (1 << 7),		// texture is created not by ShaderAPI
};

#define MAX_MRTS				8
#define MAX_VERTEXSTREAM		8
#define MAX_TEXTUREUNIT			16
#define MAX_VERTEXTEXTURES		4
#define MAX_SAMPLERSTATE		16

#define MAX_GENERIC_ATTRIB		8
#define MAX_TEXCOORD_ATTRIB		8

// Stencil-test function constants for SetStencilStateEx()
enum ER_StencilFunction : int
{
	STENCILFUNC_KEEP		= 0,
	STENCILFUNC_SET_ZERO,	// 1
	STENCILFUNC_REPLACE,	// 2
	STENCILFUNC_INVERT,		// 3
	STENCILFUNC_INCR,		// 4
	STENCILFUNC_DECR,		// 5
	STENCILFUNC_INCR_SAT,	// 6
	STENCILFUNC_DECR_SAT,	// 7
	STENCILFUNC_ALWAYS,		// 8
};

// Fillmode constants for SetFillMode()
enum ER_FillMode : int
{
	FILL_SOLID		= 0,
	FILL_WIREFRAME,	// 1
	FILL_POINT,		// 2
};

// Cull modes
enum ER_CullMode : int
{
	CULL_NONE		= 0,
	CULL_BACK,		// 1
	CULL_FRONT,		// 2
};

typedef struct BlendStateParam_s
{
	ER_BlendFactor		srcFactor{ BLENDFACTOR_ONE };
	ER_BlendFactor		dstFactor{ BLENDFACTOR_ZERO };
	ER_BlendFunction	blendFunc{ BLENDFUNC_ADD };

	int mask{ COLORMASK_ALL };

	bool blendEnable { false };
	bool alphaTest{ false };

	float alphaTestRef{ 0.9f };
}BlendStateParam_t;

typedef struct DepthStencilStateParams_s
{
	bool					depthTest{ false };
	bool					depthWrite{ false };
	ER_CompareFunc			depthFunc{ COMPFUNC_LEQUAL };

	bool					doStencilTest{ false };
	uint8					nStencilMask{ 0xFF };
	uint8					nStencilWriteMask{ 0xFF };
	uint8					nStencilRef{ 0xFF };
	ER_CompareFunc			nStencilFunc{ COMPFUNC_ALWAYS };
	ER_StencilFunction		nStencilFail{ STENCILFUNC_KEEP };
	ER_StencilFunction		nDepthFail{ STENCILFUNC_KEEP };
	ER_StencilFunction		nStencilPass{ STENCILFUNC_KEEP };

}DepthStencilStateParams_t;

typedef struct RasterizerStateParams_s
{
	ER_CullMode				cullMode{ CULL_NONE };
	ER_FillMode				fillMode{ FILL_SOLID };
	bool					useDepthBias{ false };
	float					depthBias{ 0.0f };
	float					slopeDepthBias{ 0.0f };
	bool					multiSample{ false };
	bool					scissor{ false };
}RasterizerStateParams_t;


//---------------------------------------

// shader constant setup flags to set to shader :P
enum ER_ShaderConstantSetup : int
{
	SCONST_VERTEX		= (1 << 0),
	SCONST_PIXEL		= (1 << 1),
	SCONST_GEOMETRY		= (1 << 2),
	SCONST_DOMAIN		= (1 << 3),
	SCONST_HULL			= (1 << 4),
};

// API reset type
enum ER_StateResetFlags : int
{
	STATE_RESET_SHADER		= (1 << 0),
	STATE_RESET_VF			= (1 << 1),
	STATE_RESET_VB			= (1 << 2),
	STATE_RESET_IB			= (1 << 3),
	STATE_RESET_DS			= (1 << 4),
	STATE_RESET_BS			= (1 << 5),
	STATE_RESET_RS			= (1 << 6),
	STATE_RESET_SS			= (1 << 7),
	STATE_RESET_TEX			= (1 << 8),
	STATE_RESET_SHADERCONST	= (1 << 9),

	STATE_RESET_ALL			= 0xFFFF,
	STATE_RESET_VBO			= (STATE_RESET_VF | STATE_RESET_VB | STATE_RESET_IB)
};

#ifndef BUFFER_OFFSET
#define BUFFER_OFFSET(i) ((char *) nullptr + (i))
#endif

struct KVSection;

struct shaderProgramText_t
{
	char*				text{ nullptr };
	char*				boilerplate{ nullptr };
	uint32				checksum{ 0 };
	Array<EqString>		includes{ PP_SL };
};

struct shaderProgramCompileInfo_t
{
	shaderProgramText_t data;

	// disables caching, always recompiled
	bool				disableCache{ false };

	// apiprefs now contains all needed attributes
	KVSection*		apiPrefs{ nullptr };
};

// shader API class for shader developers.
// DON'T USE TYPES IN DYNAMIC SHADER CODE!
enum ER_ShaderAPIType : int
{
	SHADERAPI_EMPTY = 0,
	SHADERAPI_OPENGL,
	SHADERAPI_DIRECT3D9,
	SHADERAPI_DIRECT3D10,
	SHADERAPI_DIRECT3D11
};

// shader setup type

#define MAKEQUAD(x0, y0, x1, y1, o)	\
	Vector2D(x0 + o, y0 + o),			\
	Vector2D(x0 + o, y1 - o),			\
	Vector2D(x1 - o, y0 + o),			\
	Vector2D(x1 - o, y1 - o)

#define MAKERECT(x0, y0, x1, y1, lw)\
	Vector2D(x0, y0),					\
	Vector2D(x0 + lw, y0 + lw),			\
	Vector2D(x1, y0),					\
	Vector2D(x1 - lw, y0 + lw),			\
	Vector2D(x1, y1),					\
	Vector2D(x1 - lw, y1 - lw),			\
	Vector2D(x0, y1),					\
	Vector2D(x0 + lw, y1 - lw),			\
	Vector2D(x0, y0),					\
	Vector2D(x0 + lw, y0 + lw)

#define MAKETEXRECT(x0, y0, x1, y1, lw)\
	Vertex2D(Vector2D(x0, y0),vec2_zero),					\
	Vertex2D(Vector2D(x0 + lw, y0 + lw),vec2_zero),			\
	Vertex2D(Vector2D(x1, y0),vec2_zero),					\
	Vertex2D(Vector2D(x1 - lw, y0 + lw),vec2_zero),			\
	Vertex2D(Vector2D(x1, y1),vec2_zero),					\
	Vertex2D(Vector2D(x1 - lw, y1 - lw),vec2_zero),			\
	Vertex2D(Vector2D(x0, y1),vec2_zero),					\
	Vertex2D(Vector2D(x0 + lw, y1 - lw),vec2_zero),			\
	Vertex2D(Vector2D(x0, y0),vec2_zero),					\
	Vertex2D(Vector2D(x0 + lw, y0 + lw),vec2_zero)

#define MAKETEXQUAD(x0, y0, x1, y1, o)			\
	Vertex2D(Vector2D(x0 + o, y0 + o), Vector2D(0, 0)),	\
	Vertex2D(Vector2D(x0 + o, y1 - o), Vector2D(0, 1)),	\
	Vertex2D(Vector2D(x1 - o, y0 + o), Vector2D(1, 0)),	\
	Vertex2D(Vector2D(x1 - o, y1 - o), Vector2D(1, 1))

#define MAKEQUADCOLORED(x0, y0, x1, y1, o, colorLT, colorLB, colorRT, colorRB)	\
	Vertex2D(Vector2D(x0 + o, y0 + o), Vector2D(0, 0), colorLT),	\
	Vertex2D(Vector2D(x0 + o, y1 - o), Vector2D(0, 1), colorLB),	\
	Vertex2D(Vector2D(x1 - o, y0 + o), Vector2D(1, 0), colorRT),	\
	Vertex2D(Vector2D(x1 - o, y1 - o), Vector2D(1, 1), colorRB)


#define TEXTURE_DEFAULT_PATH			"materials/"
#define SHADERS_DEFAULT_PATH			"shaders/"
#define TEXTURE_DEFAULT_EXTENSION		".dds"
#define TEXTURE_SECONDARY_EXTENSION		".tga"
#define TEXTURE_ANIMATED_EXTENSION		".ati"			// ATI - Animated Texture Index file
