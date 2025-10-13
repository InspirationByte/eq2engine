#include <nvrhi/nvrhi.h>
#include "core/core_common.h"

#include "NVRHIStates.h"

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