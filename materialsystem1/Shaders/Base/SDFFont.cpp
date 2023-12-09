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

static uint GenDefaultPipelineId(ArrayCRef<ETextureFormat> colorTargetFormat, ETextureFormat depthTargetFormat, const MatSysDefaultRenderPass& renderPassInfo, EPrimTopology primitiveTopology)
{
	uint id = 0;
	id |= renderPassInfo.cullMode; // 2 bits
	id |= primitiveTopology << 2;	// 3 bits
	id |= renderPassInfo.blendMode << (2 + 3); // 2 bits
	id |= renderPassInfo.depthFunc << (2 + 3 + 2); // 1 bit
	id |= renderPassInfo.depthTest << (2 + 3 + 2 + 1); // 1 bit
	id |= renderPassInfo.depthWrite << (2 + 3 + 2 + 1 + 1); // 4 bits
	id |= (GetTexFormat(colorTargetFormat[0]) & 63) << (2 + 3 + 2 + 1 + 1 + 4);
	id |= (colorTargetFormat[0] >> 10 & 3) << (2 + 3 + 2 + 1 + 1 + 4 + 6);
	id |= (depthTargetFormat & 63) << (2 + 3 + 2 + 1 + 1 + 4 + 6 + 2);
	return id;
}

BEGIN_SHADER_CLASS(SDFFont)
	SHADER_INIT_PARAMS()
	{
		m_flags |= MATERIAL_FLAG_NO_Z_TEST;
		m_fontParamsVar = m_material->GetMaterialVar("FontParams", "[0.94 0.95 0, 1]");
		m_fontBaseColor = m_material->GetMaterialVar("FontBaseColor", "[1 1 0 1]");
	}

	SHADER_INIT_TEXTURES()
	{
		SHADER_PARAM_TEXTURE_FIND(BaseTexture, m_baseTexture)
	}

	ArrayCRef<int> GetSupportedVertexLayoutIds() const
	{
		static const int supportedFormats[] = {
			StringToHashConst("DynMeshVertex")
		};
		return ArrayCRef(supportedFormats);
	}

	bool SetupRenderPass(IShaderAPI* renderAPI, const MeshInstanceFormatRef& meshInstFormat, EPrimTopology primTopology, ArrayCRef<RenderBufferInfo> uniformBuffers, const RenderPassContext& passContext) override
	{
		const MatSysDefaultRenderPass* rendPassInfo = static_cast<const MatSysDefaultRenderPass*>(passContext.data);
		ASSERT_MSG(rendPassInfo, "Must specify MatSysDefaultRenderPass in userData when drawing with default material");
		const uint pipelineId = GenDefaultPipelineId(passContext.recorder->GetRenderTargetFormats(), passContext.recorder->GetDepthTargetFormat(), *rendPassInfo, primTopology);

		auto it = m_renderPipelines.find(pipelineId);
		if (it.atEnd())
		{
			// prepare basic pipeline descriptor
			RenderPipelineDesc renderPipelineDesc = Builder<RenderPipelineDesc>()
				.ShaderName(GetName())
				.ShaderVertexLayoutId(meshInstFormat.formatId)
				.End();

			for (const VertexLayoutDesc& layoutDesc : meshInstFormat.layout)
				renderPipelineDesc.vertex.vertexLayout.append(layoutDesc);

			const ETextureFormat depthTargetFormat = passContext.recorder->GetDepthTargetFormat();
			if (depthTargetFormat != FORMAT_NONE)
			{
				if (m_flags & MATERIAL_FLAG_DECAL)
					g_matSystem->GetPolyOffsetSettings(renderPipelineDesc.depthStencil.depthBias, renderPipelineDesc.depthStencil.depthBiasSlopeScale);

				// TODO: must be selective whenever Z test is on
				Builder<DepthStencilStateParams>(renderPipelineDesc.depthStencil)
					.DepthTestOn()
					.DepthWriteOn((m_flags & MATERIAL_FLAG_NO_Z_WRITE) == 0)
					.DepthFormat(depthTargetFormat);
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

				Builder<FragmentPipelineDesc> pipelineBuilder(renderPipelineDesc.fragment);
				for (int i = 0; i < MAX_RENDERTARGETS; ++i)
				{
					const ETextureFormat format = passContext.recorder->GetRenderTargetFormats()[i];
					if (format == FORMAT_NONE)
						break;

					if (rendPassInfo->blendMode != SHADER_BLEND_NONE)
						pipelineBuilder.ColorTarget("CT", format, colorBlend, alphaBlend);
					else
						pipelineBuilder.ColorTarget("CT", format);
				}
				pipelineBuilder.End();
			}

			Builder<PrimitiveDesc>(renderPipelineDesc.primitive)
				.Topology(primTopology)
				.Cull(rendPassInfo->cullMode)
				.StripIndex(primTopology == PRIM_TRIANGLE_STRIP ? STRIPINDEX_UINT16 : STRIPINDEX_NONE)
				.End();

			it = m_renderPipelines.insert(pipelineId);
			PipelineInfo& newPipelineInfo = *it;
			newPipelineInfo.vertexLayoutId = meshInstFormat.formatId;

			newPipelineInfo.pipeline = renderAPI->CreateRenderPipeline(renderPipelineDesc);
		}

		const PipelineInfo& pipelineInfo = *it;

		if (!pipelineInfo.pipeline)
			return false;

		passContext.recorder->SetPipeline(pipelineInfo.pipeline);
		passContext.recorder->SetBindGroup(BINDGROUP_CONSTANT, GetBindGroup(renderAPI, BINDGROUP_CONSTANT, pipelineInfo, uniformBuffers, passContext));
		return true;
	}

	IGPUBindGroupPtr GetBindGroup(IShaderAPI* renderAPI, EBindGroupId bindGroupId, const PipelineInfo& pipelineInfo, ArrayCRef<RenderBufferInfo> uniformBuffers, const RenderPassContext& passContext) const
	{
		if (bindGroupId == BINDGROUP_CONSTANT)
		{
			const MatSysDefaultRenderPass* rendPassInfo = static_cast<const MatSysDefaultRenderPass*>(passContext.data);
			ASSERT_MSG(rendPassInfo, "Must specify MatSysDefaultRenderPass in userData when drawing with SDFFont material");

			TextureView whiteTexView = g_matSystem->GetWhiteTexture();
			const TextureView& baseTexture = rendPassInfo->texture ? rendPassInfo->texture : whiteTexView;

			// can use either fixed array or CMemoryStream with on-stack storage
			FixedArray<Vector4D, 4> bufferData;
			bufferData.append(m_fontBaseColor.Get());
			bufferData.append(m_fontParamsVar.Get());

			MatSysCamera cameraParams;
			g_matSystem->GetCameraParams(cameraParams, true);

			GPUBufferView cameraParamsBuffer = g_matSystem->GetTransientUniformBuffer(&cameraParams, sizeof(cameraParams));
			GPUBufferView materialParamsBuffer = g_matSystem->GetTransientUniformBuffer(bufferData.ptr(), sizeof(bufferData[0]) * bufferData.numElem());

			BindGroupDesc bindGroupDesc = Builder<BindGroupDesc>()
				.Buffer(0, cameraParamsBuffer)
				.Buffer(1, materialParamsBuffer)
				.Sampler(2, baseTexture.texture->GetSamplerState())
				.Texture(3, baseTexture)
				.End();
			IGPUBindGroupPtr materialBindGroup = CreateBindGroup(bindGroupDesc, bindGroupId, renderAPI, pipelineInfo);
			return materialBindGroup;
		}

		return GetEmptyBindGroup(renderAPI, bindGroupId, pipelineInfo);
	}

	MatTextureProxy		m_baseTexture;
	MatVec4Proxy		m_fontBaseColor;
	MatVec4Proxy		m_fontParamsVar;
END_SHADER_CLASS