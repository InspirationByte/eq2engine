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
	}

	SHADER_INIT_TEXTURES()
	{
		SHADER_PARAM_TEXTURE_FIND(BaseTexture, m_baseTexture);
	}

	IGPUBindGroupPtr GetBindGroup(IShaderAPI* renderAPI, EBindGroupId bindGroupId, const BindGroupSetupParams& setupParams) const
	{
		if (bindGroupId == BINDGROUP_CONSTANT)
		{
			BindGroupDesc bindGroupDesc = Builder<BindGroupDesc>()
				.Sampler(0, SamplerStateParams(TEXFILTER_LINEAR, TEXADDRESS_CLAMP))
				.Texture(1, m_baseTexture.Get())
				.End();

			return CreateBindGroup(bindGroupDesc, bindGroupId, renderAPI, setupParams.pipelineInfo);
		}

		return GetEmptyBindGroup(renderAPI, bindGroupId, setupParams.pipelineInfo);
	}

	MatTextureProxy m_baseTexture;
END_SHADER_CLASS