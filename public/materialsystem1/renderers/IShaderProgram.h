//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: Shader program for ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#ifndef ISHADERPROGRAM_H
#define ISHADERPROGRAM_H

#include "core/ppmem.h"
#include "utils/refcounted.h"

class IShaderProgram : public RefCountedObject
{
public:
	// Get shader name
	virtual const char*		GetName() = 0;

	// Will set new shader name (WARNING! Special purpose using only!)
	virtual void			SetName(const char* pszName) = 0;

	// Get constant count
	virtual int				GetConstantsNum() = 0;

	// Get sampler count
	virtual int				GetSamplersNum() = 0;
};

#endif //ISHADERPROGRAM_H
