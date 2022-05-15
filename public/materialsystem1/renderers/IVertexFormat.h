//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Middle-Level rendering API (ShaderAPI)
//				Vertex Format interface declaration
//////////////////////////////////////////////////////////////////////////////////

#ifndef IVERTEXFORMAT_H
#define IVERTEXFORMAT_H

#include "ShaderAPI_defs.h"
#include "core/ppmem.h"

class IVertexFormat
{
public:
	virtual	~IVertexFormat() {}

	virtual const char*		GetName() const = 0;

	virtual int				GetVertexSize(int stream) = 0;
	virtual void			GetFormatDesc(VertexFormatDesc_t** desc, int& numAttribs) = 0;
};

#endif // IVERTEXFORMAT_H
