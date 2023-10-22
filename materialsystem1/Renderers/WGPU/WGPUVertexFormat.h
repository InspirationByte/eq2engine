#pragma once
#include "renderers/IVertexFormat.h"

class CWGPUVertexFormat : public IVertexFormat
{
public:
	CWGPUVertexFormat(const char* name, const VertexFormatDesc* desc, int nAttribs)
	{
		m_name = name;
		m_nameHash = StringToHash(name);
		memset(m_streamStride, 0, sizeof(m_streamStride));

		m_vertexDesc.setNum(nAttribs);
		for (int i = 0; i < nAttribs; i++)
			m_vertexDesc[i] = desc[i];
	}

	const char* GetName() const { return m_name.ToCString(); }
	int			GetNameHash() const { return m_nameHash; }

	int	GetVertexSize(int stream) const
	{
		return 0;
	}

	ArrayCRef<VertexFormatDesc> GetFormatDesc() const
	{
		return m_vertexDesc;
	}

protected:
	int						m_streamStride[MAX_VERTEXSTREAM];
	EqString				m_name;
	int						m_nameHash;
	Array<VertexFormatDesc>	m_vertexDesc{ PP_SL };
};
