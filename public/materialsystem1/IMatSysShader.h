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

	virtual void				Init(IShaderAPI* renderAPI, IMaterial* assignee) = 0;
	virtual void				Unload() = 0;

	virtual void				InitTextures(IShaderAPI* renderAPI) = 0;
	virtual void				InitShader(IShaderAPI* renderAPI) = 0;

	virtual void				SetupShader(IShaderAPI* renderAPI) = 0;
	virtual void				SetupConstants(IShaderAPI* renderAPI, uint paramMask) = 0;

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

typedef IMaterialSystemShader* (*DISPATCH_CREATE_SHADER)(void);
struct ShaderFactory
{
	DISPATCH_CREATE_SHADER dispatcher;
	const char* shader_name;
};
using FactoryList = Array<ShaderFactory>;

#define DECLARE_INTERNAL_SHADERS()       \
	FactoryList* s_internalShaderReg = nullptr;                            \
	FactoryList& _InternalShaderList() { if(!s_internalShaderReg) s_internalShaderReg = new FactoryList(PP_SL); return *s_internalShaderReg; }

#define REGISTER_INTERNAL_SHADERS()								\
	for(int i = 0; i < _InternalShaderList().numElem(); i++)	\
		g_matSystem->RegisterShader( _InternalShaderList()[i].shader_name, _InternalShaderList()[i].dispatcher );

extern FactoryList& _InternalShaderList();

#define DEFINE_SHADER(stringName, className)								\
	static IMaterialSystemShader* C##className##Factory( void )						\
	{																				\
		IMaterialSystemShader *pShader = static_cast< IMaterialSystemShader * >(new className()); 	\
		return pShader;																\
	}																				\
	class C_ShaderClassFactoryFoo													\
	{																				\
	public:																			\
		C_ShaderClassFactoryFoo( void )											\
		{																			\
			ShaderFactory factory;												\
			factory.dispatcher = &C##className##Factory;							\
			factory.shader_name = stringName;										\
			_InternalShaderList().append(factory);						\
		}																			\
	};																				\
	static C_ShaderClassFactoryFoo g_CShaderClassFactoryFoo;
