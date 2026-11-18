//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2021
//////////////////////////////////////////////////////////////////////////////////
// Description: Full screen anti-aliasing (FXAA, SMAA 1x)
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "materialsystem1/IMaterialVar.h"

class ConVar;
class IGPURenderPassRecorder;
class IGPUComputePipeline;
class IGPURenderPipeline;
class IGPUBuffer;
using IGPUComputePipelinePtr = CRefPtr<IGPUComputePipeline>;
using IGPURenderPipelinePtr = CRefPtr<IGPURenderPipeline>;
using IGPUBufferPtr = CRefPtr<IGPUBuffer>;

enum EEdgeAAMode : int
{
	EDGEAA_NONE = 0,
	EDGEAA_FXAA,
	EDGEAA_SMAA,
};

class CRenderFullScreenEdgeAA
{
public:
	bool			Init(EEdgeAAMode mode);
	void			Shutdown();

	// re-creates resources for the new mode if initialized
	void			SetMode(EEdgeAAMode mode);
	bool			IsEnabled() const;

	void			PreRender(IGPUCommandRecorder* cmdRecorder);
	void			Render(IGPURenderPassRecorder* rendPassRecorder);

protected:
	EEdgeAAMode		GetMode() const;

	IVector2D		CopyBackbuffer(IGPUCommandRecorder* cmdRecorder);

	void			PreRenderFXAA(IGPUCommandRecorder* cmdRecorder);
	void			RenderFXAA(IGPURenderPassRecorder* rendPassRecorder);

	bool			InitSMAA(const IVector2D& screenSize);
	void			PreRenderSMAA(IGPUCommandRecorder* cmdRecorder);
	void			RenderSMAA(IGPURenderPassRecorder* rendPassRecorder);

	EEdgeAAMode				m_mode{ EDGEAA_NONE };
	bool					m_initialized{ false };

	IMaterialPtr			m_edgeAAMat;
	MatVec4Proxy			m_edgeAASettings;
	ITexturePtr				m_lumaFramebuffer;
	IGPUComputePipelinePtr	m_edgeAALumaPipeline;

	// SMAA 1x
	ITexturePtr				m_smaaEdgesTarget;
	ITexturePtr				m_smaaBlendTarget;
	ITexturePtr				m_smaaAreaTex;
	ITexturePtr				m_smaaSearchTex;
	IGPUBufferPtr			m_smaaParamsBuffer;
	IGPURenderPipelinePtr	m_smaaEdgesPipeline;
	IGPURenderPipelinePtr	m_smaaWeightsPipeline;
	IGPURenderPipelinePtr	m_smaaBlendPipeline;
	ETextureFormat			m_smaaBlendTargetFormat{ FORMAT_NONE };
};
