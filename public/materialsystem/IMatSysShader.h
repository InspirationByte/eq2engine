//******************* Copyright (C) Illusion Way, L.L.C 2010 *********************
//
// Description: DarkTech abstract shader class
//
//****************************************************************************

#ifndef ISHADER_H
#define ISHADER_H

#include "IMaterial.h"
#include "math/DkMath.h"

class IShaderProgram;
class ITexture;
class IMaterial;

#define SHADERCONST_WORLDVIEWPROJ		90
#define SHADERCONST_WORLDTRANSFORM		94

enum ShaderDefaultParams_e
{
	SHADERPARAM_INVALID = -1,	// invalid parameter.

	SHADERPARAM_BASETEXTURE,	// base texture						s0-s3
	SHADERPARAM_BUMPMAP,		// bumpmap texture					s4-s7
	SHADERPARAM_SPECULARILLUM,	// specular and illumination		s8-s11

	SHADERPARAM_CUBEMAP,		// some cubemap						s12

	SHADERPARAM_ALPHASETUP,		// alphatest or translucensy setup
	SHADERPARAM_DEPTHSETUP,		// depth mode setup
	SHADERPARAM_RASTERSETUP,	// culling mode, etc setup

	SHADERPARAM_COLOR,			// material color					c0

	SHADERPARAM_FOG,			// fog parameters

	SHADERPARAM_ANIMFRAME,		// animation frame
	SHADERPARAM_TRANSFORM,		// transformation from matsystem to vertex shader, also an texture transform setup

	// forward-shading specified lighting setup (next three enums must be equal to DynLightType_e)
	SHADERPARAM_LIGHTING_POINT, // point light setup
	SHADERPARAM_LIGHTING_SPOT,	// spot light setup
	SHADERPARAM_LIGHTING_SUN,	// sun light setup

	SHADERPARAM_COUNT,
};


class IMaterialSystemShader
{
public:
	PPMEM_MANAGED_OBJECT();

	virtual ~IMaterialSystemShader() {}

	// Initializes parameters
	virtual void			Init(IMaterial* assignee) = 0;

	// loads textures
	virtual void			InitTextures() = 0;

	// initializes shader(s)
	virtual void			InitShader() = 0;

	// sets up shader
	virtual void			SetupShader() = 0;

	// sets up constants
	virtual void			SetupConstants(uint paramMask) = 0;

	// unloads shader
	virtual void			Unload() = 0;

	// returns real shader name
	virtual const char*		GetName() = 0;

	// is error?
	virtual bool			IsError() = 0;

	// initialization status
	virtual bool			IsInitialized() = 0;

	// returns material assigned to this shader
	virtual IMaterial*		GetAssignedMaterial() = 0;

	// returns material flags
	virtual int				GetFlags() = 0;

	// returns base texture from shader
	virtual ITexture*		GetBaseTexture(int stage = 0) = 0;

	// returns base stage count
	virtual int				GetBaseTextureStageCount() = 0;

	// returns bump texture from shader
	virtual ITexture*		GetBumpTexture(int stage = 0) = 0;

	// returns bump stage count
	virtual int				GetBumpStageCount() = 0;
};

#endif // ISHADER_H
