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
	const uint pipelineFlags =
		static_cast<uint>(renderPassInfo.cullMode)
		| (renderPassInfo.depthTest << 2)
		| (renderPassInfo.depthWrite << 3)
		| (renderPassInfo.depthFunc << 4)
		| (static_cast<uint>(primitiveTopology) << 7)
		| (renderPassInfo.blendMode << 10);

	uint hash = pipelineFlags;
	hash *= 31;
	hash += pipelineFlags;
	for (int i = 0; i < colorTargetFormat.numElem(); ++i)
	{
		const ETextureFormat format = colorTargetFormat[i];
		if (format == FORMAT_NONE)
			break;
		hash *= 31;
		hash += static_cast<uint>(i);
		hash *= 31;
		hash += static_cast<uint>(format);
	}
	hash *= 31;
	hash += static_cast<uint>(depthTargetFormat);
	return hash;
}

BEGIN_SHADER_CLASS(
	SDFFont,
	SHADER_VERTEX_ID(DynMeshVertex)
)
	SHADER_INIT_PARAMS()
	{
		m_flags |= MATERIAL_FLAG_NO_Z_TEST;
		m_fontParamsVar = m_material->GetMaterialVar("FontParams", "[0.94 0.95 0, 1]");
		m_fontBaseColor = m_material->GetMaterialVar("FontBaseColor", "[1 1 0 1]");

		m_shadowParamsVar = m_material->GetMaterialVar("ShadowParams", "[0.94 0.95 0, 1]");
		m_shadowColor = m_material->GetMaterialVar("ShadowColor", "[0 0 0 1]");
		m_shadowOffset = m_material->GetMaterialVar("ShadowOffset", "[0 0]");
	}

	SHADER_INIT_TEXTURES()
	{
		SHADER_PARAM_TEXTURE_FIND(BaseTexture, m_baseTexture)
	}

	bool SetupRenderPass(IShaderAPI* renderAPI, const PipelineInputParams& pipelineParams, ArrayCRef<RenderBufferInfo> uniformBuffers, const RenderPassContext& passContext, IMaterial* originalMaterial) override
	{
		const MatSysDefaultRenderPass* rendPassInfo = static_cast<const MatSysDefaultRenderPass*>(passContext.data);
		ASSERT_MSG(rendPassInfo, "Must specify MatSysDefaultRenderPass in userData when drawing with default material");
		const uint pipelineId = GenDefaultPipelineId(passContext.recorder->GetRenderTargetFormats(), passContext.recorder->GetDepthTargetFormat(), *rendPassInfo, pipelineParams.primitiveTopology);

		auto it = m_renderPipelines.find(pipelineId);
		if (it.atEnd())
		{
			// prepare basic pipeline descriptor
			RenderPipelineDesc renderPipelineDesc = Builder<RenderPipelineDesc>()
				.ShaderName(GetName())
				.ShaderVertexLayoutId(pipelineParams.meshInstFormat.formatId)
				.End();

			for (const VertexLayoutDesc& layoutDesc : pipelineParams.meshInstFormat.layout)
				renderPipelineDesc.vertex.vertexLayout.append(layoutDesc);

			const ETextureFormat depthTargetFormat = passContext.recorder->GetDepthTargetFormat();
			if (depthTargetFormat != FORMAT_NONE)
			{
				if (m_flags & MATERIAL_FLAG_DECAL)
					g_matSystem->GetPolyOffsetSettings(renderPipelineDesc.depthStencil.depthBias, renderPipelineDesc.depthStencil.depthBiasSlopeScale);

				// TODO: must be selective whenever Z test is on
				Builder<DepthStencilStateParams>(renderPipelineDesc.depthStencil)
					.DepthTestOn(rendPassInfo->depthTest)
					.DepthFunction(rendPassInfo->depthFunc)
					.DepthWriteOn(rendPassInfo->depthWrite && (m_flags & MATERIAL_FLAG_NO_Z_WRITE) == 0)
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
				.Topology(pipelineParams.primitiveTopology)
				.Cull(rendPassInfo->cullMode)
				.StripIndex(pipelineParams.primitiveTopology == PRIM_TRIANGLE_STRIP ? STRIPINDEX_UINT16 : STRIPINDEX_NONE)
				.End();

			it = m_renderPipelines.insert(pipelineId);
			PipelineInfo& newPipelineInfo = *it;
			newPipelineInfo.vertexLayoutId = pipelineParams.meshInstFormat.formatId;

			newPipelineInfo.pipeline = renderAPI->CreateRenderPipeline(renderPipelineDesc);
		}

		const PipelineInfo& pipelineInfo = *it;

		if (!pipelineInfo.pipeline)
			return false;

		const BindGroupSetupParams setupParams {
			uniformBuffers,
			pipelineInfo,
			passContext,
			originalMaterial
		};

		passContext.recorder->SetPipeline(pipelineInfo.pipeline);
		passContext.recorder->SetBindGroup(BINDGROUP_CONSTANT, GetBindGroup(renderAPI, BINDGROUP_CONSTANT, setupParams));
		return true;
	}

	IGPUBindGroupPtr GetBindGroup(IShaderAPI* renderAPI, EBindGroupId bindGroupId, const BindGroupSetupParams& setupParams) const
	{
		if (bindGroupId == BINDGROUP_CONSTANT)
		{
			const MatSysDefaultRenderPass* rendPassInfo = static_cast<const MatSysDefaultRenderPass*>(setupParams.passContext.data);
			ASSERT_MSG(rendPassInfo, "Must specify MatSysDefaultRenderPass in userData when drawing with SDFFont material");

			TextureView whiteTexView = g_matSystem->GetWhiteTexture();
			const TextureView& baseTexture = rendPassInfo->texture ? rendPassInfo->texture : whiteTexView;

			// can use either fixed array or CMemoryStream with on-stack storage
			FixedArray<Vector4D, 8> bufferData;
			bufferData.append(m_fontBaseColor.Get());
			bufferData.append(m_fontParamsVar.Get());
			bufferData.append(m_shadowColor.Get());
			bufferData.append(m_shadowParamsVar.Get());
			bufferData.append(Vector4D(m_shadowOffset.Get(), 0.0f));

			GPUBufferView cameraParamsBuffer;
			for (const RenderBufferInfo& rendBuffer : setupParams.uniformBuffers)
			{
				if (rendBuffer.signature == s_matSysCameraBufferId)
					cameraParamsBuffer = rendBuffer.bufferView;
			}

			if(!cameraParamsBuffer)
			{
				cameraParamsBuffer = m_currentCameraBuffer;

				MatSysCamera cameraParams;
				const int cameraChangeId = g_matSystem->GetCameraParams(cameraParams);
				if (m_currentCameraId != cameraChangeId)
				{
					m_currentCameraId = cameraChangeId;
					m_currentCameraBuffer = g_matSystem->GetTransientUniformBuffer(&cameraParams, sizeof(cameraParams));
					cameraParamsBuffer = m_currentCameraBuffer;
				}
			}

			GPUBufferView materialParamsBuffer = g_matSystem->GetTransientUniformBuffer(bufferData.ptr(), sizeof(bufferData[0]) * bufferData.numElem());
			BindGroupDesc bindGroupDesc = Builder<BindGroupDesc>()
				.Buffer(0, cameraParamsBuffer)
				.Buffer(1, materialParamsBuffer)
				.Sampler(2, baseTexture.texture->GetSamplerState())
				.Texture(3, baseTexture)
				.End();
			IGPUBindGroupPtr materialBindGroup = CreateBindGroup(bindGroupDesc, bindGroupId, renderAPI, setupParams.pipelineInfo);
			return materialBindGroup;
		}

		return GetEmptyBindGroup(renderAPI, bindGroupId, setupParams.pipelineInfo);
	}

	mutable GPUBufferView	m_currentCameraBuffer;
	mutable int				m_currentCameraId{ -1 };

	MatTextureProxy		m_baseTexture;
	MatVec4Proxy		m_fontBaseColor;
	MatVec4Proxy		m_fontParamsVar;

	MatVec4Proxy		m_shadowColor;
	MatVec4Proxy		m_shadowParamsVar;
	MatVec2Proxy		m_shadowOffset;
END_SHADER_CLASS