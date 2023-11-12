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
		ref.targets.append(std::move(x)); return *this;
	}
	ThisType& ColorTarget(const char* name, ETextureFormat format)
	{
		ref.targets.append({ name, format }); return *this;
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

//------------------------------------------------------------

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
		// maybe somewhere in BaseShader before SHADER_INIT_RENDERPASS_PIPELINE
		{
			PipelineLayoutDesc pipelineLayoutDesc;
			FillPipelineLayoutDesc(pipelineLayoutDesc);
			m_pipelineLayout = renderAPI->CreatePipelineLayout(pipelineLayoutDesc);

			// prepare basic pipeline descriptor
			RenderPipelineDesc renderPipelineDesc = Builder<RenderPipelineDesc>()
				.ShaderName(GetName())
				.ShaderVertexLayoutName("DynMeshVertex")
				.VertexState(
					Builder<VertexPipelineDesc>()
					.VertexLayout(g_matSystem->GetDynamicMesh()->GetVertexLayoutDesc()[0])
					.End()
				)
				.FragmentState(
					Builder<FragmentPipelineDesc>()
					.ColorTarget("Default", g_matSystem->GetBackBufferColorFormat())
					.End()
				)
				.End();

			FillRenderPipelineDesc(renderPipelineDesc);
			m_renderPipeline = renderAPI->CreateRenderPipeline(m_pipelineLayout, renderPipelineDesc);

			// When drawing with this material:
			//{
			//	IGPUBindGroupPtr currentBindGroup = GetMaterialBindGroup(renderAPI);
			//	
			//}
		}

		if(SHADER_PASS(Unlit))
			return true;

		// begin shader definitions
		SHADERDEFINES_BEGIN;

		// compile without fog
		SHADER_FIND_OR_COMPILE(Unlit, "Default");

		return true;
	}

	IGPURenderPipelinePtr GetRenderPipeline(IShaderAPI* renderAPI) const
	{
		// TODO: for other materials there would
		// be render pass and vertex layout selector
		return m_renderPipeline;
	}

	void FillMaterialBindGroupLayout(BindGroupLayoutDesc& bindGroupLayout) const
	{
		Builder<BindGroupLayoutDesc>(bindGroupLayout)
			.Buffer("materialParams", 0, SHADERKIND_VERTEX | SHADERKIND_FRAGMENT, BUFFERBIND_UNIFORM)
			.Sampler("BaseTextureSampler", 1, SHADERKIND_FRAGMENT, SAMPLERBIND_FILTERING)
			.Texture("BaseTexture", 2, SHADERKIND_FRAGMENT, TEXSAMPLE_FLOAT, TEXDIMENSION_2D)
			.End();
	}

	// this function returns material group.
	// Default material has transient all transient resources 
	// as it's used for immediate drawing
	IGPUBindGroupPtr GetMaterialBindGroup(IShaderAPI* renderAPI) const
	{
		IGPUBindGroupPtr materialBindGroup;
		IGPUBufferPtr materialParamsBuffer;
		{
			ITexturePtr baseTexture = m_baseTexture.Get() ? m_baseTexture.Get() : g_matSystem->GetWhiteTexture();

			// can use either fixed array or CMemoryStream with on-stack storage
			FixedArray<Vector4D, 4> bufferData;
			bufferData.append(g_matSystem->GetAmbientColor());
			bufferData.append(GetTextureTransform(m_baseTextureTransformVar, m_baseTextureScaleVar));

			materialParamsBuffer = renderAPI->CreateBuffer(BufferInfo(bufferData.ptr(), bufferData.numElem()), BUFFERUSAGE_UNIFORM, "materialParams");
			BindGroupDesc shaderBindGroupDesc = Builder<BindGroupDesc>()
				.Buffer(0, materialParamsBuffer, 0, 16)
				.Sampler(1, baseTexture->GetSamplerState())
				.Texture(2, baseTexture)
				.End();
			materialBindGroup = renderAPI->CreateBindGroup(m_pipelineLayout, 1, shaderBindGroupDesc);
		}
		return materialBindGroup;
	}

	// those are persistent resources
	IGPURenderPipelinePtr	m_renderPipeline;
	IGPUPipelineLayoutPtr	m_pipelineLayout;
	MatTextureProxy			m_baseTexture;

	//------------------------------------------------------------------
	// DEPRECATED all below

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

	
END_SHADER_CLASS
