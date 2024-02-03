#pragma once

#include "renderers/ShaderAPI_defs.h"
#include "renderers/IVertexFormat.h"
#include "renderers/IGPUCommandRecorder.h"

class IVertexFormat;
class IMaterial;

class ITexture;
using ITexturePtr = CRefPtr<ITexture>;

class IGPUBuffer;
using IGPUBufferPtr = CRefPtr<IGPUBuffer>;

enum EPrimTopology : int;

struct Vertex2D
{
	Vertex2D() = default;
	Vertex2D(const Vector2D& p, const Vector2D& t)
	{
		position = p;
		texCoord = t;
		color = color_white.pack();
	}

	Vertex2D(const Vector2D& p, const Vector2D& t, const Vector4D& c)
		: position(p)
		, texCoord(t)
		, color(MColor(c).pack())
	{}

	Vertex2D(const Vector2D& p, const Vector2D& t, const MColor& c)
		: position(p)
		, texCoord(t)
		, color(c.pack())
	{}

	Vector2D		position;
	float			unused{ 0.0f };
	Vector2D		texCoord{ vec2_zero };
	uint			color{ color_white.pack() };
};

static Vertex2D InterpolateVertex2D(const Vertex2D& a, const Vertex2D& b, float fac)
{
	return Vertex2D(
		lerp(a.position, b.position, fac), 
		lerp(a.texCoord, b.texCoord, fac),
		lerp(MColor(a.color).v, MColor(b.color).v, fac));
}

struct Vertex3D
{
	Vertex3D() = default;
	Vertex3D(const Vector3D& p, const Vector2D& t)
		: position(p)
		, texCoord(t)
	{}

	Vertex3D(const Vector3D& p, const Vector2D& t, const ColorRGBA& c)
		: position(p)
		, texCoord(t)
		, color(MColor(c).pack())
	{}

	Vertex3D(const Vector3D& p, const Vector2D& t, const MColor& c)
		: position(p)
		, texCoord(t)
		, color(c.pack())
	{}

	Vector3D		position{ vec3_zero };
	Vector2D		texCoord{ vec2_zero };
	uint			color{ color_white.pack()};
};

enum EVertexFVF
{
	VERTEX_FVF_XYZ			= (1 << 0),
	VERTEX_FVF_UVS			= (1 << 1),
	VERTEX_FVF_NORMAL		= (1 << 2),
	VERTEX_FVF_COLOR_DW		= (1 << 3),
	VERTEX_FVF_COLOR_VEC	= (1 << 4)
};

static int VertexFVFSize(int fvf)
{
	return
		((fvf & VERTEX_FVF_XYZ) ? sizeof(Vector3D) : 0)
		+ ((fvf & VERTEX_FVF_UVS) ? sizeof(Vector2D) : 0)
		+ ((fvf & VERTEX_FVF_NORMAL) ? sizeof(Vector3D) : 0)
		+ ((fvf & VERTEX_FVF_COLOR_DW) ? sizeof(uint) : 0)
		+ ((fvf & VERTEX_FVF_COLOR_VEC) ? sizeof(Vector4D) : 0);
}

template<typename T>
struct VertexFVFResolver;

template<>
struct VertexFVFResolver<Vertex2D>
{
	inline static int value = VERTEX_FVF_XYZ | VERTEX_FVF_UVS | VERTEX_FVF_COLOR_DW;
};

template<>
struct VertexFVFResolver<Vertex3D>
{
	inline static int value = VERTEX_FVF_XYZ | VERTEX_FVF_UVS | VERTEX_FVF_COLOR_DW;
};

#define MAKEQUAD(x0, y0, x1, y1, o)	\
	Vector2D(x0 + o, y0 + o),	\
	Vector2D(x0 + o, y1 - o),	\
	Vector2D(x1 - o, y0 + o),	\
	Vector2D(x1 - o, y1 - o)

#define MAKERECT(x0, y0, x1, y1, lw) \
	Vector2D(x0, y0),			\
	Vector2D(x0 + lw, y0 + lw),	\
	Vector2D(x1, y0),			\
	Vector2D(x1 - lw, y0 + lw),	\
	Vector2D(x1, y1),			\
	Vector2D(x1 - lw, y1 - lw),	\
	Vector2D(x0, y1),			\
	Vector2D(x0 + lw, y1 - lw),	\
	Vector2D(x0, y0),			\
	Vector2D(x0 + lw, y0 + lw)

#define MAKETEXRECT(x0, y0, x1, y1, lw) \
	Vertex2D(Vector2D(x0, y0),vec2_zero),			\
	Vertex2D(Vector2D(x0 + lw, y0 + lw),vec2_zero),	\
	Vertex2D(Vector2D(x1, y0),vec2_zero),			\
	Vertex2D(Vector2D(x1 - lw, y0 + lw),vec2_zero),	\
	Vertex2D(Vector2D(x1, y1),vec2_zero),			\
	Vertex2D(Vector2D(x1 - lw, y1 - lw),vec2_zero),	\
	Vertex2D(Vector2D(x0, y1),vec2_zero),			\
	Vertex2D(Vector2D(x0 + lw, y1 - lw),vec2_zero),	\
	Vertex2D(Vector2D(x0, y0),vec2_zero),			\
	Vertex2D(Vector2D(x0 + lw, y0 + lw),vec2_zero)

#define MAKETEXQUAD(x0, y0, x1, y1, o) \
	Vertex2D(Vector2D(x0 + o, y0 + o), Vector2D(0, 0)),	\
	Vertex2D(Vector2D(x0 + o, y1 - o), Vector2D(0, 1)),	\
	Vertex2D(Vector2D(x1 - o, y0 + o), Vector2D(1, 0)),	\
	Vertex2D(Vector2D(x1 - o, y1 - o), Vector2D(1, 1))

#define MAKEQUADCOLORED(x0, y0, x1, y1, o, colorLT, colorLB, colorRT, colorRB) \
	Vertex2D(Vector2D(x0 + o, y0 + o), Vector2D(0, 0), colorLT),	\
	Vertex2D(Vector2D(x0 + o, y1 - o), Vector2D(0, 1), colorLB),	\
	Vertex2D(Vector2D(x1 - o, y0 + o), Vector2D(1, 0), colorRT),	\
	Vertex2D(Vector2D(x1 - o, y1 - o), Vector2D(1, 1), colorRB)


struct RenderBufferInfo
{
	GPUBufferView	bufferView;
	int				signature;	// use MAKECHAR4(a,b,c,d)
};

struct MeshInstanceFormatRef
{
	using LayoutList = ArrayCRef<VertexLayoutDesc>;

	const char*		name{ nullptr };
	int				formatId{ 0 };
	LayoutList		layout{ nullptr };
	uint			usedLayoutBits{ 0xff };
};

struct MeshInstanceData
{
	IGPUBufferPtr	buffer;
	int				first{ 0 };
	int				count{ 0 };
	int				stride{ 0 };
	int				offset{ 0 };
};

struct RenderInstanceInfo
{
	RenderInstanceInfo()
	{
		vertexBuffers.assureSizeEmplace(vertexBuffers.numAllocated(), GPUBufferView{});
	}

	using BufferArray = FixedArray<RenderBufferInfo, 8>;
	using VertexBufferArray = FixedArray<GPUBufferView, MAX_VERTEXSTREAM>;

	// use SetVertexBuffer
	VertexBufferArray		vertexBuffers;

	// use SetIndexBuffer
	GPUBufferView			indexBuffer;
	EIndexFormat			indexFormat{ INDEXFMT_UINT16 };

	// use SetInstanceFormat
	MeshInstanceFormatRef	instFormat;

	// use SetInstanceData
	MeshInstanceData		instData;

	// use AddUniformBuffer
	BufferArray				uniformBuffers;
};

struct RenderDrawBatch
{
	IMaterial*		material{ nullptr };
	EPrimTopology	primTopology{ (EPrimTopology)0 };
	int				firstVertex{ 0 };
	int				firstIndex{ 0 };
	int				numVertices{ 0 };
	int				numIndices{ 0 };
	int				baseVertex{ 0 };
};

// render command to draw geometry
struct RenderDrawCmd
{
	RenderInstanceInfo	instanceInfo;
	RenderDrawBatch		batchInfo;

	RenderDrawCmd()
	{
		instanceInfo.instData.count = 1;
	}

	RenderDrawCmd& SetMaterial(IMaterial* _material)
	{
		batchInfo.material = _material;
		return *this;
	}

	RenderDrawCmd& SetInstanceFormat(const MeshInstanceFormat& meshInst)
	{
		instanceInfo.instFormat.name = meshInst.name;
		instanceInfo.instFormat.formatId = meshInst.nameHash;
		instanceInfo.instFormat.layout = meshInst.layout;
		return *this;
	}

	// DEPRECATED
	RenderDrawCmd& SetInstanceFormat(const IVertexFormat* vertFormat)
	{
		instanceInfo.instFormat.name = vertFormat->GetName();
		instanceInfo.instFormat.formatId = vertFormat->GetNameHash();
		instanceInfo.instFormat.layout = vertFormat->GetFormatDesc();
		return *this;
	}

	RenderDrawCmd& SetInstanceCount(int count)
	{
		instanceInfo.instData.count = count;
		return *this;
	}

	RenderDrawCmd& SetInstanceData(const MeshInstanceData& instData)
	{
		instanceInfo.instData = instData;
		return *this;
	}

	RenderDrawCmd& SetInstanceData(IGPUBufferPtr buffer, int instanceSize = 1, int instanceCount = 1, int offset = 0)
	{
		instanceInfo.instData.buffer = buffer;
		instanceInfo.instData.count = instanceCount;
		instanceInfo.instData.stride = instanceSize;
		instanceInfo.instData.offset = offset;
		return *this;
	}

	RenderDrawCmd& SetVertexBuffer(int idx, IGPUBuffer* buffer, int64 offset = 0, int64 size = -1)
	{
		instanceInfo.vertexBuffers[idx] = GPUBufferView(buffer, offset, size);
		return *this;
	}

	RenderDrawCmd& SetIndexBuffer(IGPUBufferPtr buffer, EIndexFormat indexFormat, int offset = 0, int size = -1)
	{
		instanceInfo.indexBuffer = GPUBufferView(buffer, offset, size);
		instanceInfo.indexFormat = indexFormat;
		return *this;
	}

	RenderDrawCmd& SetDrawIndexed(EPrimTopology topology, int idxCount, int firstIdx, int vertCount = -1, int firstVert = 0, int baseVert = 0)
	{
		batchInfo.primTopology = topology;
		batchInfo.firstVertex = firstVert;
		batchInfo.numVertices = vertCount;
		batchInfo.firstIndex = firstIdx;
		batchInfo.numIndices = idxCount;
		batchInfo.baseVertex = baseVert;
		return *this;
	}

	RenderDrawCmd& SetDrawNonIndexed(EPrimTopology topology, int vertCount = -1, int firstVert = 0)
	{
		batchInfo.primTopology = topology;
		batchInfo.firstVertex = firstVert;
		batchInfo.numVertices = vertCount;
		batchInfo.firstIndex = -1;
		batchInfo.numIndices = 0;
		return *this;
	}

	RenderDrawCmd& AddUniformBuffer(int signature, IGPUBuffer* buffer, int64 offset = 0, int64 size = -1)
	{
		instanceInfo.uniformBuffers.append({ GPUBufferView(buffer, offset, size), signature });
		return *this;
	}

	RenderDrawCmd& AddUniformBufferView(int signature, const GPUBufferView& bufferView)
	{
		instanceInfo.uniformBuffers.append({ bufferView, signature });
		return *this;
	}
};

enum EShaderBlendMode : int
{
	SHADER_BLEND_NONE = 0,
	SHADER_BLEND_TRANSLUCENT,		// is transparent
	SHADER_BLEND_ADDITIVE,			// additive transparency
	SHADER_BLEND_MODULATE,			// modulate
};

enum ERenderPassType
{
	RENDERPASS_DEFAULT,
	RENDERPASS_SHADOW,			// shadow texture drawing (decal)
	RENDERPASS_DEPTH,			// depth pass (shadow mapping and other things)
	RENDERPASS_GAME_SCENE_VIEW,	// scene drawing
};

struct RenderPassBaseData
{
	RenderPassBaseData(ERenderPassType type) : type(type) {}

	ERenderPassType		type;
	uint				id{ 0 };
	uint				version{ 0 };
	GPUBufferView		cameraParamsBuffer;
};

// used for debug geometry and UIs
struct MatSysDefaultRenderPass : public RenderPassBaseData
{
	MatSysDefaultRenderPass() : RenderPassBaseData(RENDERPASS_DEFAULT) {}

	MColor				drawColor{ color_white };
	IAARectangle		scissorRectangle{ -1, -1, -1, -1 };
	TextureView			texture;
	ECullMode			cullMode{ CULL_NONE };
	EShaderBlendMode	blendMode{ SHADER_BLEND_NONE };
	ECompareFunc		depthFunc{ COMPFUNC_LEQUAL };
	bool				depthTest{ false };
	bool				depthWrite{ false };
};


struct RenderPassContext
{
	RenderPassContext() = default;
	RenderPassContext(IGPURenderPassRecorder* recorder, const RenderPassBaseData* passData) 
		: recorder(recorder)
		, data(passData)
	{}

	using BeforeMaterialSetupFunc = EqFunction<IMaterial* (IMaterial* material)>;

	IGPURenderPassRecorderPtr	recorder;			// render pass recorder

	const RenderPassBaseData*	data{ nullptr };
	BeforeMaterialSetupFunc		beforeMaterialSetup{ nullptr };
};