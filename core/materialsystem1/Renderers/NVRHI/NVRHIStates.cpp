#include <nvrhi/nvrhi.h>
#include "core/core_common.h"

#include "NVRHIStates.h"
#include "NVRHIBuffer.h"
#include "NVRHIRenderAPI.h"

void nvrhiFillSamplerDesc(const SamplerStateParams& samplerParams, nvrhi::SamplerDesc& rhiSamplerDesc)
{
	ASSERT(samplerParams.maxAnisotropy > 0);

	rhiSamplerDesc
		.setReductionType(samplerParams.compareFunc == COMPFUNC_NONE ? nvrhi::SamplerReductionType::Standard : nvrhi::SamplerReductionType::Comparison)
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

	static thread_local BitArray::STORAGE_TYPE usedBindEntryBits[32];
	memset(usedBindEntryBits, 0, sizeof(usedBindEntryBits));

	BitArray usedBindingEntries(usedBindEntryBits, sizeof(usedBindEntryBits) * 8);

	for (const int moduleIdx : shaderModuleIdxs)
	{
		if (moduleIdx < 0)
			continue;

		const ShaderInfo::Module& shaderModule = shaderInfo.modules[moduleIdx];
		ArrayCRef<int> bindingIds = shaderInfo.GetBindingIds(shaderModule);
		for (int i = 0; i < bindingIds.numElem(); ++i)
		{
			if (usedBindingEntries[bindingIds[i]])
				continue;

			if (!shaderModule.usedBindings[i])
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

			usedBindingEntries.setTrue(bindingIds[i]);

			nvrhiFillBindingDesc(bindGroupDesc.entries[entryIdx], binding, rhiSamplers, rhiBindingSetDesc);
		}
	}

	ASSERT_MSG(bindGroupDesc.entries.numElem() >= bindingsToResolve, "Bad binding entry count: %d, expected %d", bindGroupDesc.entries.numElem(), bindingsToResolve);
	ASSERT_MSG(rhiBindingSetDesc.bindings.size() == bindingsToResolve, "Incorrect binding ids, resolved: %d, expected %d", rhiBindingSetDesc.bindings.size(), bindingsToResolve);
}

void CNVRHIBindingLayout::FillBindingSetDescByLayoutMap(const BindGroupDesc& bindGroupDesc, const ShaderInfo& shaderInfo, ArrayCRef<int> shaderModuleIdxs, NVRHISamplerHandleList& rhiSamplers, nvrhi::BindingSetDesc& rhiBindingSetDesc) const
{
	const BindGroupLayoutMap& groupLayoutMap = m_layoutMap[bindGroupDesc.groupIdx];

	ASSERT_FAIL("Unimplemented");
	//for (const BindGroupDesc::Entry& bindGroupEntry : bindGroupDesc.entries)
	//{
	//	WGPUBindGroupEntry& rhiBindGroupEntryDesc = rhiBindGroupEntryList.append();
	//	FindWGPUBindGroupEntry(rhiDevice, bindGroupEntry, "", rhiBindGroupEntryDesc);
	//
	//	if (bindGroupEntry.binding > maxBindingIndex)
	//	{
	//		auto it = bindGroupMap.find(bindGroupEntry.binding);
	//		if (it)
	//			rhiBindGroupEntryDesc.binding = *it;
	//	}
	//}

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
