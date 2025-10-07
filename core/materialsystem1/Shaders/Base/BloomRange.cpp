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
		m_rangeProps = GetMaterialVar("RangeProps", "[0.6 40 100 100]");

		m_proxyBuffer = MakeParameterUniformBuffer(
			m_rangeProps.Get()
		);
	}

	SHADER_INIT_TEXTURES()
	{
		m_bloomSource = GetMaterialVar("BloomSource", "");
	}

	//void FillBindGroupLayout_Constant(const MeshInstanceFormatRef& meshInstFormat, BindGroupLayoutDesc& bindGroupLayout) const override
	//{
	//	Builder<BindGroupLayoutDesc>(bindGroupLayout)
	//		.Buffer("materialParams", 0, SHADERKIND_VERTEX | SHADERKIND_FRAGMENT, BUFFERBIND_UNIFORM)
	//		.Sampler("BaseTextureSampler", 1, SHADERKIND_FRAGMENT, SAMPLERBIND_FILTERING)
	//		.Texture("BaseTexture", 2, SHADERKIND_FRAGMENT, TEXSAMPLE_FLOAT, TEXDIMENSION_2D)
	//		.End();
	//}

	void UpdateProxy(IGPUCommandRecorder* cmdRecorder) const override
	{
		FixedArray<Vector4D, 4> bufferData;
		bufferData.append(m_rangeProps.Get());
		cmdRecorder->WriteBuffer(m_proxyBuffer, bufferData.ptr(), bufferData.numElem() * sizeof(bufferData[0]), 0);
	}

	IGPUBindGroupPtr GetBindGroup(IShaderAPI* renderAPI, EBindGroupId bindGroupId, const BindGroupSetupParams& setupParams) const override
	{
		if (bindGroupId == BINDGROUP_CONSTANT)
		{
			const ITexturePtr& baseTexture = m_bloomSource.Get() ? m_bloomSource.Get() : g_matSystem->GetErrorCheckerboardTexture();
			BindGroupDesc bindGroupDesc = Builder<BindGroupDesc>()
				.Buffer(StringIdConst24("materialParams"), m_proxyBuffer)
				.Sampler(StringIdConst24("BaseTextureSampler"), baseTexture->GetSamplerState())
				.Texture(StringIdConst24("BaseTexture"), baseTexture)
				.End();
			return CreateBindGroup(bindGroupDesc, bindGroupId, renderAPI, setupParams.pipelineInfo);
		}

		return setupParams.pipelineInfo.bindGroup[bindGroupId];
	}

	IGPUBufferPtr				m_proxyBuffer;

	MatVec4Proxy				m_rangeProps;
	MatTextureProxy				m_bloomSource;
END_SHADER_CLASS