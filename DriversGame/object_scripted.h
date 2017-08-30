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
	DECLARE_CLASS( CObject_Scripted, CGameObject )

	CObject_Scripted( kvkeybase_t* kvdata );
	~CObject_Scripted();

	void					OnRemove();
	void					Spawn();

	void					Draw( int nRenderFlags );

	void					Simulate(float fDt);

	int						ObjType() const		{return GO_SCRIPTED;}

protected:
	EqString				m_scriptName;

	EqLua::LuaTableFuncRef	m_onSpawn;
	EqLua::LuaTableFuncRef	m_onRemove;
	EqLua::LuaTableFuncRef	m_simulate;
};


#ifndef __INTELLISENSE__
OOLUA_PROXY(CObject_Scripted, CGameObject)

OOLUA_PROXY_END
#endif //  __INTELLISENSE__

#endif // OBJECT_PHYSICS_H