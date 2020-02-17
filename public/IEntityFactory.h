//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: abstract Factory of entities class
//////////////////////////////////////////////////////////////////////////////////

#ifndef IENTITYFACTORY_H
#define IENTITYFACTORY_H

typedef void IEntity;

#define IENTFACTORY_INTERFACE_VERSION "IEntityFactory_001"

typedef IEntity* (*DISPATCH_ENTFUNC)( void );

struct Entfactory_t
{
	DISPATCH_ENTFUNC entdispatch;
	char *classname;
};

class IEntityFactory : public ICoreModuleInterface
{
public:
	virtual void					AddEntity(const char *classname,DISPATCH_ENTFUNC dispatch ) = 0;
	virtual int						GetEntityFactoryCount() = 0;
	virtual IEntity*				CreateEntityByName(const char *entName) = 0;
};

extern IEntityFactory *entityfactory;

#define DECLARE_ENTITYFACTORY( localName, className )												\
	extern void ImportEngineInterfaces();															\
	static IEntity *C##localName##Factory( void )													\
	{																								\
		BaseEntity *newent = static_cast< BaseEntity * >(new className); 							\
		newent->SetClassName(#localName);															\
		newent->SetName("");																		\
		return newent;																				\
	};																								\
	class C##localName##Foo																			\
	{																								\
	public:																							\
		C##localName##Foo( void )																	\
		{																							\
			ImportEngineInterfaces();																\
			entityfactory->AddEntity( #localName, &C##localName##Factory );							\
		}																							\
	};																								\
	static C##localName##Foo g_C##localName##Foo;

#endif