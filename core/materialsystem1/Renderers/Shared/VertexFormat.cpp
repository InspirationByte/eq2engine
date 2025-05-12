#include "core/core_common.h"

#include "renderers/ShaderAPI_defs.h"
#include "VertexFormat.h"
#include "ShaderAPI.h"

CVertexFormat::CVertexFormat(const char* name, ArrayCRef<VertexLayoutDesc> desc)
{
	m_name = name;
	m_nameHash = StringId24(name);

	m_vertexDesc.setNum(desc.numElem());
	for (int i = 0; i < desc.numElem(); i++)
		m_vertexDesc[i] = desc[i];
}

void CVertexFormat::Ref_DeleteObject()
{
	ShaderAPI_Base::Instance.DestroyVertexFormat(this);
	RefCountedObject::Ref_DeleteObject();
}