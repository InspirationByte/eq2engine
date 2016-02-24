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
	glBeginQuery(GL_SAMPLES_PASSED, m_query);
	m_ready = false;
}

// ends the occlusion query issue
void CGLOcclusionQuery::End()
{
	glEndQuery(GL_SAMPLES_PASSED);
	glFlush();
}

// returns status
bool CGLOcclusionQuery::IsReady()
{
	if(m_ready)
		return true;

	m_pixelsVisible = 0;

	GLint available;
	glGetQueryObjectiv(m_query, GL_QUERY_RESULT_AVAILABLE, &available);

	m_ready = (available > 0);

	if(m_ready)
	{
		glGetQueryObjectuiv(m_query, GL_QUERY_RESULT, &m_pixelsVisible);
	}

	return m_ready;
}

// after End() and IsReady() == true it does matter
uint32 CGLOcclusionQuery::GetVisiblePixels()
{
	return m_pixelsVisible;
}
