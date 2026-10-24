//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2026
//////////////////////////////////////////////////////////////////////////////////
// Description: Converts decoded YUV 4:2:0 movie frames into RGBA texture on GPU
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "materialsystem1/renderers/IShaderAPI.h"

class IGPURenderPipeline;
using IGPURenderPipelinePtr = CRefPtr<IGPURenderPipeline>;

struct MovieVideoFrame;

class CMovieYUVConverter
{
public:
	bool			Init(const char* name, int width, int height);
	void			Shutdown();

	// uploads frame planes and queues conversion pass into target texture
	void			Convert(const MovieVideoFrame& frame);

	ITexturePtr		GetTarget() const { return m_target; }

private:
	void			UpdateParams(IGPUCommandRecorder* cmdRecorder, const MovieVideoFrame& frame);

	ITexturePtr				m_planes[3];
	ITexturePtr				m_target;
	IGPURenderPipelinePtr	m_pipeline;
	IGPUBufferPtr			m_paramsBuffer;
	IGPUBindGroupPtr		m_bindGroup;

	int						m_paramsMatrix{ -1 };
	bool					m_paramsFullRange{ false };
};
