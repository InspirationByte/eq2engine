//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: MatSystem abstraction of shader
//////////////////////////////////////////////////////////////////////////////////

#pragma once

class IShaderProgram;

class ITexture;
using ITexturePtr = CRefPtr<ITexture>;

class IMaterial;

enum EShaderParamSetup
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
	virtual ~IMaterialSystemShader() = default;

	virtual void				Init(IMaterial* assignee) = 0;
	virtual void				Unload() = 0;

	virtual void				InitTextures() = 0;
	virtual void				InitShader() = 0;

	virtual void				SetupShader() = 0;
	virtual void				SetupConstants(uint paramMask) = 0;

	virtual const char*			GetName() const = 0;

	virtual bool				IsError() const = 0;
	virtual bool				IsInitialized() const = 0;

	virtual IMaterial*			GetAssignedMaterial() const = 0;
	virtual int					GetFlags() const = 0;

	virtual const ITexturePtr&	GetBaseTexture(int stage = 0) const = 0;
	virtual int					GetBaseTextureStageCount() const = 0;

	virtual const ITexturePtr&	GetBumpTexture(int stage = 0) const = 0;
	virtual int					GetBumpStageCount() const = 0;
};
