#pragma once
#include "renderers/IShaderAPI.h"

class CNVRHIPipelineLayout : public IGPUPipelineLayout
{
public:
	nvrhi::BindingLayoutHandle		m_rhiPipelineLayout;
};

class CNVRHIRenderPipeline : public IGPURenderPipeline
{
public:
	nvrhi::GraphicsPipelineHandle	m_rhiRenderPipeline;
};

class CNVRHIComputePipeline : public IGPUComputePipeline
{
public:
	nvrhi::ComputePipelineHandle	m_rhiComputePipeline;
};

class CNVRHIBindGroup : public IGPUBindGroup
{
public:
	nvrhi::BindingSetHandle			m_rhiBindingSet;
};

class CNVRHICommandBuffer : public IGPUCommandBuffer
{
public:
	nvrhi::CommandListHandle		m_rhiCommandList;
};