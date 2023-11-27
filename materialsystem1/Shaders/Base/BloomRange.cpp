//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Bloom range compression shader
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "IMaterialSystem.h"
#include "BaseShader.h"

BEGIN_SHADER_CLASS(BloomRange)
	SHADER_INIT_PARAMS()
	{
		m_flags |= MATERIAL_FLAG_NO_Z_TEST;
		m_rangeProps = m_material->GetMaterialVar("RangeProps", "[0.6 40 100 100]");

		FixedArray<Vector4D, 4> bufferData;
		bufferData.append(m_rangeProps.Get());
		m_materialParamsBuffer = renderAPI->CreateBuffer(BufferInfo(bufferData.ptr(), bufferData.numElem()), BUFFERUSAGE_UNIFORM, "materialParams");
	}

	SHADER_INIT_TEXTURES()
	{
		m_bloomSource = m_material->GetMaterialVar("BloomSource", "");
	}

	bool IsSupportInstanceFormat(int nameHash) const
	{
		return nameHash == StringToHashConst("DynMeshVertex");
	}

	void FillBindGroupLayout_Constant(BindGroupLayoutDesc& bindGroupLayout) const
	{
		Builder<BindGroupLayoutDesc>(bindGroupLayout)
			.Buffer("materialParams", 0, SHADERKIND_VERTEX | SHADERKIND_FRAGMENT, BUFFERBIND_UNIFORM)
			.Sampler("BaseTextureSampler", 1, SHADERKIND_FRAGMENT, SAMPLERBIND_FILTERING)
			.Texture("BaseTexture", 2, SHADERKIND_FRAGMENT, TEXSAMPLE_FLOAT, TEXDIMENSION_2D)
			.End();
	}

	IGPUBindGroupPtr GetBindGroup(uint frameIdx, EBindGroupId bindGroupId, IShaderAPI* renderAPI, IGPURenderPassRecorder* rendPassRecorder, const void* userData) const
	{
		if (bindGroupId == BINDGROUP_CONSTANT)
		{
			if (!m_materialBindGroup)
			{
				ITexturePtr baseTexture = m_bloomSource.Get() ? m_bloomSource.Get() : g_matSystem->GetErrorCheckerboardTexture();
				BindGroupDesc shaderBindGroupDesc = Builder<BindGroupDesc>()
					.Buffer(0, m_materialParamsBuffer)
					.Sampler(1, baseTexture->GetSamplerState())
					.Texture(2, baseTexture)
					.End();
				m_materialBindGroup = renderAPI->CreateBindGroup(GetPipelineLayout(renderAPI), 1, shaderBindGroupDesc);
			}

			return m_materialBindGroup;
		}

		return GetEmptyBindGroup(bindGroupId, renderAPI);
	}

	mutable IGPUBindGroupPtr	m_materialBindGroup;
	IGPUBufferPtr				m_materialParamsBuffer;

	MatVec4Proxy				m_rangeProps;
	MatTextureProxy				m_bloomSource;
END_SHADER_CLASS