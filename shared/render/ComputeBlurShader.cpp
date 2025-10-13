#include "core/core_common.h"
#include "ComputeBlurShader.h"
#include "materialsystem1/IMatSysShader.h"

DEFINE_SHADER_NOFACTORY(ComputeBlur)

ComputeBlurShader::ComputeBlurShader(int iterations, int filterSize, int blurFlags)
	: m_iterations(iterations)
	, m_filterSize(filterSize)
	, m_blurFlags(blurFlags)
{
	m_pipeline = g_renderAPI->CreateComputePipeline(
		Builder<ComputePipelineDesc>()
		.ShaderName("ComputeBlur")
		.End());

	// blur shader constants
	const int tileDim = 128;
	const int batchY = 4;

	struct
	{
		int filterDim;
		uint blockDim;

		int _padding[2];
	} blurParams;

	blurParams.blockDim = tileDim - (m_filterSize - 1);
	blurParams.filterDim = m_filterSize;

	struct
	{
		uint flipValue;
		int _padding[3];
	} flipData0, flipData1;

	if (m_blurFlags == BLUR_BOTH)
	{
		flipData0.flipValue = 0;	// X
		flipData1.flipValue = 1;	// Y
		m_oneByBlockDim = 1.0f / blurParams.blockDim;
		m_oneByBatchSizeY = 1.0f / batchY;
	}
	else if (m_blurFlags == BLUR_VERTICAL)
	{
		flipData0.flipValue = 1;	// Y
		flipData1.flipValue = 1;	// Y

		m_oneByBlockDim = 1.0f / blurParams.blockDim;
		m_oneByBatchSizeY = 1.0f / batchY;			// TODO: ples fix, it's a bugged shit that wastes Compute performance
	}
	else if (m_blurFlags == BLUR_HORIZONTAL)
	{
		flipData0.flipValue = 0;	// X
		flipData1.flipValue = 0;	// X

		m_oneByBlockDim = 1.0f / blurParams.blockDim;
		m_oneByBatchSizeY = 1.0f / batchY;
	}

	m_switchBuffer0 = g_renderAPI->CreateBuffer(BufferInfo(&flipData0, 1), BUFFERUSAGE_UNIFORM, "SwitchBuffer0");
	m_switchBuffer1 = g_renderAPI->CreateBuffer(BufferInfo(&flipData1, 1), BUFFERUSAGE_UNIFORM, "SwitchBuffer1");

	IGPUBufferPtr paramsBuffer = g_renderAPI->CreateBuffer(BufferInfo(&blurParams, 1), BUFFERUSAGE_UNIFORM, "ParamsBuffer");

	m_bindGroupConst = g_renderAPI->CreateBindGroup(m_pipeline, Builder<BindGroupDesc>()
		.GroupIndex(0)
		.Name("BlurConst")
		.Sampler(StringIdConst24("Sampler"), SamplerStateParams(TEXFILTER_LINEAR, TEXADDRESS_CLAMP))
		.Buffer(StringIdConst24("params"), paramsBuffer)
		.End());
}

void ComputeBlurShader::SetDestinationTexture(ITexture* dest)
{
	m_dstTexture = dest;

	const IVector2D dstSize = m_dstTexture->GetSize();

	m_blurTemp1 = g_renderAPI->CreateRenderTarget(
		Builder<TextureDesc>()
		.Name("_blurTemp1")
		.Format(FORMAT_RGBA8)
		.Size(dstSize)
		.Flags(TEXFLAG_STORAGE | TEXFLAG_COPY_SRC | TEXFLAG_TRANSIENT)
		.End()
	);

	m_blurTemp2 = g_renderAPI->CreateRenderTarget(
		Builder<TextureDesc>()
		.Name("_blurTemp2")
		.Format(FORMAT_RGBA8)
		.Size(dstSize)
		.Flags(TEXFLAG_STORAGE | TEXFLAG_COPY_SRC | TEXFLAG_TRANSIENT)
		.End()
	);

	m_bindGroupStg1 = g_renderAPI->CreateBindGroup(m_pipeline, Builder<BindGroupDesc>()
		.GroupIndex(1)
		.Name("BlurParams1")
		.Texture(StringIdConst24("BaseTexture"), m_blurTemp1)
		.StorageTexture(StringIdConst24("OutputTexture"), m_blurTemp2)
		.Buffer(StringIdConst24("flip"), m_switchBuffer1)
		.End());

	m_bindGroupStg2 = g_renderAPI->CreateBindGroup(m_pipeline, Builder<BindGroupDesc>()
		.GroupIndex(1)
		.Name("BlurParams2")
		.Texture(StringIdConst24("BaseTexture"), m_blurTemp2)
		.StorageTexture(StringIdConst24("OutputTexture"), m_blurTemp1)
		.Buffer(StringIdConst24("flip"), m_switchBuffer0)
		.End());
}

void ComputeBlurShader::SetupExecute(IGPUCommandRecorder* commandRecorder, int arraySlice)
{
	commandRecorder->DbgPushGroup("ComputeBlurShader");

	const IVector2D dstSize = m_dstTexture->GetSize();

	IGPUBindGroupPtr bindGroupStg0 = g_renderAPI->CreateBindGroup(m_pipeline, Builder<BindGroupDesc>()
		.GroupIndex(1)
		.Name("BlurParams")
		.Texture(StringIdConst24("BaseTexture"), m_srcTexture, arraySlice == -1 ? ITexture::DEFAULT_VIEW : ITexture::ViewArraySlice(arraySlice))
		.StorageTexture(StringIdConst24("OutputTexture"), m_blurTemp1)
		.Buffer(StringIdConst24("flip"), m_switchBuffer0)
		.End());

	IGPUComputePassRecorderPtr blurPassRecorder = commandRecorder->BeginComputePass("ComputeBlur");
	blurPassRecorder->SetPipeline(m_pipeline);
	blurPassRecorder->SetBindGroup(0, m_bindGroupConst);

	IVector2D invocations1(ceil(dstSize.x * m_oneByBlockDim), ceil(dstSize.y * m_oneByBatchSizeY));
	IVector2D invocations2(ceil(dstSize.y * m_oneByBlockDim), ceil(dstSize.x * m_oneByBatchSizeY));

	if (m_blurFlags == BLUR_VERTICAL)
		invocations1 = invocations2;
	else if (m_blurFlags == BLUR_HORIZONTAL)
		invocations2 = invocations1;		// FIXME: is that correct?

	blurPassRecorder->SetBindGroup(1, bindGroupStg0);
	blurPassRecorder->DispatchWorkgroups(invocations1.x, invocations1.y, 1);

	blurPassRecorder->SetBindGroup(1, m_bindGroupStg1);
	blurPassRecorder->DispatchWorkgroups(invocations2.x, invocations2.y, 1);

	for (int i = 0; i < m_iterations - 1; ++i)
	{
		blurPassRecorder->SetBindGroup(1, m_bindGroupStg2);
		blurPassRecorder->DispatchWorkgroups(invocations1.x, invocations1.y, 1);

		blurPassRecorder->SetBindGroup(1, m_bindGroupStg1);
		blurPassRecorder->DispatchWorkgroups(invocations2.x, invocations2.y, 1);
	}

	blurPassRecorder->Complete();

	TextureCopyInfo srcTex{ m_blurTemp2 };
	TextureCopyInfo dstTex{ m_dstTexture };
	dstTex.origin.arraySlice = arraySlice < 0 ? 0 : arraySlice;

	TextureExtent texExtents{ dstSize };
	commandRecorder->CopyTextureToTexture(srcTex, dstTex, texExtents);

	commandRecorder->DbgPopGroup();
}

