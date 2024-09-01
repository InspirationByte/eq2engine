#pragma once
#include "materialsystem1/renderers/IShaderAPI.h"

enum EComputeSortValueType
{
	COMPUTESORT_FLOAT = 0,
	COMPUTESORT_INT,
	COMPUTESORT_CUSTOM_START
};

// Bitonic Merge sort running on GPU
// NOTE: keys buffer's first element is key count
class ComputeBitonicMergeSortShader : public RefCountedObject<ComputeBitonicMergeSortShader>
{
public:
	ComputeBitonicMergeSortShader();

	int		AddSortPipeline(const char* name, const char* shaderName = nullptr);

	void	InitKeys(IGPUCommandRecorder* cmdRecorder, IGPUBufferPtr keys, int keysCount);
	void	SortKeys(int dataTypeId, IGPUCommandRecorder* cmdRecorder, IGPUBufferPtr keys, int keysCount, IGPUBufferPtr values);

protected:
	void	RunSortPipeline(IGPUComputePipeline* sortPipeline, IGPUCommandRecorder* cmdRecorder, IGPUBufferPtr keys, int keysCount, IGPUBufferPtr values);

	Array<IGPUBufferPtr>	m_blocks{ PP_SL };
	IGPUComputePipelinePtr	m_initPipeline;

	Map<int, IGPUComputePipelinePtr> m_sortPipelines{ PP_SL };
};

using ComputeBitonicMergeSortShaderPtr = CRefPtr<ComputeBitonicMergeSortShader>;