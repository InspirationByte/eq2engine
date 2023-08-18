//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Middle-Level rendering API (ShaderAPI)
//				Vertex Format interface declaration
//////////////////////////////////////////////////////////////////////////////////

#pragma once

typedef struct VertexFormatDesc_s VertexFormatDesc_t;

class IVertexFormat
{
public:
	virtual	~IVertexFormat() = default;

	virtual const char*		GetName() const = 0;

	virtual int				GetVertexSize(int stream) const = 0;
	virtual ArrayCRef<VertexFormatDesc_t>	GetFormatDesc() const = 0;
};