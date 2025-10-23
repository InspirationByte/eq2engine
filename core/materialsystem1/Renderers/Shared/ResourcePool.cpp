#include "core/core_common.h"
#include "ResourcePool.h"

Array<RenderResources::IResourcePool*> RenderResources::s_resourcePools{ PP_SL };

void RenderResources::Terminate()
{
	for (IResourcePool* effPool : s_resourcePools)
		effPool->Clear();
	s_resourcePools.clear(true);
}