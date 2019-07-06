//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Universal sprite builder
//////////////////////////////////////////////////////////////////////////////////

#ifndef SPRITEBUILDER_H
#define SPRITEBUILDER_H

#include "IMaterialSystem.h"

#define SVBO_MAX_SIZE(s, T)	((size_t)s*sizeof(T)*4)
#define SIBO_MAX_SIZE(s)	((size_t)s*(sizeof(uint16)*6))

template <class VTX_TYPE>
class CSpriteBuilder
{
	friend class CParticleLowLevelRenderer;
public:
						CSpriteBuilder();
			virtual		~CSpriteBuilder();

	virtual void		Init( int maxQuads = 16384 );
	virtual void		Shutdown();

	void				SetTriangleListMode(bool enable) {m_triangleListMode = enable;}

	void				ClearBuffers();

	// adds triangle strip break indices
	void				AddStripBreak();

	// allocates a fixed strip for further use.
	// returns vertex start index. Returns -1 if failed
	// terminate with AddStripBreak();
	// this provides less copy operations
	virtual int			AllocateGeom( int nVertices, int nIndices, VTX_TYPE** verts, uint16** indices, bool preSetIndices = false );

	// adds strip
	virtual void		AddParticleStrip(VTX_TYPE* verts, int nVertices);

protected:

	// allocates a fixed strip for further use.
	// returns vertex start index. Returns -1 if failed
	// terminate with AddStripBreak();
	// this provides less copy operations
	int					_AllocateGeom( int nVertices, int nIndices, VTX_TYPE** verts, uint16** indices, bool preSetIndices = false );

	// adds strip to list
	void				_AddParticleStrip(VTX_TYPE* verts, int nVertices);

	// internal use only
	void				AddVertices(VTX_TYPE* verts, int nVerts);
	void				AddIndices(uint16* indices, int nIndx);

	VTX_TYPE*			m_pVerts;
	uint16*				m_pIndices;

	uint16				m_numVertices;
	uint16				m_numIndices;

	uint				m_maxQuads;

	bool				m_initialized;
	bool				m_triangleListMode;
};

template <class VTX_TYPE>
CSpriteBuilder<VTX_TYPE>::CSpriteBuilder() :
	m_pVerts(NULL),
	m_pIndices(NULL),
	m_numVertices(0),
	m_numIndices(0),
	m_initialized(false),
	m_maxQuads(0),
	m_triangleListMode(false)
{

}

template <class VTX_TYPE>
CSpriteBuilder<VTX_TYPE>::~CSpriteBuilder()
{
	Shutdown();
}

template <class VTX_TYPE>
void CSpriteBuilder<VTX_TYPE>::Init( int maxQuads )
{
	m_maxQuads = maxQuads;

	DevMsg(DEVMSG_CORE, "[SPRITEVBO] Allocating %d quads (%d bytes VB and %d bytes IB)\n", m_maxQuads, SVBO_MAX_SIZE(m_maxQuads, VTX_TYPE), SIBO_MAX_SIZE(m_maxQuads));

	// init buffers
	m_pVerts	= (VTX_TYPE*)PPAlloc(SVBO_MAX_SIZE(m_maxQuads, VTX_TYPE));
	m_pIndices	= (uint16*)PPAlloc(SIBO_MAX_SIZE(m_maxQuads));

	if(!m_pVerts)
		ASSERT(!"FAILED TO ALLOCATE VERTICES!\n");

	if(!m_pIndices)
		ASSERT(!"FAILED TO ALLOCATE INDICES!\n");

	m_numVertices = 0;
	m_numIndices = 0;

	m_initialized = true;
}

template <class VTX_TYPE>
void CSpriteBuilder<VTX_TYPE>::Shutdown()
{
	if(!m_initialized)
		return;

	m_initialized = false;

	PPFree(m_pVerts);
	PPFree(m_pIndices);

	m_pIndices = NULL;
	m_pVerts = NULL;
}

template <class VTX_TYPE>
void CSpriteBuilder<VTX_TYPE>::AddVertices(VTX_TYPE* verts, int nVerts)
{
	if ((uint)(m_numVertices + nVerts) > m_maxQuads*4)
		return;

	memcpy(&m_pVerts[m_numVertices], verts, nVerts*sizeof(VTX_TYPE));
	m_numVertices += nVerts;
}

template <class VTX_TYPE>
void CSpriteBuilder<VTX_TYPE>::AddIndices(uint16 *indices, int nIndx)
{
	if ((uint)(m_numIndices + nIndx) > m_maxQuads*6)
		return;

	memcpy(&m_pIndices[m_numIndices], indices, nIndx*sizeof(uint16));
	m_numIndices += nIndx;
}

template <class VTX_TYPE>
int CSpriteBuilder<VTX_TYPE>::AllocateGeom( int nVertices, int nIndices, VTX_TYPE** verts, uint16** indices, bool preSetIndices )
{
	return _AllocateGeom(nVertices, nIndices, verts, indices, preSetIndices);
}

template <class VTX_TYPE>
void CSpriteBuilder<VTX_TYPE>::AddParticleStrip(VTX_TYPE* verts, int nVertices)
{
	_AddParticleStrip(verts, nVertices);
}

// allocates a fixed strip for further use.
// returns vertex start index. Returns -1 if failed
// this provides less copy operations
template <class VTX_TYPE>
int CSpriteBuilder<VTX_TYPE>::_AllocateGeom( int nVertices, int nIndices, VTX_TYPE** verts, uint16** indices, bool preSetIndices )
{
	if((uint)(m_numVertices+nVertices) > m_maxQuads*4)
	{
		// don't warn me about overflow
		m_numVertices = 0;
		m_numIndices = 0;
		return -1;
	}

	if(nVertices == 0)
		return -1;

	AddStripBreak();

	int startVertex = m_numVertices;
	int startIndex = m_numIndices;

	// give the pointers
	*verts = &m_pVerts[startVertex];

	// indices are optional
	if(indices)
		*indices = &m_pIndices[startIndex];

	// apply offsets
	m_numVertices += nVertices;
	m_numIndices += nIndices;

	// make it linear and end with strip break
	if(preSetIndices)
	{
		for(int i = 0; i < nIndices; i++)
			m_pIndices[startIndex+i] = startVertex+i;
	}

	return startVertex;
}

// adds strip to list
template <class VTX_TYPE>
void CSpriteBuilder<VTX_TYPE>::_AddParticleStrip(VTX_TYPE* verts, int nVertices)
{
	if(m_triangleListMode)
		return;

	if(nVertices == 0)
		return;

	if((uint)(m_numVertices + nVertices) > m_maxQuads*4)
	{
		MsgWarning("ParticleRenderGroup overflow\n");

		m_numVertices = 0;
		m_numIndices = 0;

		return;
	}

	int num_ind = m_numIndices;

	uint16 nIndicesCurr = 0;

	// if it's a second, first I'll add last index (3 if first, and add first one from fourIndices)
	if( num_ind > 0 )
	{
		uint16 lastIdx = m_pIndices[ num_ind-1 ];

		nIndicesCurr = lastIdx+1;

		// add two last indices to make degenerates
		uint16 degenerate[2] = {lastIdx, nIndicesCurr};

		AddIndices(degenerate, 2);
	}

	// add indices

	AddVertices(verts, nVertices);

	for(int i = 0; i < nVertices; i++)
		m_pIndices[m_numIndices++] = nIndicesCurr+i;
}

template <class VTX_TYPE>
void CSpriteBuilder<VTX_TYPE>::AddStripBreak()
{
	if(m_triangleListMode)
		return;

	int num_ind = m_numIndices;

	uint16 nIndicesCurr = 0;

	// if it's a second, first I'll add last index (3 if first, and add first one from fourIndices)
	if( num_ind > 0 )
	{
		uint16 lastIdx = m_pIndices[ num_ind-1 ];

		nIndicesCurr = lastIdx+1;

		// add two last indices to make degenerates
		uint16 degenerate[2] = {lastIdx, nIndicesCurr};

		AddIndices(degenerate, 2);
	}

	//AddIndex( 0xFFFF );
}

template <class VTX_TYPE>
void CSpriteBuilder<VTX_TYPE>::ClearBuffers()
{
	m_numIndices = 0;
	m_numVertices = 0;
}

#endif // SPRITEBUILDER_H
