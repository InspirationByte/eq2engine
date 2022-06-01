//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: Shader program for ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#ifndef ISHADERPROGRAM_H
#define ISHADERPROGRAM_H

#include "core/ppmem.h"
#include "ds/refcounted.h"

class IShaderProgram : public RefCountedObject<IShaderProgram>
{
public:
	// Get shader name
	virtual const char*		GetName() const = 0;

	// returns name hash
	virtual int				GetNameHash() const = 0;

	// Get constant count
	virtual int				GetConstantsNum() const = 0;

	// Get sampler count
	virtual int				GetSamplersNum() const = 0;
};

#endif //ISHADERPROGRAM_H
