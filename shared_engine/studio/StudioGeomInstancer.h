//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: EGF model instancer
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "studio/StudioGeom.h"
#include "materialsystem1/renderers/IShaderAPI.h"

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

struct EGFInstBuffer
{
	IGPUBufferPtr	instanceVB{ nullptr };
	void*			instances{ nullptr };
	ushort			numInstances{ 0 };
	ushort			upToDateInstanes{ 0 };

	~EGFInstBuffer();
	void Init(int sizeOfInstance);
};

//---------------------------------------------------------------------
class CBaseEqGeomInstancer
{
public:
	CBaseEqGeomInstancer();
	~CBaseEqGeomInstancer();

	void			Init(IVertexFormat* instVertexFormat, ArrayCRef<EGFHwVertex::VertexStreamId> instVertStreamMapping, int sizeOfInstance);
	void			Cleanup();

	void			ValidateAssert();
	bool			HasInstances() const;
	void*			AllocInstance(int bodyGroup, int lod, int materialGroup);

	void			Upload();
	void			Invalidate();

	void			Draw(CEqStudioGeom* model);

	static ushort MakeInstId(int bodyGroup, int lod, int matGroup)
	{
		return bodyGroup | (lod << EGF_INST_BODYGROUP_BITS) | (matGroup << (EGF_INST_BODYGROUP_BITS + EGF_INST_LOD_BITS));
	}

protected:
	Map<ushort, EGFInstBuffer>	m_data{ PP_SL };
	ArrayCRef<EGFHwVertex::VertexStreamId> m_vertexStreamMapping{ nullptr };
	IVertexFormat*				m_vertFormat{ nullptr };
	int							m_instanceSize{ 0 };

	uint8						m_bodyGroupBounds[2]{ 0 };
	uint8						m_lodBounds[2]{ 0 };
	uint8						m_matGroupBounds[2]{ 0 };

	bool						m_ownsVertexFormat{ false };
};


// for each bodygroup
template <class IT>
class CEqGeomInstancer : public CBaseEqGeomInstancer
{
public:
	void							Init(IVertexFormat* instVertexFormat, ArrayCRef<EGFHwVertex::VertexStreamId> instVertStreamMapping);
	IT&								NewInstance(int bodyGroup, int lod, int materialGroup = 0 );

	static CEqGeomInstancer<IT>*	Get(CEqStudioGeom* model, IVertexFormat* vertexFormatInstanced, ArrayCRef<EGFHwVertex::VertexStreamId> instVertStreamMapping);
};

//-------------------------------------------------------

template <class IT>
inline void CEqGeomInstancer<IT>::Init(IVertexFormat* instVertexFormat, ArrayCRef<EGFHwVertex::VertexStreamId> instVertStreamMapping)
{
	CBaseEqGeomInstancer::Init(instVertexFormat, instVertStreamMapping, sizeof(IT));
}

template <class IT>
inline IT& CEqGeomInstancer<IT>::NewInstance(int bodyGroup, int lod, int materialGroup)
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

template <class IT>
CEqGeomInstancer<IT>* CEqGeomInstancer<IT>::Get(CEqStudioGeom* model, IVertexFormat* vertexFormatInstanced, ArrayCRef<EGFHwVertex::VertexStreamId> instVertStreamMapping)
{
	CEqGeomInstancer<IT>* instancer = reinterpret_cast<CEqGeomInstancer<IT>*>(model->GetInstancer());

	if (!instancer && g_renderAPI->GetCaps().isInstancingSupported)
	{
		instancer = PPNew CEqGeomInstancer<IT>();
		instancer->Init(vertexFormatInstanced, instVertStreamMapping);
		model->SetInstancer(instancer);
	}

	return instancer;
}