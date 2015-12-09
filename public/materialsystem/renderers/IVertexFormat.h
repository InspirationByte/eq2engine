//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech Middle-Level rendering API (ShaderAPI)
//				Vertex Format interface declaration
//////////////////////////////////////////////////////////////////////////////////

#ifndef IVERTEXFORMAT_H
#define IVERTEXFORMAT_H


#include "Platform.h"
#include "ShaderAPI_defs.h"

class IVertexFormat
{
public:
	virtual ~IVertexFormat() {}

	PPMEM_MANAGED_OBJECT();

	virtual int8 GetVertexSizePerStream(int16 nStream) = 0;
};

#endif // IVERTEXFORMAT_H