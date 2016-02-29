//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: Mesh builder for fonts
//////////////////////////////////////////////////////////////////////////////////

#ifndef MESHBUILDER_H
#define MESHBUILDER_H

#include <stdio.h>
#include "platform/Platform.h"
#include "math/Vector.h"
#include "ShaderAPI_defs.h"

//
// Must help for animations!
//

class IMeshBuilder
{
public:
	virtual void		Begin(PrimitiveType_e type) = 0;
	virtual void		End() = 0;

	// color setup
	virtual void		Color3f( float r, float g, float b ) = 0;;
	virtual void		Color3fv( float const *rgb ) = 0;;
	virtual void		Color4f( float r, float g, float b, float a ) = 0;;
	virtual void		Color4fv( float const *rgba ) = 0;;

	// position setup
	virtual void		Position3f	(float x, float y, float z) = 0;
	virtual void		Position3fv	(const float *v) = 0;

	// normal setup
	virtual void		Normal3f	(float nx, float ny, float nz) = 0;
	virtual void		Normal3fv	(const float *v) = 0;

	virtual void		TexCoord2f	(float s, float t) = 0;
	virtual void		TexCoord2fv	(const float *v) = 0;
	virtual void		TexCoord3f	(float s, float t, float r) = 0;
	virtual void		TexCoord3fv	(const float *v) = 0;

	virtual void		AdvanceVertex() = 0;	// advances vertex
};

#endif //MESHBUILDER_H
