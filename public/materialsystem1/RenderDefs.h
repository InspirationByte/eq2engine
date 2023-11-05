#pragma once

#include "renderers/ShaderAPICaps.h"

class IVertexFormat;
class IIndexBuffer;
class IVertexBuffer;
class IMaterial;

class ITexture;
using ITexturePtr = CRefPtr<ITexture>;

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


struct RenderBoneTransform
{
	Quaternion	quat;
	Vector4D	origin;
};

// must be exactly two regs
assert_sizeof(RenderBoneTransform, sizeof(Vector4D) * 2);

// render command to draw geometry
struct RenderDrawCmd
{
	// TODO: render states

	FixedArray<IVertexBuffer*, MAX_VERTEXSTREAM>	vertexBuffers;
	IVertexFormat*	vertexLayout{ nullptr };
	IIndexBuffer*	indexBuffer{ nullptr };
	IVertexBuffer*	instanceBuffer{ nullptr };

	EPrimTopology	primitiveTopology{ (EPrimTopology)0 };

	IMaterial*		material{ nullptr };

	ArrayCRef<RenderBoneTransform> boneTransforms{ nullptr }; // TODO: buffer
	// TODO: atm material vars are used but for newer GAPI we should
	// use bind groups to setup extra material properties
	// suitable for skinned mesh, world properties, car damage stuff
	// and so on. We can do extra material textures there too.

	int				firstVertex{ 0 };
	int				firstIndex{ 0 };
	int				numVertices{ 0 };
	int				numIndices{ 0 };
	int				baseVertex{ 0 };

	RenderDrawCmd()
	{
		vertexBuffers.assureSizeEmplace(vertexBuffers.numAllocated(), nullptr);
	}

	void SetDrawIndexed(int idxCount, int firstIdx, int vertCount = -1, int firstVert = 0, int baseVert = 0)
	{
		firstVertex = firstVert;
		numVertices = vertCount;
		firstIndex = firstIdx;
		numIndices = idxCount;
		baseVertex = baseVert;
	}

	void SetDrawNonIndexed(int vertCount = -1, int firstVert = 0)
	{
		firstVertex = firstVert;
		numVertices = vertCount;
		firstIndex = -1;
		numIndices = 0;
	}
};