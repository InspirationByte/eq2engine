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

static uint GenDefaultPipelineId(const MatSysDefaultRenderPass& renderPass, EPrimTopology primitiveTopology)
{
	uint id = 0;
	id |= renderPass.cullMode; // 2 bits
	id |= primitiveTopology << 2;	// 3 bits
	id |= renderPass.blendMode << (2+3); // 2 bits
	id |= renderPass.depthFunc << (2+3+2); // 1 bit
	id |= renderPass.depthTest << (2+3+2+1); // 1 bit
	id |= renderPass.depthWrite << (2+3+2+1+1); // 4 bits
	return id;
}

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
		.ColorTarget("BackBuffer", g_matSystem->GetCurrentBackbuffer()->GetFormat())
		.DepthFormat(g_matSystem->GetDefaultDepthBuffer()->GetFormat())
		.VertexLayout(
			Builder<VertexLayoutDesc>()
			.Attribute(VERTEXATTRIB_POSITION, "position", 0, 0, ATTRIBUTEFORMAT_FLOAT, 3)
			.End())
		.End();
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

	bool IsSupportVertexFormat(int nameHash) const
	{
		return nameHash == StringToHashConst("DynMeshVertex");
	}

	IGPURenderPipelinePtr GetRenderPipeline(IShaderAPI* renderAPI, const IGPURenderPassRecorder* renderPass, const IVertexFormat* vertexLayout, EPrimTopology primitiveTopology, const void* userData) const
	{
		const MatSysDefaultRenderPass* rendPassInfo = reinterpret_cast<const MatSysDefaultRenderPass*>(userData);
		ASSERT_MSG(rendPassInfo, "Must specify MatSysDefaultRenderPass in userData when drawing with default material");
		const uint pipelineId = GenDefaultPipelineId(*rendPassInfo, primitiveTopology);
		auto it = m_renderPipelines.find(pipelineId);

		if (it.atEnd())
		{
			// prepare basic pipeline descriptor
			RenderPipelineDesc renderPipelineDesc = Builder<RenderPipelineDesc>()
				.ShaderName(GetName())
				.ShaderVertexLayoutId(vertexLayout->GetNameHash())
				.End();

			for (const VertexLayoutDesc& layoutDesc : vertexLayout->GetFormatDesc())
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
			
			IGPURenderPipelinePtr renderPipeline = renderAPI->CreateRenderPipeline(GetPipelineLayout(), renderPipelineDesc);
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
	IGPUBindGroupPtr GetBindGroup(EBindGroupId bindGroupId, IShaderAPI* renderAPI, const void* userData) const
	{
		if (bindGroupId == BINDGROUP_TRANSIENT)
		{
			const MatSysDefaultRenderPass* rendPassInfo = reinterpret_cast<const MatSysDefaultRenderPass*>(userData);
			ASSERT_MSG(rendPassInfo, "Must specify MatSysDefaultRenderPass in userData when drawing with default material");

			ITexturePtr baseTexture = rendPassInfo->texture ? rendPassInfo->texture : g_matSystem->GetWhiteTexture();

			// can use either fixed array or CMemoryStream with on-stack storage
			FixedArray<Vector4D, 4> bufferData;
			bufferData.append(rendPassInfo->drawColor);
			bufferData.append(GetTextureTransform(m_baseTextureTransformVar, m_baseTextureScaleVar));

			IGPUBufferPtr cameraParamsBuffer = GetRenderPassCameraParamsBuffer(renderAPI);
			IGPUBufferPtr materialParamsBuffer = renderAPI->CreateBuffer(BufferInfo(bufferData.ptr(), bufferData.numElem()), BUFFERUSAGE_UNIFORM, "materialParams");
			BindGroupDesc shaderBindGroupDesc = Builder<BindGroupDesc>()
				.Buffer(0, cameraParamsBuffer, 0, cameraParamsBuffer->GetSize())
				.Buffer(1, materialParamsBuffer, 0, materialParamsBuffer->GetSize())
				.Sampler(2, baseTexture->GetSamplerState())
				.Texture(3, baseTexture)
				.End();

			return renderAPI->CreateBindGroup(GetPipelineLayout(), bindGroupId, shaderBindGroupDesc);
		}
		return GetEmptyBindGroup(bindGroupId, renderAPI);
	}

	mutable Map<uint, IGPURenderPipelinePtr>	m_renderPipelines{ PP_SL };
	MatTextureProxy								m_baseTexture;

END_SHADER_CLASS
