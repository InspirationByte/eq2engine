//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2021
//////////////////////////////////////////////////////////////////////////////////
// Description: DrvSyn post-processing effects
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "materialsystem1/IMaterialVar.h"

class ConVar;
class IGPURenderPassRecorder;
class IGPUComputePipeline;
using IGPUComputePipelinePtr = CRefPtr<IGPUComputePipeline>;

class CRenderFullScreenEdgeAA
{
public:
	bool			Init();
	void			Shutdown();

	bool			IsEnabled() const;

	void			PreRender(IGPUCommandRecorder* cmdRecorder);
	void			Render(IGPURenderPassRecorder* rendPassRecorder);
protected:
	IMaterialPtr			m_edgeAAMat;
	MatVec4Proxy			m_edgeAASettings;
	ITexturePtr				m_lumaFramebuffer;
	IGPUComputePipelinePtr	m_edgeAALumaPipeline;
};
