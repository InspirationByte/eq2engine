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
#include "render/StudioRenderDefs.h"

BEGIN_SHADER_CLASS(
	BaseUnlit,
	SHADER_VERTEX_ID(DynMeshVertex),
	SHADER_VERTEX_ID(EGFVertex),
	SHADER_VERTEX_ID(EGFVertexSkinned)
)
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

	void BuildPipelineShaderQuery(Array<EqString>& shaderQuery) const
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
	}

	IGPUBindGroupPtr GetBindGroup(IShaderAPI* renderAPI, EBindGroupId bindGroupId, const PipelineInfo& pipelineInfo, ArrayCRef<RenderBufferInfo> uniformBuffers, const RenderPassContext& passContext) const
	{
		if (bindGroupId == BINDGROUP_CONSTANT)
		{
			if (!m_materialBindGroup)
			{
				const ITexturePtr& baseTexture = m_baseTexture.Get() ? m_baseTexture.Get() : g_matSystem->GetErrorCheckerboardTexture();
				BindGroupDesc bindGroupDesc = Builder<BindGroupDesc>()
					.Buffer(0, m_materialParamsBuffer)
					.Sampler(1, SamplerStateParams(m_texFilter, m_texAddressMode))
					.Texture(2, baseTexture)
					.End();
				m_materialBindGroup = CreateBindGroup(bindGroupDesc, bindGroupId, renderAPI, pipelineInfo);
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
			g_matSystem->GetCameraParams(passParams.camera, true);
			passParams.textureTransform = GetTextureTransform(m_baseTextureTransformVar, m_baseTextureScaleVar);
			
			GPUBufferView passParamsBuffer = g_matSystem->GetTransientUniformBuffer(&passParams, sizeof(passParams));
			BindGroupDesc bindGroupDesc = Builder<BindGroupDesc>()
				.Buffer(0, passParamsBuffer)
				.End();
			return CreateBindGroup(bindGroupDesc, bindGroupId, renderAPI, pipelineInfo);
		}
		else if (bindGroupId == BINDGROUP_TRANSIENT)
		{
			if (pipelineInfo.vertexLayoutId == StringToHashConst("EGFVertexSkinned"))
			{
				ASSERT(uniformBuffers[0].signature == RenderBoneTransformID)

				BindGroupDesc bindGroupDesc = Builder<BindGroupDesc>()
					.Buffer(0, uniformBuffers[0].bufferView)
					.End();
				return CreateBindGroup(bindGroupDesc, bindGroupId, renderAPI, pipelineInfo);
			}
		}
		return GetEmptyBindGroup(renderAPI, bindGroupId, pipelineInfo);
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
