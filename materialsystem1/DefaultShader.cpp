//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Default MatSystem shader
//				free of states, textures. Just uses matrix and fog setup
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "materialsystem1/IMaterialSystem.h"
#include "materialsystem1/BaseShader.h"

struct ShaderRenderPassDesc
{
	struct ColorTargetDesc
	{
		EqString		name;
		ETextureFormat	format{ FORMAT_NONE };
	};
	using VertexLayoutDescList = Array<VertexLayoutDesc>;
	using ColorTargetList = FixedArray<ColorTargetDesc, MAX_RENDERTARGETS>;

	EqString				name;
	ColorTargetList			targets;
	ETextureFormat			depthFormat{ FORMAT_NONE };
	VertexLayoutDescList	vertexLayout{ PP_SL };
};

FLUENT_BEGIN_TYPE(ShaderRenderPassDesc);
	FLUENT_SET_VALUE(name, Name);
	FLUENT_SET_VALUE(depthFormat, DepthFormat);
	ThisType& VertexLayout(VertexLayoutDesc&& x) { vertexLayout.append(std::move(x)); return *this; }
	ThisType& ColorTarget(ColorTargetDesc&& x)
	{
		targets.append(std::move(x)); return *this;
	}
	ThisType& ColorTarget(const char* name, ETextureFormat format)
	{
		targets.append({ name, format }); return *this;
	}
FLUENT_END_TYPE

#include "DynamicMesh.h"

static void ShaderRenderPassDescBuild()
{
	// g_matSystem->GetDynamicMesh()->GetVertexLayout()
	ShaderRenderPassDesc renderPassDesc = Builder<ShaderRenderPassDesc>()
		.Name("Default")
		.ColorTarget("BackBuffer", g_matSystem->GetBackBufferColorFormat())
		.DepthFormat(g_matSystem->GetBackBufferDepthFormat())
		.VertexLayout(
			Builder<VertexLayoutDesc>()
			.Attribute(VERTEXATTRIB_POSITION, "position", 0, 0, ATTRIBUTEFORMAT_FLOAT, 3)
			.End())
		.End();
}

BEGIN_SHADER_CLASS(Default)

	bool IsSupportVertexFormat(int nameHash) const
	{
		return nameHash == StringToHashConst("DynMeshVertex");
	}

	SHADER_INIT_PARAMS()
	{
		SetParameterFunctor(SHADERPARAM_BASETEXTURE, &ThisShaderClass::SetupBaseTexture);
		SetParameterFunctor(SHADERPARAM_COLOR, &ThisShaderClass::SetColorModulation);
	}

	SHADER_INIT_TEXTURES()
	{
		SHADER_PARAM_TEXTURE_FIND(BaseTexture, m_baseTexture)
	}

	SHADER_INIT_RENDERPASS_PIPELINE()
	{
		if(SHADER_PASS(Unlit))
			return true;

		// begin shader definitions
		SHADERDEFINES_BEGIN;

		// compile without fog
		SHADER_FIND_OR_COMPILE(Unlit, "Default");

		return true;
	}

	SHADER_SETUP_STAGE()
	{
		SHADER_BIND_PASS_SIMPLE(Unlit);
	}

	SHADER_SETUP_CONSTANTS()
	{
		SetupDefaultParameter(SHADERPARAM_TRANSFORM);
		SetupDefaultParameter(SHADERPARAM_BONETRANSFORMS);
		SetupDefaultParameter(SHADERPARAM_BASETEXTURE);
		SetupDefaultParameter(SHADERPARAM_COLOR);
	}

	void SetupBaseTexture(IShaderAPI* renderAPI)
	{
		renderAPI->SetTexture(StringToHashConst("BaseTextureSampler"), m_baseTexture.Get() ? m_baseTexture.Get() : g_matSystem->GetWhiteTexture());
	}

	void SetColorModulation(IShaderAPI* renderAPI)
	{
		ColorRGBA setColor = g_matSystem->GetAmbientColor();
		renderAPI->SetShaderConstant(StringToHashConst("AmbientColor"), setColor);
	}

	SHADER_DECLARE_PASS(Unlit);

	MatTextureProxy m_baseTexture;

END_SHADER_CLASS
