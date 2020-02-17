//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Factory of entities class
//////////////////////////////////////////////////////////////////////////////////

#ifndef ENTITYFACTORY_H_
#define ENTITYFACTORY_H_

#include "string.h"
#include "IEntityFactory.h"
#include "interfacemanager.h"
#include "Utils/DkLinkedList.h"

typedef void IEntity;

class CEntityFactory  : public IEntityFactory
{
public:
	CEntityFactory();

	bool			IsInitialized() const {return true;}
	const char*		GetInterfaceName() const {return IENTFACTORY_INTERFACE_VERSION;};

	void			AddEntity(const char *classname,DISPATCH_ENTFUNC dispatch );
	IEntity*		CreateEntityByName(const char *entName);
	int				GetEntityFactoryCount() {return m_hEntities.getCount();}

private:
	DkLinkedList<Entfactory_t>		m_hEntities;
};

extern IEntityFactory* entityfactory;
//EXPOSE_SINGLE_INTERFACE(entfactory,EntityFactory,IEntityFactory)

#endif