//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: EGF model instancer
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "egf/IEqModel.h"

#define MAX_EGF_INSTANCES			256
#define MAX_INSTANCE_BODYGROUPS		16		// only 4 groups can be instanced
#define MAX_INSTANCE_LODS			4		// only 4 lods can be instanced

class IVertexFormat;
class IVertexBuffer;

//---------------------------------------------------------------------
class CEGFInstancerBase : public IEqModelInstancer
{
public:
	CEGFInstancerBase();
	~CEGFInstancerBase();

	void			InitEx(const VertexFormatDesc_t* instVertexFormat, int numAttrib, int sizeOfInstance);
	void			Init(IVertexFormat* instVertexFormat, IVertexBuffer* instBuffer);
	void			Cleanup();

	void			ValidateAssert();
	bool			HasInstances() const;

	virtual void	Draw(int renderFlags, IEqModel* model);

protected:
	IVertexFormat*		m_vertFormat{ nullptr };
	IVertexBuffer*		m_instanceBuf{ nullptr };

	void*				m_instances[MAX_INSTANCE_BODYGROUPS][MAX_INSTANCE_LODS];
	ushort				m_numInstances[MAX_INSTANCE_BODYGROUPS][MAX_INSTANCE_LODS];

	bool				m_hasInstances{ false };
	bool				m_preallocatedHWBuffer{ false };
};


// for each bodygroup
template <class IT>
class CEGFInstancer : public CEGFInstancerBase
{
friend class IEqModel;
public:
	void			InitEx(const VertexFormatDesc_t* instVertexFormat, int numAttrib);
	IT&				NewInstance( ubyte bodyGroupFlags, int lod );
};

//-------------------------------------------------------

template <class IT>
inline void CEGFInstancer<IT>::InitEx(const VertexFormatDesc_t* instVertexFormat, int numAttrib)
{
	CEGFInstancerBase::InitEx(instVertexFormat, numAttrib, sizeof(IT));
}

template <class IT>
inline IT& CEGFInstancer<IT>::NewInstance(ubyte bodyGroup, int lod)
{
	static IT dummy;

	if (bodyGroup == 0xFF)
		return dummy;

	int numInst = m_numInstances[bodyGroup][lod];

	if (numInst >= MAX_EGF_INSTANCES)
		return dummy; // overflow

	if (!m_instances[bodyGroup][lod])
		m_instances[bodyGroup][lod] = PPAllocStructArray(IT, MAX_EGF_INSTANCES);

	m_hasInstances = true;

	// assign instance
	IT& theInst = ((IT*)m_instances[bodyGroup][lod])[numInst];

	numInst++;
	m_numInstances[bodyGroup][lod] = numInst;

	return theInst;
}
