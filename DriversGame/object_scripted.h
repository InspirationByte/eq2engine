//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: scripted object
//////////////////////////////////////////////////////////////////////////////////

#ifndef OBJECT_SCRIPTED_H
#define OBJECT_SCRIPTED_H

#include "GameObject.h"
#include "world.h"

class CObject_Scripted : public CGameObject
{
public:
	CObject_Scripted( kvkeybase_t* kvdata );
	~CObject_Scripted();

	void				OnRemove();
	void				Spawn();

	void				Draw( int nRenderFlags );

	void				Simulate(float fDt);

	int					ObjType() const		{return GO_SCRIPTED;}

protected:

};

#endif // OBJECT_PHYSICS_H