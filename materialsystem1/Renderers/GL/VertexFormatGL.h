//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium OpenGL ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#ifndef VERTEXFORMATGL_H
#define VERTEXFORMATGL_H

#include "renderers/IVertexFormat.h"

#define MAX_GL_GENERIC_ATTRIB 24


//**************************************
// Vertex attribute descriptor
//**************************************

struct eqGLVertAttrDesc_t
{
	int					streamId;
	int					sizeInBytes;

	ER_AttributeFormat	attribFormat;
	int					offsetInBytes;

};

//**************************************
// Vertex format
//**************************************

class CVertexFormatGL : public IVertexFormat
{
	friend class		ShaderAPIGL;
public:
	CVertexFormatGL(const char* name, VertexFormatDesc_t* desc, int numAttribs);
	~CVertexFormatGL();

	const char*			GetName() const {return m_name.ToCString(); }

	int					GetVertexSize(int stream);
	void				GetFormatDesc(VertexFormatDesc_t** desc, int& numAttribs);

protected:
	int					m_streamStride[MAX_VERTEXSTREAM];
	EqString			m_name;
	VertexFormatDesc_t*	m_vertexDesc;
	int					m_numAttribs;

	eqGLVertAttrDesc_t	m_genericAttribs[MAX_GL_GENERIC_ATTRIB];

#ifndef GL_NO_DEPRECATED_ATTRIBUTES
	eqGLVertAttrDesc_t	m_hTexCoord[MAX_TEXCOORD_ATTRIB];
	eqGLVertAttrDesc_t	m_hVertex;
	eqGLVertAttrDesc_t	m_hNormal;
	eqGLVertAttrDesc_t	m_hColor;
#endif // GL_NO_DEPRECATED_ATTRIBUTES

};

#endif // VERTEXFORMATGL_H