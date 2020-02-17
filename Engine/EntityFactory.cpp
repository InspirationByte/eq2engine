//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Factory of entities class
//////////////////////////////////////////////////////////////////////////////////

#include "IEngineGame.h"
#include "EntityFactory.h"
#include "string.h"
#include "core_base_header.h"

CEntityFactory::CEntityFactory()
{
	GetCore()->RegisterInterface( IENTFACTORY_INTERFACE_VERSION, this );
}

void CEntityFactory::AddEntity(const char *classname, DISPATCH_ENTFUNC dispatch )
{
	Entfactory_t factory;
	factory.entdispatch = dispatch;
	factory.classname = xstrdup(classname);

	bool bCanAdd = true;
	if (m_hEntities.goToFirst()){
		do 
		{
			if (!stricmp((char*)m_hEntities.getCurrent().classname,classname))
				bCanAdd = false;
		} 
		while (m_hEntities.goToNext());
	}
	if(bCanAdd)
		m_hEntities.addLast(factory);
	else
		ErrorMsg("Entity '%s' already linked!!!\n", classname);
}


IEntity* CEntityFactory::CreateEntityByName(const char *entName)
{
	if(engine->GetGameState() == IEngineGame::GAME_NOTRUNNING)
	{
		MsgWarning("Cannot create entities, game is not running\n");
		return NULL;
	}

	if (m_hEntities.goToFirst())
	{
		do 
		{
			Entfactory_t *pEnt = (Entfactory_t*)&m_hEntities.getCurrent();
			if (!stricmp(pEnt->classname,entName))
			{
				return (pEnt->entdispatch)();
			}
		} 
		while (m_hEntities.goToNext());
	}
	MsgError("Failed to create '%s', the entity is NULL\n",entName);
	return NULL;
}

// expose and initialize
static CEntityFactory g_EntFactory;
IEntityFactory* entityfactory = ( IEntityFactory * )&g_EntFactory;