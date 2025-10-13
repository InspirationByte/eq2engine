#pragma once
#include "WGPUBackend.h"
#include "renderers/IShaderAPI.h"

struct ShaderInfo;

class CWGPUBindingLayout : public IGPUBindingLayout
{
public:
	~CWGPUBindingLayout();

	using BindGroupLayoutMap = Map<int, int>;
	FixedArray<BindGroupLayoutMap, MAX_BINDGROUPS>	m_layoutMap;
	FixedArray<WGPUBindGroupLayout, MAX_BINDGROUPS>	m_rhiBindGroupLayout;
	int						m_maxBindingIndex[MAX_BINDGROUPS]{ 0 };
	WGPUPipelineLayout		m_rhiPipelineLayout{ nullptr };
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
	uint					m_pipelineId{ COM_UINT_MAX };
};

class CWGPUComputePipeline : public IGPUComputePipeline
{
public:
	~CWGPUComputePipeline();

	// TODO: name
	WGPUComputePipeline		m_rhiComputePipeline{ nullptr };
	const ShaderInfo*		m_shaderInfo{ nullptr };
	int						m_computeShaderModuleIdx{ -1 };
	uint					m_pipelineId{ COM_UINT_MAX };
};

class CWGPUBindGroup : public IGPUBindGroup
{
public:
	~CWGPUBindGroup();

	WGPUBindGroup		m_rhiBindGroup{ nullptr };
};

class CWGPUCommandBuffer : public IGPUCommandBuffer
{
public:
	~CWGPUCommandBuffer();

	WGPUCommandBuffer	m_rhiCommandBuffer{ nullptr };
};