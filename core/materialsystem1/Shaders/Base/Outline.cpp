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

		m_outlineColor = GetMaterialVar("OutlineColor", "[0.0 0.5 0.0 1.0]");
		m_outlineWidth = GetMaterialVar("OutlineWidth", "16");

		OutlineParams materialParams;
		materialParams.outlineColor = m_outlineColor.Get();
		materialParams.outlineWidth = m_outlineWidth.Get();

		m_proxyBuffer = MakeParameterUniformBuffer(materialParams);
	}

	SHADER_INIT_TEXTURES()
	{
		SHADER_PARAM_TEXTURE_FIND(SourceDepth, m_sourceTex);
	}

	void UpdateProxy(IGPUCommandRecorder* cmdRecorder) const override
	{
		OutlineParams materialParams;
		materialParams.outlineColor = m_outlineColor.Get();
		materialParams.outlineWidth = m_outlineWidth.Get();

		cmdRecorder->WriteBuffer(m_proxyBuffer, &materialParams, sizeof(materialParams), 0);
	}

	IGPUBindGroupPtr GetBindGroup(IShaderAPI* renderAPI, EBindGroupId bindGroupId, const BindGroupSetupParams& setupParams) const override
	{
		if (bindGroupId == BINDGROUP_CONSTANT)
		{
			const ITexturePtr& baseTexture = m_sourceTex.Get() ? m_sourceTex.Get() : g_matSystem->GetErrorCheckerboardTexture();
			BindGroupDesc bindGroupDesc = Builder<BindGroupDesc>()
				.Buffer(StringIdConst24("materialParams"), m_proxyBuffer)
				.Sampler(StringIdConst24("DepthSampler"), baseTexture->GetSamplerState())
				.Texture(StringIdConst24("DepthTexture"), baseTexture)
				.End();

			return CreateBindGroup(bindGroupDesc, bindGroupId, renderAPI, setupParams.pipelineInfo);
		}

		return setupParams.pipelineInfo.bindGroup[bindGroupId];
	}

	IGPUBufferPtr				m_proxyBuffer;

	MatVec4Proxy				m_outlineColor;
	MatIntProxy					m_outlineWidth;
	MatTextureProxy				m_sourceTex;
END_SHADER_CLASS