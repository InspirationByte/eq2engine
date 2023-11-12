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

class IGPURenderPipeline;
using IGPURenderPipelinePtr = CRefPtr<IGPURenderPipeline>;

class IGPUBindGroup;
using IGPUBindGroupPtr = CRefPtr<IGPUBindGroup>;

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
	SHADERPARAM_BONETRANSFORMS,	// skinning bone transform matrices

	// forward-shading specified lighting setup (next three enums must be equal to DynLightType_e)
	SHADERPARAM_LIGHTING_POINT, // point light setup
	SHADERPARAM_LIGHTING_SPOT,	// spot light setup
	SHADERPARAM_LIGHTING_SUN,	// sun light setup

	SHADERPARAM_COUNT,
};


class IMatSystemShader
{
public:
	virtual ~IMatSystemShader() = default;

	virtual void					Init(IShaderAPI* renderAPI, IMaterial* assignee) = 0;
	virtual void					Unload() = 0;

	virtual void					InitTextures(IShaderAPI* renderAPI) = 0;
	virtual void					InitShader(IShaderAPI* renderAPI) = 0;

	virtual const char*				GetName() const = 0;

	virtual bool					IsError() const = 0;
	virtual bool					IsInitialized() const = 0;
	virtual int						GetFlags() const = 0;

	virtual const ITexturePtr&		GetBaseTexture(int stage = 0) const = 0;
	virtual const ITexturePtr&		GetBumpTexture(int stage = 0) const = 0;

	virtual bool					IsSupportVertexFormat(int nameHash) const = 0;
	virtual IGPURenderPipelinePtr	GetRenderPipeline(IShaderAPI* renderAPI) const = 0;
	virtual IGPUBindGroupPtr		GetMaterialBindGroup(IShaderAPI* renderAPI) const = 0;

	// DEPRECATED
	virtual void					SetupShader(IShaderAPI* renderAPI) = 0;
	virtual void					SetupConstants(IShaderAPI* renderAPI, uint paramMask) = 0;
};

typedef IMatSystemShader* (*DISPATCH_CREATE_SHADER)(void);
struct ShaderFactory
{
	DISPATCH_CREATE_SHADER dispatcher;
	const char* shader_name;
};
using FactoryList = Array<ShaderFactory>;

extern FactoryList& _InternalShaderList();

#define DECLARE_INTERNAL_SHADERS()  \
	FactoryList* s_internalShaderReg = nullptr; \
	FactoryList& _InternalShaderList() { if(!s_internalShaderReg) s_internalShaderReg = new FactoryList(PP_SL); return *s_internalShaderReg; }

#define REGISTER_INTERNAL_SHADERS()	\
	for(const ShaderFactory& factory : _InternalShaderList())	\
		g_matSystem->RegisterShader( factory.shader_name, factory.dispatcher );

#define DEFINE_SHADER(stringName, className) \
	static IMatSystemShader* C##className##Factory() { \
		IMatSystemShader *pShader = static_cast< IMatSystemShader * >(new className()); \
		return pShader;	\
	} \
	class C_ShaderClassFactoryFoo {	\
	public: \
		C_ShaderClassFactoryFoo() { \
			ShaderFactory factory; \
			factory.dispatcher = &C##className##Factory; \
			factory.shader_name = stringName; \
			_InternalShaderList().append(factory); \
		} \
	}; \
	static C_ShaderClassFactoryFoo g_CShaderClassFactoryFoo;
