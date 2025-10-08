#include "core/core_common.h"

#include "WGPUStates.h"

CWGPUBindingLayout::~CWGPUBindingLayout()
{
	if(m_rhiPipelineLayout)
		wgpuPipelineLayoutRelease(m_rhiPipelineLayout);

	for (WGPUBindGroupLayout layout : m_rhiBindGroupLayout)
	{
		if(layout)
			wgpuBindGroupLayoutRelease(layout);
	}
}

//--------------------------------------------

CWGPURenderPipeline::~CWGPURenderPipeline()
{
	wgpuRenderPipelineRelease(m_rhiRenderPipeline);
}

//--------------------------------------------

CWGPUComputePipeline::~CWGPUComputePipeline()
{
	wgpuComputePipelineRelease(m_rhiComputePipeline);
}

//--------------------------------------------

CWGPUBindGroup::~CWGPUBindGroup()
{
	wgpuBindGroupRelease(m_rhiBindGroup);
}

//--------------------------------------------

CWGPUCommandBuffer::~CWGPUCommandBuffer()
{
	wgpuCommandBufferRelease(m_rhiCommandBuffer);
}