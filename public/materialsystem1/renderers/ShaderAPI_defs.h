//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Constant types for Equilibrium renderer
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "ShaderAPICaps.h"
#include "IGPUBuffer.h" // for GPUBufferView
#include "ITexture.h"	// for TextureView

enum ERHIWindowType : int
{
	RHI_WINDOW_HANDLE_UNKNOWN = -1,
	
	RHI_WINDOW_HANDLE_SDL,
	RHI_WINDOW_HANDLE_NATIVE_WINDOWS,
	RHI_WINDOW_HANDLE_NATIVE_X11,
	RHI_WINDOW_HANDLE_NATIVE_WAYLAND,
	RHI_WINDOW_HANDLE_NATIVE_COCOA,
	RHI_WINDOW_HANDLE_NATIVE_ANDROID,
};

struct RenderWindowInfo;

// designed to be sent as windowHandle param
struct RenderWindowInfo
{
	enum Attribute
	{
		DISPLAY,
		WINDOW,
		SURFACE,
		TOPLEVEL,
		EXTENSIONS,
	};
	using GetterFunc = void*(RenderWindowInfo::*)(Attribute attrib, void* arg) const;

	void*				get(Attribute attrib, void* arg = nullptr) const { return (this->*getFunc)(attrib, arg); }

	ERHIWindowType		windowType{ RHI_WINDOW_HANDLE_UNKNOWN };
	GetterFunc 			getFunc{ nullptr };
	RenderWindowInfo*	parent{ nullptr };
	void*				userData{ nullptr };
};

//---------------------------------------

// comparison functions
enum ECompareFunc : uint8
{
	COMPFUNC_NONE = 0,

	COMPFUNC_NEVER,
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

enum ETexFilterMode : uint8
{
	TEXFILTER_NEAREST = 0,
	TEXFILTER_LINEAR,
	TEXFILTER_BILINEAR,
	TEXFILTER_TRILINEAR,
	TEXFILTER_BILINEAR_ANISO,
	TEXFILTER_TRILINEAR_ANISO,
};

enum ETexAddressMode : uint8
{
	TEXADDRESS_WRAP = 0,
	TEXADDRESS_CLAMP,
	TEXADDRESS_MIRROR
};

struct SamplerStateParams
{
	SamplerStateParams() = default;
	SamplerStateParams(ETexFilterMode filterType, ETexAddressMode address, ECompareFunc compareFunc = COMPFUNC_NONE, int maxAnisotropy = 16)
		: minFilter(filterType)
		, magFilter((filterType == TEXFILTER_NEAREST) ? TEXFILTER_NEAREST : TEXFILTER_LINEAR)
		, mipmapFilter((filterType == TEXFILTER_NEAREST) ? TEXFILTER_NEAREST : TEXFILTER_LINEAR)
		, addressU(address)
		, addressV(address)
		, addressW(address)
		, compareFunc(compareFunc)
		, maxAnisotropy((filterType >= TEXFILTER_BILINEAR_ANISO) ? 16 : 1)
	{
	}

	// TODO: minLodClamp/maxLodClamp
	ETexFilterMode	minFilter{ TEXFILTER_NEAREST };
	ETexFilterMode	magFilter{ TEXFILTER_NEAREST };
	ETexFilterMode	mipmapFilter{ TEXFILTER_NEAREST }; // NOTE: TEXFILTER_NEAREST or TEXFILTER_LINEAR are accepted

	ECompareFunc	compareFunc{ COMPFUNC_NONE };

	ETexAddressMode	addressU{ TEXADDRESS_WRAP };
	ETexAddressMode	addressV{ TEXADDRESS_WRAP };
	ETexAddressMode	addressW{ TEXADDRESS_WRAP };
	uint8			maxAnisotropy{ 16 };
};

//---------------------------------------
// Blending factors

enum EColorMask : uint8
{
	COLORMASK_RED	= 0x1,
	COLORMASK_GREEN = 0x2,
	COLORMASK_BLUE	= 0x4,
	COLORMASK_ALPHA = 0x8,

	COLORMASK_ALL	= (COLORMASK_RED | COLORMASK_GREEN | COLORMASK_BLUE | COLORMASK_ALPHA)
};

enum EBlendFactor : uint16
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
enum EBlendFunc : uint8
{
	// Function of blending
	BLENDFUNC_ADD				= 0,
	BLENDFUNC_SUBTRACT,			// 1
	BLENDFUNC_REVERSE_SUBTRACT,	// 2
	BLENDFUNC_MIN,				// 3
	BLENDFUNC_MAX,				// 4
};

struct BlendStateParams
{
	EBlendFactor	srcFactor{ BLENDFACTOR_ONE };
	EBlendFactor	dstFactor{ BLENDFACTOR_ZERO };
	EBlendFunc		blendFunc{ BLENDFUNC_ADD };
};

static const BlendStateParams BlendStateAdditive = {
	BLENDFACTOR_ONE, BLENDFACTOR_ONE, 
	BLENDFUNC_ADD
};

static const BlendStateParams BlendStateTranslucent = {
	BLENDFACTOR_SRC_ALPHA, BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
	BLENDFUNC_ADD
};

static const BlendStateParams BlendStateTranslucentAlpha = {
	BLENDFACTOR_ONE, BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
	BLENDFUNC_ADD
};

static const BlendStateParams BlendStateModulate = {
	BLENDFACTOR_SRC_COLOR, BLENDFACTOR_DST_COLOR,
	BLENDFUNC_ADD
};

// HOW BLENDING WORKS:
// (TextureRGBA * SRCBlendingFactor) (Blending Function) (FrameBuffer * DSTBlendingFactor)

//-------------------------------------------
// Depth-Stencil builder

enum EStencilFunc : uint8
{
	STENCILFUNC_KEEP		= 0,
	STENCILFUNC_SET_ZERO,	// 1
	STENCILFUNC_REPLACE,	// 2
	STENCILFUNC_INVERT,		// 3
	STENCILFUNC_INCR_WRAP,	// 4
	STENCILFUNC_DECR_WRAP,	// 5
	STENCILFUNC_INCR_CLAMP,	// 6
	STENCILFUNC_DECR_CLAMP,	// 7
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
	ETextureFormat	format{ FORMAT_NONE };

	bool			depthTest{ false };
	bool			depthWrite{ false };
	ECompareFunc	depthFunc{ COMPFUNC_GEQUAL };

	float			depthBias{ 0.0f }; // TODO: int
	float			depthBiasSlopeScale{ 0.0f };

	StencilFaceStateParams stencilFront;
	StencilFaceStateParams stencilBack;
	uint32			stencilMask{ 0xFFFFFFFF };
	uint32			stencilWriteMask{ 0xFFFFFFFF };

	uint8			stencilRef{ 0xFF }; // TODO: remove
	bool			stencilTest{ false };
	bool			useDepthBias{ false }; // TODO: remove

};

FLUENT_BEGIN_TYPE(DepthStencilStateParams);
	FLUENT_SET_VALUE(format, DepthFormat);
	FLUENT_SET(depthTest, DepthTestOn, true);
	FLUENT_SET(depthWrite, DepthWriteOn, true);
	FLUENT_SET_VALUE(depthFunc, DepthFunction);
	FLUENT_SET(stencilTest, StencilTestOn, true);
	FLUENT_SET_VALUE(depthBias, DepthBias);
	FLUENT_SET_VALUE(depthBiasSlopeScale, DepthBiasSlopeScale);
	FLUENT_SET_VALUE(stencilMask, StencilMask);
	FLUENT_SET_VALUE(stencilWriteMask, StencilWriteMask);
FLUENT_END_TYPE

//------------------------------------------------------------
// Pipeline builders

// Attribute format
enum EVertAttribFormat : uint8
{
	ATTRIBUTEFORMAT_NONE = 0,
	ATTRIBUTEFORMAT_UINT8,
	ATTRIBUTEFORMAT_HALF,
	ATTRIBUTEFORMAT_FLOAT,
};

enum EVertexStepMode : uint8
{
	VERTEX_STEPMODE_VERTEX = 0,
	VERTEX_STEPMODE_INSTANCE,
};

struct VertexLayoutDesc
{
	struct AttribDesc
	{
		int					nameId{ 0 };	// StringId24
		int					offset{ 0 };	// in bytes
		int					count{ 0 };
		EVertAttribFormat	format{ ATTRIBUTEFORMAT_FLOAT };
	};

	using VertexAttribList = FixedArray<AttribDesc, 24>;
	VertexAttribList		attributes;
	int						stride{ 0 };
	int						userId{ 0 };
	EVertexStepMode			stepMode{ VERTEX_STEPMODE_VERTEX };
};

FLUENT_BEGIN_TYPE(VertexLayoutDesc)
	FLUENT_SET_VALUE(userId, UserId)
	FLUENT_SET_VALUE(stride, Stride)
	FLUENT_SET_VALUE(stepMode, StepMode)
	ThisType& Attribute(AttribDesc&& x)
	{
		ASSERT(ref.attributes.isFull() == false);
		ref.attributes.append(std::move(x));
		return *this; 
	}
	ThisType& Attribute(int nameId, int offset, EVertAttribFormat format, int count)
	{
		ASSERT_MSG(count > 0 && count <= 4, "Vertex attribute count incorrect (%d, while must be <= 4)", count);
		ASSERT(ref.attributes.isFull() == false);
		ref.attributes.append({ nameId, offset, count, format });
		return *this; 
	}
FLUENT_END_TYPE

struct VertexPipelineDesc
{
	using VertexLayoutDescList = FixedArray<VertexLayoutDesc, MAX_VERTEXSTREAM>;
	VertexLayoutDescList	vertexLayout;
	EqString				shaderEntryPoint{ "main" };
};

FLUENT_BEGIN_TYPE(VertexPipelineDesc)
	FLUENT_SET_VALUE(shaderEntryPoint, ShaderEntry)
	ThisType& VertexLayout(VertexLayoutDesc&& x)
	{ 
		ASSERT(ref.vertexLayout.isFull() == false);
		ref.vertexLayout.append(std::move(x));
		return *this;
	}
	ThisType& VertexLayout(const VertexLayoutDesc& x)
	{
		ASSERT(ref.vertexLayout.isFull() == false);
		ref.vertexLayout.append(x);
		return *this;
	}
FLUENT_END_TYPE

//-------------------------------------------

// Cull modes
enum ECullMode : uint8
{
	CULL_NONE	= 0,
	CULL_BACK,
	CULL_FRONT,
};

// for mesh builder and type of drawing the world model
enum EPrimTopology : uint8
{
	PRIM_POINTS = 0,
	PRIM_LINES,
	PRIM_LINE_STRIP,
	PRIM_TRIANGLES,
	PRIM_TRIANGLE_STRIP
};

enum EStripIndexFormat : uint8
{
	STRIPINDEX_NONE = 0,
	STRIPINDEX_UINT16,
	STRIPINDEX_UINT32,
};

enum EIndexFormat : uint8
{
	INDEXFMT_UINT16 = STRIPINDEX_UINT16,
	INDEXFMT_UINT32 = STRIPINDEX_UINT32,
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
	EStripIndexFormat		stripIndex{ STRIPINDEX_NONE };
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
		EqString			name;
		ETextureFormat		format{ FORMAT_NONE };
		bool				blendEnable{ false };
		int					writeMask{ COLORMASK_ALL };
		BlendStateParams	colorBlend;
		BlendStateParams	alphaBlend;
	};
	using ColorTargetList = FixedArray<ColorTargetDesc, MAX_RENDERTARGETS>;

	ColorTargetList			targets;
	EqString				shaderEntryPoint{ "main" };
};

FLUENT_BEGIN_TYPE(FragmentPipelineDesc);
	FLUENT_SET_VALUE(shaderEntryPoint, ShaderEntry)
	ThisType& ColorTarget(ColorTargetDesc&& x)
	{
		ASSERT(ref.targets.isFull() == false);
		ref.targets.append(std::move(x)); return *this;
	}
	ThisType& ColorTarget(const char* name, ETextureFormat format)
	{
		ASSERT(ref.targets.isFull() == false);
		ref.targets.append({ name, format, false }); return *this;
	}

	// with blending on
	ThisType& ColorTarget(const char* name, ETextureFormat format, const BlendStateParams& colorBlend, const BlendStateParams& alphaBlend) 
	{
		ASSERT(ref.targets.isFull() == false);
		ref.targets.append({ name, format, true, COLORMASK_ALL, colorBlend, alphaBlend }); return *this;
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

	EqString				shaderName;
	int						shaderVertexLayoutId{ 0 };
	ArrayCRef<EqString>		shaderQuery{ nullptr };
};

//-------------------------------------------

FLUENT_BEGIN_TYPE(RenderPipelineDesc);
	FLUENT_SET_VALUE(vertex, VertexState);
	FLUENT_SET_VALUE(depthStencil, DepthState);
	FLUENT_SET_VALUE(fragment, FragmentState);
	FLUENT_SET_VALUE(multiSample, MultiSampleState);
	FLUENT_SET_VALUE(primitive, PrimitiveState);
	FLUENT_SET_VALUE(shaderName, ShaderName)
	FLUENT_SET_VALUE(shaderQuery, ShaderQuery)
	FLUENT_SET_VALUE(shaderVertexLayoutId, ShaderVertexLayoutId)
FLUENT_END_TYPE

enum EBufferBindType : uint8
{
	BUFFERBIND_UNIFORM = 0,
	BUFFERBIND_STORAGE,
	BUFFERBIND_STORAGE_READONLY,
};

//-------------------------------------------

enum ESamplerBindType : uint8
{
	SAMPLERBIND_NONE = 0,
	SAMPLERBIND_FILTERING,
	SAMPLERBIND_NONFILTERING,
	SAMPLERBIND_COMPARISON,
};

//-------------------------------------------

enum ETextureSampleType : uint8
{
	TEXSAMPLE_FLOAT = 0,
	TEXSAMPLE_UNFILTERABLEFLOAT,
	TEXSAMPLE_DEPTH,
	TEXSAMPLE_SINT,
	TEXSAMPLE_UINT,
};

enum ETextureDimension : uint8
{
	TEXDIMENSION_1D = 0,
	TEXDIMENSION_2D,
	TEXDIMENSION_2DARRAY,
	TEXDIMENSION_CUBE,
	TEXDIMENSION_CUBEARRAY,
	TEXDIMENSION_3D,
};

//-------------------------------------------

enum EStorageTextureAccess : uint8
{
	STORAGETEX_WRITEONLY = 0,
	STORAGETEX_READONLY,
	STORAGETEX_READWRITE,
};

//-------------------------------------------

enum EShaderKind : uint8
{
	SHADERKIND_VERTEX	= (1 << 0),
	SHADERKIND_FRAGMENT	= (1 << 1),
	SHADERKIND_COMPUTE	= (1 << 2),
};

enum EBindEntryType : uint8
{
	BINDENTRY_BUFFER = 0,
	BINDENTRY_SAMPLER,
	BINDENTRY_TEXTURE,
	BINDENTRY_STORAGETEXTURE
};

struct BindGroupLayoutDesc
{
	struct BindBuffer
	{
		EBufferBindType		bindType{ BUFFERBIND_UNIFORM };
	};

	struct BindTexture
	{
		ETextureSampleType	sampleType{ TEXSAMPLE_FLOAT };
		ETextureDimension	dimension{ TEXDIMENSION_1D };
		bool				multisampled{ false };
	};

	struct BindStorageTexture
	{
		ETextureFormat			format{ FORMAT_NONE };
		EStorageTextureAccess	access{ STORAGETEX_WRITEONLY };
		ETextureDimension		dimension{ TEXDIMENSION_1D };
	};

	struct BindSampler
	{
		ESamplerBindType	bindType{ SAMPLERBIND_FILTERING };
	};

	struct Entry
	{
		Entry() {}
		union {
			BindBuffer			buffer;
			BindSampler			sampler;
			BindTexture			texture;
			BindStorageTexture	storageTexture;
		};
		int				nameId;				// StringId24
		int				binding{ 0 };
		uint8			visibility{ 0 };	// EShaderKind
		EBindEntryType	type{ BINDENTRY_BUFFER };
	};

	using EntryList = FixedArray<Entry, MAX_BINDGROUP_BINDINGS>;
	EntryList			entries;
	EqString			name;
};

FLUENT_BEGIN_TYPE(BindGroupLayoutDesc)
	FLUENT_SET_VALUE(name, Name)
	ThisType& Buffer(int nameId, int binding, int shaderVisibility, EBufferBindType bindType)
	{
		Entry& entry = AddEntry(BINDENTRY_BUFFER, nameId, binding, shaderVisibility);

		entry.buffer = {};
		entry.buffer.bindType = bindType;
		return *this; 
	}
	ThisType& Sampler(int nameId, int binding, int shaderVisibility, ESamplerBindType bindType)
	{
		Entry& entry = AddEntry(BINDENTRY_SAMPLER, nameId, binding, shaderVisibility);

		entry.sampler = {};
		entry.sampler.bindType = bindType;
		return *this;
	}
	ThisType& Texture(int nameId, int binding, int shaderVisibility, ETextureSampleType sampleType, ETextureDimension dimension, bool multisample = false)
	{
		Entry& entry = AddEntry(BINDENTRY_TEXTURE, nameId, binding, shaderVisibility);

		entry.texture = {};
		entry.texture.sampleType = sampleType;
		entry.texture.dimension = dimension;
		entry.texture.multisampled = multisample;
		return *this;
	}
	ThisType& StorageTexture(int nameId, int binding, int shaderVisibility, ETextureFormat format, EStorageTextureAccess access, ETextureDimension dimension)
	{
		Entry& entry = AddEntry(BINDENTRY_STORAGETEXTURE, nameId, binding, shaderVisibility);

		entry.storageTexture = {};
		entry.storageTexture.format = format;
		entry.storageTexture.access = access;
		entry.storageTexture.dimension = dimension;
		return *this;
	}

	Entry& AddEntry(EBindEntryType type, int nameId, int binding, int shaderVisibility)
	{
		ASSERT_MSG(arrayFindIndexF(entries, [binding](const Entry& entry) { return entry.binding == binding; }) == -1, "Already taken binding %d", binding);

		ASSERT(ref.entries.isFull() == false);
		Entry& entry = ref.entries.append();
		entry.nameId = nameId;
		entry.binding = binding;
		entry.visibility = shaderVisibility;
		entry.type = type;
		return entry;
	}
FLUENT_END_TYPE

struct BindingLayoutDesc
{
	using BindGroupDescList = FixedArray<BindGroupLayoutDesc, MAX_BINDGROUPS>;
	BindGroupDescList	bindGroups;
	EqString			name;
};

FLUENT_BEGIN_TYPE(BindingLayoutDesc)
	FLUENT_SET_VALUE(name, Name)
	ThisType& Group(BindGroupLayoutDesc&& x)
	{
		ASSERT(!ref.bindGroups.isFull());
		ref.bindGroups.append(std::move(x));
		return *this; 
	}
FLUENT_END_TYPE

//------------------------------------------------------------
// BindGroup builder

struct BindGroupDesc
{
	struct EntryTexture
	{
		EntryTexture() = default;
		EntryTexture(const TextureView& view)
		{
			ptr = view.texture.Ptr();
			arraySlice = view.arraySlice;
		}
		ITexture*	ptr;
		int			arraySlice;
	};

	struct EntryBuffer
	{
		EntryBuffer() = default;
		EntryBuffer(const GPUBufferView& view)
		{
			ptr = view.buffer.Ptr();
			offset = view.offset;
			size = view.size;
		}
		IGPUBuffer* ptr;
		int64		offset;
		int64		size;
	};

	struct Entry
	{
		union {
			int					_dummy{ 0 };
			SamplerStateParams	sampler;
			EntryTexture		texture;
			EntryBuffer			buffer;
		};
		int					binding{ 0 };
		EBindEntryType		type{ static_cast<EBindEntryType>(-1) };
	};

	using EntryList = FixedArray<Entry, MAX_BINDGROUP_BINDINGS>;
	EntryList			entries;
	EqString			name;
	int					groupIdx{ -1 };
};

FLUENT_BEGIN_TYPE(BindGroupDesc)
	FLUENT_SET_VALUE(name, Name)
	FLUENT_SET_VALUE(groupIdx, GroupIndex)
	ThisType& Buffer(int binding, const GPUBufferView& buffer)
	{
		Entry& entry = AddEntry(BINDENTRY_BUFFER, binding);
		entry.buffer = EntryBuffer(buffer);
		return *this; 
	}
	ThisType& Buffer(int binding, IGPUBufferPtr buffer, int64 offset = 0, int64 size = -1)
	{
		return Buffer(binding, GPUBufferView(buffer, offset, size));
	}
	ThisType& Sampler(int binding, const SamplerStateParams& samplerParams)
	{
		Entry& entry = AddEntry(BINDENTRY_SAMPLER, binding);
		entry.sampler = samplerParams;
		return *this;
	}
	ThisType& Texture(int binding, const TextureView& texView)
	{
		Entry& entry = AddEntry(BINDENTRY_TEXTURE, binding);
		entry.texture = EntryTexture(texView);
		return *this;
	}
	ThisType& StorageTexture(int binding, const TextureView& texView)
	{
		Entry& entry = AddEntry(BINDENTRY_STORAGETEXTURE, binding);
		entry.texture = EntryTexture(texView);
		return *this;
	}
	ThisType& Texture(int binding, ITexture* texture, int arraySlice)
	{
		return Texture(binding, TextureView(texture, arraySlice));
	}
	ThisType& StorageTexture(int binding, ITexture* texture, int arraySlice)
	{
		return StorageTexture(binding, TextureView(texture, arraySlice));
	}

	Entry& AddEntry(EBindEntryType type, int binding)
	{
		ASSERT_MSG(arrayFindIndexF(entries, [binding](const Entry& entry) { return entry.binding == binding; }) == -1, "Already taken binding %d", binding);

		ASSERT(ref.entries.isFull() == false);
		Entry& entry = ref.entries.append();
		entry.binding = binding;
		entry.type = type;
		return entry;
	}
FLUENT_END_TYPE

//------------------------------------------------------------
// Buffer builder

enum EBufferUsage : int
{
	BUFFERUSAGE_UNIFORM		= (1 << 0),
	BUFFERUSAGE_VERTEX		= (1 << 1),
	BUFFERUSAGE_INDEX		= (1 << 2),
	BUFFERUSAGE_INDIRECT	= (1 << 3),
	BUFFERUSAGE_STORAGE		= (1 << 4),

	BUFFERUSAGE_READ		= (1 << 5),	// allows buffer to be mapped for read
	BUFFERUSAGE_WRITE		= (1 << 6),	// allows buffer to be mapped for write
	BUFFERUSAGE_COPY_SRC	= (1 << 7),	// buffer can be used as Copy source
	BUFFERUSAGE_COPY_DST	= (1 << 8),	// buffer can be used as Copy destination (also allows Update/WriteBuffer)
	
	BUFFERUSAGE_TRANSIENT	= (1 << 9),	// buffer is using pre-allocated GPU heap. It's fast but it's also may be limited in size
	BUFFERUSAGE_UPLOAD		= (1 << 10),// buffer is using pre-allocated heap similar to transient but optimized for uploading data to GPU
};

struct BufferInfo
{
	BufferInfo() = default;

	BufferInfo(int elementSize, int capacity)
		: elementCapacity(capacity)
		, elementSize(elementSize)
	{
	}

	BufferInfo(const void* data, int elementSize, int capacity)
		: elementCapacity(capacity)
		, elementSize(elementSize)
		, data(data)
		, dataSize(elementSize * capacity)
	{
	}

	template<typename T>
	BufferInfo(int capacity)
		: elementCapacity(capacity)
		, elementSize(sizeof(T))
	{
	}

	template<typename T>
	BufferInfo(const T* array, int numElem)
		: elementCapacity(numElem)
		, elementSize(sizeof(T))
		, data(array)
		, dataSize(sizeof(T) * numElem)
	{
	}

	template<typename ARRAY_TYPE>
	BufferInfo(const ARRAY_TYPE& array)
		: elementCapacity(array.numElem())
		, elementSize(sizeof(typename ARRAY_TYPE::ITEM))
		, data(array.ptr())
		, dataSize(sizeof(typename ARRAY_TYPE::ITEM) * array.numElem())
	{
	}

	int					GetBufferSize() const { return elementCapacity * elementSize; }

	int					elementCapacity{ 0 };
	int					elementSize{ 0 };

	const void*			data{ nullptr };
	int					dataSize{ 0 };
};

//-------------------------------
// Texture descriptor

struct TextureDesc
{
	TextureDesc() = default;
	TextureDesc(const char* name, int flags, ETextureFormat format,
		int width, int height, int arraySize = 1, int mipmapCount = 1, int sampleCount = 1,
		const SamplerStateParams& sampler = {})
			: name(name), flags(flags), format(format)
			, size({ width ,height , arraySize }), mipmapCount(1), sampleCount(sampleCount)
			, sampler(sampler)
	{
	}

	EqString			name;
	int					flags{ 0 };
	ETextureFormat		format{ FORMAT_NONE };

	TextureExtent		size;
	int16				mipmapCount{ 1 };
	int16				sampleCount{ 1 };

	SamplerStateParams	sampler{};
};

FLUENT_BEGIN_TYPE(TextureDesc)
	FLUENT_SET_VALUE(name, Name)
	FLUENT_SET_VALUE(format, Format)
	FLUENT_SET_VALUE(flags, Flags)
	FLUENT_SET_VALUE(mipmapCount, MipCount)
	FLUENT_SET_VALUE(sampleCount, SampleCount)
	FLUENT_SET_VALUE(sampler, Sampler)
	ThisType& Size(int width, int height, int arraySize = 1)
	{
		ref.size = { width ,height , arraySize };
		return *this;
	}
	ThisType& Size(const TextureExtent& extent)
	{
		ref.size = extent;
		return *this;
	}
	ThisType& Size(const IVector2D& size, int arraySize = 1)
	{
		ref.size = { size.x, size.y, arraySize};
		return *this;
	}
FLUENT_END_TYPE

//-------------------------------
// Render pass builders

enum ELoadFunc : uint8
{
	LOADFUNC_LOAD = 0,
	LOADFUNC_CLEAR,
};

enum EStoreFunc : uint8
{
	STOREFUNC_STORE = 0,
	STOREFUNC_DISCARD,
};

struct RenderPassDesc
{
	struct ColorTargetDesc
	{
		TextureView	target;
		TextureView	resolveTarget;
		MColor		clearColor{ color_black };
		ELoadFunc	loadOp{ LOADFUNC_LOAD };
		EStoreFunc	storeOp{ STOREFUNC_STORE };	// DEPRECATED
	};
	using ColorTargetList = FixedArray<ColorTargetDesc, MAX_RENDERTARGETS>;
	ColorTargetList	colorTargets;

	EqString		name;
	int				nameHash{ 0 };

	TextureView		depthStencil;
	float			depthClearValue{ 0.0f };
	ELoadFunc		depthLoadOp{ LOADFUNC_LOAD };
	EStoreFunc		depthStoreOp{ STOREFUNC_STORE };	// DEPRECATED
	bool			depthReadOnly{ false };

	uint			stencilClearValue{ 0 };
	ELoadFunc		stencilLoadOp{ LOADFUNC_LOAD };
	EStoreFunc		stencilStoreOp{ STOREFUNC_STORE };
	bool			stencilReadOnly{ false };
};

FLUENT_BEGIN_TYPE(RenderPassDesc)
	ThisType& Name(const char* str)
	{
		ref.name = str;
		ref.nameHash = StringId24(str);
		return *this; 
	}
	ThisType& ColorTarget(const TextureView& colorTarget, bool clear = false, const MColor& clearColor = color_black, bool discard = false, const TextureView& resolveTarget = nullptr)
	{
		ColorTargetDesc& entry = ref.colorTargets.append();
		entry.target = colorTarget;
		entry.resolveTarget = resolveTarget;
		entry.loadOp = clear ? LOADFUNC_CLEAR : LOADFUNC_LOAD;
		entry.storeOp = discard ? STOREFUNC_DISCARD : STOREFUNC_STORE;
		entry.clearColor = clearColor;
		return *this;
	}
	ThisType& DepthStencilTarget(ITexture* depthTarget, int arraySlice = 0)
	{
		ref.depthStencil = TextureView(depthTarget, arraySlice);
		return *this;
	}
	FLUENT_SET_VALUE(depthStoreOp, DepthStoreOp)
	FLUENT_SET_VALUE(depthReadOnly, DepthReadOnly)
	ThisType& DepthClear(float clearValue = 0.0f)
	{
		ref.depthClearValue = clearValue;
		ref.depthLoadOp = LOADFUNC_CLEAR;
		return *this; 
	}
	FLUENT_SET_VALUE(stencilStoreOp, StencilStoreOp)
	FLUENT_SET_VALUE(stencilReadOnly, StencilReadOnly)
	ThisType& StencilClear(int clearValue = 0)
	{
		ref.stencilClearValue = clearValue;
		ref.stencilLoadOp = LOADFUNC_CLEAR;
		return *this; 
	}
FLUENT_END_TYPE

//-------------------------------
// Compute pipeline descs
struct ComputePipelineDesc
{
	EqString				shaderName;
	int						shaderLayoutId{ 0 };
	EqString				shaderEntryPoint{ "main" };

	ArrayCRef<EqString>		shaderQuery{ nullptr };
};

FLUENT_BEGIN_TYPE(ComputePipelineDesc);
	FLUENT_SET_VALUE(shaderEntryPoint, ShaderEntryPoint);
	FLUENT_SET_VALUE(shaderName, ShaderName)
	FLUENT_SET_VALUE(shaderQuery, ShaderQuery)
	FLUENT_SET_VALUE(shaderLayoutId, ShaderLayoutId)
FLUENT_END_TYPE
