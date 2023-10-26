#pragma once

#include "IRenderLibrary.h"

class CEqRenderManager : public IRenderManager
{
public:
    CEqRenderManager();
    ~CEqRenderManager();

	IRenderLibrary* CreateRenderer(const ShaderAPIParams &params) const;
    IRenderLibrary* GetRenderer() const;
};