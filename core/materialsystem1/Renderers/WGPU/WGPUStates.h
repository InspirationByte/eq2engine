#pragma once
#include "WGPUBackend.h"
#include "renderers/IShaderAPI.h"

struct ShaderInfo;

class CWGPUPipelineLayout : public IGPUPipelineLayout
{
public:
	~CWGPUPipelineLayout();

	// TODO: name
	FixedArray<WGPUBindGroupLayout, MAX_BINDGROUPS>	m_rhiBindGroupLayout;
	WGPUPipelineLayout								m_rhiPipelineLayout{ nullptr };
};

class CWGPURenderPipeline : public IGPURenderPipeline
{
public:
	~CWGPURenderPipeline();

	// TODO: name
	WGPURenderPipeline		m_rhiRenderPipeline{ nullptr };
	const ShaderInfo*		m_shaderInfo{ nullptr };
	int						m_vertexShaderModuleIdx{ -1 };
	int						m_fragmentShaderModuleIdx{ -1 };
};

class CWGPUComputePipeline : public IGPUComputePipeline
{
public:
	~CWGPUComputePipeline();

	// TODO: name
	WGPUComputePipeline		m_rhiComputePipeline{ nullptr };
	const ShaderInfo*		m_shaderInfo{ nullptr };
	int						m_computeShaderModuleIdx{ -1 };
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