//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: OpenGL Occlusion query 
//////////////////////////////////////////////////////////////////////////////////

#include "GLOcclusionQuery.h"
#include "ShaderAPIGL.h"
#include "gl_caps.hpp"

CGLOcclusionQuery::CGLOcclusionQuery()
{
	m_ready = false;
	m_pixelsVisible = 0;
	m_query = gl::NONE;

	gl::GenQueries(1, &m_query);
}

CGLOcclusionQuery::~CGLOcclusionQuery()
{
	gl::DeleteQueries(1, &m_query);
}

// begins the occlusion query issue
void CGLOcclusionQuery::Begin()
{
	gl::BeginQuery(gl::SAMPLES_PASSED, m_query);
	m_ready = false;
}

// ends the occlusion query issue
void CGLOcclusionQuery::End()
{
	gl::EndQuery(gl::SAMPLES_PASSED);
	gl::Flush();
}

// returns status
bool CGLOcclusionQuery::IsReady()
{
	if(m_ready)
		return true;

	m_pixelsVisible = 0;

	GLint available;
	gl::GetQueryObjectiv(m_query, gl::QUERY_RESULT_AVAILABLE, &available);

	m_ready = (available > 0);

	if(m_ready)
	{
		gl::GetQueryObjectuiv(m_query, gl::QUERY_RESULT, &m_pixelsVisible);
	}

	return m_ready;
}

// after End() and IsReady() == true it does matter
uint32 CGLOcclusionQuery::GetVisiblePixels()
{
	return m_pixelsVisible;
}