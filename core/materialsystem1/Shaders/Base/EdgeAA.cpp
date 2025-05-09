//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2025
//////////////////////////////////////////////////////////////////////////////////
// Description: Bloom range compression shader
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "IMaterialSystem.h"
#include "BaseShader.h"

BEGIN_SHADER_CLASS(EdgeAA)
	SHADER_INIT_PARAMS()
	{		
		m_flags |= MATERIAL_FLAG_NO_Z_TEST | MATERIAL_FLAG_NO_CULL;
		m_edgeAASettings = GetMaterialVar("Settings", "[0.75 0.125 0.0625]");

		m_settingsBuffer = MakeParameterUniformBuffer(
			m_edgeAASettings.Get()
		);
	}

	SHADER_INIT_TEXTURES()
	{
		SHADER_PARAM_TEXTURE_FIND(BaseTexture, m_baseTexture);
	}

	void UpdateProxy(IGPUCommandRecorder* cmdRecorder) const override
	{
		FixedArray<Vector4D, 4> bufferData;
		bufferData.append(m_edgeAASettings.Get());
		cmdRecorder->WriteBuffer(m_settingsBuffer, bufferData.ptr(), bufferData.numElem() * sizeof(bufferData[0]), 0);
	}

	IGPUBindGroupPtr GetBindGroup(IShaderAPI* renderAPI, EBindGroupId bindGroupId, const BindGroupSetupParams& setupParams) const override
	{
		if (bindGroupId == BINDGROUP_CONSTANT)
		{
			BindGroupDesc bindGroupDesc = Builder<BindGroupDesc>()
				.Sampler(0, SamplerStateParams(TEXFILTER_LINEAR, TEXADDRESS_CLAMP))
				.Texture(1, m_baseTexture.Get())
				.Buffer(2, m_settingsBuffer)
				.End();

			return CreateBindGroup(bindGroupDesc, bindGroupId, renderAPI, setupParams.pipelineInfo);
		}

		return GetEmptyBindGroup(renderAPI, bindGroupId, setupParams.pipelineInfo);
	}

	MatTextureProxy		m_baseTexture;
	IGPUBufferPtr		m_settingsBuffer;
	MatVec4Proxy		m_edgeAASettings;
END_SHADER_CLASS