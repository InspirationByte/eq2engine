#include "core/core_common.h"
#include "ComputeBlurShader.h"

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

	m_oneByBlockDim = 1.0f / blurParams.blockDim;
	m_oneByBatchSizeY = 1.0f / batchY;

	if (m_blurFlags == BLUR_BOTH)
	{
		flipData0.flipValue = 0;	// X
		flipData1.flipValue = 1;	// Y
	}
	else if (m_blurFlags == BLUR_VERTICAL)
	{
		flipData0.flipValue = 1;	// Y
		flipData1.flipValue = 1;	// Y
		m_oneByBatchSizeY *= 2;
	}
	else if (m_blurFlags == BLUR_HORIZONTAL)
	{
		flipData0.flipValue = 0;	// X
		flipData1.flipValue = 0;	// X
		m_oneByBatchSizeY *= 2;
	}

	m_switchBuffer0 = g_renderAPI->CreateBuffer(BufferInfo(&flipData0, 1), BUFFERUSAGE_UNIFORM, "SwitchBuffer0");
	m_switchBuffer1 = g_renderAPI->CreateBuffer(BufferInfo(&flipData1, 1), BUFFERUSAGE_UNIFORM, "SwitchBuffer1");

	m_bindGroupConst = g_renderAPI->CreateBindGroup(m_pipeline, Builder<BindGroupDesc>()
		.GroupIndex(0)
		.Name("BlurConst")
		.Sampler(0, SamplerStateParams(TEXFILTER_LINEAR, TEXADDRESS_CLAMP))
		.Buffer(1, g_renderAPI->CreateBuffer(BufferInfo(&blurParams, 1), BUFFERUSAGE_UNIFORM, "ParamsBuffer"))
		.End());
}

void ComputeBlurShader::SetDestinationTexture(ITexture* dest)
{
	m_dstTexture = dest;

	const int dstWidth = m_dstTexture->GetWidth();
	const int dstHeight = m_dstTexture->GetHeight();

	m_blurTemp = g_renderAPI->CreateRenderTarget(
		Builder<TextureDesc>()
		.Name("_blurTemp")
		.Format(FORMAT_RGBA8)
		.Size(dstWidth, dstHeight, 2)
		.Flags(TEXFLAG_STORAGE | TEXFLAG_COPY_SRC | TEXFLAG_TRANSIENT)
		.End()
	);

	m_bindGroupStg1 = g_renderAPI->CreateBindGroup(m_pipeline, Builder<BindGroupDesc>()
		.GroupIndex(1)
		.Name("BlurParams1")
		.Texture(0, m_blurTemp, ITexture::ViewArraySlice(0))
		.StorageTexture(1, m_blurTemp, ITexture::ViewArraySlice(1))
		.Buffer(2, m_switchBuffer1)
		.End());

	m_bindGroupStg2 = g_renderAPI->CreateBindGroup(m_pipeline, Builder<BindGroupDesc>()
		.GroupIndex(1)
		.Name("BlurParams2")
		.Texture(0, m_blurTemp, ITexture::ViewArraySlice(1))
		.StorageTexture(1, m_blurTemp, ITexture::ViewArraySlice(0))
		.Buffer(2, m_switchBuffer0)
		.End());
}

void ComputeBlurShader::SetupExecute(IGPUCommandRecorder* commandRecorder, int arraySlice)
{
	const int dstWidth = m_dstTexture->GetWidth();
	const int dstHeight = m_dstTexture->GetHeight();

	IGPUBindGroupPtr bindGroupStg0 = g_renderAPI->CreateBindGroup(m_pipeline, Builder<BindGroupDesc>()
		.GroupIndex(1)
		.Name("BlurParams")
		.Texture(0, m_srcTexture, arraySlice == -1 ? ITexture::DEFAULT_VIEW : ITexture::ViewArraySlice(arraySlice))
		.StorageTexture(1, m_blurTemp, ITexture::ViewArraySlice(0))
		.Buffer(2, m_switchBuffer0)
		.End());

	IGPUComputePassRecorderPtr blurPassRecorder = commandRecorder->BeginComputePass("ComputeBlur");
	blurPassRecorder->SetPipeline(m_pipeline);
	blurPassRecorder->SetBindGroup(0, m_bindGroupConst);

	blurPassRecorder->SetBindGroup(1, bindGroupStg0);
	blurPassRecorder->DispatchWorkgroups(ceil(dstWidth * m_oneByBlockDim), ceil(dstHeight * m_oneByBatchSizeY), 1);

	blurPassRecorder->SetBindGroup(1, m_bindGroupStg1);
	blurPassRecorder->DispatchWorkgroups(ceil(dstHeight * m_oneByBlockDim), ceil(dstWidth * m_oneByBatchSizeY), 1);

	for (int i = 0; i < m_iterations - 1; ++i)
	{
		blurPassRecorder->SetBindGroup(1, m_bindGroupStg2);
		blurPassRecorder->DispatchWorkgroups(ceil(dstWidth * m_oneByBlockDim), ceil(dstHeight * m_oneByBatchSizeY), 1);

		blurPassRecorder->SetBindGroup(1, m_bindGroupStg1);
		blurPassRecorder->DispatchWorkgroups(ceil(dstHeight * m_oneByBlockDim), ceil(dstWidth * m_oneByBatchSizeY), 1);
	}

	blurPassRecorder->Complete();

	TextureCopyInfo srcTex{ m_blurTemp };
	srcTex.origin.arraySlice = 1;

	TextureCopyInfo dstTex{ m_dstTexture };
	dstTex.origin.arraySlice = arraySlice < 0 ? 0 : arraySlice;

	TextureExtent texExtents{ dstWidth, dstHeight, 1 };
	commandRecorder->CopyTextureToTexture(srcTex, dstTex, texExtents);
}