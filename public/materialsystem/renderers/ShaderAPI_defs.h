//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: Constant types for DarkTech renderer
//////////////////////////////////////////////////////////////////////////////////

#ifndef RENDERER_CONSTANTS_H
#define RENDERER_CONSTANTS_H

#include "math/Vector.h"
#include "math/Matrix.h"
#include "math/Rectangle.h"
#include "platform/Platform.h"
#include "textureformats.h"
#include "utils/DkList.h"
#include "utils/eqstring.h"
#include "ppmem.h"

//---------------------------------------
//        HIGH LEVEL CONSTANTS
//---------------------------------------

// comparison functions
enum CompareFunc_e
{
	COMP_NEVER			= 0,

	COMP_LESS,			// 1
	COMP_EQUAL,			// 2
	COMP_LEQUAL,		// 3
	COMP_GREATER,		// 4
	COMP_NOTEQUAL,		// 5
	COMP_GEQUAL,		// 6
	COMP_ALWAYS,		// 7
};

// Reserved, used for shaders only
enum ConstantType_e
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

static int constantTypeSizes[CONSTANT_TYPE_COUNT] = {
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

// for SetDepthState()
enum DepthState_e
{
	DS_NODEPTHTEST,
	DS_NODEPTHWRITE,
};

// for SetCullingMode()
enum RasterizerState_e
{
	RASTER_CULL_NONE,
	RASTER_CULL_BACK,
	RASTER_CULL_FRONT,
};

// Texture filtering type for SetTextureFilteringMode()
enum Filter_e
{
	TEXFILTER_NEAREST	= 0,
	TEXFILTER_LINEAR,
	TEXFILTER_BILINEAR,
	TEXFILTER_TRILINEAR,
	TEXFILTER_BILINEAR_ANISO,
	TEXFILTER_TRILINEAR_ANISO,
};

// for SetMatrixMode()
enum MatrixMode_e
{
	MATRIXMODE_VIEW	= 0,			// view tranformation matrix
	MATRIXMODE_PROJECTION,			// projection mode matrix
	MATRIXMODE_WORLD,				// world transformation matrix
	MATRIXMODE_TEXTURE,
};

// for SetTextureClamp()
enum AddressMode_e
{
	ADDRESSMODE_WRAP	= 0,
	ADDRESSMODE_CLAMP,
};

// for mesh builder and type of drawing the world model
enum PrimitiveType_e
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
	return numPrimitives * 0.5;
}

static int PrimCount_ListStrip( int numPrimitives )
{
	return numPrimitives - 1;
}

static int PrimCount_Points( int numPrimitives )
{
	return numPrimitives;
}

static int PrimCount_None( int numPrimitives )
{
	return 0;
}

// Vertex type
enum VertexType_e
{
	VERTEXTYPE_NONE		= 0,	// or unused

	VERTEXTYPE_COLOR,
	VERTEXTYPE_VERTEX,
	VERTEXTYPE_TEXCOORD,
	VERTEXTYPE_NORMAL,
	VERTEXTYPE_TANGENT,
	VERTEXTYPE_BINORMAL,
};

// Attribute format
enum AttributeFormat_e
{
	ATTRIBUTEFORMAT_FLOAT = 0,
	ATTRIBUTEFORMAT_HALF,
	ATTRIBUTEFORMAT_UBYTE,
};

static int formatSize[] =
{
	sizeof(float),
	sizeof(half),
	sizeof(ubyte)
};

// Index type
enum IndexType_e
{
	INDEXTYPE_SHORT = 0,
	INDEXTYPE_INTEGER,
};

// Buffer access type (for VBO)
enum BufferAccessType_e
{
	BUFFER_DEFAULT		= 0,
	BUFFER_STATIC,
	BUFFER_DYNAMIC,
};

typedef struct VertexFormatDesc_s
{
	int					m_nStream;
	int					m_nSize;

	VertexType_e		m_nType;
	AttributeFormat_e	m_nFormat;

}VertexFormatDesc_t;

typedef struct SamplerStateParam_s
{
	SamplerStateParam_s()
	{
		userData = NULL;
		nComparison = COMP_LESS;
	}

	Filter_e		minFilter;
	Filter_e		magFilter;

	CompareFunc_e	nComparison;

	AddressMode_e	wrapS;
	AddressMode_e	wrapT;
	AddressMode_e	wrapR;
	int				aniso;

	float			lod;

	void*			userData;		// user data to store API-specific pointers
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

enum BlendingFactor_e
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
enum BlendingFunction_e
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

enum ETextureFlags
{
	// texture creating flags
	TEXFLAG_CUBEMAP					= (1 << 0),		// should create cubemap

	TEXFLAG_SAMPLEDEPTH				= (1 << 1),		// depth texture sampling enabled
	TEXFLAG_SAMPLESLICES			= (1 << 2),		// sample single slices instead of array
	TEXFLAG_RENDERSLICES			= (1 << 3),		// render single slices instead of array

	TEXFLAG_GENMIPMAPS				= (1 << 4),		// generate mipmaps (could be used on rendertargets)
	TEXFLAG_NOQUALITYLOD			= (1 << 5),		// not affected by texture quality Cvar, always load all mip levels
	TEXFLAG_REALFILEPATH			= (1 << 6),		// real file patch instead of loading from 'textures/'

	TEXFLAG_USE_SRGB				= (1 << 7),

	// texture identification flags
	TEXFLAG_MANAGED					= (1 << 8),		// managed by video driver. Internal.
	TEXFLAG_RENDERTARGET			= (1 << 9),		// this is a rendertarget texture
	TEXFLAG_FOREIGN					= (1 << 10),	// texture is created not by ShaderAPI

	TEXFLAG_NULL_ON_ERROR			= (1 << 11),
};

#define MAX_MRTS				8
#define MAX_VERTEXSTREAM		8
#define MAX_TEXTUREUNIT			16
#define MAX_VERTEXTEXTURES		4
#define MAX_SAMPLERSTATE		16

#define MAX_GENERIC_ATTRIB		8
#define MAX_TEXCOORD_ATTRIB		8

// vertex texture index access helper
#define VERTEX_TEXTURE_INDEX(idx)	((-MAX_VERTEXTEXTURES - 1)+idx)

// Stencil-test function constants for SetStencilStateEx()
enum StencilFunction_e
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
enum FillMode_e
{
	FILL_SOLID		= 0,
	FILL_WIREFRAME,	// 1
	FILL_POINT,		// 2
};

// Cull modes
enum CullMode_e
{
	CULL_NONE		= 0,
	CULL_BACK,		// 1
	CULL_FRONT,		// 2
};

typedef struct BlendStateParam_s
{
	BlendStateParam_s()
	{
		srcFactor = BLENDFACTOR_ONE;
		dstFactor = BLENDFACTOR_ZERO;
		blendFunc = BLENDFUNC_ADD;

		mask = COLORMASK_ALL;
		blendEnable = false;
		alphaTest = false;
		alphaTestRef = 0.9f;
	}

	BlendingFactor_e	srcFactor;
	BlendingFactor_e	dstFactor;
	BlendingFunction_e	blendFunc;

	int mask;

	bool blendEnable;
	bool alphaTest;

	float alphaTestRef;
}BlendStateParam_t;

typedef struct DepthStencilStateParams_s
{
	DepthStencilStateParams_s()
	{
		depthFunc = COMP_LEQUAL;
		doStencilTest = false;
		nStencilMask = 0xFF;
		nStencilWriteMask = 0xFF;
		nStencilRef = 0xFF;
		nStencilFunc = COMP_ALWAYS,
		nStencilFail = STENCILFUNC_KEEP;
		nDepthFail = STENCILFUNC_KEEP;
		nStencilPass = STENCILFUNC_KEEP;
	}

	bool					depthTest;
	bool					depthWrite;
	CompareFunc_e			depthFunc;

	bool					doStencilTest;
	uint8					nStencilMask;
	uint8					nStencilWriteMask;
	uint8					nStencilRef;
	CompareFunc_e			nStencilFunc;
	StencilFunction_e		nStencilFail;
	StencilFunction_e		nDepthFail;
	StencilFunction_e		nStencilPass;

}DepthStencilStateParams_t;

typedef struct RasterizerStateParams_s
{
	RasterizerStateParams_s()
	{
		cullMode = CULL_NONE;
		fillMode = FILL_SOLID;
		multiSample = false;
		scissor = false;
		useDepthBias = false;
		depthBias = 0.0f;
		slopeDepthBias = 0.0f;
	}

	CullMode_e				cullMode;
	FillMode_e				fillMode;
	bool					useDepthBias;
	float					depthBias;
	float					slopeDepthBias;
	bool					multiSample;
	bool					scissor;
}RasterizerStateParams_t;


//---------------------------------------

// shader constant setup flags to set to shader :P
enum EShaderConstantSetup
{
	SCONST_VERTEX		= (1 << 0),
	SCONST_PIXEL		= (1 << 1),
	SCONST_GEOMETRY		= (1 << 2),
	SCONST_DOMAIN		= (1 << 3),
	SCONST_HULL			= (1 << 4),
};

// API reset type
enum EStateResetFlags
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
};

// Reset() flags
#define STATE_RESET_ALL			0xFFFF
#define STATE_RESET_VBO			(STATE_RESET_VF | STATE_RESET_VB | STATE_RESET_IB)

#ifndef BUFFER_OFFSET
#define BUFFER_OFFSET(i) ((char *) NULL + (i))
#endif

#define MAX_SAMPLER_NAMELEN 64

typedef struct Sampler_s
{
	char	name[MAX_SAMPLER_NAMELEN];
	uint	index;
	uint	gsIndex;
	uint	vsIndex;
}Sampler_t;

struct kvkeybase_t;

struct shaderProgramText_t
{
	shaderProgramText_t() : text(nullptr), checksum(-1)
	{}

	char*				text;
	long				checksum;
	DkList<EqString>	includes;
};

struct shaderProgramCompileInfo_t
{
	shaderProgramCompileInfo_t() : disableCache(false), apiPrefs(nullptr)
	{}

	shaderProgramText_t	vs;
	shaderProgramText_t	ps;
	shaderProgramText_t	gs;
	shaderProgramText_t	hs;
	shaderProgramText_t	ds;

	// disables caching, always recompiled
	bool				disableCache;

	// apiprefs now contains all needed attributes
	kvkeybase_t*		apiPrefs;
};

// shader API class for shader developers.
// DON'T USE TYPES IN DYNAMIC SHADER CODE!
enum ShaderAPIClass_e
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
#define TEXTURE_ANIMATED_EXTENSION		".ati"

#endif // RENDERER_CONSTANTS_H
