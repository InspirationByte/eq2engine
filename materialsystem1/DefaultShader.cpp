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

static uint GenDefaultPipelineId(const IGPURenderPassRecorder* renderPass, const MatSysDefaultRenderPass& renderPassInfo, EPrimTopology primitiveTopology)
{
	uint id = 0;
	id |= renderPassInfo.cullMode; // 2 bits
	id |= primitiveTopology << 2;	// 3 bits
	id |= renderPassInfo.blendMode << (2+3); // 2 bits
	id |= renderPassInfo.depthFunc << (2+3+2); // 1 bit
	id |= renderPassInfo.depthTest << (2+3+2+1); // 1 bit
	id |= renderPassInfo.depthWrite << (2+3+2+1+1); // 4 bits
	id |= (GetTexFormat(renderPass->GetRenderTargetFormat(0)) & 63) << (2+3+2+1+1+4);
	id |= (renderPass->GetRenderTargetFormat(0) >> 10 & 3) << (2+3+2+1+1+4+6);
	id |= (renderPass->GetDepthTargetFormat() & 63) << (2+3+2+1+1+4+6+2);
	return id;
}

//------------------------------------------------------------

BEGIN_SHADER_CLASS(Default)
	SHADER_INIT_PARAMS()
	{
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
		ASSERT_MSG(rendPassInfo, "Must specify MatSysDefaultRenderPass in userData when drawing with default material");
		const uint pipelineId = GenDefaultPipelineId(renderPass, *rendPassInfo, primitiveTopology);
		auto it = m_renderPipelines.find(pipelineId);

		if (it.atEnd())
		{
			// prepare basic pipeline descriptor
			RenderPipelineDesc renderPipelineDesc = Builder<RenderPipelineDesc>()
				.ShaderName(GetName())
				.ShaderVertexLayoutId(meshInstFormat.nameHash)
				.End();

			for (const VertexLayoutDesc& layoutDesc : meshInstFormat.layout)
				renderPipelineDesc.vertex.vertexLayout.append(layoutDesc);

			const ETextureFormat depthTargetFormat = renderPass->GetDepthTargetFormat();
			if(depthTargetFormat != FORMAT_NONE)
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
					const ETextureFormat format = renderPass->GetRenderTargetFormat(i);
					if (format == FORMAT_NONE)
						break;
					pipelineBuilder.ColorTarget("CT", format, colorBlend, alphaBlend);
				}
				pipelineBuilder.End();
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

	// this function returns material group.
	// Default material has transient all transient resources 
	// as it's used for immediate drawing
	IGPUBindGroupPtr GetBindGroup(uint frameIdx, EBindGroupId bindGroupId, IShaderAPI* renderAPI, IGPURenderPassRecorder* rendPassRecorder, const void* userData) const
	{
		if (bindGroupId == BINDGROUP_TRANSIENT)
		{
			const MatSysDefaultRenderPass* rendPassInfo = reinterpret_cast<const MatSysDefaultRenderPass*>(userData);
			ASSERT_MSG(rendPassInfo, "Must specify MatSysDefaultRenderPass in userData when drawing with default material");

			const ITexturePtr& baseTexture = rendPassInfo->texture ? rendPassInfo->texture : g_matSystem->GetWhiteTexture();

			// can use either fixed array or CMemoryStream with on-stack storage
			FixedArray<Vector4D, 4> bufferData;
			bufferData.append(rendPassInfo->drawColor);
			bufferData.append(GetTextureTransform(m_baseTextureTransformVar, m_baseTextureScaleVar));

			MatSysCamera cameraParams;
			g_matSystem->GetCameraParams(cameraParams, true);

			GPUBufferPtrView cameraParamsBuffer = g_matSystem->GetTransientUniformBuffer(&cameraParams, sizeof(cameraParams));
			GPUBufferPtrView materialParamsBuffer = g_matSystem->GetTransientUniformBuffer(bufferData.ptr(), sizeof(bufferData[0]) * bufferData.numElem());

			BindGroupDesc shaderBindGroupDesc = Builder<BindGroupDesc>()
				.Buffer(0, cameraParamsBuffer)
				.Buffer(1, materialParamsBuffer)
				.Sampler(2, baseTexture->GetSamplerState())
				.Texture(3, baseTexture)
				.End();

			return renderAPI->CreateBindGroup(GetPipelineLayout(renderAPI), bindGroupId, shaderBindGroupDesc);
		}
		return GetEmptyBindGroup(bindGroupId, renderAPI);
	}

	mutable Map<uint, IGPURenderPipelinePtr>	m_renderPipelines{ PP_SL };
	MatTextureProxy								m_baseTexture;

END_SHADER_CLASS
