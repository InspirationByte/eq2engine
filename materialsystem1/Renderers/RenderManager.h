#pragma once

#include "IRenderLibrary.h"

class CEqRenderManager : public IRenderManager
{
public:
    CEqRenderManager();
    ~CEqRenderManager();

	IRenderLibrary* CreateRenderer(const shaderAPIParams_t &params) const;

    IRenderLibrary* GetRenderer() const;
};

extern CEqRenderManager g_renderManager;