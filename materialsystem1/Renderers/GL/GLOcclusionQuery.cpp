//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: OpenGL Occlusion query
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "shaderapigl_def.h"
#include "GLOcclusionQuery.h"

CGLOcclusionQuery::CGLOcclusionQuery()
{
	glGenQueries(1, &m_query);
}

CGLOcclusionQuery::~CGLOcclusionQuery()
{
	glDeleteQueries(1, &m_query);
}

// begins the occlusion query issue
void CGLOcclusionQuery::Begin()
{
#ifdef USE_GLES2
	glBeginQuery(GL_ANY_SAMPLES_PASSED, m_query);
#else
	glBeginQuery(GL_SAMPLES_PASSED, m_query);
#endif // USE_GLES2
	m_ready = false;
}

// ends the occlusion query issue
void CGLOcclusionQuery::End()
{
#ifdef USE_GLES2
	glEndQuery(GL_ANY_SAMPLES_PASSED);
#else
	glEndQuery(GL_SAMPLES_PASSED);
#endif // USE_GLES2
	
	glFlush();
}

// returns status
bool CGLOcclusionQuery::IsReady()
{
	if(m_ready)
		return true;

	m_pixelsVisible = 0;

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

	return m_ready;
}

// after End() and IsReady() == true it does matter
uint32 CGLOcclusionQuery::GetVisiblePixels()
{
	return m_pixelsVisible;
}
