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

		FixedArray<Vector4D, 2> bufferData;
		bufferData.append(m_colorVar.Get());
		bufferData.append(Vector4D(1, 1, 0, 0));

		m_materialParamsBuffer = renderAPI->CreateBuffer(BufferInfo(bufferData.ptr(), bufferData.numElem()), BUFFERUSAGE_UNIFORM | BUFFERUSAGE_COPY_DST, "materialParams");
	}

	SHADER_INIT_TEXTURES()
	{
		SHADER_PARAM_TEXTURE(BaseTexture, m_baseTexture);
	}

	void BuildPipelineShaderQuery(Array<EqString>& shaderQuery) const
	{
		bool fogEnable = true;
		SHADER_PARAM_BOOL_NEG(NoFog, fogEnable, false);
		if (fogEnable)
			shaderQuery.append("DOFOG");

		bool vertexColor = false;
		SHADER_PARAM_BOOL(VertexColor, vertexColor, false);
		if (vertexColor)
			shaderQuery.append("VERTEXCOLOR");

		if (m_flags & MATERIAL_FLAG_ALPHATESTED)
			shaderQuery.append("ALPHATEST");
	}

	void UpdateProxy(IGPUCommandRecorder* cmdRecorder) const
	{
		Vector4D bufferData[2];
		Vector4D textureTransform = GetTextureTransform(m_baseTextureTransformVar, m_baseTextureScaleVar);

		bufferData[0] = m_colorVar.Get();
		bufferData[1] = textureTransform;

		cmdRecorder->WriteBuffer(m_materialParamsBuffer, &bufferData, sizeof(bufferData), 0);
	}

	IGPUBindGroupPtr GetBindGroup(IShaderAPI* renderAPI, EBindGroupId bindGroupId, const BindGroupSetupParams& setupParams) const
	{
		if (bindGroupId == BINDGROUP_CONSTANT)
		{
			if (!setupParams.pipelineInfo.bindGroup[bindGroupId])
			{
				const ITexturePtr& baseTexture = m_baseTexture.Get() ? m_baseTexture.Get() : g_matSystem->GetErrorCheckerboardTexture();
				BindGroupDesc bindGroupDesc = Builder<BindGroupDesc>()
					.Buffer(0, m_materialParamsBuffer)
					.Sampler(1, SamplerStateParams(m_texFilter, m_texAddressMode))
					.Texture(2, baseTexture)
					.End();
				CreateBindGroup(bindGroupDesc, bindGroupId, renderAPI, setupParams.pipelineInfo);
			}
			return setupParams.pipelineInfo.bindGroup[bindGroupId];
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
				.Buffer(0, cameraParamsBuffer)
				.End();
			return CreateBindGroup(bindGroupDesc, bindGroupId, renderAPI, setupParams.pipelineInfo);
		}
		else if (bindGroupId == BINDGROUP_TRANSIENT)
		{
			if (setupParams.pipelineInfo.vertexLayoutId == StringToHashConst("EGFVertexSkinned"))
			{
				ASSERT(setupParams.uniformBuffers[0].signature == RenderBoneTransformID)

				BindGroupDesc bindGroupDesc = Builder<BindGroupDesc>()
					.Buffer(0, setupParams.uniformBuffers[0].bufferView)
					.End();
				return CreateBindGroup(bindGroupDesc, bindGroupId, renderAPI, setupParams.pipelineInfo);
			}
		}
		return GetEmptyBindGroup(renderAPI, bindGroupId, setupParams.pipelineInfo);
	}

	const ITexturePtr& GetBaseTexture(int stage) const
	{
		return m_baseTexture.Get();
	}

	mutable GPUBufferView		m_currentCameraBuffer;
	mutable int					m_currentCameraId{ -1 };
	IGPUBufferPtr				m_materialParamsBuffer;

	MatTextureProxy				m_baseTexture;
	MatVec4Proxy				m_colorVar;
END_SHADER_CLASS
