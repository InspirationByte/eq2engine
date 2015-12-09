//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Direct3D9 Mesh builder
//				TODO: kill it with fire
//////////////////////////////////////////////////////////////////////////////////

#include "IMeshBuilder.h"

#ifndef D3D9MB_H
#define D3D9MB_H

class D3D9MeshBuilder : public IMeshBuilder
{
public:
	 void		Begin(PrimitiveType_e type);
	 void		End();

	// color setting
	 void		Color3f( float r, float g, float b );
	 void		Color3fv( float const *rgb );
	 void		Color4f( float r, float g, float b, float a );
	 void		Color4fv( float const *rgba );

	// position setting
	 void		Position3f	(float x, float y, float z);
	 void		Position3fv	(const float *v);

	 void		Normal3f	(float nx, float ny, float nz);
	 void		Normal3fv	(const float *v);

	 void		TexCoord2f	(float s, float t);
	 void		TexCoord2fv	(const float *v);
	 void		TexCoord3f	(float s, float t, float r);
	 void		TexCoord3fv	(const float *v);

	 void		AdvanceVertex();	// advances vertexx
};

#endif // D3D9MB_H