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

static uint GenDefaultPipelineId(const IGPURenderPassRecorder* renderPass, const MatSysDefaultRenderPass& renderPassInfo, EPrimTopology primitiveTopology)
{
	uint id = 0;
	id |= renderPassInfo.cullMode; // 2 bits
	id |= primitiveTopology << 2;	// 3 bits
	id |= renderPassInfo.blendMode << (2 + 3); // 2 bits
	id |= renderPassInfo.depthFunc << (2 + 3 + 2); // 1 bit
	id |= renderPassInfo.depthTest << (2 + 3 + 2 + 1); // 1 bit
	id |= renderPassInfo.depthWrite << (2 + 3 + 2 + 1 + 1); // 4 bits
	id |= (GetTexFormat(renderPass->GetRenderTargetFormat(0)) & 63) << (2 + 3 + 2 + 1 + 1 + 4);
	id |= (renderPass->GetRenderTargetFormat(0) >> 10 & 3) << (2 + 3 + 2 + 1 + 1 + 4 + 6);
	id |= (renderPass->GetDepthTargetFormat() & 63) << (2 + 3 + 2 + 1 + 1 + 4 + 6 + 2);
	return id;
}

BEGIN_SHADER_CLASS(SDFFont)
	SHADER_INIT_PARAMS()
	{
		m_flags |= MATERIAL_FLAG_NO_Z_TEST;
		m_fontParamsVar = m_material->GetMaterialVar("FontParams", "[0.94 0.95, 0, 1]");
	}

	SHADER_INIT_TEXTURES()
	{
		SHADER_PARAM_TEXTURE_FIND(BaseTexture, m_baseTexture)
	}

	bool IsSupportInstanceFormat(int nameHash) const
	{
		return nameHash == StringToHashConst("DynMeshVertex");
	}

	IGPURenderPipelinePtr GetRenderPipeline(IShaderAPI* renderAPI, const IGPURenderPassRecorder* renderPass, const MeshInstanceFormatRef& meshInstFormat, int vertexLayoutUsedBufferBits, EPrimTopology primitiveTopology, const void* userData) const override
	{
		const MatSysDefaultRenderPass* rendPassInfo = reinterpret_cast<const MatSysDefaultRenderPass*>(userData);
		ASSERT_MSG(rendPassInfo, "Must specify MatSysDefaultRenderPass in userData when drawing with SDFFont material");
		const uint pipelineId = GenDefaultPipelineId(renderPass, *rendPassInfo, primitiveTopology);
		auto it = m_renderPipelines.find(pipelineId);

		if (it.atEnd())
		{
			// prepare basic pipeline descriptor
			RenderPipelineDesc renderPipelineDesc = Builder<RenderPipelineDesc>()
				.ShaderName(GetName())
				.ShaderVertexLayoutId(meshInstFormat.nameHash)
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
				.Topology(primitiveTopology)
				.Cull(rendPassInfo->cullMode)
				.StripIndex(primitiveTopology == PRIM_TRIANGLE_STRIP ? STRIPINDEX_UINT16 : STRIPINDEX_NONE)
				.End();
			
			IGPURenderPipelinePtr renderPipeline = renderAPI->CreateRenderPipeline(GetPipelineLayout(renderAPI), renderPipelineDesc);
			it = m_renderPipelines.insert(pipelineId, renderPipeline);
		}
		return *it;
	}

	void FillBindGroupLayout_Transient(BindGroupLayoutDesc& bindGroupLayout) const
	{
		Builder<BindGroupLayoutDesc>(bindGroupLayout)
			.Buffer("cameraParams", 0, SHADERKIND_VERTEX | SHADERKIND_FRAGMENT, BUFFERBIND_UNIFORM)
			.Buffer("materialParams", 1, SHADERKIND_VERTEX | SHADERKIND_FRAGMENT, BUFFERBIND_UNIFORM)
			.Sampler("BaseTextureSampler", 2, SHADERKIND_FRAGMENT, SAMPLERBIND_FILTERING)
			.Texture("BaseTexture", 3, SHADERKIND_FRAGMENT, TEXSAMPLE_FLOAT, TEXDIMENSION_2D)
			.End();
	}

	IGPUBindGroupPtr GetBindGroup(uint frameIdx, EBindGroupId bindGroupId, IShaderAPI* renderAPI, IGPURenderPassRecorder* rendPassRecorder, const void* userData) const
	{
		if (bindGroupId == BINDGROUP_TRANSIENT)
		{
			const MatSysDefaultRenderPass* rendPassInfo = reinterpret_cast<const MatSysDefaultRenderPass*>(userData);
			ASSERT_MSG(rendPassInfo, "Must specify MatSysDefaultRenderPass in userData when drawing with SDFFont material");

			ITexturePtr baseTexture = rendPassInfo->texture ? rendPassInfo->texture : g_matSystem->GetWhiteTexture();

			// can use either fixed array or CMemoryStream with on-stack storage
			FixedArray<Vector4D, 4> bufferData;
			bufferData.append(g_matSystem->GetAmbientColor()); // TODO ???
			bufferData.append(m_fontParamsVar.Get());

			MatSysCamera cameraParams;
			g_matSystem->GetCameraParams(cameraParams, true);

			IGPUBufferPtr cameraParamsBuffer = renderAPI->CreateBuffer(BufferInfo(&cameraParams, 1), BUFFERUSAGE_UNIFORM, "matSysCamera");
			IGPUBufferPtr materialParamsBuffer = renderAPI->CreateBuffer(BufferInfo(bufferData.ptr(), bufferData.numElem()), BUFFERUSAGE_UNIFORM, "materialParams");
			BindGroupDesc shaderBindGroupDesc = Builder<BindGroupDesc>()
				.Buffer(0, cameraParamsBuffer)
				.Buffer(1, materialParamsBuffer)
				.Sampler(2, baseTexture->GetSamplerState())
				.Texture(3, baseTexture)
				.End();
			IGPUBindGroupPtr materialBindGroup = renderAPI->CreateBindGroup(GetPipelineLayout(renderAPI), bindGroupId, shaderBindGroupDesc);
			return materialBindGroup;
		}

		return GetEmptyBindGroup(bindGroupId, renderAPI);
	}

	mutable Map<uint, IGPURenderPipelinePtr>	m_renderPipelines{ PP_SL };
	MatTextureProxy		m_baseTexture;
	MatVec4Proxy		m_fontParamsVar;
END_SHADER_CLASS