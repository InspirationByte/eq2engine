//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: ShaderAPI render state base
//////////////////////////////////////////////////////////////////////////////////

#ifndef IRENDERSTATE_H
#define IRENDERSTATE_H

#include "ShaderAPI_defs.h"
#include "core/ppmem.h"

enum RenderStateType_e
{
	RENDERSTATE_DEPTHSTENCIL = 0,
	RENDERSTATE_BLENDING,
	RENDERSTATE_RASTERIZER,
	RENDERSTATE_SAMPLER,

	RENDERSTATE_COUNT,
};

class IRenderState
{
public:

	PPMEM_MANAGED_OBJECT()

	IRenderState()
	{
		m_numReferences = 0;
	}

	// type of render state
	virtual RenderStateType_e	GetType() = 0;
	
	// adds reference pointers count
	void						AddReference()		{m_numReferences++;}

	void						RemoveReference()	{m_numReferences--;}

	// reference count
	int							GetReferenceNum()	{return m_numReferences;}

	virtual	void*				GetDescPtr() = 0;

protected:
	int							m_numReferences;
};

#endif // IRENDERSTATE_H