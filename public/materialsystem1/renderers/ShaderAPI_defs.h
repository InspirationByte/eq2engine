//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Constant types for Equilibrium renderer
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "ShaderAPICaps.h"

enum ERHIWindowType : int
{
	RHI_WINDOW_HANDLE_UNKNOWN = -1,

	RHI_WINDOW_HANDLE_NATIVE_WINDOWS,
	RHI_WINDOW_HANDLE_NATIVE_X11,
	RHI_WINDOW_HANDLE_NATIVE_WAYLAND,
	RHI_WINDOW_HANDLE_NATIVE_COCOA,
	RHI_WINDOW_HANDLE_NATIVE_ANDROID,
};

// designed to be sent as windowHandle param
struct RenderWindowInfo
{
	enum Attribute
	{
		DISPLAY,
		WINDOW,
		SURFACE,
		TOPLEVEL
	};
	using GetterFunc = void*(*)(void* userData, Attribute attrib);

	ERHIWindowType	windowType{ RHI_WINDOW_HANDLE_UNKNOWN };
	GetterFunc 		get{ nullptr };
	void*			userData{ nullptr };
};

//---------------------------------------
//        HIGH LEVEL CONSTANTS
//---------------------------------------

// Reserved, used for shaders only
enum EConstantType : int
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

    CONSTANT_COUNT
};

static int s_constantTypeSizes[CONSTANT_COUNT] = {
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
enum ETexFilterMode : int
{
	TEXFILTER_NEAREST	= 0,
	TEXFILTER_LINEAR,
	TEXFILTER_BILINEAR,
	TEXFILTER_TRILINEAR,
	TEXFILTER_BILINEAR_ANISO,
	TEXFILTER_TRILINEAR_ANISO,
};

inline bool HasMipmaps(ETexFilterMode filter)
{
	return (filter >= TEXFILTER_BILINEAR);
}

inline bool HasAniso(ETexFilterMode filter)
{
	return (filter >= TEXFILTER_BILINEAR_ANISO);
}

enum EMatrixMode : int
{
	MATRIXMODE_VIEW	= 0,			// view tranformation matrix
	MATRIXMODE_PROJECTION,			// projection mode matrix

	MATRIXMODE_WORLD,				// world transformation matrix
	MATRIXMODE_WORLD2,				// world transformation - used as offset for MATRIXMODE_WORLD

	MATRIXMODE_TEXTURE,
};

// for SetTextureClamp()
enum ETexAddressMode : int
{
	TEXADDRESS_WRAP	= 0,
	TEXADDRESS_CLAMP,
	TEXADDRESS_MIRROR
};

// for mesh builder and type of drawing the world model
enum EPrimTopology : int
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
enum EVertAttribType : int
{
	VERTEXATTRIB_UNUSED		= 0,

	VERTEXATTRIB_COLOR,
	VERTEXATTRIB_POSITION,
	VERTEXATTRIB_TEXCOORD,
	VERTEXATTRIB_NORMAL,
	VERTEXATTRIB_TANGENT,
	VERTEXATTRIB_BINORMAL,

	VERTEXATTRIB_COUNT,
	VERTEXATTRIB_MASK = 31
};

enum EVertAttribFlags : int
{
	VERTEXATTRIB_FLAG_INSTANCE = (1 << 15)
};

// Attribute format
enum EVertAttribFormat : int
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

// old buffer access flags
enum EBufferAccessType : int
{
	BUFFER_STREAM		= 0,
	BUFFER_STATIC,		// = 1,
	BUFFER_DYNAMIC,		// = 2
};

enum EBufferFlags : int
{
	BUFFER_FLAG_READ	= (1 << 0),	// allows reading from buffer to system memory
	BUFFER_FLAG_WRITE	= (1 << 1),	// allows writing to buffer (effectively marking it as dynamic)
};

struct VertexFormatDesc
{
	int					streamId{ 0 };
	int					elemCount{ 0 };

	int					attribType{ 0 }; // also holds flags, use VERTEXATTRIB_MASK to retrieve attribute type.
	EVertAttribFormat	attribFormat{ ATTRIBUTEFORMAT_FLOAT };

	const char*			name{ nullptr };
};

// comparison functions
enum ECompareFunc : int
{
	COMPFUNC_NONE = -1,

	COMPFUNC_NEVER = 0,
	COMPFUNC_LESS,			// 1
	COMPFUNC_EQUAL,			// 2
	COMPFUNC_LEQUAL,		// 3
	COMPFUNC_GREATER,		// 4
	COMPFUNC_NOTEQUAL,		// 5
	COMPFUNC_GEQUAL,		// 6
	COMPFUNC_ALWAYS,		// 7
};

struct SamplerStateParams
{
	SamplerStateParams() = default;
	SamplerStateParams(ETexFilterMode filterType, ETexAddressMode address, ECompareFunc compareFunc = COMPFUNC_LESS, float lod = 0.0f, int aniso = 16)
		: minFilter(filterType)
		, magFilter((filterType == TEXFILTER_NEAREST) ? TEXFILTER_NEAREST : TEXFILTER_LINEAR)
		, addressS(address)
		, addressT(address)
		, addressR(address)
		, compareFunc(compareFunc)
		, lod(lod)
		, aniso((filterType == TEXFILTER_BILINEAR_ANISO) ? 16 : 0)
	{

	}

	ETexFilterMode	minFilter{ TEXFILTER_NEAREST };
	ETexFilterMode	magFilter{ TEXFILTER_NEAREST };

	ECompareFunc	compareFunc{ COMPFUNC_NONE };

	ETexAddressMode	addressS{ TEXADDRESS_WRAP };
	ETexAddressMode	addressT{ TEXADDRESS_WRAP };
	ETexAddressMode	addressR{ TEXADDRESS_WRAP };

	int				aniso{ 16 };
	float			lod{ 0.0f };
};

//---------------------------------------
//        LOWER LEVEL CONSTANTS
//---------------------------------------

// Mask constants
enum EColorMask
{
	COLORMASK_RED	= 0x1,
	COLORMASK_GREEN = 0x2,
	COLORMASK_BLUE	= 0x4,
	COLORMASK_ALPHA = 0x8,

	COLORMASK_ALL	= (COLORMASK_RED | COLORMASK_GREEN | COLORMASK_BLUE | COLORMASK_ALPHA)
};

// Blending factors

// HOW BLENDING WORKS:
// (TextureRGBA * SRCBlendingFactor) (Blending Function) (FrameBuffer * DSTBlendingFactor)

enum EBlendFactor : int
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

	BLENDFACTOR_COUNT
};

// Function of blending
enum EBlendFunction : int
{
	// Function of blending
	BLENDFUNC_ADD				= 0,
	BLENDFUNC_SUBTRACT,			// 1
	BLENDFUNC_REVERSE_SUBTRACT,	// 2
	BLENDFUNC_MIN,				// 3
	BLENDFUNC_MAX,				// 4

	BLENDFUNC_COUNT
};

//-----------------------------------------------------------------------------
// Texture flags
//-----------------------------------------------------------------------------

enum ETextureFlags : int
{
	// texture creation flags
	TEXFLAG_PROGRESSIVE_LODS		= (1 << 0),		// progressive LOD uploading, might improve performance
	TEXFLAG_NULL_ON_ERROR			= (1 << 1),
	TEXFLAG_CUBEMAP					= (1 << 2),		// should create cubemap
	TEXFLAG_NOQUALITYLOD			= (1 << 3),		// not affected by texture quality Cvar, always load all mip levels
	TEXFLAG_SRGB					= (1 << 4),		// texture should be sampled as in sRGB color space

	// texture identification flags
	TEXFLAG_RENDERTARGET			= (1 << 5),		// this is a rendertarget texture
	TEXFLAG_RENDERDEPTH				= (1 << 6),		// rendertarget with depth texture
};

// Stencil-test function
enum EStencilFunction : int
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

	STENCILFUNC_COUNT,
};

// Fillmode constants
enum EFillMode : int
{
	FILL_SOLID		= 0,
	FILL_WIREFRAME,	// 1
	FILL_POINT,		// 2

	FILL_COUNT,
};

// Cull modes
enum ECullMode : int
{
	CULL_NONE		= 0,
	CULL_BACK,		// 1
	CULL_FRONT,		// 2

	CULL_COUNT,
};

struct BlendStateParams
{
	EBlendFactor	srcFactor{ BLENDFACTOR_ONE };
	EBlendFactor	dstFactor{ BLENDFACTOR_ZERO };
	EBlendFunction	blendFunc{ BLENDFUNC_ADD };
	int				mask{ COLORMASK_ALL };
	bool			blendEnable { false };
};

struct DepthStencilStateParams
{
	bool					depthTest{ false };
	bool					depthWrite{ false };
	ECompareFunc			depthFunc{ COMPFUNC_LEQUAL };

	bool					doStencilTest{ false };
	uint8					nStencilMask{ 0xFF };
	uint8					nStencilWriteMask{ 0xFF };
	uint8					nStencilRef{ 0xFF };
	ECompareFunc			nStencilFunc{ COMPFUNC_ALWAYS };
	EStencilFunction		nStencilFail{ STENCILFUNC_KEEP };
	EStencilFunction		nDepthFail{ STENCILFUNC_KEEP };
	EStencilFunction		nStencilPass{ STENCILFUNC_KEEP };

};

struct RasterizerStateParams
{
	ECullMode				cullMode{ CULL_NONE };
	EFillMode				fillMode{ FILL_SOLID };
	bool					useDepthBias{ false };
	float					depthBias{ 0.0f };
	float					slopeDepthBias{ 0.0f };
	bool					multiSample{ false };
	bool					scissor{ false };
};


//---------------------------------------

// shader constant setup flags to set to shader :P
enum EShaderConstSetup : int
{
	SCONST_VERTEX	= (1 << 0),
	SCONST_PIXEL	= (1 << 1),
	SCONST_GEOMETRY	= (1 << 2),
	SCONST_DOMAIN	= (1 << 3),
	SCONST_HULL		= (1 << 4),

	SCONST_COUNT,
};

// API reset type
enum EStateResetFlags : int
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

struct ShaderProgText
{
	char*			text{ nullptr };
	char*			boilerplate{ nullptr };
	uint32			checksum{ 0 };
	Array<EqString>	includes{ PP_SL };
};

struct ShaderProgCompileInfo
{
	ShaderProgText	data;

	// disables caching, always recompiled
	bool			disableCache{ false };

	// apiprefs now contains all needed attributes
	KVSection*		apiPrefs{ nullptr };
};

enum EShaderAPIType : int
{
	SHADERAPI_EMPTY = 0,
	SHADERAPI_OPENGL,
	SHADERAPI_DIRECT3D9,
	SHADERAPI_DIRECT3D10,
	SHADERAPI_WEBGPU
};

struct TextureInfo
{
	const char*			name{ nullptr };
	int					width{ -1 };
	int					height{ -1 };
	int					depth{ 1 };
	int					arraySize{ 1 };
	ETextureFormat		format{ FORMAT_RGBA8 };
	SamplerStateParams	samplerParams; // NOTE: OpenGL have sampler and textures working together, while newer APIs don't
	int					flags = 0;

	const ubyte*		data = nullptr;
	int					dataSize = 0;
};

struct BufferInfo
{
	BufferInfo() = default;

	BufferInfo(int elementSize, int capacity, EBufferAccessType accessType = BUFFER_STATIC, int flags = 0)
		: accessType(accessType)
		, elementCapacity(capacity)
		, elementSize(elementSize)
		, flags(flags)
	{
	}

	BufferInfo(const void* data, int elementSize, int capacity, EBufferAccessType accessType = BUFFER_STATIC, int flags = 0)
		: accessType(accessType)
		, elementCapacity(capacity)
		, elementSize(elementSize)
		, flags(flags)
		, data(data)
		, dataSize(elementSize * capacity)
	{
	}

	template<typename T>
	BufferInfo(int capacity, EBufferAccessType accessType = BUFFER_STATIC, int flags = 0)
		: accessType(accessType)
		, elementCapacity(capacity)
		, elementSize(sizeof(T))
		, flags(flags)
	{
	}

	template<typename T>
	BufferInfo(const T* array, int numElem, EBufferAccessType accessType = BUFFER_STATIC, int flags = 0)
		: accessType(accessType)
		, elementCapacity(numElem)
		, elementSize(sizeof(T))
		, flags(flags)
		, data(array)
		, dataSize(sizeof(T) * numElem)
	{
	}

	template<typename ARRAY_TYPE>
	BufferInfo(const ARRAY_TYPE& array, EBufferAccessType accessType = BUFFER_STATIC, int flags = 0)
		: accessType(accessType)
		, elementCapacity(array.numElem())
		, elementSize(sizeof(typename ARRAY_TYPE::ITEM))
		, flags(flags)
		, data(array.ptr())
		, dataSize(sizeof(typename ARRAY_TYPE::ITEM) * array.numElem())
	{
	}

	EBufferAccessType	accessType{ BUFFER_STATIC };
	int					elementCapacity{ 0 };
	int					elementSize{ 0 };
	int					flags{ 0 };

	const void*			data{ nullptr };
	int					dataSize{ 0 };
};