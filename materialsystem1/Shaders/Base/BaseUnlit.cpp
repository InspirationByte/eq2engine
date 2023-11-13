//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Unlit Shader with fog support
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "IMaterialSystem.h"
#include "BaseShader.h"
#include "IDynamicMesh.h"

BEGIN_SHADER_CLASS(BaseUnlit)

	bool IsSupportVertexFormat(int nameHash) const
	{
		// must support any vertex
		return true;
	}

	SHADER_INIT_PARAMS()
	{
		m_colorVar = m_material->GetMaterialVar("color", "[1 1 1 1]");
	}

	SHADER_INIT_TEXTURES()
	{
		// parse material variables
		SHADER_PARAM_TEXTURE_NOERROR(BaseTexture, m_baseTexture);
	}

	void FillMaterialBindGroupLayout(BindGroupLayoutDesc& bindGroupLayout) const
	{
		Builder<BindGroupLayoutDesc>(bindGroupLayout)
			.Buffer("materialParams", 0, SHADERKIND_VERTEX | SHADERKIND_FRAGMENT, BUFFERBIND_UNIFORM)
			.Sampler("BaseTextureSampler", 1, SHADERKIND_FRAGMENT, SAMPLERBIND_FILTERING)
			.Texture("BaseTexture", 2, SHADERKIND_FRAGMENT, TEXSAMPLE_FLOAT, TEXDIMENSION_2D)
			.End();
	}

	SHADER_INIT_RENDERPASS_PIPELINE()
	{
		if(SHADER_PASS(Unlit))
			return true;

		{
			PipelineLayoutDesc pipelineLayoutDesc;
			FillPipelineLayoutDesc(pipelineLayoutDesc);
			m_pipelineLayout = renderAPI->CreatePipelineLayout(pipelineLayoutDesc);
		}

		{
			FixedArray<Vector4D, 4> bufferData;
			bufferData.append(m_colorVar.Get() /* * g_matSystem->GetAmbientColor()*/);
			bufferData.append(GetTextureTransform(m_baseTextureTransformVar, m_baseTextureScaleVar));
			m_materialParamsBuffer = renderAPI->CreateBuffer(BufferInfo(bufferData.ptr(), bufferData.numElem()), BUFFERUSAGE_UNIFORM, "materialParams");
		}

		{
			ITexturePtr baseTexture = m_baseTexture.Get() ? m_baseTexture.Get() : g_matSystem->GetWhiteTexture();
			BindGroupDesc shaderBindGroupDesc = Builder<BindGroupDesc>()
				.Buffer(0, m_materialParamsBuffer, 0, m_materialParamsBuffer->GetSize())
				.Sampler(1, baseTexture->GetSamplerState())
				.Texture(2, baseTexture)
				.End();
			m_materialBindGroup = renderAPI->CreateBindGroup(m_pipelineLayout, 1, shaderBindGroupDesc);
		}

		{
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
					.ColorTarget("Default", g_matSystem->GetCurrentBackbuffer()->GetFormat())
					.End()
				)
				.End();

			FillRenderPipelineDesc(renderPipelineDesc);

			Array<EqString>& shaderQuery = renderPipelineDesc.shaderQuery;

			bool vertexColor = false;
			SHADER_PARAM_BOOL(VertexColor, vertexColor, false);
			if (vertexColor)
				shaderQuery.append("VERTEXCOLOR");

			if (m_flags & MATERIAL_FLAG_ALPHATESTED)
				shaderQuery.append("ALPHATEST");

			EPrimTopology primitiveTopology = PRIM_TRIANGLE_STRIP;

			Builder<PrimitiveDesc>(renderPipelineDesc.primitive)
				.Topology(primitiveTopology)
				.Cull((m_flags & MATERIAL_FLAG_NO_CULL) ? CULL_NONE : CULL_BACK) // TODO: variant
				.StripIndex(primitiveTopology == PRIM_TRIANGLE_STRIP ? STRIPINDEX_UINT16 : STRIPINDEX_NONE) // TODO: variant
				.End();

			m_renderPipelines[0] = renderAPI->CreateRenderPipeline(m_pipelineLayout, renderPipelineDesc);
		}

		// m_renderPipelines = ;

		bool fogEnable = true;
		SHADER_PARAM_BOOL_NEG(NoFog, fogEnable, false)

		// begin shader definitions
		SHADERDEFINES_BEGIN;

		// alphatesting
		SHADER_DECLARE_SIMPLE_DEFINITION((m_flags & MATERIAL_FLAG_ALPHATESTED), "ALPHATEST");

		bool bVertexColor = false;
		SHADER_PARAM_BOOL(StudioVertexColor, bVertexColor, false);
		SHADER_DECLARE_SIMPLE_DEFINITION(bVertexColor, "STUDIOVERTEXCOLOR");

		// compile without fog
		{
			SHADER_FIND_OR_COMPILE(Unlit_noskin, "BaseUnlit");
		}
		
		{
			EqString oldDefines = defines;
			EqString oldFindQuery = findQuery;

			SHADER_DECLARE_SIMPLE_DEFINITION(true, "SKIN");
			SHADER_FIND_OR_COMPILE(Unlit, "BaseUnlit");

			defines = oldDefines;
			findQuery = oldFindQuery;
		}

		// define fog parameter.
		SHADER_DECLARE_SIMPLE_DEFINITION(fogEnable, "DOFOG");

		// compile with fog
		{
			SHADER_FIND_OR_COMPILE(Unlit_noskin_fog, "BaseUnlit");
		}

		{
			SHADER_DECLARE_SIMPLE_DEFINITION(true, "SKIN");
			SHADER_FIND_OR_COMPILE(Unlit_fog, "BaseUnlit");
		}

		// set texture setup
		if (m_baseTexture.IsValid())
			SetParameterFunctor(SHADERPARAM_BASETEXTURE, &ThisShaderClass::SetupBaseTexture0);

		SetParameterFunctor(SHADERPARAM_COLOR, &ThisShaderClass::SetColorModulation);

		return true;
	}

	IGPUPipelineLayoutPtr GetPipelineLayout() const
	{
		return m_pipelineLayout;
	}

	IGPURenderPipelinePtr GetRenderPipeline(IShaderAPI* renderAPI, EPrimTopology primitiveTopology, const void* userData) const
	{
		return m_renderPipelines[0];
	}

	IGPUBindGroupPtr GetMaterialBindGroup(IShaderAPI* renderAPI, const void* userData) const
	{
		return m_materialBindGroup;
	}

	IGPURenderPipelinePtr	m_renderPipelines[2];
	IGPUPipelineLayoutPtr	m_pipelineLayout;
	IGPUBufferPtr			m_materialParamsBuffer;
	IGPUBindGroupPtr		m_materialBindGroup;

	SHADER_SETUP_STAGE()
	{
		if (g_matSystem->IsSkinningEnabled())
		{
			SHADER_BIND_PASS_FOGSELECT(Unlit)
		}
		else
		{
			SHADER_BIND_PASS_FOGSELECT(Unlit_noskin)
		}
	}

	SHADER_SETUP_CONSTANTS()
	{
		SetupDefaultParameter(SHADERPARAM_TRANSFORM);
		SetupDefaultParameter(SHADERPARAM_BONETRANSFORMS);

		SetupDefaultParameter(SHADERPARAM_BASETEXTURE);

		SetupDefaultParameter(SHADERPARAM_ALPHASETUP);
		SetupDefaultParameter(SHADERPARAM_DEPTHSETUP);
		SetupDefaultParameter(SHADERPARAM_RASTERSETUP);
		SetupDefaultParameter(SHADERPARAM_COLOR);
		SetupDefaultParameter(SHADERPARAM_FOG);
	}

	void SetColorModulation(IShaderAPI* renderAPI)
	{
		ColorRGBA setColor = m_colorVar.Get() * g_matSystem->GetAmbientColor();

		renderAPI->SetShaderConstant(StringToHashConst("AmbientColor"), setColor);
	}

	void SetupBaseTexture0(IShaderAPI* renderAPI)
	{
		ITexturePtr setupTexture = g_matSystem->GetConfiguration().wireframeMode ? g_matSystem->GetWhiteTexture() : m_baseTexture.Get();

		renderAPI->SetTexture(StringToHashConst("BaseTextureSampler"), setupTexture);
	}

	const ITexturePtr& GetBaseTexture(int stage) const {return m_baseTexture.Get();}

	MatTextureProxy	m_baseTexture;
	MatVec4Proxy	m_colorVar;

	SHADER_DECLARE_PASS(Unlit);
	SHADER_DECLARE_FOGPASS(Unlit);

	SHADER_DECLARE_PASS(Unlit_noskin);
	SHADER_DECLARE_FOGPASS(Unlit_noskin);

END_SHADER_CLASS
