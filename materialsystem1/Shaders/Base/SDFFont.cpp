//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Signed Distance Field font
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "IMaterialSystem.h"
#include "IDynamicMesh.h"
#include "BaseShader.h"

static uint GenDefaultPipelineId(const MatSysDefaultRenderPass& renderPass)
{
	uint id = 0;
	id |= renderPass.cullMode; // 2 bits
	id |= renderPass.primitiveTopology << 2;	// 3 bits
	id |= renderPass.blendMode << (2 + 3); // 2 bits
	id |= renderPass.depthFunc << (2 + 3 + 2); // 1 bit
	id |= renderPass.depthTest << (2 + 3 + 2 + 1); // 1 bit
	id |= renderPass.depthWrite << (2 + 3 + 2 + 1 + 1); // 4 bits
	return id;
}

BEGIN_SHADER_CLASS(SDFFont)
	bool IsSupportVertexFormat(int nameHash) const
	{
		return nameHash == StringToHashConst("DynMeshVertex");
	}

	SHADER_INIT_PARAMS()
	{
		m_flags |= MATERIAL_FLAG_NO_Z_TEST;
		m_fontParamsVar = m_material->GetMaterialVar("FontParams", "[0.94 0.95, 0, 1]");
		SetParameterFunctor(SHADERPARAM_COLOR, &ThisShaderClass::SetColorModulation);
		SetParameterFunctor(SHADERPARAM_BASETEXTURE, &ThisShaderClass::SetupBaseTexture);
	}

	SHADER_INIT_TEXTURES()
	{
		SHADER_PARAM_TEXTURE_FIND(BaseTexture, m_baseTexture)
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

		PipelineLayoutDesc pipelineLayoutDesc;
		FillPipelineLayoutDesc(pipelineLayoutDesc);
		m_pipelineLayout = renderAPI->CreatePipelineLayout(pipelineLayoutDesc);

		// begin shader definitions
		SHADERDEFINES_BEGIN;

		// compile without fog
		SHADER_FIND_OR_COMPILE(Unlit, "SDFFont");

		return true;
	}

	IGPURenderPipelinePtr GetRenderPipeline(IShaderAPI* renderAPI, const void* userData) const
	{
		const MatSysDefaultRenderPass* rendPassInfo = reinterpret_cast<const MatSysDefaultRenderPass*>(userData);
		ASSERT_MSG(rendPassInfo, "Must specify MatSysDefaultRenderPass in userData when drawing with SDFFont material");
		const uint pipelineId = GenDefaultPipelineId(*rendPassInfo);
		auto it = m_renderPipelines.find(pipelineId);

		if (it.atEnd())
		{
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
					.ColorTarget("Default", g_matSystem->GetCurrentBackbuffer()->GetFormat())
					.End()
				)
				.End();

			// TODO: depth state!
			{
				if (m_flags & MATERIAL_FLAG_DECAL)
					g_matSystem->GetPolyOffsetSettings(renderPipelineDesc.depthStencil.depthBias, renderPipelineDesc.depthStencil.depthBiasSlopeScale);

				// TODO: must be selective whenever Z test is on
				//Builder<DepthStencilStateParams>(renderPipelineDesc.depthStencil)
				//	.DepthTestOn()
				//	.DepthWriteOn((m_flags & MATERIAL_FLAG_NO_Z_WRITE) == 0)
				//	.DepthFormat(g_matSystem->GetDefaultDepthBuffer()->GetFormat());
			}
			{
				BlendStateParams colorBlend;
				BlendStateParams alphaBlend;
				switch (rendPassInfo->blendMode)
				{
				case SHADER_BLEND_TRANSLUCENT:
					colorBlend = BlendStateTranslucent;
					alphaBlend = BlendStateTranslucentAlpha;
					break;
				case SHADER_BLEND_ADDITIVE:
					colorBlend = BlendStateAdditive;
					alphaBlend = BlendStateAdditive;
					break;
				case SHADER_BLEND_MODULATE:
					colorBlend = BlendStateModulate;
					alphaBlend = BlendStateModulate; // TODO: figure out
					break;
				}

				// FIXME: apply differently?
				for (FragmentPipelineDesc::ColorTargetDesc& colorTarget : renderPipelineDesc.fragment.targets)
				{
					colorTarget.alphaBlend = alphaBlend;
					colorTarget.colorBlend = colorBlend;
				}
			}

			Builder<PrimitiveDesc>(renderPipelineDesc.primitive)
				.Topology(rendPassInfo->primitiveTopology)
				.Cull(rendPassInfo->cullMode)
				.StripIndex(rendPassInfo->primitiveTopology == PRIM_TRIANGLE_STRIP ? STRIPINDEX_UINT16 : STRIPINDEX_NONE)
				.End();
			
			IGPURenderPipelinePtr renderPipeline = renderAPI->CreateRenderPipeline(m_pipelineLayout, renderPipelineDesc);
			it = m_renderPipelines.insert(pipelineId, renderPipeline);
		}
		return *it;
	}

	IGPUPipelineLayoutPtr GetPipelineLayout() const
	{
		return m_pipelineLayout;
	}

	IGPUBindGroupPtr GetMaterialBindGroup(IShaderAPI* renderAPI, const void* userData) const
	{
		const MatSysDefaultRenderPass* rendPassInfo = reinterpret_cast<const MatSysDefaultRenderPass*>(userData);
		ASSERT_MSG(rendPassInfo, "Must specify MatSysDefaultRenderPass in userData when drawing with SDFFont material");

		IGPUBindGroupPtr materialBindGroup;
		IGPUBufferPtr materialParamsBuffer;
		{
			ITexturePtr baseTexture = rendPassInfo->texture ? rendPassInfo->texture : g_matSystem->GetWhiteTexture();

			// can use either fixed array or CMemoryStream with on-stack storage
			FixedArray<Vector4D, 4> bufferData;
			bufferData.append(g_matSystem->GetAmbientColor()); // TODO ???
			bufferData.append(m_fontParamsVar.Get());

			materialParamsBuffer = renderAPI->CreateBuffer(BufferInfo(bufferData.ptr(), bufferData.numElem()), BUFFERUSAGE_UNIFORM, "materialParams");
			BindGroupDesc shaderBindGroupDesc = Builder<BindGroupDesc>()
				.Buffer(0, materialParamsBuffer, 0, bufferData.numElem() * sizeof(bufferData[0]))
				.Sampler(1, baseTexture->GetSamplerState())
				.Texture(2, baseTexture)
				.End();
			materialBindGroup = renderAPI->CreateBindGroup(m_pipelineLayout, 1, shaderBindGroupDesc);
		}
		return materialBindGroup;
	}

	mutable Map<uint, IGPURenderPipelinePtr>	m_renderPipelines{ PP_SL };
	IGPUPipelineLayoutPtr	m_pipelineLayout;

	SHADER_SETUP_STAGE()
	{
		SHADER_BIND_PASS_SIMPLE(Unlit);
	}

	SHADER_SETUP_CONSTANTS()
	{
		SetupDefaultParameter(SHADERPARAM_TRANSFORM);
		SetupDefaultParameter(SHADERPARAM_BASETEXTURE);

		SetupDefaultParameter(SHADERPARAM_COLOR);

		renderAPI->SetShaderConstant(StringToHashConst("FontParams"), m_fontParamsVar.Get());
	}

	void SetupBaseTexture(IShaderAPI* renderAPI)
	{
		renderAPI->SetTexture(StringToHashConst("BaseTextureSampler"), m_baseTexture.Get());
	}

	void SetColorModulation(IShaderAPI* renderAPI)
	{
		ColorRGBA setColor = g_matSystem->GetAmbientColor();
		renderAPI->SetShaderConstant(StringToHashConst("AmbientColor"), setColor);
	}

	SHADER_DECLARE_PASS(Unlit);

	MatTextureProxy	m_baseTexture;
	MatVec4Proxy	m_fontParamsVar;
END_SHADER_CLASS