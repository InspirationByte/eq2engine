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
	PristineGrid,
	SHADER_VERTEX_ID(DynMeshVertex),
)
	SHADER_INIT_PARAMS()
	{
		m_lineWidth = GetMaterialVar("lineWidth", "1");
		m_lineSpacing = GetMaterialVar("lineSpacing", "1");
		m_blendMode = SHADER_BLEND_TRANSLUCENT;
		m_flags |= MATERIAL_FLAG_NO_Z_WRITE;

		m_materialParamsBuffer = MakeParameterUniformBuffer(
			m_lineWidth.Get(), 
			m_lineSpacing.Get(),
			0.0f,
			0.0f
		);
	}

	SHADER_INIT_TEXTURES()
	{
	}

	void UpdateProxy(IGPUCommandRecorder* cmdRecorder) const
	{
		Vector4D bufferData[1];
		bufferData[0].x = m_lineWidth.Get();
		bufferData[0].y = m_lineSpacing.Get();

		cmdRecorder->WriteBuffer(m_materialParamsBuffer, &bufferData, sizeof(bufferData), 0);
	}

	IGPUBindGroupPtr GetBindGroup(IShaderAPI* renderAPI, EBindGroupId bindGroupId, const BindGroupSetupParams& setupParams) const
	{
		if (bindGroupId == BINDGROUP_CONSTANT)
		{
			if (!setupParams.pipelineInfo.bindGroup[bindGroupId])
			{
				BindGroupDesc bindGroupDesc = Builder<BindGroupDesc>()
					.Buffer(0, m_materialParamsBuffer)
					.End();
				CreatePersistentBindGroup(bindGroupDesc, bindGroupId, renderAPI, setupParams.pipelineInfo);
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
		return setupParams.pipelineInfo.bindGroup[bindGroupId];
	}

	const ITexturePtr& GetBaseTexture(int stage) const
	{
		return ITexturePtr::Null();
	}

	mutable GPUBufferView		m_currentCameraBuffer;
	mutable int					m_currentCameraId{ -1 };
	IGPUBufferPtr				m_materialParamsBuffer;

	MatFloatProxy				m_lineWidth;
	MatFloatProxy				m_lineSpacing;
END_SHADER_CLASS
