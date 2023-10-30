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

//-----------------------------------------------------------------------------
// Sampler state and texture flags

enum ETexFilterMode : int
{
	TEXFILTER_NEAREST = 0,
	TEXFILTER_LINEAR,
	TEXFILTER_BILINEAR,
	TEXFILTER_TRILINEAR,
	TEXFILTER_BILINEAR_ANISO,
	TEXFILTER_TRILINEAR_ANISO,
};

enum ETexAddressMode : int
{
	TEXADDRESS_WRAP = 0,
	TEXADDRESS_CLAMP,
	TEXADDRESS_MIRROR
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
// Blending factors

enum EColorMask
{
	COLORMASK_RED	= 0x1,
	COLORMASK_GREEN = 0x2,
	COLORMASK_BLUE	= 0x4,
	COLORMASK_ALPHA = 0x8,

	COLORMASK_ALL	= (COLORMASK_RED | COLORMASK_GREEN | COLORMASK_BLUE | COLORMASK_ALPHA)
};

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
enum EBlendFunc : int
{
	// Function of blending
	BLENDFUNC_ADD				= 0,
	BLENDFUNC_SUBTRACT,			// 1
	BLENDFUNC_REVERSE_SUBTRACT,	// 2
	BLENDFUNC_MIN,				// 3
	BLENDFUNC_MAX,				// 4

	BLENDFUNC_COUNT
};

struct BlendStateParams
{
	EBlendFactor	srcFactor{ BLENDFACTOR_ONE };
	EBlendFactor	dstFactor{ BLENDFACTOR_ZERO };
	EBlendFunc		blendFunc{ BLENDFUNC_ADD };
	int				mask{ COLORMASK_ALL };			// TODO: remove
	bool			enable { false };
};

static const BlendStateParams BlendAdditive = {
	BLENDFACTOR_ONE, BLENDFACTOR_ONE, 
	BLENDFUNC_ADD, COLORMASK_ALL, true
};

static const BlendStateParams BlendAlpha = {
	BLENDFACTOR_ONE, BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
	BLENDFUNC_ADD, COLORMASK_ALL, true
};

static const BlendStateParams BlendModulate = {
	BLENDFACTOR_SRC_COLOR, BLENDFACTOR_DST_COLOR,
	BLENDFUNC_ADD, COLORMASK_ALL, true
};

// HOW BLENDING WORKS:
// (TextureRGBA * SRCBlendingFactor) (Blending Function) (FrameBuffer * DSTBlendingFactor)

//-------------------------------------------
// Depth-Stencil builder

enum EStencilFunc : int
{
	STENCILFUNC_KEEP		= 0,
	STENCILFUNC_SET_ZERO,	// 1
	STENCILFUNC_REPLACE,	// 2
	STENCILFUNC_INVERT,		// 3
	STENCILFUNC_INCR_WRAP,	// 4
	STENCILFUNC_DECR_WRAP,	// 5
	STENCILFUNC_INCR_CLAMP,	// 6
	STENCILFUNC_DECR_CLAMP,	// 7

	STENCILFUNC_COUNT,
};

struct StencilFaceStateParams
{
	ECompareFunc	compareFunc{ COMPFUNC_ALWAYS };
	EStencilFunc	failOp{ STENCILFUNC_KEEP };
	EStencilFunc	depthFailOp{ STENCILFUNC_KEEP };
	EStencilFunc	passOp{ STENCILFUNC_KEEP };
};

struct DepthStencilStateParams
{
	bool			depthTest{ false };
	bool			depthWrite{ false };
	ECompareFunc	depthFunc{ COMPFUNC_LEQUAL };

	ETextureFormat	format{ FORMAT_NONE };

	float			depthBias{ 0.0f }; // TODO: int
	float			depthBiasSlopeScale{ 0.0f };

	StencilFaceStateParams stencilFront;
	StencilFaceStateParams stencilBack;
	uint32			stencilMask{ 0xFFFFFFFF };
	uint32			stencilWriteMask{ 0xFFFFFFFF };

	uint8			stencilRef{ 0xFF }; // TODO: remove
	bool			stencilTest{ false }; // TODO: remove
	bool			useDepthBias{ false }; // TODO: remove

};

FLUENT_BEGIN_TYPE(DepthStencilStateParams);
	FLUENT_SET(depthTest, DepthTestOn, true);
	FLUENT_SET(depthWrite, DepthWriteOn, true);
	FLUENT_SET_VALUE(format, DepthFormat);
	FLUENT_SET(stencilTest, StencilTestOn, true);
	FLUENT_SET_VALUE(depthBias, DepthBias);
	FLUENT_SET_VALUE(depthBiasSlopeScale, DepthBiasSlopeScale);
	FLUENT_SET_VALUE(stencilMask, StencilMask);
	FLUENT_SET_VALUE(stencilWriteMask, StencilWriteMask);
FLUENT_END_TYPE

//------------------------------------------------------------
// Pipeline builders

// Attribute format
enum EVertAttribFormat : int
{
	ATTRIBUTEFORMAT_NONE = 0,
	ATTRIBUTEFORMAT_UINT8,
	ATTRIBUTEFORMAT_HALF,
	ATTRIBUTEFORMAT_FLOAT,
};

static int s_attributeSize[] =
{
	0,
	sizeof(ubyte),
	sizeof(half),
	sizeof(float)
};

enum EVertexStepMode : int
{
	VERTEX_STEPMODE_VERTEX = 0,
	VERTEX_STEPMODE_INSTANCE,
};

struct VertexLayoutDesc
{
	struct AttribDesc
	{
		int					location{ 0 };
		int					offset{ 0 };	// in bytes
		EVertAttribFormat	format{ ATTRIBUTEFORMAT_FLOAT };
		int					count{ 0 };
		const char*			name{ nullptr };
	};

	using VertexAttribList = Array<AttribDesc>;
	VertexAttribList		attributes{ PP_SL };
	int						stride{ 0 };
	EVertexStepMode			stepMode{ VERTEX_STEPMODE_VERTEX };
};

FLUENT_BEGIN_TYPE(VertexLayoutDesc)
	FLUENT_SET_VALUE(stride, Stride)
	FLUENT_SET_VALUE(stepMode, StepMode)
	ThisType& Attribute(AttribDesc&& x) { attributes.append(std::move(x)); return *this; }
	ThisType& Attribute(int location, int offset, EVertAttribFormat format, int count, const char* name = nullptr) { attributes.append({location, offset, format, count, name}); return *this; }
FLUENT_END_TYPE

struct VertexPipelineDesc
{
	using VertexLayoutDescList = Array<VertexLayoutDesc>;
	VertexLayoutDescList	vertexLayout{ PP_SL };
	EqString				shaderEntryPoint{ "main" };
};

FLUENT_BEGIN_TYPE(VertexPipelineDesc)
	FLUENT_SET_VALUE(shaderEntryPoint, ShaderEntry)
	ThisType& VertexLayout(VertexLayoutDesc&& x) { vertexLayout.append(std::move(x)); return *this; }
FLUENT_END_TYPE



// Cull modes
enum ECullMode : int
{
	CULL_NONE	= 0,
	CULL_BACK,
	CULL_FRONT,
};

// for mesh builder and type of drawing the world model
enum EPrimTopology : int
{
	PRIM_POINTS = 0,
	PRIM_LINES,
	PRIM_LINE_STRIP,
	PRIM_TRIANGLES,
	PRIM_TRIANGLE_STRIP
};

enum EStripIndexFormat : int
{
	STRIP_INDEX_NONE = 0,
	STRIP_INDEX_UINT16,
	STRIP_INDEX_UINT32,
};

typedef int (*PRIMCOUNTER)(int numPrimitives);

static int PrimCount_TriangleList(int numPrimitives) { return numPrimitives / 3; }
static int PrimCount_TriangleFanStrip(int numPrimitives) { return numPrimitives - 2; }
static int PrimCount_LineList(int numPrimitives) { return numPrimitives / 2; }
static int PrimCount_LineStrip(int numPrimitives) { return numPrimitives - 1; }
static int PrimCount_Points(int numPrimitives) { return numPrimitives; }

static PRIMCOUNTER s_primCount[] =
{
	PrimCount_Points,
	PrimCount_LineList,
	PrimCount_LineStrip,
	PrimCount_TriangleList,
	PrimCount_TriangleFanStrip,
};

struct PrimitiveDesc
{
	ECullMode				cullMode{ CULL_NONE };
	EPrimTopology			topology{ PRIM_TRIANGLES };
	EStripIndexFormat		stripIndex{ STRIP_INDEX_NONE };
};

FLUENT_BEGIN_TYPE(PrimitiveDesc)
	FLUENT_SET_VALUE(cullMode, Cull)
	FLUENT_SET_VALUE(topology, Topology)
	FLUENT_SET_VALUE(stripIndex, StripIndex)
FLUENT_END_TYPE

//-------------------------------------------

struct FragmentPipelineDesc
{
	struct ColorTargetDesc
	{
		ETextureFormat		format{ FORMAT_NONE };
		BlendStateParams	colorBlend;
		BlendStateParams	alphaBlend;
		int					writeMask{ COLORMASK_ALL };
	};
	using ColorTargetList = FixedArray<ColorTargetDesc, MAX_RENDERTARGETS>;

	ColorTargetList			targets;
	EqString				shaderEntryPoint{ "main" };
};

FLUENT_BEGIN_TYPE(FragmentPipelineDesc);
	FLUENT_SET_VALUE(shaderEntryPoint, ShaderEntry)
	ThisType& ColorTarget(ColorTargetDesc&& x)
	{
		targets.append(std::move(x)); return *this;
	}
	ThisType& ColorTarget(ETextureFormat format, const BlendStateParams& colorBlend = BlendStateParams{}, const BlendStateParams& alphaBlend = BlendStateParams{}) 
	{
		targets.append({ format, colorBlend, alphaBlend }); return *this;
	}
FLUENT_END_TYPE

//-------------------------------------------

struct MultiSampleState
{
	int		count{ 1 };
	uint32	mask{ 0xFFFFFFFF };
	bool	alphaToCoverage{ false };
};

struct RenderPipelineDesc
{
	VertexPipelineDesc		vertex;
	DepthStencilStateParams	depthStencil;
	FragmentPipelineDesc	fragment;
	MultiSampleState		multiSample;
	PrimitiveDesc			primitive;
};

//-------------------------------------------

FLUENT_BEGIN_TYPE(RenderPipelineDesc);
	FLUENT_SET_VALUE(vertex, VertexState);
	FLUENT_SET_VALUE(depthStencil, DepthState);
	FLUENT_SET_VALUE(fragment, FragmentState);
	FLUENT_SET_VALUE(primitive, PrimitiveState);
FLUENT_END_TYPE

enum EBufferBindType : int
{
	BUFFERBIND_UNIFORM = 0,
	BUFFERBIND_STORAGE,
	BUFFERBIND_STORAGE_READONLY,
};

struct BindBuffer
{
	EBufferBindType		bindType{ BUFFERBIND_UNIFORM };
	bool				hasDynamicOffset{ false };
	// TODO: there are other fields
};

//-------------------------------------------

enum ESamplerBindType : int
{
	SAMPLERBIND_FILTERING = 0,
	SAMPLERBIND_NONFILTERING,
	SAMPLERBIND_COMPARISON,
};

struct BindSampler
{
	ESamplerBindType	bindType{ SAMPLERBIND_FILTERING };
};

//-------------------------------------------

enum ETextureSampleType
{
	TEXSAMPLE_FLOAT = 0,
	TEXSAMPLE_UNFILTERABLEFLOAT,
	TEXSAMPLE_DEPTH,
	TEXSAMPLE_SINT,
	TEXSAMPLE_UINT,
};

enum ETextureDimension
{
	TEXDIMENSION_1D = 0,
	TEXDIMENSION_2D,
	TEXDIMENSION_2DARRAY,
	TEXDIMENSION_CUBE,
	TEXDIMENSION_CUBEARRAY,
	TEXDIMENSION_3D,
};

struct BindTexture
{
	ETextureSampleType	sampleType{ TEXSAMPLE_FLOAT };
	ETextureDimension	dimension{ TEXDIMENSION_1D };
	bool				multisampled{ false };
};

//-------------------------------------------

enum EStorageTextureAccess
{
	STORAGETEX_WRITEONLY = 0,
	STORAGETEX_READONLY,
	STORAGETEX_READWRITE,
};

struct BindStorageTexture
{
	EStorageTextureAccess	access{ STORAGETEX_WRITEONLY };
	ETextureFormat			format{ FORMAT_NONE };
	ETextureDimension		dimension{ TEXDIMENSION_1D };
};

//-------------------------------------------

enum EShaderVisibility : int
{
	SHADER_VISIBLE_VERTEX	= (1 << 0),
	SHADER_VISIBLE_FRAGMENT	= (1 << 1),
	SHADER_VISIBLE_COMPUTE	= (1 << 2),
};

struct BindGroupDesc
{
	enum EEntryType
	{
		ENTRY_BUFFER = 0,
		ENTRY_SAMPLER,
		ENTRY_TEXTURE,
		ENTRY_STORAGETEXTURE
	};

	struct Entry
	{
		union {
			uint				_dummy{ 0 };
			BindBuffer			buffer;
			BindSampler			sampler;
			BindTexture			texture;
			BindStorageTexture	storageTexture;
		};
		EEntryType		type{ ENTRY_BUFFER };
		int				binding{ 0 };
		int				visibility{ 0 };	// EShaderVisibility
	};

	using EntryList = Array<Entry>;
	EntryList			entries{ PP_SL };
};

FLUENT_BEGIN_TYPE(BindGroupDesc)
	ThisType& Buffer(int binding, int visibilityFlags, EBufferBindType bindType)
	{
		Entry& entry = entries.append();
		entry.visibility = visibilityFlags;
		entry.binding = binding;
		entry.type = ENTRY_BUFFER;
		entry.buffer.bindType = bindType;
		return *this; 
	}
	ThisType& Sampler(int binding, int visibilityFlags, ESamplerBindType bindType)
	{
		Entry& entry = entries.append();
		entry.visibility = visibilityFlags;
		entry.binding = binding;
		entry.type = ENTRY_SAMPLER;
		entry.sampler.bindType = bindType;
		return *this;
	}
	ThisType& Texture(int binding, int visibilityFlags, ETextureSampleType sampleType, ETextureDimension dimension, bool multisample = false)
	{
		Entry& entry = entries.append();
		entry.visibility = visibilityFlags;
		entry.binding = binding;
		entry.type = ENTRY_TEXTURE;
		entry.texture.sampleType = sampleType;
		entry.texture.dimension = dimension;
		entry.texture.multisampled = multisample;
		return *this;
	}
	ThisType& StorageTexture(int binding, int visibilityFlags, ETextureFormat format, EStorageTextureAccess access, ETextureDimension dimension)
	{
		Entry& entry = entries.append();
		entry.visibility = visibilityFlags;
		entry.binding = binding;
		entry.type = ENTRY_STORAGETEXTURE;
		entry.storageTexture.format = format;
		entry.storageTexture.access = access;
		entry.storageTexture.dimension = dimension;
		return *this;
	}
FLUENT_END_TYPE

struct RenderPipelineLayoutDesc
{
	using BindGroupDescList = Array<BindGroupDesc>;
	BindGroupDescList	bindGroups{ PP_SL };
};

FLUENT_BEGIN_TYPE(RenderPipelineLayoutDesc)
	ThisType& Group(BindGroupDesc&& x)
	{
		bindGroups.append(std::move(x));
		return *this; 
	}
FLUENT_END_TYPE

//-------------------------------------------

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

//------------------------------------------------------------
// Texture builder

struct TextureInfo // TODO: use
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

//------------------------------------------------------------
// Buffer builder

// old buffer access flags
enum EBufferAccessType : int
{
	BUFFER_STREAM = 0,
	BUFFER_STATIC,		// = 1,
	BUFFER_DYNAMIC,		// = 2
};

enum EBufferFlags : int
{
	BUFFER_FLAG_READ = (1 << 0),	// allows reading from buffer to system memory
	BUFFER_FLAG_WRITE = (1 << 1),	// allows writing to buffer (effectively marking it as dynamic)
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

// ------------------------------
// DEPRECATED STRUCTURES

// Fillmode constants
enum EFillMode : int
{
	FILL_SOLID		= 0,
	FILL_WIREFRAME,
	FILL_POINT,
};

enum EVertAttribFlags : int
{
	VERTEXATTRIB_FLAG_INSTANCE = (1 << 15)
};

// Vertex type
enum EVertAttribType : int  // DEPRECATED
{
	VERTEXATTRIB_UNUSED = 0,

	VERTEXATTRIB_COLOR,
	VERTEXATTRIB_POSITION,
	VERTEXATTRIB_TEXCOORD,
	VERTEXATTRIB_NORMAL,
	VERTEXATTRIB_TANGENT,
	VERTEXATTRIB_BINORMAL,

	VERTEXATTRIB_COUNT,
	VERTEXATTRIB_MASK = 31
};

struct VertexFormatDesc // DEPRECATED
{
	int					streamId{ 0 };
	int					elemCount{ 0 };

	int					attribType{ 0 }; // EVertAttribType | flags; Use VERTEXATTRIB_MASK to retrieve attribute type.
	EVertAttribFormat	attribFormat{ ATTRIBUTEFORMAT_FLOAT };

	const char* name{ nullptr };
};

struct RasterizerStateParams // DEPRECATED
{
	ECullMode	cullMode{ CULL_NONE };
	EFillMode	fillMode{ FILL_SOLID };
	bool		multiSample{ false };
	bool		scissor{ false };
};