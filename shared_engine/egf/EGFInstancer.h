//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: EGF model instancer
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "egf/StudioGeom.h"

static constexpr const int EGF_INST_BODYGROUP_BITS	= 4;
static constexpr const int EGF_INST_LOD_BITS		= 4;
static constexpr const int EGF_INST_MATGROUP_BITS	= 3;

static constexpr const int EGF_INST_MAX_BODYGROUPS = (1 << EGF_INST_BODYGROUP_BITS);
static constexpr const int EGF_INST_MAX_LODS = (1 << EGF_INST_LOD_BITS);
static constexpr const int EGF_INST_MAX_MATGROUPS = (1 << EGF_INST_MATGROUP_BITS);

static constexpr const int EGF_INST_BODYGROUPS_MASK = EGF_INST_MAX_BODYGROUPS - 1;
static constexpr const int EGF_INST_LODS_MASK = EGF_INST_MAX_LODS - 1;
static constexpr const int EGF_INST_MATGROUPS_MASK = EGF_INST_MAX_MATGROUPS - 1;

static constexpr const int EGF_INST_BUFFERS = 2;

static constexpr const int EGF_INST_POOL_MAX_INSTANCES = 256;

class IVertexFormat;
class IVertexBuffer;

struct EGFInstBuffer;

//---------------------------------------------------------------------
class CEGFInstancerBase : public IEqModelInstancer
{
public:
	CEGFInstancerBase();
	~CEGFInstancerBase();

	void			InitEx(const VertexFormatDesc_t* instVertexFormat, int numAttrib, int sizeOfInstance);
	void			Init(IVertexFormat* instVertexFormat, int sizeOfInstance);
	void			Cleanup();

	void			ValidateAssert();
	bool			HasInstances() const;
	void*			AllocInstance(int bodyGroup, int lod, int materialGroup);

	void			Upload();
	void			Invalidate();

	void			Draw(int renderFlags, CEqStudioGeom* model);

	static ushort MakeInstId(int bodyGroup, int lod, int matGroup)
	{
		return bodyGroup | (lod << EGF_INST_BODYGROUP_BITS) | (matGroup << (EGF_INST_BODYGROUP_BITS + EGF_INST_LOD_BITS));
	}

protected:
	Map<ushort, EGFInstBuffer>	m_data{ PP_SL };
	IVertexFormat*				m_vertFormat{ nullptr };
	int							m_instanceSize{ 0 };

	uint8						m_bodyGroupBounds[2];
	uint8						m_lodBounds[2];
	uint8						m_matGroupBounds[2];

	bool						m_hasInstances{ false };
	bool						m_ownsVertexFormat{ false };
};


// for each bodygroup
template <class IT>
class CEGFInstancer : public CEGFInstancerBase
{
friend class CEqStudioGeom;
public:
	void			Init(IVertexFormat* instVertexFormat);
	void			InitEx(const VertexFormatDesc_t* instVertexFormat, int numAttrib);
	IT&				NewInstance(int bodyGroup, int lod, int materialGroup = 0 );
};

//-------------------------------------------------------

template <class IT>
inline void CEGFInstancer<IT>::Init(IVertexFormat* instVertexFormat)
{
	CEGFInstancerBase::Init(instVertexFormat, sizeof(IT));
}

template <class IT>
inline void CEGFInstancer<IT>::InitEx(const VertexFormatDesc_t* instVertexFormat, int numAttrib)
{
	CEGFInstancerBase::InitEx(instVertexFormat, numAttrib, sizeof(IT));
}

template <class IT>
inline IT& CEGFInstancer<IT>::NewInstance(int bodyGroup, int lod, int materialGroup)
{
	static IT dummy;

	if (bodyGroup == 0xFF)
		return dummy;

	ASSERT_MSG(sizeof(IT) == m_instanceSize, "Incorrect instance size for type %s", typeid(IT).name());

	void* newInst = AllocInstance(bodyGroup, lod, materialGroup);
	if (!newInst)
		return dummy;

	return *reinterpret_cast<IT*>(newInst);
}
