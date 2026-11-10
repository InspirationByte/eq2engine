//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2021
//////////////////////////////////////////////////////////////////////////////////
// Description: Full screen anti-aliasing (FXAA, SMAA 1x)
//////////////////////////////////////////////////////////////////////////////////

#include <AreaTex.h>
#include <SearchTex.h>

#include "core/core_common.h"
#include "core/ConVar.h"
#include "utils/KeyValues.h"
#include "EdgeAA.h"

#include "materialsystem1/IMaterialSystem.h"
#include "materialsystem1/IMatSysShader.h"
#include "materialsystem1/MeshBuilder.h"

DEFINE_SHADER_NOFACTORY(SMAA)

DECLARE_CVAR_CLAMP(r_fxaaQualitySubpix, "0.25", 0.25f, 1.0f, nullptr, CV_ARCHIVE);
DECLARE_CVAR_CLAMP(r_fxaaQualityEdgeThreshold, "0.11", 0.063f, 0.5f, nullptr, CV_ARCHIVE);
DECLARE_CVAR_CLAMP(r_fxaaQualityEdgeThresholdMin, "0.063", 0.0f, 1.0f, nullptr, CV_ARCHIVE);

DECLARE_CVAR_CLAMP(r_smaaThreshold, "0.1", 0.01f, 0.5f, "SMAA edge detection luma threshold, lower is more edges", CV_ARCHIVE);
DECLARE_CVAR_CLAMP(r_smaaMaxSearchSteps, "16", 1, 112, "SMAA horizontal / vertical edge search distance (pixels / 2)", CV_ARCHIVE);
DECLARE_CVAR_CLAMP(r_smaaMaxSearchStepsDiag, "8", 0, 20, "SMAA diagonal edge search distance", CV_ARCHIVE);
DECLARE_CVAR_CLAMP(r_smaaCornerRounding, "25", 0, 100, "SMAA corner rounding percent", CV_ARCHIVE);

// must match SMAAParams in shadersSRC/source_SMAA.fx
struct SMAAParams
{
	Vector4D	rtMetrics;
	float		threshold;
	float		maxSearchSteps;
	float		maxSearchStepsDiag;
	float		cornerRounding;
};

static ETextureFormat fsaaGetTextureFormat()
{
	return FORMAT_RGBA8;
}

// passLayoutId is the SMAA shader vertex layout that selects the pass, see shadersSRC/SMAA.def
static IGPURenderPipelinePtr smaaCreatePipeline(int passLayoutId, ETextureFormat targetFormat)
{
	return g_renderAPI->CreateRenderPipeline(
		Builder<RenderPipelineDesc>()
		.ShaderName("SMAA")
		.ShaderVertexLayoutId(passLayoutId)
		.FragmentState(
			Builder<FragmentPipelineDesc>()
			.ColorTarget("Target", targetFormat)
			.End()
		)
		.PrimitiveState(Builder<PrimitiveDesc>()
			.Topology(PRIM_TRIANGLES)
			.Cull(CULL_NONE)
			.End()
		)
		.End()
	);
}

//-------------------------------------------------------------------------------------------------

EEdgeAAMode CRenderFullScreenEdgeAA::GetMode() const
{
	if (!m_edgeAALumaPipeline)
		return EDGEAA_NONE;

	if (m_mode == EDGEAA_SMAA)
	{
		if (m_smaaEdgesPipeline && m_smaaWeightsPipeline)
			return EDGEAA_SMAA;

		// SMAA shaders are not available
		return EDGEAA_FXAA;
	}

	return m_mode;
}

bool CRenderFullScreenEdgeAA::IsEnabled() const
{
	return GetMode() != EDGEAA_NONE;
}

void CRenderFullScreenEdgeAA::SetMode(EEdgeAAMode mode)
{
	if (!m_initialized || mode == m_mode)
		return;

	Shutdown();
	Init(mode);
}

// creates only the resources of the mode, SetMode re-initializes
bool CRenderFullScreenEdgeAA::Init(EEdgeAAMode mode)
{
	m_initialized = true;
	m_mode = mode;

	if (mode == EDGEAA_NONE)
		return true;

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

	if (mode == EDGEAA_SMAA && !InitSMAA(screenSize))
		MsgWarning("SMAA is not available, FXAA will be used instead\n");

	return true;
}

bool CRenderFullScreenEdgeAA::InitSMAA(const IVector2D& screenSize)
{
	m_smaaEdgesTarget = g_renderAPI->CreateRenderTarget(
		Builder<TextureDesc>()
		.Name("_rt_SMAAEdges")
		.Format(FORMAT_RG8)
		.Size(screenSize.x, screenSize.y)
		.End()
	);

	m_smaaBlendTarget = g_renderAPI->CreateRenderTarget(
		Builder<TextureDesc>()
		.Name("_rt_SMAABlend")
		.Format(FORMAT_RGBA8)
		.Size(screenSize.x, screenSize.y)
		.End()
	);

	m_smaaAreaTex = g_renderAPI->CreateProceduralTexture(
		Builder<TextureDesc>()
		.Name("_smaaAreaTex")
		.Format(FORMAT_RG8)
		.Size(AREATEX_WIDTH, AREATEX_HEIGHT)
		.Sampler(SamplerStateParams(TEXFILTER_LINEAR, TEXADDRESS_CLAMP))
		.End(),
		areaTexBytes, sizeof(areaTexBytes)
	);

	m_smaaSearchTex = g_renderAPI->CreateProceduralTexture(
		Builder<TextureDesc>()
		.Name("_smaaSearchTex")
		.Format(FORMAT_R8)
		.Size(SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT)
		.Sampler(SamplerStateParams(TEXFILTER_LINEAR, TEXADDRESS_CLAMP))
		.End(),
		searchTexBytes, sizeof(searchTexBytes)
	);

	m_smaaParamsBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(SMAAParams), 1), BUFFERUSAGE_UNIFORM | BUFFERUSAGE_COPY_DST, "SMAAParams");

	m_smaaEdgesPipeline = smaaCreatePipeline(StringIdConst24("EdgeDetection"), FORMAT_RG8);
	m_smaaWeightsPipeline = smaaCreatePipeline(StringIdConst24("BlendingWeight"), FORMAT_RGBA8);

	// neighborhood blending pipeline is created by the back buffer format in RenderSMAA
	m_smaaBlendPipeline = nullptr;
	m_smaaBlendTargetFormat = FORMAT_NONE;

	if (!m_smaaEdgesTarget || !m_smaaBlendTarget || !m_smaaAreaTex || !m_smaaSearchTex || !m_smaaParamsBuffer
		|| !m_smaaEdgesPipeline || !m_smaaWeightsPipeline)
	{
		m_smaaEdgesPipeline = nullptr;
		m_smaaWeightsPipeline = nullptr;
		return false;
	}

	return true;
}

void CRenderFullScreenEdgeAA::Shutdown()
{
	m_initialized = false;
	m_mode = EDGEAA_NONE;

	m_lumaFramebuffer = nullptr;
	m_edgeAALumaPipeline = nullptr;
	m_edgeAAMat = nullptr;

	m_smaaEdgesTarget = nullptr;
	m_smaaBlendTarget = nullptr;
	m_smaaAreaTex = nullptr;
	m_smaaSearchTex = nullptr;
	m_smaaParamsBuffer = nullptr;
	m_smaaEdgesPipeline = nullptr;
	m_smaaWeightsPipeline = nullptr;
	m_smaaBlendPipeline = nullptr;
	m_smaaBlendTargetFormat = FORMAT_NONE;
}

void CRenderFullScreenEdgeAA::PreRender(IGPUCommandRecorder* cmdRecorder)
{
	switch (GetMode())
	{
		case EDGEAA_FXAA:
			PreRenderFXAA(cmdRecorder);
			break;
		case EDGEAA_SMAA:
			PreRenderSMAA(cmdRecorder);
			break;
		default:
			break;
	}
}

// copies back buffer into _rt_AAFramebuffer (with luma in alpha for FXAA), returns its size
IVector2D CRenderFullScreenEdgeAA::CopyBackbuffer(IGPUCommandRecorder* cmdRecorder)
{
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

	return framebufferSize;
}

void CRenderFullScreenEdgeAA::PreRenderFXAA(IGPUCommandRecorder* cmdRecorder)
{
	CopyBackbuffer(cmdRecorder);
}

void CRenderFullScreenEdgeAA::PreRenderSMAA(IGPUCommandRecorder* cmdRecorder)
{
	const IVector2D framebufferSize = CopyBackbuffer(cmdRecorder);

	g_renderAPI->ResizeRenderTarget(m_smaaEdgesTarget, { framebufferSize.x, framebufferSize.y, 1 });
	g_renderAPI->ResizeRenderTarget(m_smaaBlendTarget, { framebufferSize.x, framebufferSize.y, 1 });

	SMAAParams params;
	params.rtMetrics = Vector4D(1.0f / framebufferSize.x, 1.0f / framebufferSize.y, framebufferSize.x, framebufferSize.y);
	params.threshold = r_smaaThreshold.GetFloat();
	params.maxSearchSteps = r_smaaMaxSearchSteps.GetInt();
	params.maxSearchStepsDiag = r_smaaMaxSearchStepsDiag.GetInt();
	params.cornerRounding = r_smaaCornerRounding.GetInt();
	cmdRecorder->WriteBuffer(m_smaaParamsBuffer, &params, sizeof(params), 0);

	const SamplerStateParams linearClampSampler(TEXFILTER_LINEAR, TEXADDRESS_CLAMP);

	// edges and weights passes discard or skip pixels, so the targets are cleared to zero
	const MColor clearColor(0.0f, 0.0f, 0.0f, 0.0f);

	cmdRecorder->DbgPushGroup("SMAAEdgeDetection");
	{
		IGPURenderPassRecorderPtr rendPassRecorder = cmdRecorder->BeginRenderPass(
			Builder<RenderPassDesc>()
			.Name("SMAAEdgeDetection")
			.ColorTarget(m_smaaEdgesTarget, true, clearColor)
			.End()
		);
		rendPassRecorder->SetPipeline(m_smaaEdgesPipeline);
		rendPassRecorder->SetBindGroup(0, g_renderAPI->CreateBindGroup(m_smaaEdgesPipeline,
			Builder<BindGroupDesc>().GroupIndex(0)
			.Sampler(StringIdConst24("LinearSampler"), linearClampSampler)
			.Buffer(StringIdConst24("smaaParams"), m_smaaParamsBuffer)
			.Texture(StringIdConst24("ColorTexture"), m_lumaFramebuffer)
			.End())
		);
		rendPassRecorder->Draw(3, 0, 1);
		rendPassRecorder->Complete();
	}
	cmdRecorder->DbgPopGroup();

	cmdRecorder->DbgPushGroup("SMAABlendingWeight");
	{
		IGPURenderPassRecorderPtr rendPassRecorder = cmdRecorder->BeginRenderPass(
			Builder<RenderPassDesc>()
			.Name("SMAABlendingWeight")
			.ColorTarget(m_smaaBlendTarget, true, clearColor)
			.End()
		);
		rendPassRecorder->SetPipeline(m_smaaWeightsPipeline);
		rendPassRecorder->SetBindGroup(0, g_renderAPI->CreateBindGroup(m_smaaWeightsPipeline,
			Builder<BindGroupDesc>().GroupIndex(0)
			.Sampler(StringIdConst24("LinearSampler"), linearClampSampler)
			.Buffer(StringIdConst24("smaaParams"), m_smaaParamsBuffer)
			.Texture(StringIdConst24("EdgesTexture"), m_smaaEdgesTarget)
			.Texture(StringIdConst24("AreaTexture"), m_smaaAreaTex)
			.Texture(StringIdConst24("SearchTexture"), m_smaaSearchTex)
			.End())
		);
		rendPassRecorder->Draw(3, 0, 1);
		rendPassRecorder->Complete();
	}
	cmdRecorder->DbgPopGroup();
}

void CRenderFullScreenEdgeAA::Render(IGPURenderPassRecorder* rendPassRecorder)
{
	switch (GetMode())
	{
		case EDGEAA_FXAA:
			RenderFXAA(rendPassRecorder);
			break;
		case EDGEAA_SMAA:
			RenderSMAA(rendPassRecorder);
			break;
		default:
			break;
	}
}

void CRenderFullScreenEdgeAA::RenderFXAA(IGPURenderPassRecorder* rendPassRecorder)
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

void CRenderFullScreenEdgeAA::RenderSMAA(IGPURenderPassRecorder* rendPassRecorder)
{
	const ETextureFormat targetFormat = rendPassRecorder->GetRenderTargetFormats()[0];
	if (!m_smaaBlendPipeline || m_smaaBlendTargetFormat != targetFormat)
	{
		m_smaaBlendTargetFormat = targetFormat;
		m_smaaBlendPipeline = smaaCreatePipeline(StringIdConst24("NeighborhoodBlending"), targetFormat);
	}

	if (!m_smaaBlendPipeline)
		return;

	rendPassRecorder->SetPipeline(m_smaaBlendPipeline);
	rendPassRecorder->SetBindGroup(0, g_renderAPI->CreateBindGroup(m_smaaBlendPipeline,
		Builder<BindGroupDesc>().GroupIndex(0)
		.Sampler(StringIdConst24("LinearSampler"), SamplerStateParams(TEXFILTER_LINEAR, TEXADDRESS_CLAMP))
		.Buffer(StringIdConst24("smaaParams"), m_smaaParamsBuffer)
		.Texture(StringIdConst24("ColorTexture"), m_lumaFramebuffer)
		.Texture(StringIdConst24("BlendTexture"), m_smaaBlendTarget)
		.End())
	);
	rendPassRecorder->Draw(3, 0, 1);
}
