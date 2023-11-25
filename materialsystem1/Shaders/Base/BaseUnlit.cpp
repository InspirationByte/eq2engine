//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Unlit Shader with fog support
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "IMaterialSystem.h"
#include "BaseShader.h"
#include "IDynamicMesh.h"

BEGIN_SHADER_CLASS(BaseUnlit)
	SHADER_INIT_PARAMS()
	{
		m_colorVar = m_material->GetMaterialVar("color", "[1 1 1 1]");

		FixedArray<Vector4D, 4> bufferData;
		bufferData.append(m_colorVar.Get());
		m_materialParamsBuffer = renderAPI->CreateBuffer(BufferInfo(bufferData.ptr(), bufferData.numElem()), BUFFERUSAGE_UNIFORM, "materialParams");
	}

	SHADER_INIT_TEXTURES()
	{
		SHADER_PARAM_TEXTURE(BaseTexture, m_baseTexture);
	}

	bool IsSupportInstanceFormat(int nameHash) const
	{
		// must support any vertex
		return true;
	}

	void BuildPipelineShaderQuery(const MeshInstanceFormatRef& meshInstFormat, int vertexLayoutUsedBufferBits, Array<EqString>& shaderQuery) const
	{
		bool vertexColor = false;
		SHADER_PARAM_BOOL(VertexColor, vertexColor, false);

		bool fogEnable = true;
		SHADER_PARAM_BOOL_NEG(NoFog, fogEnable, false);
		if (fogEnable)
			shaderQuery.append("DOFOG");

		if (vertexColor)
			shaderQuery.append("VERTEXCOLOR");

		if (m_flags & MATERIAL_FLAG_ALPHATESTED)
			shaderQuery.append("ALPHATEST");

		if (meshInstFormat.nameHash == StringToHashConst("EGFVertexSkinned"))
			shaderQuery.append("SKIN");
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
				BindGroupDesc shaderBindGroupDesc = Builder<BindGroupDesc>()
					.Buffer(0, m_materialParamsBuffer, 0, m_materialParamsBuffer->GetSize())
					.Sampler(1, SamplerStateParams(m_texFilter, m_texAddressMode))
					.Texture(2, baseTexture)
					.End();
				m_materialBindGroup = renderAPI->CreateBindGroup(GetPipelineLayout(), bindGroupId, shaderBindGroupDesc);
			}

			// TODO: update BaseTextureTransform

			return m_materialBindGroup;
		}
		else if (bindGroupId == BINDGROUP_RENDERPASS)
		{
			struct {
				MatSysCamera camera;
				Vector4D textureTransform;
			} passParams;
			GetCameraParams(passParams.camera, true);
			passParams.textureTransform = GetTextureTransform(m_baseTextureTransformVar, m_baseTextureScaleVar);
			
			IGPUBufferPtr passParamsBuffer = renderAPI->CreateBuffer(BufferInfo(&passParams, 1), BUFFERUSAGE_UNIFORM, "matSysCamera");
			BindGroupDesc shaderBindGroupDesc = Builder<BindGroupDesc>()
				.Buffer(0, passParamsBuffer, 0, passParamsBuffer->GetSize())
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
	IGPUBufferPtr				m_materialParamsBuffer;

	MatTextureProxy				m_baseTexture;
	MatVec4Proxy				m_colorVar;
END_SHADER_CLASS
