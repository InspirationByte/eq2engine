//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Depth combiner shader
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "IMaterialSystem.h"
#include "BaseShader.h"

BEGIN_SHADER_CLASS(DepthCombiner)
	SHADER_INIT_PARAMS()
	{
		m_flags |= MATERIAL_FLAG_NO_Z_TEST;
	}

	SHADER_INIT_TEXTURES()
	{
		// parse material variables
		SHADER_PARAM_TEXTURE_FIND(Texture1, m_textures[0]);
		SHADER_PARAM_TEXTURE_FIND(Texture2, m_textures[1]);
	}

	bool IsSupportInstanceFormat(int nameHash) const
	{
		return nameHash == StringToHashConst("DynMeshVertex");
	}

	void FillBindGroupLayout_Constant(BindGroupLayoutDesc& bindGroupLayout) const
	{
		Builder<BindGroupLayoutDesc>(bindGroupLayout)
			.Buffer("materialParams", 0, SHADERKIND_VERTEX | SHADERKIND_FRAGMENT, BUFFERBIND_UNIFORM)
			.Sampler("TextureSampler", 1, SHADERKIND_FRAGMENT, SAMPLERBIND_FILTERING)
			.Texture("Texture1", 2, SHADERKIND_FRAGMENT, TEXSAMPLE_FLOAT, TEXDIMENSION_2D)
			.Texture("Texture2", 2, SHADERKIND_FRAGMENT, TEXSAMPLE_FLOAT, TEXDIMENSION_2D)
			.End();
	}

	IGPUBindGroupPtr GetBindGroup(IShaderAPI* renderAPI, const IGPURenderPassRecorder* rendPassRecorder, EBindGroupId bindGroupId, const MeshInstanceFormatRef& meshInstFormat, ArrayCRef<RenderBufferInfo> uniformBuffers, const void* userData) const
	{
		if (bindGroupId == BINDGROUP_CONSTANT)
		{
			if (!m_materialBindGroup)
			{
				const ITexturePtr& tex1 = m_textures[0].Get();
				const ITexturePtr& tex2 = m_textures[1].Get();
				BindGroupDesc bindGroupDesc = Builder<BindGroupDesc>()
					.Sampler(1, SamplerStateParams(TEXFILTER_NEAREST, TEXADDRESS_CLAMP))
					.Texture(2, tex1)
					.Texture(3, tex2)
					.End();
				m_materialBindGroup = CreateBindGroup(bindGroupDesc, bindGroupId, renderAPI, rendPassRecorder);
			}
			return m_materialBindGroup;
		}
		return GetEmptyBindGroup(bindGroupId, renderAPI);
	}

	const ITexturePtr& GetBaseTexture(int stage) const
	{
		return m_textures[stage & 1].Get();
	}

	mutable IGPUBindGroupPtr	m_materialBindGroup;
	MatTextureProxy				m_textures[2];
END_SHADER_CLASS