//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2025
//////////////////////////////////////////////////////////////////////////////////
// Description: UI overlay mask
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "IMaterialSystem.h"
#include "BaseShader.h"

BEGIN_SHADER_CLASS(UIOverlayMask)
	SHADER_INIT_PARAMS()
	{		
		m_flags |= MATERIAL_FLAG_NO_Z_TEST | MATERIAL_FLAG_NO_CULL;
		m_blendMode = SHADER_BLEND_TRANSLUCENT;
	}

	SHADER_INIT_TEXTURES()
	{
		SHADER_PARAM_TEXTURE_FIND(BaseTexture, m_baseTexture);
		SHADER_PARAM_TEXTURE(MaskTexture, m_maskTexture);
	}

	IGPUBindGroupPtr GetBindGroup(IShaderAPI* renderAPI, EBindGroupId bindGroupId, const BindGroupSetupParams& setupParams) const override
	{
		if (bindGroupId == BINDGROUP_CONSTANT)
		{
			const ITexturePtr& maskTexture = m_maskTexture.Get() ? m_maskTexture.Get() : g_matSystem->GetErrorCheckerboardTexture();
			BindGroupDesc bindGroupDesc = Builder<BindGroupDesc>()
				.Sampler(StringIdConst24("BaseTextureSampler"), SamplerStateParams(m_texFilter, m_texAddressMode))
				.Texture(StringIdConst24("BaseTexture"), m_baseTexture.Get())
				.Texture(StringIdConst24("MaskTexture"), maskTexture)
				.End();

			return CreateBindGroup(bindGroupDesc, bindGroupId, renderAPI, setupParams.pipelineInfo);
		}
		else if (bindGroupId == BINDGROUP_RENDERPASS)
		{
			GPUBufferView cameraParamsBuffer;
			for (const RenderBufferInfo& rendBuffer : setupParams.uniformBuffers)
			{
				if (rendBuffer.signature == s_matSysCameraBufferId)
					cameraParamsBuffer = rendBuffer.bufferView;
			}

			if (!cameraParamsBuffer)
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

			BindGroupDesc bindGroupDesc = Builder<BindGroupDesc>()
				.Buffer(StringIdConst24("camera"), cameraParamsBuffer)
				.End();
			return CreateBindGroup(bindGroupDesc, bindGroupId, renderAPI, setupParams.pipelineInfo);
		}

		return GetEmptyBindGroup(renderAPI, bindGroupId, setupParams.pipelineInfo);
	}

	MatTextureProxy		m_baseTexture;
	MatTextureProxy		m_maskTexture;

	mutable GPUBufferView		m_currentCameraBuffer;
	mutable int					m_currentCameraId{ -1 };
END_SHADER_CLASS