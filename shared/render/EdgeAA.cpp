//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2021
//////////////////////////////////////////////////////////////////////////////////
// Description: DrvSyn post-processing effects
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/ConVar.h"
#include "utils/KeyValues.h"
#include "EdgeAA.h"

#include "materialsystem1/IMaterialSystem.h"
#include "materialsystem1/MeshBuilder.h"


DECLARE_CVAR(r_fxaa, "1", "Fast Approximate Anti-aliasing", CV_ARCHIVE);
DECLARE_CVAR_CLAMP(r_fxaaQualitySubpix, "0.25", 0.25f, 1.0f, nullptr, CV_ARCHIVE);
DECLARE_CVAR_CLAMP(r_fxaaQualityEdgeThreshold, "0.11", 0.063f, 0.5f, nullptr, CV_ARCHIVE);
DECLARE_CVAR_CLAMP(r_fxaaQualityEdgeThresholdMin, "0.063", 0.0f, 1.0f, nullptr, CV_ARCHIVE);

static ETextureFormat fsaaGetTextureFormat()
{
	return FORMAT_RGBA8;
}

bool CRenderFullScreenEdgeAA::IsEnabled() const
{
	if (!r_fxaa.GetBool())
		return false;

	if (!m_edgeAALumaPipeline)
		return false;

	return true;
}

bool CRenderFullScreenEdgeAA::Init()
{
	ITexture* backBufferTex = g_matSystem->GetDefaultDepthBuffer();
	const IVector2D screenSize = backBufferTex->GetSize().xy();

	m_lumaFramebuffer = g_renderAPI->CreateRenderTarget(
		Builder<TextureDesc>()
		.Name("_rt_AAFramebuffer")
		.Format(fsaaGetTextureFormat())
		.Size(screenSize.x, screenSize.y)
		.Flags(TEXFLAG_STORAGE)
		.End()
	);
	
	m_edgeAALumaPipeline = g_renderAPI->CreateComputePipeline(Builder<ComputePipelineDesc>()
		.ShaderName("EdgeAA")
		.End()
	);

	if (!m_edgeAAMat)
	{
		KVSection lumaMat;
		lumaMat.SetName("EdgeAA");
		lumaMat.SetKey("BaseTexture", "_rt_AAFramebuffer");
		m_edgeAAMat = g_matSystem->CreateMaterial("_edgeAA", &lumaMat);
		m_edgeAAMat->LoadShaderAndTextures();

		m_edgeAASettings = m_edgeAAMat->GetMaterialVar("Settings");
	}
	return true;
}

void CRenderFullScreenEdgeAA::Shutdown()
{
	m_lumaFramebuffer = nullptr;
	m_edgeAALumaPipeline = nullptr;
	m_edgeAAMat = nullptr;
}

void CRenderFullScreenEdgeAA::PreRender(IGPUCommandRecorder* cmdRecorder)
{
	if (!m_edgeAALumaPipeline)
		return;

	// NOTE: see shader source for workgroups size
	constexpr const int workgroupSize = 32;
	constexpr const int blocksPerWorkgroup = workgroupSize * 4;
	constexpr const float oneByBlocksPerWorkgroup = 1.0f / static_cast<float>(blocksPerWorkgroup);

	ITexturePtr framebufferSrcTex = g_matSystem->GetCurrentBackbuffer();

	const IVector2D framebufferSize = framebufferSrcTex->GetSize().xy();
	g_renderAPI->ResizeRenderTarget(m_lumaFramebuffer, { framebufferSize.x, framebufferSize.y, 1 });

	IVector2D workgroupCount;
	workgroupCount.x = ceil(framebufferSize.x * oneByBlocksPerWorkgroup);
	workgroupCount.y = ceil(framebufferSize.y * oneByBlocksPerWorkgroup);

	cmdRecorder->DbgPushGroup("FXAALuma");

	IGPUComputePassRecorderPtr computePassRecorder = cmdRecorder->BeginComputePass("FXAALuma");
	computePassRecorder->SetPipeline(m_edgeAALumaPipeline);
	computePassRecorder->SetBindGroup(0, g_renderAPI->CreateBindGroup(m_edgeAALumaPipeline,
		Builder<BindGroupDesc>().GroupIndex(0)
		.StorageTexture(StringIdConst24("FBSrcTexture"), framebufferSrcTex)
		.StorageTexture(StringIdConst24("FBDstTexture"), m_lumaFramebuffer)
		.End())
	);
	computePassRecorder->DispatchWorkgroups(workgroupCount.x, workgroupCount.y);
	computePassRecorder->Complete();

	cmdRecorder->DbgPopGroup();
}

void CRenderFullScreenEdgeAA::Render(IGPURenderPassRecorder* rendPassRecorder)
{
	Vector4D settings;
	settings.x = r_fxaaQualitySubpix.GetFloat();
	settings.y = r_fxaaQualityEdgeThreshold.GetFloat();
	settings.z = r_fxaaQualityEdgeThresholdMin.GetFloat();
	settings.w = 0;
	m_edgeAASettings.Set(settings);

	RenderDrawCmd drawCmd;
	drawCmd.SetMaterial(m_edgeAAMat);
	drawCmd.SetDrawNonIndexed(PRIM_TRIANGLES, 3, 0);
	g_matSystem->SetupDrawCommand(drawCmd, RenderPassContext(rendPassRecorder, nullptr));
}
