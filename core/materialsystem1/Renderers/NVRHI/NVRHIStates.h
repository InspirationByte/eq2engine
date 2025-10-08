#pragma once
#include "renderers/IShaderAPI.h"

using NVRHIBindingLayoutList = FixedArray<nvrhi::BindingLayoutHandle, MAX_BINDGROUPS>;
using NVRHIBindingLayoutsCRef = ArrayCRef<nvrhi::BindingLayoutHandle>;

// this shit is really for purposes of delaying bindgroup validation
// need to get rid of this
class CNVRHIBindingLayout : public IGPUBindingLayout
{
public:
	BindingLayoutDesc				m_pipelineLayoutDesc;
	EqString						m_dbgName;
};

class CNVRHIRenderPipeline : public IGPURenderPipeline
{
public:
	nvrhi::FramebufferInfo			m_rhiFramebufferinfo;
	nvrhi::GraphicsPipelineDesc		m_rhiPipelineDesc;
	nvrhi::GraphicsPipelineHandle	m_rhiRenderPipeline;
	EqString						m_dbgName; 
	uint							m_vertexShaderModuleId{ 0 };
	uint							m_fragmentShaderModuleId{ 0 };
};

class CNVRHIComputePipeline : public IGPUComputePipeline
{
public:
	NVRHIBindingLayoutList			m_rhiBindingLayout;
	nvrhi::ComputePipelineHandle	m_rhiComputePipeline;
	EqString						m_dbgName;
	uint							m_shaderModuleId{ 0 };
};

class CNVRHIBindGroup : public IGPUBindGroup
{
public:
	nvrhi::BindingSetHandle		m_rhiBindingSet;
	EqString					m_dbgName;
};

class CNVRHICommandBuffer : public IGPUCommandBuffer
{
public:
	nvrhi::CommandListHandle	m_rhiCommandList;
	EqString					m_dbgName;
};