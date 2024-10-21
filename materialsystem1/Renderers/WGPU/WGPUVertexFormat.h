#pragma once
#include "renderers/IVertexFormat.h"

class CWGPUVertexFormat : public IVertexFormat
{
public:
	CWGPUVertexFormat(const char* name, ArrayCRef<VertexLayoutDesc> desc)
	{
		m_name = name;
		m_nameHash = StringId24(name);

		m_vertexDesc.setNum(desc.numElem());
		for (int i = 0; i < desc.numElem(); i++)
			m_vertexDesc[i] = desc[i];
	}

	const char* GetName() const { return m_name.ToCString(); }
	int			GetNameHash() const { return m_nameHash; }

	int	GetVertexSize(int stream) const
	{
		return m_vertexDesc[stream].stride;
	}

	ArrayCRef<VertexLayoutDesc> GetFormatDesc() const
	{
		return m_vertexDesc;
	}

protected:
	EqString				m_name;
	int						m_nameHash;
	Array<VertexLayoutDesc>	m_vertexDesc{ PP_SL };
};
