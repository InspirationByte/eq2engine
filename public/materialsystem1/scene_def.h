//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Shared scene parameter definitions
//////////////////////////////////////////////////////////////////////////////////

#ifndef SCENE_DEF_H
#define SCENE_DEF_H

#include "dktypes.h"
#include "math/DkMath.h"

// fog parameters
struct FogInfo_t
{
	FogInfo_t() : enableFog(false)
	{}

	bool		enableFog;

	Vector3D	viewPos;

	// TODO: fog mode (linear, exponental (2x), etc)

	float		fogdensity;
	float		fognear;
	float		fogfar;

	Vector3D	fogColor;
};

class ITexture;

// Scene render modes
typedef enum
{
	RENDERMODE_NORMAL = 0,	// Draw scene normally (by default)
	RENDERMODE_SHADOWS,		// Shadow map update mode
}SceneRenderMode_t;

// Scene render modes
typedef enum
{
	LIGHTING_NONE = 0,		// Draw scene normally (by default)
	LIGHTING_FORWARD,		// forward rendering
	LIGHTING_DEFERRED,		// deferred light mode
							// ADD: You may add own tehnuque of drawing scene and may use callback to realize
}LightMode_t;

struct suninfo_t
{
	ColorRGB	m_cSunColor;
	float		intensity;
	Vector3D	sun_ang;
	bool		isAvailable;

	bool		isSkyVisible;
};

static void SunDefaults(suninfo_t* p)
{
	p->intensity = 1;
	p->isAvailable = false;
}

// Scene info
struct sceneinfo_t
{
	float		m_fZNear;
	float		m_fZFar;

	ColorRGB	m_cAmbientColor;

	bool		m_bFogEnable;
	float		m_fFogRange;
	float		m_fFogNear;

	ColorRGB	m_cFogColor;
};

//-------------------------------------------
// Lights and it's flags
//-------------------------------------------

enum DynLightType_e
{
	DLT_OMNIDIRECTIONAL = 0,
	DLT_SPOT,
	DLT_SUN,

	DLT_COUNT,
};

enum LightFlags_e
{
	LFLAG_RAINMAP				= (1 << 0),		// rain rendering
	LFLAG_DYNAMICMODELS_ONLY	= (1 << 1),		// only used for EGF model shaders, unused
	LFLAG_NOSHADOWS				= (1 << 2),		// shadow casting disabled, faster, and ugler
	LFLAG_MISCLIGHT				= (1 << 3),		// misclenation light, in r_misclights 0 and in forward shading wasn't rendered. It could be a shadow light.
	LFLAG_VOLUMETRIC			= (1 << 4),		// llight volume will be rendered for it
	LFLAG_MATRIXSET				= (1 << 5),		// matrix is set, internal use only
	LFLAG_SIMULATE_REFLECTOR	= (1 << 6),		// simulate reflector (DLT_SPOT only and shadows)
};

struct dlight_t;

inline void SetLightDefaults(dlight_t* light);

// dynamic light structure for material system
struct dlight_t
{
	dlight_t()
	{
		SetLightDefaults(this);
	}

	ubyte					nType;				// light type

	int						nFlags;				// LightFlags_e flags

	float					dietime;			// -1 for instant deletion
	float					fadetime;			// fade time, -1 = no fade
	float					curfadetime;

	Vector3D				position;			// light position
	Vector3D				angles;				// light angles

	Vector3D				radius;				// x,y,z are the ellipsoid radius. Spot only uses x component

	ColorRGB				color;				// light colour

	ITexture*				pMaskTexture;		// if you want

	Vector3D				fovWH;				// x, y are dimensions for spot. z is a FOV
	float					intensity;			// color multiplier / light intensity
	float					curintensity;

	Vector3D				lightBoundMin;		// light bounds
	Vector3D				lightBoundMax;

	Matrix4x4				lightWVP;			// light's world-view-projection
	Matrix4x4				lightWVP2;			// light's world-view-projection

	//Matrix4x4				lightView;			// light world view
	Matrix3x3				lightRotation;		// ellipsoid rotation, omni only

	void*					extraData;			// extra light data, area visibility for eqengine
};

inline void SetupLightDepthProps(const sceneinfo_t& info, int lighttype, float bias)
{
	float zNear = info.m_fZNear;
	float zFar = info.m_fZFar;

	Vector2D nearfar(zFar * zNear / (zNear - zFar), zFar / (zFar - zNear) + bias);

	g_pShaderAPI->SetShaderConstantVector2D("LightShadowProps", nearfar);
}

inline void BuildLightVolume(dlight_t* light)
{
	light->lightBoundMin = light->position - light->radius;
	light->lightBoundMax = light->position + light->radius;
}

inline void SetLightDefaults(dlight_t* light)
{
	light->pMaskTexture = NULL;

	light->nType = DLT_OMNIDIRECTIONAL;

	light->color = Vector3D(1.0f);
	light->fovWH = Vector3D(1.0f,1.0f,-1.0f);
	light->intensity = 1.0f;
	light->curintensity = 1.0f;
	light->radius = 100.0f;
	light->dietime = -1.0f;
	light->fadetime = -1.0f;
	light->curfadetime = -1.0f;
	light->angles = vec3_zero;
	light->nFlags = 0;
	light->lightRotation = identity3();
	light->extraData = NULL;
}

// Computes matrix for object
inline Matrix4x4 ComputeWorldMatrix(Vector3D &origin, Vector3D &angles, Vector3D &vScale, bool bYZXRotation = false, bool doScale = true)
{
	Matrix4x4 rotation;

	if(bYZXRotation)
		rotation = rotateYZX4(DEG2RAD(angles.x),DEG2RAD(angles.y),DEG2RAD(angles.z));
	else
		rotation = rotateXYZ4(DEG2RAD(angles.x),DEG2RAD(angles.y),DEG2RAD(angles.z));

	if(doScale)
		return translate(origin) * rotation * scale4(vScale.x,vScale.y,vScale.z);
	else
	{
		rotation.translate(origin);
		return rotation;
	}
}


// lightmap static light - sent to shader
struct slight_t
{
	slight_t()
	{
		origin_radius = vec4_zero;
		colorintensity = vec4_zero;
	}

	slight_t(Vector4D &origin_rad, Vector4D &colint)
	{
		origin_radius = origin_rad;
		colorintensity = colint;
	}

	Vector4D	origin_radius;
	Vector4D	colorintensity;
};

// Scene renderer flags
#define SR_ORTHO					(1 << 1) // Draw with orthogonal projection
#define SR_CUBEMAP					(1 << 2) // Draw with orthogonal projection
#define SR_NOMATERIALS				(1 << 3) // Don't bind any material of world/renderable groups
#define SR_NO_SKYBOX				(1 << 4) // Don't render skybox
#define SR_NO_OPAQUE				(1 << 5) // No solid objects
#define SR_NO_TRANSLUCENT			(1 << 6) // No translucent objects
#define SR_USE_PREVIOUS_VIS_SPACE	(1 << 7) // previous visibility space (or no visibility updates)
#define SR_NO_LIGHTING				(1 << 8) // disable all lighting (only ambient)
#define SR_NO_OBJECTS				(1 << 9) // disable objects rendering
#define SR_NO_WORLD					(1 << 10) // disable world rendering
#define SR_ONLYSKY					(1 << 11) // only sky
#define SR_CUBESHADOWS				(1 << 12) // Cubemap shadows
#define SR_STATICPROPS_ONLY			(1 << 14) // Draw only static props
#define SR_NO_PATCHES				(1 << 15) // Disable terrains

// Scene object flags
#define SO_NO_DRAW					(1 << 1) // Don't render
#define SO_NO_SHADOWS				(1 << 2) // No render for shadows
#define SO_NO_GBUFFER				(1 << 3) // No G-Buffer
#define SO_SHADOWS_ONLY				(1 << 4) // Only cast shadows from renderable
#define SO_ALWAYS_VISIBLE			(1 << 5) // Not affected by visibility
#define SO_NO_DYNAMICLIGHTS			(1 << 6) // Not affected by lights (always single color)
#define SO_ZXY_ANGLES				(1 << 7) // Use Z-X-Y angles when building matrix for this object

// scene visibilty flags
#define SU_DONT_UPDATEWORLD			(1 << 1) // doesn't updates world geometry (only calculates visibility of surfaces, and doesn't touching IBO)

//---------------------------------------------------------------------------------
// water rendering
//---------------------------------------------------------------------------------

class IMaterial;

struct WaterMaterialInfo_t
{
	float		fWaterHeightLevel;
	IMaterial*	pWaterMaterial;

	bool		bCheapReflections;
};

#endif // SCENE_DEF_H
