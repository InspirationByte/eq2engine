#pragma once
#include "renderers/IShaderAPI.h"
#include "ShaderInfo.h"

static_assert(MAX_BINDGROUPS <= nvrhi::c_MaxBindingLayouts, "Max binding layouts is not correct");

class CNVRHIComputePipeline;
struct ShaderInfo;

using NVRHIBindingLayoutList = FixedArray<nvrhi::BindingLayoutHandle, MAX_BINDGROUPS>;
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
	using LayoutMapList = FixedArray<BindGroupLayoutOrder, MAX_BINDGROUPS>;

	void			FillBindingSetDescByLayoutMap(const BindGroupDesc& bindGroupDesc, const ShaderInfo& shaderInfo, ArrayCRef<int> shaderModuleIdxs, nvrhi::BindingSetDesc& rhiBindingSetDesc) const;
	
	LayoutMapList	m_layoutOrder;
	int				m_maxBindingIndex[MAX_BINDGROUPS]{ 0 };
	EqString		m_dbgName;
};

using CNVRHIBindingLayoutPtr = CRefPtr<CNVRHIBindingLayout>;

class CNVRHIRenderPipeline : public IGPURenderPipeline
{
public:
	~CNVRHIRenderPipeline();
	CNVRHIRenderPipeline();

	nvrhi::GraphicsPipelineDesc		m_rhiPipelineDesc;
	nvrhi::FramebufferInfo			m_rhiFramebufferinfo;
	nvrhi::GraphicsPipelineHandle	m_rhiRenderPipeline;
	EqString						m_dbgName;
	const ShaderInfo*				m_shaderInfo{ nullptr };
	int								m_vertexShaderModuleIdx{ -1 };
	int								m_fragmentShaderModuleIdx{ -1 };
	uint							m_pipelineId{ 0 };
};

class CNVRHIComputePipeline : public IGPUComputePipeline
{
public:
	~CNVRHIComputePipeline();
	CNVRHIComputePipeline();

	NVRHIBindingLayoutList			m_rhiBindingLayout;
	nvrhi::ComputePipelineHandle	m_rhiComputePipeline;
	EqString						m_dbgName;
	const ShaderInfo*				m_shaderInfo{ nullptr };
	int								m_computeShaderModuleIdx{ -1 };
	uint							m_pipelineId{ 0 };
};

class CNVRHIBindGroup : public IGPUBindGroup
{
public:
	~CNVRHIBindGroup();
	CNVRHIBindGroup();

	void						MakeResourceRefs(const BindGroupDesc& sourceDesc);
	CNVRHIBindingLayoutPtr		m_bindingLayout;		// if set, it's a shared bind group
	BindGroupDesc				m_bindGroupDesc;

	// binding sets
	Map<uint, nvrhi::BindingSetHandle> m_rhiBindingSets{ PP_SL };

	EqString					m_dbgName;
};

class CNVRHICommandBuffer : public IGPUCommandBuffer
{
public:
	~CNVRHICommandBuffer() = default;

	nvrhi::CommandListHandle	m_rhiCommandList;
	EqString					m_dbgName;
	int							m_cmdListIdx{ -1 };
};

void nvrhiFillSamplerDesc(const SamplerStateParams& samplerParams, nvrhi::SamplerDesc& rhiSamplerDesc);
void nvrhiFillBindingDesc(const BindGroupDesc::Entry& bindGroupEntry, const ShaderInfo::Binding& binding, nvrhi::BindingSetDesc& rhiBindingSetDesc);
void nvrhiFillBindingSetDesc(const BindGroupDesc& bindGroupDesc, const ShaderInfo& shaderInfo, ArrayCRef<int> shaderModuleIdxs, nvrhi::BindingSetDesc& rhiBindingSetDesc);
void nvrhiCreateBindingLayouts(const ShaderInfo& shaderInfo, const IGPUBindingLayout* bindingLayout, ArrayCRef<int> shaderModuleIdxs, nvrhi::ShaderType rhiShaderType, NVRHIBindingLayoutList& rhiBindingLayouts);
void nvrhiFillBindingSets(const ShaderInfo& shaderInfo, ArrayCRef<int> shaderModuleIdxs, ArrayCRef<IGPUBindGroupPtr> bindings, const uint pipelineId, ArrayCRef<nvrhi::BindingLayoutHandle>, nvrhi::BindingSetVector& rhiBindingSets);