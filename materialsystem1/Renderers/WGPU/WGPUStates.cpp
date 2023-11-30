#include <webgpu/webgpu.h>
#include "core/core_common.h"

#include "WGPUStates.h"

CWGPUPipelineLayout::~CWGPUPipelineLayout()
{
	wgpuPipelineLayoutRelease(m_rhiPipelineLayout);
	for (WGPUBindGroupLayout layout : m_rhiBindGroupLayout)
		wgpuBindGroupLayoutRelease(layout);
}

CWGPURenderPipeline::~CWGPURenderPipeline()
{
	wgpuRenderPipelineRelease(m_rhiRenderPipeline);
}

CWGPUComputePipeline::~CWGPUComputePipeline()
{
	wgpuComputePipelineRelease(m_rhiComputePipeline);
}

CWGPUBindGroup::~CWGPUBindGroup()
{
	wgpuBindGroupRelease(m_rhiBindGroup);
}

CWGPUCommandBuffer::~CWGPUCommandBuffer()
{
	wgpuCommandBufferRelease(m_rhiCommandBuffer);
}