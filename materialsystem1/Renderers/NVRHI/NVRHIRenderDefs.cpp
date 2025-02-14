#include "core/core_common.h"
#include "NVRHIRenderDefs.h"
#include "NVRHITexture.h"

void FillNVRHISamplerDescriptor(const SamplerStateParams& samplerParams, nvrhi::SamplerDesc& rhiSamplerDesc)
{
	if (samplerParams.compareFunc != COMPFUNC_NONE)
		rhiSamplerDesc.setReductionType(nvrhi::SamplerReductionType::Comparison);
	// TODO TODO
	// g_nvrhiCompareFunc[samplerParams.compareFunc];

	rhiSamplerDesc.addressU = g_nvrhiAddressMode[samplerParams.addressU];
	rhiSamplerDesc.addressV = g_nvrhiAddressMode[samplerParams.addressV];
	rhiSamplerDesc.addressW = g_nvrhiAddressMode[samplerParams.addressW];
	rhiSamplerDesc.minFilter = samplerParams.minFilter != TEXFILTER_NEAREST;
	rhiSamplerDesc.magFilter = samplerParams.magFilter != TEXFILTER_NEAREST;
	rhiSamplerDesc.mipFilter = samplerParams.mipmapFilter != TEXFILTER_NEAREST;

	if (rhiSamplerDesc.minFilter == TEXFILTER_NEAREST)
		rhiSamplerDesc.maxAnisotropy = 1;
	else
		rhiSamplerDesc.maxAnisotropy = samplerParams.maxAnisotropy;
}
