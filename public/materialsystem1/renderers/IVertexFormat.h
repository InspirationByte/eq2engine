//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Middle-Level rendering API (ShaderAPI)
//				Vertex Format interface declaration
//////////////////////////////////////////////////////////////////////////////////

#ifndef IVERTEXFORMAT_H
#define IVERTEXFORMAT_H

typedef struct VertexFormatDesc_s VertexFormatDesc_t;

class IVertexFormat
{
public:
	virtual	~IVertexFormat() = default;

	virtual const char*		GetName() const = 0;

	virtual int				GetVertexSize(int stream) const = 0;
	virtual void			GetFormatDesc(VertexFormatDesc_t** desc, int& numAttribs) const = 0;
};

#endif // IVERTEXFORMAT_H
