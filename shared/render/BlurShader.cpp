#include "core/core_common.h"
#include "BlurShader.h"

BlurShader::BlurShader(int iterations, float filterSize, int blurFlags)
	: m_iterations(iterations)
	, m_filterSize(filterSize)
	, m_blurFlags(blurFlags)
{
	struct
	{
		uint flipValue;
		int _padding[3];
	} flipData0, flipData1;

	if (m_blurFlags == BLUR_BOTH)
	{
		flipData0.flipValue = 0;	// X
		flipData1.flipValue = 1;	// Y
	}
	else if (m_blurFlags == BLUR_VERTICAL)
	{
		flipData0.flipValue = 1;	// Y
		flipData1.flipValue = 1;	// Y
	}
	else if (m_blurFlags == BLUR_HORIZONTAL)
	{
		flipData0.flipValue = 0;	// X
		flipData1.flipValue = 0;	// X
	}

	m_switchBuffer0 = g_renderAPI->CreateBuffer(BufferInfo(&flipData0, 1), BUFFERUSAGE_UNIFORM, "SwitchBuffer0");
	m_switchBuffer1 = g_renderAPI->CreateBuffer(BufferInfo(&flipData1, 1), BUFFERUSAGE_UNIFORM, "SwitchBuffer1");
}

void BlurShader::SetDestinationTexture(ITexture* dest)
{
	m_dstTexture = dest;

	const IVector2D dstSize = m_dstTexture->GetSize();
	const ETextureFormat destTexFormat = m_dstTexture->GetFormat();

	if(m_destTextureFormat != destTexFormat || !m_pipeline)
	{
		m_destTextureFormat = destTexFormat;

		m_pipeline = g_renderAPI->CreateRenderPipeline(
			Builder<RenderPipelineDesc>()
			.ShaderName("Blur")
			.FragmentState(
				Builder<FragmentPipelineDesc>()
				.ColorTarget("Target", m_destTextureFormat)
				.End()
			)
			.PrimitiveState(Builder<PrimitiveDesc>()
				.Topology(PRIM_TRIANGLE_STRIP)
				.Cull(CULL_NONE) // just in case
				.End()
			)
			.End()
		);

		struct
		{
			float filterDim;
			int _padding[3];
		} blurParams;

		blurParams.filterDim = m_filterSize;

		m_bindGroupConst = g_renderAPI->CreateBindGroup(m_pipeline, Builder<BindGroupDesc>()
			.GroupIndex(0)
			.Name("BlurConst")
			.Sampler(0, SamplerStateParams(TEXFILTER_LINEAR, TEXADDRESS_CLAMP))
			.Buffer(1, g_renderAPI->CreateBuffer(BufferInfo(&blurParams, 1), BUFFERUSAGE_UNIFORM, "ParamsBuffer"))
			.End());
	}

	m_blurTemp = g_renderAPI->CreateRenderTarget(
		Builder<TextureDesc>()
		.Name("_blurTemp")
		.Format(m_destTextureFormat)
		.Size(dstSize, 2)
		.Flags(TEXFLAG_COPY_SRC | TEXFLAG_TRANSIENT)
		.End()
	);

	m_bindGroupStg1 = g_renderAPI->CreateBindGroup(m_pipeline, Builder<BindGroupDesc>()
		.GroupIndex(1)
		.Name("BlurParams1")
		.Texture(0, m_blurTemp, ITexture::ViewArraySlice(0))
		.Buffer(1, m_switchBuffer1)
		.End());

	m_bindGroupStg2 = g_renderAPI->CreateBindGroup(m_pipeline, Builder<BindGroupDesc>()
		.GroupIndex(1)
		.Name("BlurParams2")
		.Texture(0, m_blurTemp, ITexture::ViewArraySlice(1))
		.Buffer(1, m_switchBuffer0)
		.End());
}

void BlurShader::SetupExecute(IGPUCommandRecorder* commandRecorder, int arraySlice)
{
	commandRecorder->DbgPushGroup("BlurShader");

	const IVector2D dstSize = m_dstTexture->GetSize();

	IGPUBindGroupPtr bindGroupStg0 = g_renderAPI->CreateBindGroup(m_pipeline, Builder<BindGroupDesc>()
		.GroupIndex(1)
		.Name("BlurParams")
		.Texture(0, m_srcTexture, arraySlice == -1 ? ITexture::DEFAULT_VIEW : ITexture::ViewArraySlice(arraySlice))
		.Buffer(1, m_switchBuffer0)
		.End());

	RenderPassDesc drawToTemp0 = Builder<RenderPassDesc>()
		.ColorTarget(TextureView(m_blurTemp, ITexture::ViewArraySlice(0)))
		.End();

	RenderPassDesc drawToTemp1 = Builder<RenderPassDesc>()
		.ColorTarget(TextureView(m_blurTemp, ITexture::ViewArraySlice(1)))
		.End();

	IGPURenderPassRecorderPtr rendPassRecorder;

	// Notes:
	// bindGroupStg0 uses drawToTemp0
	// bindGroupStg1 uses drawToTemp1
	// bindGroupStg2 uses drawToTemp0

	// initial render pass into temp texture
	{
		{
			rendPassRecorder = commandRecorder->BeginRenderPass(drawToTemp0);
			rendPassRecorder->SetPipeline(m_pipeline);
			rendPassRecorder->SetBindGroup(0, m_bindGroupConst);
			rendPassRecorder->SetBindGroup(1, bindGroupStg0);
			rendPassRecorder->Draw(4, 0, 1);

			rendPassRecorder->Complete();
		}

		{
			rendPassRecorder = commandRecorder->BeginRenderPass(drawToTemp1);
			rendPassRecorder->SetPipeline(m_pipeline);
			rendPassRecorder->SetBindGroup(0, m_bindGroupConst);
			rendPassRecorder->SetBindGroup(1, m_bindGroupStg1);
			rendPassRecorder->Draw(4, 0, 1);

			rendPassRecorder->Complete();
		}
	}

	for (int i = 0; i < m_iterations - 1; ++i)
	{
		{
			rendPassRecorder = commandRecorder->BeginRenderPass(drawToTemp0);
			rendPassRecorder->SetPipeline(m_pipeline);
			rendPassRecorder->SetBindGroup(0, m_bindGroupConst);
			rendPassRecorder->SetBindGroup(1, m_bindGroupStg2);
			rendPassRecorder->Draw(4, 0, 1);

			rendPassRecorder->Complete();
		}

		{
			rendPassRecorder = commandRecorder->BeginRenderPass(drawToTemp1);
			rendPassRecorder->SetPipeline(m_pipeline);
			rendPassRecorder->SetBindGroup(0, m_bindGroupConst);
			rendPassRecorder->SetBindGroup(1, m_bindGroupStg1);
			rendPassRecorder->Draw(4, 0, 1);

			rendPassRecorder->Complete();
		}		
	}

	TextureCopyInfo srcTex{ m_blurTemp };
	srcTex.origin.arraySlice = 1;

	TextureCopyInfo dstTex{ m_dstTexture };
	dstTex.origin.arraySlice = arraySlice < 0 ? 0 : arraySlice;

	TextureExtent texExtents{ dstSize };
	commandRecorder->CopyTextureToTexture(srcTex, dstTex, texExtents);

	commandRecorder->DbgPopGroup();
}