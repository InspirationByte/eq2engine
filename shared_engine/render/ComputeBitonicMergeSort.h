#pragma once
#include "materialsystem1/renderers/IShaderAPI.h"

// Bitonic Merge sort running on GPU
// NOTE: keys buffer's first element is key count!
class ComputeBitonicMergeSortShader : public RefCountedObject<ComputeBitonicMergeSortShader>
{
public:
	ComputeBitonicMergeSortShader();

	void	Init(IGPUCommandRecorder* cmdRecorder, IGPUBufferPtr keys, int keysCount);
	void	SortFloats(IGPUCommandRecorder* cmdRecorder, IGPUBufferPtr keys, int keysCount, IGPUBufferPtr values);
	void	SortInts(IGPUCommandRecorder* cmdRecorder, IGPUBufferPtr keys, int keysCount, IGPUBufferPtr values);

protected:
	void	RunSortPipeline(IGPUComputePipeline* sortPipeline, IGPUCommandRecorder* cmdRecorder, IGPUBufferPtr keys, int keysCount, IGPUBufferPtr values);

	struct BlockDim
	{
		int block;
		int dim;
	};
	Array<BlockDim>			m_blocks{ PP_SL };
	IGPUComputePipelinePtr	m_initPipeline;
	IGPUComputePipelinePtr	m_sortFloatsPipeline;
	IGPUComputePipelinePtr	m_sortIntsPipeline;
};

using ComputeBitonicMergeSortShaderPtr = CRefPtr<ComputeBitonicMergeSortShader>;