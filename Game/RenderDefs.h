//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description:
//////////////////////////////////////////////////////////////////////////////////

#ifndef RENDERDEFS_H
#define RENDERDEFS_H

#include "ViewParams.h"

struct dlight_t;

//
// Lighting pipeline
//

// light parameters setup prototype
typedef void (*LIGHTPARAMS)(CViewParams* pView, dlight_t* pLight, int &nDrawFlags);

void	ComputePointLightParams(CViewParams* pView, dlight_t* pLight, int &nDrawFlags);
void	ComputeSpotlightParams(CViewParams* pView, dlight_t* pLight, int &nDrawFlags);
void	ComputeSunlightParams(CViewParams* pView, dlight_t* pLight, int &nDrawFlags);

// Computes lighting and view parameters for the specified light
void	ComputeLightViewsAndFlags(CViewParams* pView, dlight_t* pLight, int &nDrawFlags);

#endif // RENDERDEFS_H