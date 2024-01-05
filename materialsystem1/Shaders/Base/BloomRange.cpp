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
		m_proxyBuffer = renderAPI->CreateBuffer(BufferInfo(bufferData.ptr(), bufferData.numElem()), BUFFERUSAGE_UNIFORM | BUFFERUSAGE_COPY_DST, "materialParams");
	}

	SHADER_INIT_TEXTURES()
	{
		m_bloomSource = m_material->GetMaterialVar("BloomSource", "");
	}

	void FillBindGroupLayout_Constant(BindGroupLayoutDesc& bindGroupLayout) const
	{
		Builder<BindGroupLayoutDesc>(bindGroupLayout)
			.Buffer("materialParams", 0, SHADERKIND_VERTEX | SHADERKIND_FRAGMENT, BUFFERBIND_UNIFORM)
			.Sampler("BaseTextureSampler", 1, SHADERKIND_FRAGMENT, SAMPLERBIND_FILTERING)
			.Texture("BaseTexture", 2, SHADERKIND_FRAGMENT, TEXSAMPLE_FLOAT, TEXDIMENSION_2D)
			.End();
	}

	void UpdateProxy(IGPUCommandRecorder* cmdRecorder) const
	{
		FixedArray<Vector4D, 4> bufferData;
		bufferData.append(m_rangeProps.Get());
		cmdRecorder->WriteBuffer(m_proxyBuffer, bufferData.ptr(), bufferData.numElem() , 0);
	}

	IGPUBindGroupPtr GetBindGroup(IShaderAPI* renderAPI, EBindGroupId bindGroupId, const PipelineInfo& pipelineInfo, ArrayCRef<RenderBufferInfo> uniformBuffers, const RenderPassContext& passContext) const
	{
		if (bindGroupId == BINDGROUP_CONSTANT)
		{
			if (!m_materialBindGroup)
			{
				const ITexturePtr& baseTexture = m_bloomSource.Get() ? m_bloomSource.Get() : g_matSystem->GetErrorCheckerboardTexture();
				BindGroupDesc bindGroupDesc = Builder<BindGroupDesc>()
					.Buffer(0, m_proxyBuffer)
					.Sampler(1, baseTexture->GetSamplerState())
					.Texture(2, baseTexture)
					.End();
				m_materialBindGroup = CreateBindGroup(bindGroupDesc, bindGroupId, renderAPI, pipelineInfo);
			}

			return m_materialBindGroup;
		}

		return GetEmptyBindGroup(renderAPI, bindGroupId, pipelineInfo);
	}

	mutable IGPUBindGroupPtr	m_materialBindGroup;
	IGPUBufferPtr				m_proxyBuffer;

	MatVec4Proxy				m_rangeProps;
	MatTextureProxy				m_bloomSource;
END_SHADER_CLASS