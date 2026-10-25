//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2026
//////////////////////////////////////////////////////////////////////////////////
// Description: Converts decoded YUV 4:2:0 movie frames into RGBA texture on GPU
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "materialsystem1/IMaterialSystem.h"
#include "materialsystem1/IMatSysShader.h"
#include "MovieVideoDecoder.h"
#include "MovieYUVConverter.h"

DEFINE_SHADER_NOFACTORY(MovieYUV)

// layout must match MovieYUV shader
struct MovieYUVParams
{
	float	lumaOffset;
	float	lumaScale;
	float	chromaScale;
	float	_padding;

	float	crToR;
	float	cbToG;
	float	crToG;
	float	cbToB;
};

static void MovGetMatrixWeights(int colorMatrix, float& kr, float& kb)
{
	switch (colorMatrix)
	{
	case MOVIE_COLOR_MATRIX_BT470BG:
	case MOVIE_COLOR_MATRIX_BT601:
		kr = 0.299f;
		kb = 0.114f;
		break;
	case MOVIE_COLOR_MATRIX_BT2020_NCL:
		kr = 0.2627f;
		kb = 0.0593f;
		break;
	default:
		// unspecified streams are treated as BT.709
		kr = 0.2126f;
		kb = 0.0722f;
		break;
	}
}

static void MovUploadPlane(ITexture* texture, const ubyte* data, int stride, int width, int height)
{
	ITexture::LockInOutData writeTo(TEXLOCK_DISCARD, IAARectangle(0, 0, width, height));
	if (!texture->Lock(writeTo))
		return;

	ubyte* dest = reinterpret_cast<ubyte*>(writeTo.lockData);
	const int destPitch = writeTo.lockPitch > 0 ? writeTo.lockPitch : width;

	for (int y = 0; y < height; ++y)
		memcpy(dest + y * destPitch, data + y * stride, width);

	texture->Unlock();
}

//-------------------------------------------------------------------------------------------------

bool CMovieYUVConverter::Init(const char* name, int width, int height)
{
	Shutdown();

	const int chromaWidth = (width + 1) / 2;
	const int chromaHeight = (height + 1) / 2;

	const EqString planeNames[3] = {
		EqString::Format("%s_y", name),
		EqString::Format("%s_u", name),
		EqString::Format("%s_v", name),
	};

	m_planes[0] = g_renderAPI->CreateProceduralTexture(TextureDesc(planeNames[0], 0, FORMAT_R8, width, height));
	m_planes[1] = g_renderAPI->CreateProceduralTexture(TextureDesc(planeNames[1], 0, FORMAT_R8, chromaWidth, chromaHeight));
	m_planes[2] = g_renderAPI->CreateProceduralTexture(TextureDesc(planeNames[2], 0, FORMAT_R8, chromaWidth, chromaHeight));

	m_target = g_renderAPI->CreateRenderTarget(
		Builder<TextureDesc>()
		.Name(name)
		.Format(FORMAT_RGBA8)
		.Size(width, height)
		.End()
	);

	if (!m_planes[0] || !m_planes[1] || !m_planes[2] || !m_target)
	{
		MsgError("CMovieYUVConverter - failed to create textures for %s\n", name);
		Shutdown();
		return false;
	}

	m_pipeline = g_renderAPI->CreateRenderPipeline(
		Builder<RenderPipelineDesc>()
		.ShaderName("MovieYUV")
		.FragmentState(
			Builder<FragmentPipelineDesc>()
			.ColorTarget("Target", FORMAT_RGBA8)
			.End()
		)
		.PrimitiveState(Builder<PrimitiveDesc>()
			.Topology(PRIM_TRIANGLE_STRIP)
			.Cull(CULL_NONE)
			.End()
		)
		.End()
	);

	if (!m_pipeline)
	{
		MsgError("CMovieYUVConverter - failed to create MovieYUV pipeline\n");
		Shutdown();
		return false;
	}

	m_paramsBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(MovieYUVParams), 1), BUFFERUSAGE_UNIFORM | BUFFERUSAGE_COPY_DST, "MovieYUVParams");

	m_bindGroup = g_renderAPI->CreateBindGroup(m_pipeline, Builder<BindGroupDesc>()
		.GroupIndex(0)
		.Name("MovieYUV")
		.Sampler(StringIdConst24("Sampler"), SamplerStateParams(TEXFILTER_LINEAR, TEXADDRESS_CLAMP))
		.Texture(StringIdConst24("TextureY"), m_planes[0])
		.Texture(StringIdConst24("TextureU"), m_planes[1])
		.Texture(StringIdConst24("TextureV"), m_planes[2])
		.Buffer(StringIdConst24("params"), m_paramsBuffer)
		.End());

	m_paramsMatrix = -1;
	return true;
}

void CMovieYUVConverter::Shutdown()
{
	m_bindGroup = nullptr;
	m_paramsBuffer = nullptr;
	m_pipeline = nullptr;
	m_target = nullptr;

	for (ITexturePtr& plane : m_planes)
		plane = nullptr;
}

void CMovieYUVConverter::UpdateParams(IGPUCommandRecorder* cmdRecorder, const MovieVideoFrame& frame)
{
	if (m_paramsMatrix == frame.colorMatrix && m_paramsFullRange == frame.fullRange)
		return;

	m_paramsMatrix = frame.colorMatrix;
	m_paramsFullRange = frame.fullRange;

	float kr, kb;
	MovGetMatrixWeights(frame.colorMatrix, kr, kb);
	const float kg = 1.0f - kr - kb;

	MovieYUVParams params{};
	if (frame.fullRange)
	{
		params.lumaOffset = 0.0f;
		params.lumaScale = 1.0f;
		params.chromaScale = 1.0f;
	}
	else
	{
		params.lumaOffset = 16.0f / 255.0f;
		params.lumaScale = 255.0f / 219.0f;
		params.chromaScale = 255.0f / 224.0f;
	}

	params.crToR = 2.0f * (1.0f - kr);
	params.cbToG = 2.0f * kb * (1.0f - kb) / kg;
	params.crToG = 2.0f * kr * (1.0f - kr) / kg;
	params.cbToB = 2.0f * (1.0f - kb);

	cmdRecorder->WriteBuffer(m_paramsBuffer, &params, sizeof(params), 0);
}

void CMovieYUVConverter::Convert(const MovieVideoFrame& frame)
{
	if (!m_bindGroup)
		return;

	const int width = min(frame.width, m_planes[0]->GetWidth());
	const int height = min(frame.height, m_planes[0]->GetHeight());
	const int chromaWidth = min((width + 1) / 2, m_planes[1]->GetWidth());
	const int chromaHeight = min((height + 1) / 2, m_planes[1]->GetHeight());

	MovUploadPlane(m_planes[0], frame.planes[0], frame.strides[0], width, height);
	MovUploadPlane(m_planes[1], frame.planes[1], frame.strides[1], chromaWidth, chromaHeight);
	MovUploadPlane(m_planes[2], frame.planes[2], frame.strides[2], chromaWidth, chromaHeight);

	IGPUCommandRecorderPtr cmdRecorder = g_renderAPI->CreateCommandRecorder("MovieYUV");
	cmdRecorder->DbgPushGroup("MovieYUV");

	UpdateParams(cmdRecorder, frame);

	IGPURenderPassRecorderPtr rendPassRecorder = cmdRecorder->BeginRenderPass(
		Builder<RenderPassDesc>()
		.ColorTarget(m_target)
		.End()
	);
	rendPassRecorder->SetPipeline(m_pipeline);
	rendPassRecorder->SetBindGroup(0, m_bindGroup);
	rendPassRecorder->Draw(4, 0, 1);
	rendPassRecorder->Complete();

	cmdRecorder->DbgPopGroup();

	// queued before command buffers of passes which sample the target in this frame
	g_matSystem->QueueCommandBuffer(cmdRecorder->End());
}
