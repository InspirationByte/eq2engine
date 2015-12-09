//**************Copyright (C) Two Dark Interactive Software 2009**************
//
// Description: DarkTech Middle-Level rendering API (ShaderAPI)
//				Vertex Format OpenGL declaration
//
//****************************************************************************

#ifndef VERTEXFORMATGL_H
#define VERTEXFORMATGL_H

#include "IVertexFormat.h"

#define MAX_GL_GENERIC_ATTRIB 24


//**************************************
// Vertex attribute descriptor
//**************************************

struct eqGLVertAttrDesc_t
{
	int					m_nStream;
	int					m_nSize;

	AttributeFormat_e	m_nFormat;
	int					m_nOffset;

};

//**************************************
// Vertex format
//**************************************

class CVertexFormatGL : public IVertexFormat
{
public:
	
	friend class		ShaderAPIGL;

						CVertexFormatGL();
	int8				GetVertexSizePerStream(int16 nStream);

protected:
	eqGLVertAttrDesc_t	m_hGeneric[MAX_GL_GENERIC_ATTRIB];
	eqGLVertAttrDesc_t	m_hTexCoord[MAX_TEXCOORD_ATTRIB];
	eqGLVertAttrDesc_t	m_hVertex;
	eqGLVertAttrDesc_t	m_hNormal;
	eqGLVertAttrDesc_t	m_hColor;

	int16				m_nVertexSize[MAX_VERTEXSTREAM];
	int16				m_nMaxGeneric;
	int16				m_nMaxTexCoord;
};

#endif // VERTEXFORMATGL_H