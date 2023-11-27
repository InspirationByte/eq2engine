//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Post processing effect combiner
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "IMaterialSystem.h"
#include "BaseShader.h"

BEGIN_SHADER_CLASS(GaussianBlur)
	SHADER_INIT_PARAMS()
	{
		m_flags |= MATERIAL_FLAG_NO_Z_TEST;
		m_blurProps = m_material->GetMaterialVar("BlurProps", "[0.6 40 100 100]");
	}

	SHADER_INIT_TEXTURES()
	{
		m_blurSource = m_material->GetMaterialVar("BlurSource", "");
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

	void FillBindGroupLayout_RenderPass(BindGroupLayoutDesc& bindGroupLayout) const
	{
		Builder<BindGroupLayoutDesc>(bindGroupLayout)
			.Buffer("cameraParams", 0, SHADERKIND_VERTEX | SHADERKIND_FRAGMENT, BUFFERBIND_UNIFORM)
			.End();
	}

	IGPUBindGroupPtr GetBindGroup(uint frameIdx, EBindGroupId bindGroupId, IShaderAPI* renderAPI, IGPURenderPassRecorder* rendPassRecorder, const void* userData) const
	{
		if (bindGroupId == BINDGROUP_CONSTANT)
		{
			if (!m_materialBindGroup)
			{
				const ITexturePtr& bloomTex = m_blurSource.Get();
				Vector4D textureSizeProps(1.0f);
				if (bloomTex)
				{
					textureSizeProps.x = bloomTex->GetWidth();
					textureSizeProps.y = bloomTex->GetHeight();
					textureSizeProps.z = 1.0f / textureSizeProps.x;
					textureSizeProps.w = 1.0f / textureSizeProps.y;
				}

				FixedArray<Vector4D, 4> bufferData;
				bufferData.append(m_blurProps.Get());
				bufferData.append(textureSizeProps);
				m_materialParamsBuffer = renderAPI->CreateBuffer(BufferInfo(bufferData.ptr(), bufferData.numElem()), BUFFERUSAGE_UNIFORM, "materialParams");

				ITexturePtr baseTexture = m_blurSource.Get() ? m_blurSource.Get() : g_matSystem->GetErrorCheckerboardTexture();
				BindGroupDesc shaderBindGroupDesc = Builder<BindGroupDesc>()
					.Buffer(0, m_materialParamsBuffer)
					.Sampler(1, SamplerStateParams(TEXFILTER_LINEAR, TEXADDRESS_CLAMP))
					.Texture(2, baseTexture)
					.End();
				m_materialBindGroup = renderAPI->CreateBindGroup(GetPipelineLayout(renderAPI), bindGroupId, shaderBindGroupDesc);
			}
			return m_materialBindGroup;
		}
		else if (bindGroupId == BINDGROUP_RENDERPASS)
		{
			MatSysCamera cameraParams;
			g_matSystem->GetCameraParams(cameraParams, true);

			IGPUBufferPtr cameraParamsBuffer = renderAPI->CreateBuffer(BufferInfo(&cameraParams, 1), BUFFERUSAGE_UNIFORM, "matSysCamera");
			BindGroupDesc shaderBindGroupDesc = Builder<BindGroupDesc>()
				.Buffer(0, cameraParamsBuffer)
				.End();
			return renderAPI->CreateBindGroup(GetPipelineLayout(renderAPI), bindGroupId, shaderBindGroupDesc);
		}
		return GetEmptyBindGroup(bindGroupId, renderAPI);
	}

	mutable IGPUBindGroupPtr	m_materialBindGroup;
	mutable IGPUBufferPtr		m_materialParamsBuffer;

	MatVec4Proxy				m_blurProps;
	MatTextureProxy				m_blurSource;

END_SHADER_CLASS