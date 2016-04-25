//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: OpenGL Occlusion query
//////////////////////////////////////////////////////////////////////////////////

#include "GLOcclusionQuery.h"
#include "ShaderAPIGL.h"

#ifdef USE_GLES2
#include "glad_es3.h"
#else
#include "glad.h"
#endif

CGLOcclusionQuery::CGLOcclusionQuery()
{
	m_ready = false;
	m_pixelsVisible = 0;
	m_query = GL_NONE;

	glGenQueries(1, &m_query);
}

CGLOcclusionQuery::~CGLOcclusionQuery()
{
	glDeleteQueries(1, &m_query);
}

// begins the occlusion query issue
void CGLOcclusionQuery::Begin()
{
	ShaderAPIGL* glAPI = (ShaderAPIGL*)g_pShaderAPI;
	glAPI->GL_CRITICAL();

#ifdef USE_GLES2
	glBeginQuery(GL_ANY_SAMPLES_PASSED, m_query);
#else
	glBeginQuery(GL_SAMPLES_PASSED, m_query);
#endif // USE_GLES2
	m_ready = false;

	glAPI->GL_END_CRITICAL();
}

// ends the occlusion query issue
void CGLOcclusionQuery::End()
{
	ShaderAPIGL* glAPI = (ShaderAPIGL*)g_pShaderAPI;
	glAPI->GL_CRITICAL();

#ifdef USE_GLES2
	glEndQuery(GL_ANY_SAMPLES_PASSED);
#else
	glEndQuery(GL_SAMPLES_PASSED);
#endif // USE_GLES2
	
	glFlush();

	glAPI->GL_END_CRITICAL();
}

// returns status
bool CGLOcclusionQuery::IsReady()
{
	if(m_ready)
		return true;

	m_pixelsVisible = 0;

	ShaderAPIGL* glAPI = (ShaderAPIGL*)g_pShaderAPI;
	glAPI->GL_CRITICAL();

	GLuint available;
	glGetQueryObjectuiv(m_query, GL_QUERY_RESULT_AVAILABLE, &available);

	m_ready = (available > 0);

	if(m_ready)
	{
		glGetQueryObjectuiv(m_query, GL_QUERY_RESULT, &m_pixelsVisible);
		
#ifdef USE_GLES2
		// too sad GL ES can't properly read pixel count
		m_pixelsVisible = (m_pixelsVisible & GL_TRUE) == GL_TRUE;
#endif // USE_GLES2
	}

	glAPI->GL_END_CRITICAL();

	return m_ready;
}

// after End() and IsReady() == true it does matter
uint32 CGLOcclusionQuery::GetVisiblePixels()
{
	return m_pixelsVisible;
}
