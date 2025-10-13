#pragma once
#include "renderers/IShaderAPI.h"

struct ShaderInfo;

using NVRHIBindingLayoutList = FixedArray<nvrhi::BindingLayoutHandle, MAX_BINDGROUPS>;
using NVRHIBindingLayoutsCRef = ArrayCRef<nvrhi::BindingLayoutHandle>;

// this shit is really for purposes of delaying bindgroup validation
// need to get rid of this
class CNVRHIBindingLayout : public IGPUBindingLayout
{
public:

	using BindGroupLayoutMap = Map<int, int>;
	FixedArray<BindGroupLayoutMap, MAX_BINDGROUPS>	m_layoutMap;
	int					m_maxBindingIndex[MAX_BINDGROUPS]{ 0 };
	EqString			m_dbgName;
};

using CNVRHIBindingLayoutPtr = CRefPtr<CNVRHIBindingLayout>;

class CNVRHIRenderPipeline : public IGPURenderPipeline
{
public:
	nvrhi::FramebufferInfo			m_rhiFramebufferinfo;
	nvrhi::GraphicsPipelineDesc		m_rhiPipelineDesc;
	nvrhi::GraphicsPipelineHandle	m_rhiRenderPipeline;
	EqString						m_dbgName;
	const ShaderInfo*				m_shaderInfo{ nullptr };
	int								m_vertexShaderModuleIdx{ -1 };
	int								m_fragmentShaderModuleIdx{ -1 };
};

class CNVRHIComputePipeline : public IGPUComputePipeline
{
public:
	NVRHIBindingLayoutList			m_rhiBindingLayout;
	nvrhi::ComputePipelineHandle	m_rhiComputePipeline;
	EqString						m_dbgName;
	const ShaderInfo*				m_shaderInfo{ nullptr };
	int								m_computeShaderModuleIdx{ -1 };
};

class CNVRHIBindGroup : public IGPUBindGroup
{
public:
	~CNVRHIBindGroup();

	void						MakeResourceRefs(const BindGroupDesc& sourceDesc);

	CNVRHIBindingLayoutPtr		m_bindingLayout;	// if set, it's a shared bind group
	BindGroupDesc				m_bindGroupDesc;
	nvrhi::BindingSetHandle		m_rhiBindingSet;
	EqString					m_dbgName;
};

class CNVRHICommandBuffer : public IGPUCommandBuffer
{
public:
	nvrhi::CommandListHandle	m_rhiCommandList;
	EqString					m_dbgName;
};