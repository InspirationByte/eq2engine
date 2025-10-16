#pragma once
#include "renderers/IShaderAPI.h"
#include "ShaderInfo.h"

static_assert(MAX_BINDGROUPS <= nvrhi::c_MaxBindingLayouts, "Max binding layouts is not correct");

class CNVRHIComputePipeline;
struct ShaderInfo;

using NVRHISamplerHandleList = FixedArray<nvrhi::SamplerHandle, 128>;

using NVRHIBindingLayoutList = FixedArray<nvrhi::BindingLayoutHandle, nvrhi::c_MaxBindingLayouts>;
using NVRHIBindingLayoutsCRef = ArrayCRef<nvrhi::BindingLayoutHandle>;

// this shit is really for purposes of delaying bindgroup validation
// need to get rid of this
class CNVRHIBindingLayout : public IGPUBindingLayout
{
public:
	struct EntryId
	{
		int nameId;
		int visibility;
	};
	using BindGroupLayoutOrder = Array<EntryId>;
	using LayoutMapList = FixedArray<BindGroupLayoutOrder, nvrhi::c_MaxBindingLayouts>;

	void			FillBindingSetDescByLayoutMap(const BindGroupDesc& bindGroupDesc, const ShaderInfo& shaderInfo, ArrayCRef<int> shaderModuleIdxs, NVRHISamplerHandleList& rhiSamplers, nvrhi::BindingSetDesc& rhiBindingSetDesc) const;
	
	LayoutMapList	m_layoutOrder;
	int				m_maxBindingIndex[nvrhi::c_MaxBindingLayouts]{ 0 };
	EqString		m_dbgName;
};

using CNVRHIBindingLayoutPtr = CRefPtr<CNVRHIBindingLayout>;

class CNVRHIRenderPipeline : public IGPURenderPipeline
{
public:
	nvrhi::GraphicsPipelineDesc		m_rhiPipelineDesc;
	nvrhi::FramebufferInfo			m_rhiFramebufferinfo;
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

	CNVRHIBindingLayoutPtr		m_bindingLayout;		// if set, it's a shared bind group
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

void nvrhiFillBindingDesc(const BindGroupDesc::Entry& bindGroupEntry, const ShaderInfo::Binding& binding, NVRHISamplerHandleList& rhiSamplers, nvrhi::BindingSetDesc& rhiBindingSetDesc);
void nvrhiFillBindingSetDesc(const BindGroupDesc& bindGroupDesc, const ShaderInfo& shaderInfo, ArrayCRef<int> shaderModuleIdxs, NVRHISamplerHandleList& rhiSamplers, nvrhi::BindingSetDesc& rhiBindingSetDesc);
void nvrhiCreateBindingLayouts(const ShaderInfo& shaderInfo, const IGPUBindingLayout* bindingLayout, ArrayCRef<int> shaderModuleIdxs, nvrhi::ShaderType rhiShaderType, NVRHIBindingLayoutList& rhiBindingLayouts);