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

	Vertex3D(const Vector3D& p, const Vector2D& t, const Vector4D& c)
		: position(p)
		, texCoord(t)
		, color(c)
	{}

	Vector3D		position{ vec3_zero };
	Vector2D		texCoord{ vec2_zero };
	ColorRGBA		color{ color_white };
};

enum EVertexFVF
{
	VERTEX_FVF_XYZ		= (1 << 0),
	VERTEX_FVF_UVS		= (1 << 1),
	VERTEX_FVF_NORMAL	= (1 << 2),
	VERTEX_FVF_COLOR	= (1 << 3)
};

static int VertexFVFSize(int fvf)
{
	return
		((fvf & VERTEX_FVF_XYZ) ? sizeof(Vector3D) : 0)
		+ ((fvf & VERTEX_FVF_UVS) ? sizeof(Vector2D) : 0)
		+ ((fvf & VERTEX_FVF_NORMAL) ? sizeof(Vector3D) : 0)
		+ ((fvf & VERTEX_FVF_COLOR) ? sizeof(uint) : 0);
}

template<typename T>
struct VertexFVFResolver;

template<>
struct VertexFVFResolver<Vertex2D>
{
	inline static int value = VERTEX_FVF_XYZ | VERTEX_FVF_UVS | VERTEX_FVF_COLOR;
};

template<>
struct VertexFVFResolver<Vertex3D>
{
	inline static int value = VERTEX_FVF_XYZ | VERTEX_FVF_UVS | VERTEX_FVF_COLOR;
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