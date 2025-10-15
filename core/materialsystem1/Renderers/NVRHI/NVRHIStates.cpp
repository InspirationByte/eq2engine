#include <nvrhi/nvrhi.h>
#include "core/core_common.h"

#include "NVRHIStates.h"
#include "NVRHIBuffer.h"
#include "NVRHIRenderAPI.h"

static void nvrhiFillSamplerDesc(const SamplerStateParams& samplerParams, nvrhi::SamplerDesc& rhiSamplerDesc)
{
	ASSERT(samplerParams.maxAnisotropy > 0);

	rhiSamplerDesc
		.setReductionType(samplerParams.compareFunc == COMPFUNC_NONE ? nvrhi::SamplerReductionType::Standard : nvrhi::SamplerReductionType::Comparison)
		.setComparisonFunc(g_nvrhiCompareFunc[samplerParams.compareFunc])
		.setAddressU(g_nvrhiAddressMode[samplerParams.addressU])
		.setAddressV(g_nvrhiAddressMode[samplerParams.addressV])
		.setAddressW(g_nvrhiAddressMode[samplerParams.addressW])
		.setMinFilter(samplerParams.minFilter != TEXFILTER_NEAREST)
		.setMagFilter(samplerParams.magFilter != TEXFILTER_NEAREST)
		.setMipFilter(samplerParams.mipmapFilter != TEXFILTER_NEAREST)
		.setMaxAnisotropy(rhiSamplerDesc.minFilter == TEXFILTER_NEAREST ? 1 : samplerParams.maxAnisotropy);
}

void nvrhiFillBindingDesc(const BindGroupDesc::Entry& bindGroupEntry, const ShaderInfo::Binding& binding, NVRHISamplerHandleList& rhiSamplers, nvrhi::BindingSetDesc& rhiBindingSetDesc)
{
	nvrhi::IDevice* rhiDevice = CNVRHIRenderAPI::Instance.GetNVRHIDevice();

	switch (bindGroupEntry.type)
	{
	case BINDENTRY_BUFFER:
	{
		ASSERT(binding.type == bindGroupEntry.type);
		// TODO: check buffer usage

		CNVRHIBuffer* buffer = static_cast<CNVRHIBuffer*>(bindGroupEntry.buffer.ptr);
		if (buffer)
		{
			int64 minAligment = binding.rangeType == BINDING_RANGE_CBV ?
				CNVRHIRenderAPI::Instance.GetCaps().minUniformBufferOffsetAlignment :
				CNVRHIRenderAPI::Instance.GetCaps().minStorageBufferOffsetAlignment;

			//ASSERT_MSG((bindGroupEntry.buffer.offset & CNVRHIRenderAPI::Instance.GetCaps().minUniformBufferOffsetAlignment) == 0, "Invalid buffer offset alignment");

			int64 minAligmentMask = minAligment - 1;
			const int alignedSize = min(bindGroupEntry.buffer.size + minAligmentMask & ~minAligmentMask, buffer->GetSize() - bindGroupEntry.buffer.offset);
			nvrhi::BufferRange bufferRange = bindGroupEntry.buffer.size < 0 ? nvrhi::EntireBuffer : nvrhi::BufferRange(bindGroupEntry.buffer.offset, alignedSize);

			switch (binding.rangeType)
			{
			case BINDING_RANGE_CBV:
				rhiBindingSetDesc.addItem(
					nvrhi::BindingSetItem()
					.ConstantBuffer(binding.registerIdx, buffer->GetNVRHIBufferHandle(), bufferRange)
				);
				break;
			case BINDING_RANGE_SRV:
				rhiBindingSetDesc.addItem(
					nvrhi::BindingSetItem()
					.RawBuffer_SRV(binding.registerIdx, buffer->GetNVRHIBufferHandle(), bufferRange)
				);
				break;
			case BINDING_RANGE_UAV:
				rhiBindingSetDesc.addItem(
					nvrhi::BindingSetItem()
					.RawBuffer_UAV(binding.registerIdx, buffer->GetNVRHIBufferHandle(), bufferRange)
				);
				break;
			}
		}
		else
			ASSERT_FAIL("NULL buffer for binding %d", bindGroupEntry.binding);

		break;
	}
	case BINDENTRY_SAMPLER:
	{
		ASSERT(binding.type == bindGroupEntry.type);
		ASSERT(binding.rangeType == BINDING_RANGE_SAMPLER);

		auto rhiSamplerDesc = nvrhi::SamplerDesc();
		nvrhiFillSamplerDesc(bindGroupEntry.sampler, rhiSamplerDesc);

		nvrhi::SamplerHandle rhiSampler = rhiDevice->createSampler(rhiSamplerDesc);

		rhiBindingSetDesc.addItem(
			nvrhi::BindingSetItem()
			.Sampler(binding.registerIdx, rhiSampler)
		);
		rhiSamplers.append(rhiSampler);
		break;
	}
	case BINDENTRY_STORAGETEXTURE:
	case BINDENTRY_TEXTURE:
		ASSERT(binding.type == BINDENTRY_STORAGETEXTURE || binding.type == BINDENTRY_TEXTURE);
		// TODO: check texture usage
		CNVRHITexture* texture = static_cast<CNVRHITexture*>(bindGroupEntry.texture.ptr);
		if (texture)
		{
			ASSERT_MSG(texture->GetNVRHITextureViewCount(), "Texture '%s' has no views", texture->GetName());

			switch (binding.rangeType)
			{
			case BINDING_RANGE_SRV:
				rhiBindingSetDesc.addItem(
					nvrhi::BindingSetItem()
					.Texture_SRV(binding.registerIdx, texture->GetNVRHITextureHandle(), nvrhi::Format::UNKNOWN, texture->GetNVRHITextureView(bindGroupEntry.texture.arraySlice))
				);
				break;
			case BINDING_RANGE_UAV:
				rhiBindingSetDesc.addItem(
					nvrhi::BindingSetItem()
					.Texture_UAV(binding.registerIdx, texture->GetNVRHITextureHandle(), nvrhi::Format::UNKNOWN, texture->GetNVRHITextureView(bindGroupEntry.texture.arraySlice))
				);
				break;
			}
		}
		else
			ASSERT_FAIL("NULL texture for binding %d", bindGroupEntry.binding);
		break;
	}
}

void nvrhiFillBindingSetDesc(const BindGroupDesc& bindGroupDesc, const ShaderInfo& shaderInfo, ArrayCRef<int> shaderModuleIdxs, NVRHISamplerHandleList& rhiSamplers, nvrhi::BindingSetDesc& rhiBindingSetDesc)
{
	int bindingsToResolve = 0;

	static thread_local Map<int, int> usedShaderBindingIdxs{ PP_SL };
	usedShaderBindingIdxs.clear();

	for (const int moduleIdx : shaderModuleIdxs)
	{
		if (moduleIdx < 0)
			continue;

		const ShaderInfo::Module& shaderModule = shaderInfo.modules[moduleIdx];
		ArrayCRef<int> bindingIds = shaderInfo.GetBindingIds(shaderModule);
		for (int i = 0; i < bindingIds.numElem(); ++i)
		{
			if (!shaderModule.usedBindings[i])
				continue;

			if (usedShaderBindingIdxs.find(bindingIds[i]))
				continue;

			const ShaderInfo::Binding& binding = shaderInfo.bindings[bindingIds[i]];
			if (binding.descriptorSetIdx != bindGroupDesc.groupIdx)
				continue;

			++bindingsToResolve;

			const int entryIdx = arrayFindIndexF(bindGroupDesc.entries, [&](const BindGroupDesc::Entry& bindGroupEntry) {
				// check if name id is used
				if (bindGroupEntry.binding > bindingIds.numElem())
					return bindGroupEntry.binding == binding.nameId;

				return bindGroupDesc.groupIdx == binding.descriptorSetIdx && bindGroupEntry.binding == binding.index;
			});

			if (entryIdx == -1)
				continue;

			usedShaderBindingIdxs.insert(bindingIds[i], entryIdx);
			//nvrhiFillBindingDesc(bindGroupDesc.entries[entryIdx], binding, rhiSamplers, rhiBindingSetDesc);
		}
	}

	for (auto bindingIt = usedShaderBindingIdxs.begin(); bindingIt; ++bindingIt)
	{
		const ShaderInfo::Binding& binding = shaderInfo.bindings[bindingIt.key()];
		nvrhiFillBindingDesc(bindGroupDesc.entries[bindingIt.value()], binding, rhiSamplers, rhiBindingSetDesc);
	}

	ASSERT_MSG(bindGroupDesc.entries.numElem() >= bindingsToResolve, "Bad binding entry count: %d, expected %d", bindGroupDesc.entries.numElem(), bindingsToResolve);
	ASSERT_MSG(usedShaderBindingIdxs.size() == bindingsToResolve, "Incorrect binding ids, resolved: %d, expected %d", usedShaderBindingIdxs.size(), bindingsToResolve);
}

void CNVRHIBindingLayout::FillBindingSetDescByLayoutMap(const BindGroupDesc& bindGroupDesc, const ShaderInfo& shaderInfo, ArrayCRef<int> shaderModuleIdxs, NVRHISamplerHandleList& rhiSamplers, nvrhi::BindingSetDesc& rhiBindingSetDesc) const
{
#if 1
	const BindGroupLayoutOrder& layoutOrder = m_layoutOrder[bindGroupDesc.groupIdx];
	for (const int nameId : layoutOrder)
	{
		const int idx = arrayFindIndexF(bindGroupDesc.entries, [&](const BindGroupDesc::Entry& entry) {
			return entry.binding == nameId;
		});
		if (idx == -1) 
		{
			MsgError("Bindroup missing binding");
			continue;
		}

		int bindingIdx = -1;

		// find binding in shader module
		for (const int moduleIdx : shaderModuleIdxs)
		{
			if (moduleIdx < 0)
				continue;

			const ShaderInfo::Module& shaderModule = shaderInfo.modules[moduleIdx];
			ArrayCRef<int> bindingIds = shaderInfo.GetBindingIds(shaderModule);

			for (int i = 0; i < bindingIds.numElem(); ++i)
			{
				if (!shaderModule.usedBindings[i])
					continue;

				const ShaderInfo::Binding& binding = shaderInfo.bindings[bindingIds[i]];
				if (binding.descriptorSetIdx != bindGroupDesc.groupIdx)
					continue;

				if (binding.nameId == nameId)
				{
					bindingIdx = bindingIds[i]; 
					break;
				}
			}

			if (bindingIdx != -1)
				break;
		}

		if (bindingIdx == -1)
			continue;	// binding not found for this shader - skip

		nvrhiFillBindingDesc(bindGroupDesc.entries[idx], shaderInfo.bindings[bindingIdx], rhiSamplers, rhiBindingSetDesc);
	}
#else
	nvrhiFillBindingSetDesc(bindGroupDesc, shaderInfo, shaderModuleIdxs, rhiSamplers, rhiBindingSetDesc);
#endif
}

static void nvrhiAddBindingToLayout(nvrhi::BindingLayoutDesc& layoutDesc, const ShaderInfo::Binding& binding)
{
	switch (binding.type)
	{
	case BINDENTRY_BUFFER:
		switch (binding.rangeType)
		{
		case BINDING_RANGE_CBV:
			layoutDesc.addItem(nvrhi::BindingLayoutItem::ConstantBuffer(binding.registerIdx));
			break;
		case BINDING_RANGE_SRV:
			layoutDesc.addItem(nvrhi::BindingLayoutItem::RawBuffer_SRV(binding.registerIdx));
			break;
		case BINDING_RANGE_UAV:
			layoutDesc.addItem(nvrhi::BindingLayoutItem::RawBuffer_UAV(binding.registerIdx));
			break;
		}
		break;
	case BINDENTRY_SAMPLER:
		layoutDesc.addItem(nvrhi::BindingLayoutItem::Sampler(binding.registerIdx));
		break;
	case BINDENTRY_TEXTURE:
		layoutDesc.addItem(nvrhi::BindingLayoutItem::Texture_SRV(binding.registerIdx));
		break;
	case BINDENTRY_STORAGETEXTURE:
		if (binding.rangeType == BINDING_RANGE_SRV)
			layoutDesc.addItem(nvrhi::BindingLayoutItem::Texture_SRV(binding.registerIdx));
		else
			layoutDesc.addItem(nvrhi::BindingLayoutItem::Texture_UAV(binding.registerIdx));
		break;
	}
}

void nvrhiCreateBindingLayouts(const ShaderInfo& shaderInfo, const IGPUBindingLayout* bindingLayout, ArrayCRef<int> shaderModuleIdxs, nvrhi::ShaderType rhiShaderType, NVRHIBindingLayoutList& rhiBindingLayouts)
{
	static thread_local Set<uint> usedShaderBindingIdxs{ PP_SL };
	usedShaderBindingIdxs.clear();

	static thread_local Map<int, int> bindingNamesToIdx{ PP_SL };
	bindingNamesToIdx.clear();

	// used ranges and registers
	static thread_local Set<uint> usedRegisters{ PP_SL };
	usedRegisters.clear();

	// create shader binding layouts
	int maxBindGroupIdx = -1;
	{
		for (const int moduleIdx : shaderModuleIdxs)
		{
			if (moduleIdx < 0)
				continue;

			const ShaderInfo::Module& shaderModule = shaderInfo.modules[moduleIdx];
			ArrayCRef<int> bindingIds = shaderInfo.GetBindingIds(shaderModule);
			for (int i = 0; i < bindingIds.numElem(); ++i)
			{
				if (!shaderModule.usedBindings[i])
					continue;

				const int bindingIdx = bindingIds[i];
				if (usedShaderBindingIdxs.find(bindingIdx))
					continue;

				const ShaderInfo::Binding& binding = shaderInfo.bindings[bindingIdx];
				maxBindGroupIdx = max(maxBindGroupIdx, binding.descriptorSetIdx);

				if (usedRegisters.find(binding.rangeType | (binding.registerIdx << 8)))
				{
#ifdef DEBUG_SHADER_BINDINGS
					ASSERT_FAIL("Pipeline %s has binding %s registers overlapping between Vertex & Fragment stages, please merge shader bindings in one file\n", shaderInfo.shaderName.ToCString(), binding.name.ToCString());
#else
					ASSERT_FAIL("Pipeline %s has binding registers overlapping between Vertex & Fragment stages, please merge shader bindings in one file\n", shaderInfo.shaderName.ToCString());
#endif
					break;
				}

				usedRegisters.insert(binding.rangeType | (binding.registerIdx << 8));
				usedShaderBindingIdxs.insert(bindingIdx);
				bindingNamesToIdx.insert(binding.nameId, bindingIdx);
			}
		}
	}

	if (maxBindGroupIdx >= 0)
	{
		FixedArray<nvrhi::BindingLayoutDesc, nvrhi::c_MaxBindingLayouts> rhiBindingLayoutDescList;
		rhiBindingLayoutDescList.setNum(maxBindGroupIdx + 1);

		const CNVRHIBindingLayout* bindingLayoutImpl = static_cast<const CNVRHIBindingLayout*>(bindingLayout);
		if (bindingLayoutImpl)
		{
			// validate provided binding layout and order bindings in it's way
			for (ArrayCRef<int> layoutOrderList : bindingLayoutImpl->m_layoutOrder)
			{
				for (const int nameId : layoutOrderList)
				{
					auto it = bindingNamesToIdx.find(nameId);
					if (!it)
						continue;

					const ShaderInfo::Binding& binding = shaderInfo.bindings[*it];
					nvrhiAddBindingToLayout(rhiBindingLayoutDescList[binding.descriptorSetIdx], binding);
				}
			}
		}
		else
		{
			for (auto bindingIt = usedShaderBindingIdxs.begin(); bindingIt; ++bindingIt)
			{
				const ShaderInfo::Binding& binding = shaderInfo.bindings[bindingIt.key()];
				nvrhiAddBindingToLayout(rhiBindingLayoutDescList[binding.descriptorSetIdx], binding);
			}
		}

		nvrhi::IDevice* rhiDevice = CNVRHIRenderAPI::Instance.GetNVRHIDevice();

		for (nvrhi::BindingLayoutDesc& rhiDesc : rhiBindingLayoutDescList)
		{
			rhiDesc.setVisibility(rhiShaderType);
			rhiBindingLayouts.append(rhiDevice->createBindingLayout(rhiDesc));
		}
	}
}

CNVRHIBindGroup::~CNVRHIBindGroup()
{
	for (const BindGroupDesc::Entry& entry : m_bindGroupDesc.entries)
	{
		switch (entry.type)
		{
		case BINDENTRY_BUFFER:
			entry.buffer.ptr->Ref_Drop();
			break;
		case BINDENTRY_STORAGETEXTURE:
		case BINDENTRY_TEXTURE:
			entry.texture.ptr->Ref_Drop();
			break;
		}
	}
}

void CNVRHIBindGroup::MakeResourceRefs(const BindGroupDesc& sourceDesc)
{
	m_bindGroupDesc.name = sourceDesc.name;
	m_bindGroupDesc.groupIdx = sourceDesc.groupIdx;
	m_bindGroupDesc.entries.reserve(sourceDesc.entries.numElem());
	for (const BindGroupDesc::Entry& entry : sourceDesc.entries)
	{
		switch (entry.type)
		{
		case BINDENTRY_BUFFER:
			entry.buffer.ptr->Ref_Grab();
			break;
		case BINDENTRY_STORAGETEXTURE:
		case BINDENTRY_TEXTURE:
			entry.texture.ptr->Ref_Grab();
			break;
		}
		m_bindGroupDesc.entries.append(entry);
	}
}
