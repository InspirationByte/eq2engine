//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Middle-Level rendering API (ShaderAPI)
//				Vertex Format interface declaration
//////////////////////////////////////////////////////////////////////////////////

#pragma once

struct VertexLayoutDesc;

class IVertexFormat
{
public:
	virtual	~IVertexFormat() = default;

	virtual const char*		GetName() const = 0;
	virtual int				GetNameHash() const = 0;

	virtual int				GetVertexSize(int stream) const = 0;
	virtual ArrayCRef<VertexLayoutDesc>	GetFormatDesc() const = 0;
};