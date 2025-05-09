#pragma once
#include "materialsystem1/renderers/IShaderAPI.h"

class IGPUCommandRecorder;
class IGPURenderPipeline;
using IGPURenderPipelinePtr = CRefPtr<IGPURenderPipeline>;

// Blur shader using render pipeline
// Supports more formats like R16, R32 but might be slower
class BlurShader : public RefCountedObject<BlurShader>
{
public:
	enum EBlurFlags
	{
		BLUR_VERTICAL = (1 << 0),
		BLUR_HORIZONTAL = (1 << 1),
		BLUR_BOTH = (BLUR_VERTICAL | BLUR_HORIZONTAL)
	};

	BlurShader(int iterations = 4, float filterSize = 2.0, int blurFlags = BLUR_BOTH);

	void	SetSourceTexture(ITexture* source) { m_srcTexture = source; }
	void	SetDestinationTexture(ITexture* dest);

	void	SetupExecute(IGPUCommandRecorder* commandRecorder, int arraySlice = -1);

protected:
	IGPURenderPipelinePtr	m_pipeline;
	IGPUBufferPtr		m_switchBuffer0;
	IGPUBufferPtr		m_switchBuffer1;
	IGPUBindGroupPtr	m_bindGroupConst;

	ITexturePtr			m_blurTemp;
	IGPUBindGroupPtr	m_bindGroupStg1;
	IGPUBindGroupPtr	m_bindGroupStg2;

	ITexture*			m_srcTexture{ nullptr };
	ITexture*			m_dstTexture{ nullptr };

	ETextureFormat		m_destTextureFormat{ FORMAT_NONE };
	int					m_iterations{ 4 };
	float				m_filterSize{ 2.0f };
	int					m_blurFlags{ 0 };
};

using BlurShaderPtr = CRefPtr<BlurShader>;