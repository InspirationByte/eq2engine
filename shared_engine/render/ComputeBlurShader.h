#pragma once
#include "materialsystem1/renderers/IShaderAPI.h"

class IGPUCommandRecorder;
class IGPUComputePipeline;
using IGPUComputePipelinePtr = CRefPtr<IGPUComputePipeline>;

enum EBlurFlags
{
	BLUR_VERTICAL		= (1 << 0),
	BLUR_HORIZONTAL		= (1 << 1),
	BLUR_BOTH			= (BLUR_VERTICAL | BLUR_HORIZONTAL)
};

class ComputeBlurShader : public RefCountedObject<ComputeBlurShader>
{
public:
	ComputeBlurShader(int iterations = 4, int filterSize = 2, int blurFlags = BLUR_BOTH);

	void	SetSourceTexture(ITexture* source) { m_srcTexture = source; }
	void	SetDestinationTexture(ITexture* dest);

	void	SetupExecute(IGPUCommandRecorder* commandRecorder, int arraySlice = -1);

protected:
	IGPUComputePipelinePtr	m_pipeline;
	IGPUBufferPtr		m_switchBuffer0;
	IGPUBufferPtr		m_switchBuffer1;
	IGPUBindGroupPtr	m_bindGroupConst;

	ITexturePtr			m_blurTemp;
	IGPUBindGroupPtr	m_bindGroupStg1;
	IGPUBindGroupPtr	m_bindGroupStg2;

	ITexture*			m_srcTexture{ nullptr };
	ITexture*			m_dstTexture{ nullptr };

	int					m_iterations{ 4 };
	int					m_filterSize{ 2 };
	int					m_blurFlags{ 0 };

	float				m_oneByBlockDim{ 1.0f };
	float				m_oneByBatchSizeY{ 1.0f };
};

using ComputeBlurShaderPtr = CRefPtr<ComputeBlurShader>;