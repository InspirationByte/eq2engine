#pragma once
#include "renderers/IShaderAPI.h"

class CWGPUPipelineLayout : public IGPUPipelineLayout
{
public:
	~CWGPUPipelineLayout();

	// TODO: name
	Array<WGPUBindGroupLayout>	m_rhiBindGroupLayout{ PP_SL };
	WGPUPipelineLayout			m_rhiPipelineLayout{ nullptr };
};

class CWGPURenderPipeline : public IGPURenderPipeline
{
public:
	~CWGPURenderPipeline();

	// TODO: name
	WGPURenderPipeline	m_rhiRenderPipeline{ nullptr };
};

class CWGPUBindGroup : public IGPUBindGroup
{
public:
	~CWGPUBindGroup();

	// TODO: name
	WGPUBindGroup		m_rhiBindGroup{ nullptr };
};

class CWGPUCommandBuffer : public IGPUCommandBuffer
{
public:
	~CWGPUCommandBuffer();

	WGPUCommandBuffer	m_rhiCommandBuffer{ nullptr };
};