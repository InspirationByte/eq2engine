//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Bloom range compression shader
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "IMaterialSystem.h"
#include "BaseShader.h"

struct OutlineParams {
	Vector4D outlineColor;
	int outlineWidth;
	int _pad[3];
};

BEGIN_SHADER_CLASS(Outline)
	SHADER_INIT_PARAMS()
	{
		m_flags |= MATERIAL_FLAG_NO_Z_WRITE;
		m_blendMode = SHADER_BLEND_ADDITIVE;

		m_outlineColor = m_material->GetMaterialVar("OutlineColor", "[0.0 0.5 0.0 1.0]");
		m_outlineWidth = m_material->GetMaterialVar("OutlineWidth", "16");

		OutlineParams materialParams;
		materialParams.outlineColor = m_outlineColor.Get();
		materialParams.outlineWidth = m_outlineWidth.Get();

		m_proxyBuffer = renderAPI->CreateBuffer(BufferInfo(&materialParams, 1), BUFFERUSAGE_UNIFORM | BUFFERUSAGE_COPY_DST, "materialParams");
	}

	SHADER_INIT_TEXTURES()
	{
		SHADER_PARAM_TEXTURE_FIND(SourceDepth, m_sourceTex)
	}

	/*void FillBindGroupLayout_Constant(const MeshInstanceFormatRef& meshInstFormat, BindGroupLayoutDesc& bindGroupLayout) const
	{
		Builder<BindGroupLayoutDesc>(bindGroupLayout)
			.Buffer("materialParams", 0, SHADERKIND_VERTEX | SHADERKIND_FRAGMENT, BUFFERBIND_UNIFORM)
			.Sampler("DepthSampler", 1, SHADERKIND_FRAGMENT, SAMPLERBIND_NONFILTERING)
			.Texture("DepthTexture", 2, SHADERKIND_FRAGMENT, TEXSAMPLE_FLOAT, TEXDIMENSION_2D)
			.End();
	}*/

	void UpdateProxy(IGPUCommandRecorder* cmdRecorder) const
	{
		OutlineParams materialParams;
		materialParams.outlineColor = m_outlineColor.Get();
		materialParams.outlineWidth = m_outlineWidth.Get();

		cmdRecorder->WriteBuffer(m_proxyBuffer, &materialParams, sizeof(materialParams), 0);
	}

	IGPUBindGroupPtr GetBindGroup(IShaderAPI* renderAPI, EBindGroupId bindGroupId, const BindGroupSetupParams& setupParams) const
	{
		if (bindGroupId == BINDGROUP_CONSTANT)
		{
			const ITexturePtr& baseTexture = m_sourceTex.Get() ? m_sourceTex.Get() : g_matSystem->GetErrorCheckerboardTexture();
			BindGroupDesc bindGroupDesc = Builder<BindGroupDesc>()
				.Buffer(0, m_proxyBuffer)
				.Sampler(1, baseTexture->GetSamplerState())
				.Texture(2, baseTexture)
				.End();

			return CreateBindGroup(bindGroupDesc, bindGroupId, renderAPI, setupParams.pipelineInfo);
		}

		return GetEmptyBindGroup(renderAPI, bindGroupId, setupParams.pipelineInfo);
	}

	IGPUBufferPtr				m_proxyBuffer;

	MatVec4Proxy				m_outlineColor;
	MatIntProxy					m_outlineWidth;
	MatTextureProxy				m_sourceTex;
END_SHADER_CLASS