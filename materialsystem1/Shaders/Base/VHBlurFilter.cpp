//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Blur filter with vertical and horizontal kernels
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "IMaterialSystem.h"
#include "BaseShader.h"

BEGIN_SHADER_CLASS(VHBlurFilter)

	bool IsSupportVertexFormat(int nameHash) const
	{
		return nameHash == StringToHashConst("DynMeshVertex");
	}

	SHADER_INIT_PARAMS()
	{
		m_flags |= MATERIAL_FLAG_NO_Z_TEST;

		bool blurXLow = false;
		bool blurYLow = false;
		bool blurXHigh = false;
		bool blurYHigh = false;
		SHADER_PARAM_BOOL(XLow, blurXLow, false);
		SHADER_PARAM_BOOL(YLow, blurYLow, false);
		SHADER_PARAM_BOOL(XHigh, blurXHigh, false);
		SHADER_PARAM_BOOL(YHigh, blurYHigh, false);

		m_blurModes |= blurXLow ? 0x1 : 0;
		m_blurModes |= blurYLow ? 0x2 : 0;
		m_blurModes |= blurXHigh ? 0x4 : 0;
		m_blurModes |= blurYHigh ? 0x8 : 0;
	}

	SHADER_INIT_TEXTURES()
	{
		SHADER_PARAM_TEXTURE_FIND(BaseTexture, m_baseTexture);
	}

	void BuildPipelineShaderQuery(const IVertexFormat* vertexLayout, Array<EqString>& shaderQuery) const
	{
		if(m_blurModes & 0x1)
			shaderQuery.append("BLUR_X_LOW");
		if(m_blurModes & 0x2)
			shaderQuery.append("BLUR_Y_LOW");
		if(m_blurModes & 0x4)
			shaderQuery.append("BLUR_X_HIGH");
		if(m_blurModes & 0x8)
			shaderQuery.append("BLUR_Y_HIGH");
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

	IGPUBindGroupPtr GetBindGroup(EBindGroupId bindGroupId, IShaderAPI* renderAPI, const void* userData) const
	{
		if (bindGroupId == BINDGROUP_CONSTANT)
		{
			if (!m_materialBindGroup)
			{
				ITexturePtr baseTexture = m_baseTexture.Get() ? m_baseTexture.Get() : g_matSystem->GetErrorCheckerboardTexture();

				Vector4D texSize;
				texSize.x = baseTexture->GetWidth();
				texSize.y = baseTexture->GetHeight();
				texSize.z = 1.0f / texSize.x;
				texSize.w = 1.0f / texSize.y;

				FixedArray<Vector4D, 4> bufferData;
				bufferData.append(texSize);
				bufferData.append(GetTextureTransform(m_baseTextureTransformVar, m_baseTextureScaleVar));
				IGPUBufferPtr materialParamsBuffer = renderAPI->CreateBuffer(BufferInfo(bufferData.ptr(), bufferData.numElem()), BUFFERUSAGE_UNIFORM, "materialParams");

				BindGroupDesc shaderBindGroupDesc = Builder<BindGroupDesc>()
					.Buffer(0, materialParamsBuffer, 0, materialParamsBuffer->GetSize())
					.Sampler(1, SamplerStateParams(TEXFILTER_LINEAR, TEXADDRESS_CLAMP))
					.Texture(2, baseTexture)
					.End();
				m_materialBindGroup = renderAPI->CreateBindGroup(GetPipelineLayout(), bindGroupId, shaderBindGroupDesc);
			}
			return m_materialBindGroup;
		}
		else if (bindGroupId == BINDGROUP_RENDERPASS)
		{
			IGPUBufferPtr cameraParamsBuffer = GetRenderPassCameraParamsBuffer(renderAPI, true);
			BindGroupDesc shaderBindGroupDesc = Builder<BindGroupDesc>()
				.Buffer(0, cameraParamsBuffer, 0, cameraParamsBuffer->GetSize())
				.End();
			return renderAPI->CreateBindGroup(GetPipelineLayout(), bindGroupId, shaderBindGroupDesc);
		}
		return GetEmptyBindGroup(bindGroupId, renderAPI);
	}

	const ITexturePtr& GetBaseTexture(int stage) const
	{
		return m_baseTexture.Get();
	}

	mutable IGPUBindGroupPtr	m_materialBindGroup;
	MatTextureProxy				m_baseTexture;
	int							m_blurModes{ 0 };
END_SHADER_CLASS