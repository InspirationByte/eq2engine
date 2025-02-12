#pragma once
#include "renderers/IShaderAPI.h"

class CNVRHIPipelineLayout : public IGPUPipelineLayout
{
public:
	using BindingLayoutList = FixedArray<nvrhi::BindingLayoutHandle, MAX_BINDGROUPS>;

	BindingLayoutList		m_rhiBindingLayout;
	EqString				m_dbgName;
};

class CNVRHIRenderPipeline : public IGPURenderPipeline
{
public:
	nvrhi::GraphicsPipelineHandle	m_rhiRenderPipeline;
	EqString						m_dbgName;
};

class CNVRHIComputePipeline : public IGPUComputePipeline
{
public:
	nvrhi::ComputePipelineHandle	m_rhiComputePipeline;
	EqString						m_dbgName;
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