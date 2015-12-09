//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: 
//////////////////////////////////////////////////////////////////////////////////

#include "RenderDefs.h"
#include "IViewRenderer.h"

void ComputePointLightParams(CViewParams* pView, dlight_t* pLight, int &nDrawFlags)
{
	nDrawFlags |= VR_FLAG_CUBEMAP;

	pView->SetOrigin(pLight->position);
	pView->SetAngles(vec3_zero);
	pView->SetFOV(90);
}

void ComputeSpotlightParams(CViewParams* pView, dlight_t* pLight, int &nDrawFlags)
{
	pView->SetOrigin(pLight->position);
	pView->SetAngles(pLight->angles);
	pView->SetFOV(pLight->fovWH.z);

	if(!(pLight->nFlags & LFLAG_MATRIXSET))
	{
		Matrix4x4 spot_proj = perspectiveMatrixY(DEG2RAD(pLight->fovWH.z), pLight->fovWH.x,pLight->fovWH.y, 1.0f, pLight->radius.x);
		Matrix4x4 spot_view = rotateZXY4(DEG2RAD(-pLight->angles.x),DEG2RAD(-pLight->angles.y),DEG2RAD(-pLight->angles.z));
		pLight->lightRotation = !spot_view.getRotationComponent();
		spot_view.translate(-pLight->position);

		pLight->lightWVP = spot_proj*spot_view;
		//pLight->lightView = spot_view;
	}

	pLight->nFlags |= LFLAG_MATRIXSET;
}

ConVar	r_sun_osize1("r_sun_osize1", "512");
ConVar	r_sun_osize2("r_sun_osize2", "8192");

ConVar	r_sun_znear("r_sun_znear", "-10000");
ConVar	r_sun_zfar("r_sun_zfar", "20000");



/*
float SnapFloat(int grid_spacing, float val)
{
	return round(val / grid_spacing) * grid_spacing;
}

Vector3D SnapVector(int grid_spacing, Vector3D &vector)
{
	return Vector3D(
		SnapFloat(grid_spacing, vector.x),
		SnapFloat(grid_spacing, vector.y),
		SnapFloat(grid_spacing, vector.z));
}
*/

void ComputeSunlightParams(CViewParams* pView, dlight_t* pLight, int& nDrawFlags)
{
	nDrawFlags |= VR_FLAG_ORTHOGONAL;

	pView->SetOrigin(pLight->position);
	pView->SetAngles(pLight->angles);
	pView->SetFOV(90);

	if(!(pLight->nFlags & LFLAG_MATRIXSET))
	{
		Matrix4x4 sun_proj = orthoMatrixR(-r_sun_osize1.GetFloat(),r_sun_osize1.GetFloat(),-r_sun_osize1.GetFloat(),r_sun_osize1.GetFloat(), r_sun_znear.GetFloat(), r_sun_zfar.GetFloat());
		Matrix4x4 sun_proj2 = orthoMatrixR(-r_sun_osize2.GetFloat(),r_sun_osize2.GetFloat(),-r_sun_osize2.GetFloat(),r_sun_osize2.GetFloat(), r_sun_znear.GetFloat(), r_sun_zfar.GetFloat());
		
		Matrix4x4 sun_view = !rotateXYZ4(DEG2RAD(pLight->angles.x),DEG2RAD(pLight->angles.y),DEG2RAD(pLight->angles.z));
		pLight->lightRotation = sun_view.getRotationComponent();

		pLight->radius.x = sun_proj.rows[2][3];

		sun_view.translate(-pLight->position);

		pLight->lightWVP = sun_proj*sun_view;
		pLight->lightWVP2 = sun_proj2*sun_view;

		//pLight->lightView = sun_view;
	}

	pLight->nFlags |= LFLAG_MATRIXSET;
}

static LIGHTPARAMS pParamFuncs[DLT_COUNT] =
{
	ComputePointLightParams,
	ComputeSpotlightParams,
	ComputeSunlightParams
};

void ComputeLightViewsAndFlags(CViewParams* pView, dlight_t* pLight, int &nDrawFlags)
{
	nDrawFlags |= VR_FLAG_NO_MATERIALS;
	pParamFuncs[pLight->nType](pView,pLight,nDrawFlags);
}