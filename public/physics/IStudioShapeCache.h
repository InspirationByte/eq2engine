//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Physics model shape cache interface
///////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "egf/model.h"

class IStudioShapeCache : public IEqCoreModule
{
public:
	CORE_INTERFACE("E2_StudioShapeCache_001")

	// checks the shape is initialized for the cache
	virtual bool	IsShapeCachePresent( StudioPhyShapeData& shapeInfo ) = 0;

	// initializes whole studio shape model with all objects
	virtual void	InitStudioCache( StudioPhysData& studioData ) = 0;
	virtual void	DestroyStudioCache( StudioPhysData& studioData ) = 0;

	// does all shape cleanup
	virtual void	Cleanup_Invalidate() = 0;
};


// empty cache
class CEmptyStudioShapeCache : public IStudioShapeCache
{
public:
	bool			IsInitialized() const { return true; }

	bool			IsShapeCachePresent( StudioPhyShapeData& shapeInfo ) {return false;}

	void			InitStudioCache( StudioPhysData& studioData )  {}
	void			DestroyStudioCache( StudioPhysData& studioData )  {}

	void			Cleanup_Invalidate() {}
};

INTERFACE_SINGLETON(IStudioShapeCache, StudioShapeCache, g_studioShapeCache)
