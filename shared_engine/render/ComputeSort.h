#pragma once
#include "materialsystem1/renderers/IShaderAPI.h"

enum EComputeSortValueType
{
	COMPUTESORT_FLOAT = 0,
	COMPUTESORT_INT,
	COMPUTESORT_CUSTOM_START
};

// Merge sort running on GPU
// NOTE: keys buffer's first element is key count
class ComputeSortShader : public RefCountedObject<ComputeSortShader>
{
public:
	ComputeSortShader();

	int		AddSortPipeline(const char* name, const char* shaderName = nullptr);

	void	InitKeys(IGPUCommandRecorder* cmdRecorder, IGPUBufferPtr keys, int keysCount);
	void	SortKeys(int dataTypeId, IGPUCommandRecorder* cmdRecorder, IGPUBufferPtr keys, int keysCount, IGPUBufferPtr values);

protected:
	void	RunSortPipeline(IGPUComputePipeline* sortPipeline, IGPUCommandRecorder* cmdRecorder, IGPUBufferPtr keys, int keysCount, IGPUBufferPtr values);

	Array<IGPUBufferPtr>	m_blocks{ PP_SL };
	IGPUComputePipelinePtr	m_initPipeline;
	IGPUComputePipelinePtr	m_prepareParamBufferPipeline;
	IGPUPipelineLayoutPtr	m_sortPipelineLayout;

	Map<int, IGPUComputePipelinePtr> m_sortPipelines{ PP_SL };
};

using ComputeSortShaderPtr = CRefPtr<ComputeSortShader>;