#include "core/core_common.h"
#include "ComputeBitonicMergeSort.h"

// Based on https://github.com/nobnak/GPUMergeSortForUnity

constexpr int GROUP_SIZE		= 256;
constexpr int MAX_DIM_GROUPS	= 1024;
constexpr int MAX_DIM_THREADS	= (GROUP_SIZE * MAX_DIM_GROUPS);

constexpr EqStringRef BITONIC_MERGE_SORT_SHADERNAME = "ComputeBitonicMergeSort";

static void bitonicCalcWorkSize(int length, int& x, int& y, int& z)
{
	if (length <= MAX_DIM_THREADS)
	{
		x = (length - 1) / GROUP_SIZE + 1;
		y = z = 1;
	}
	else
	{
		x = MAX_DIM_GROUPS;
		y = (length - 1) / MAX_DIM_THREADS + 1;
		z = 1;
	}
}

ComputeBitonicMergeSortShader::ComputeBitonicMergeSortShader()
{
	m_initPipeline = g_renderAPI->CreateComputePipeline(Builder<ComputePipelineDesc>()
		.ShaderName(BITONIC_MERGE_SORT_SHADERNAME)
		.ShaderLayoutId(StringToHashConst("InitKeys"))
		.End()
	);
	m_sortFloatsPipeline = g_renderAPI->CreateComputePipeline(Builder<ComputePipelineDesc>()
		.ShaderName(BITONIC_MERGE_SORT_SHADERNAME)
		.ShaderLayoutId(StringToHashConst("BitonicSort"))
		.End()
	);
	m_sortIntsPipeline = g_renderAPI->CreateComputePipeline(Builder<ComputePipelineDesc>()
		.ShaderName(BITONIC_MERGE_SORT_SHADERNAME)
		.ShaderLayoutId(StringToHashConst("BitonicSortInt"))
		.End()
	);
}

void ComputeBitonicMergeSortShader::Init(IGPUCommandRecorder* cmdRecorder, IGPUBufferPtr keys, int keysCount)
{
	if (keysCount <= 0)
		return;

	int x, y, z;
	bitonicCalcWorkSize(keysCount, x, y, z);

	IGPUComputePassRecorderPtr computePassRecorder = cmdRecorder->BeginComputePass("Sort");
	computePassRecorder->SetPipeline(m_initPipeline);
	IGPUBindGroupPtr inputBindGroup = g_renderAPI->CreateBindGroup(m_initPipeline, Builder<BindGroupDesc>()
		.GroupIndex(0)
		.Buffer(0, keys)
		.End()
	);
	computePassRecorder->SetBindGroup(0, inputBindGroup);
	computePassRecorder->DispatchWorkgroups(x, y, z);
	computePassRecorder->Complete();
}

void ComputeBitonicMergeSortShader::SortFloats(IGPUCommandRecorder* cmdRecorder, IGPUBufferPtr keys, int keysCount, IGPUBufferPtr values)
{
	RunSortPipeline(m_sortFloatsPipeline, cmdRecorder, keys, keysCount, values);
}

void ComputeBitonicMergeSortShader::SortInts(IGPUCommandRecorder* cmdRecorder, IGPUBufferPtr keys, int keysCount, IGPUBufferPtr values)
{
	RunSortPipeline(m_sortIntsPipeline, cmdRecorder, keys, keysCount, values);
}

void ComputeBitonicMergeSortShader::RunSortPipeline(IGPUComputePipeline* sortPipeline, IGPUCommandRecorder* cmdRecorder, IGPUBufferPtr keys, int keysCount, IGPUBufferPtr values)
{
	if (keysCount <= 0)
		return;

	int x, y, z;
	bitonicCalcWorkSize(keysCount, x, y, z);

	// prepare blocks to process
	struct BlockDim
	{
		int block;
		int dim;
	};

	int numBlockDims = 0;
	for (int dim = 2; dim <= keysCount; dim <<= 1)
	{
		for (int block = dim >> 1; block > 0; block >>= 1, ++numBlockDims)
		{
			if (numBlockDims < m_blocks.numElem())
				continue;

			BlockDim data{ block, dim };

			IGPUBufferPtr blockDimBuffer = g_renderAPI->CreateBuffer(BufferInfo(sizeof(data), 1), BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "BlockDim");
			cmdRecorder->WriteBuffer(blockDimBuffer, &data, sizeof(data), 0);

			m_blocks.append(blockDimBuffer);
		}
	}

	IGPUComputePassRecorderPtr computePassRecorder = cmdRecorder->BeginComputePass("Sort");
	computePassRecorder->SetPipeline(sortPipeline);
	{
		IGPUBindGroupPtr inputBindGroup = g_renderAPI->CreateBindGroup(sortPipeline, Builder<BindGroupDesc>()
			.GroupIndex(0)
			.Buffer(0, keys)
			.End()
		);
		IGPUBindGroupPtr outputBindGroup = g_renderAPI->CreateBindGroup(sortPipeline, Builder<BindGroupDesc>()
			.GroupIndex(2)
			.Buffer(0, values)
			.End()
		);
		computePassRecorder->SetBindGroup(0, inputBindGroup);
		computePassRecorder->SetBindGroup(2, outputBindGroup);
	}

	for (int i = 0; i < numBlockDims; ++i)
	{
		IGPUBindGroupPtr blockDimBindGroup = g_renderAPI->CreateBindGroup(sortPipeline, Builder<BindGroupDesc>()
			.GroupIndex(1)
			.Buffer(0, m_blocks[i])
			.End()
		);
		computePassRecorder->SetBindGroup(1, blockDimBindGroup);
		computePassRecorder->DispatchWorkgroups(x, y, z);
	}
	computePassRecorder->Complete();
}
