//******************* Copyright (C) Illusion Way, L.L.C 2010 *********************
//
// Description: Editor renderable objects
//
//********************************************************************************

#ifndef EDITORRENDERABLE_H
#define EDITORRENDERABLE_H

#include "BaseRenderableObject.h"

class CEditorStaticRenderable : public ÑBaseRenderableObject
{
public:
	CEditorStaticRenderable();
	// base renderable functions

	void			RenderOcclusionTest();									// draws transformed bbox for occlusion system
	void			Render();												// renders this object with current transformations

	// methods that comes from BaseEntity
	Vector3D		GetBBoxMins();					// min bbox dimensions
	Vector3D		GetBBoxMaxs();					// max bbox dimensions

	Matrix4x4		GetRenderWorldTransform();

	// the editor object
};

#endif // EDITORRENDERABLE_H