#pragma once
#include "renderers/IVertexFormat.h"

class CVertexFormat : public IVertexFormat
{
public:
	CVertexFormat(const char* name, ArrayCRef<VertexLayoutDesc> desc);

	const char*		GetName() const { return m_name.ToCString(); }
	int				GetNameHash() const { return m_nameHash; }

	int				GetVertexSize(int stream) const { return m_vertexDesc[stream].stride; }
	ArrayCRef<VertexLayoutDesc>		GetFormatDesc() const { return m_vertexDesc; }

protected:
	void			Ref_DeleteObject();

	EqString				m_name;
	int						m_nameHash;
	Array<VertexLayoutDesc>	m_vertexDesc{ PP_SL };
};
