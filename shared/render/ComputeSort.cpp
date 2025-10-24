#include "core/core_common.h"
#include "ComputeSort.h"
#include "materialsystem1/IMatSysShader.h"

DEFINE_SHADER_NOFACTORY(ComputeSort)

// Based on https://github.com/magnickolas/odd-even-mergesort

constexpr int GROUP_SIZE		= 256;
constexpr int MAX_DIM_GROUPS	= 1024;
constexpr int MAX_DIM_THREADS	= (GROUP_SIZE * MAX_DIM_GROUPS);

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

constexpr EqStringRef BITONIC_MERGE_SORT_SHADERNAME = "ComputeSort";

ComputeSortShader::ComputeSortShader()
{
	m_initPipeline = g_renderAPI->CreateComputePipeline(Builder<ComputePipelineDesc>()
		.ShaderName(BITONIC_MERGE_SORT_SHADERNAME)
		.ShaderLayoutId(StringIdConst24("InitKeys"))
		.End()
	);
	m_sortPipelines.insert(COMPUTESORT_FLOAT, g_renderAPI->CreateComputePipeline(Builder<ComputePipelineDesc>()
		.ShaderName(BITONIC_MERGE_SORT_SHADERNAME)
		.ShaderLayoutId(StringIdConst24("SortFloat"))
		.End()
	));
	m_sortPipelines.insert(COMPUTESORT_INT, g_renderAPI->CreateComputePipeline(Builder<ComputePipelineDesc>()
		.ShaderName(BITONIC_MERGE_SORT_SHADERNAME)
		.ShaderLayoutId(StringIdConst24("SortInt"))
		.End()
	));
}

int ComputeSortShader::AddSortPipeline(const char* name, const char* shaderName)
{
	const int nameHash = StringId24(name);
	m_sortPipelines.insert(nameHash, g_renderAPI->CreateComputePipeline(Builder<ComputePipelineDesc>()
		.ShaderName(shaderName ? EqStringRef(shaderName) : BITONIC_MERGE_SORT_SHADERNAME)
		.ShaderLayoutId(nameHash)
		.End()
	));
	return nameHash;
}

void ComputeSortShader::InitKeys(IGPUCommandRecorder* cmdRecorder, IGPUBufferPtr keys, int keysCount)
{
	if (keysCount <= 0)
		return;

	int x, y, z;
	bitonicCalcWorkSize(keysCount, x, y, z);

	IGPUComputePassRecorderPtr computePassRecorder = cmdRecorder->BeginComputePass("Sort");
	computePassRecorder->SetPipeline(m_initPipeline);
	IGPUBindGroupPtr inputBindGroup = g_renderAPI->CreateBindGroup(m_initPipeline, Builder<BindGroupDesc>()
		.GroupIndex(0)
		.Buffer(StringIdConst24("keyData"), keys)
		.End()
	);
	computePassRecorder->SetBindGroup(0, inputBindGroup);
	computePassRecorder->DispatchWorkgroups(x, y, z);
	computePassRecorder->Complete();
}

void ComputeSortShader::SortKeys(int dataTypeId, IGPUCommandRecorder* cmdRecorder, IGPUBufferPtr keys, int keysCount, IGPUBufferPtr values)
{
	// no point to sort
	if (keysCount <= 1)
		return;

	auto it = m_sortPipelines.find(dataTypeId);
	ASSERT_MSG(!it.atEnd(), "Can't find pipeline for specified data type id");
	if (it.atEnd())
		return;
	RunSortPipeline(*it, cmdRecorder, keys, keysCount, values);
}

void ComputeSortShader::RunSortPipeline(IGPUComputePipeline* sortPipeline, IGPUCommandRecorder* cmdRecorder, IGPUBufferPtr keys, int keysCount, IGPUBufferPtr values)
{
	if (keysCount <= 0)
		return;

	const ShaderAPICapabilities& rhiCaps = g_renderAPI->GetCaps();

	uint N = 1;
	while (N < keysCount)
		N *= 2;

	struct ParamsData
	{
		uint stride = 0;
		uint strideTrailingZeros = 0;
		uint innerReminder = 0;
		uint innerLastIdx = 0;
	};

	const uint L = (N >= 2) ? (32 - leadingZeroCnt(N)) - 1 : 0;
	const int paramsDataCount = static_cast<size_t>(L) * (L + 1) / 2;
	const int alignedParamSize = max(sizeof(ParamsData), rhiCaps.minStorageBufferOffsetAlignment);

	const BufferInfo paramBufferInfo(alignedParamSize, paramsDataCount);

	IGPUBufferPtr paramsBuffer = m_paramsBuffer;
	if(!paramsBuffer || paramsBuffer->GetSize() < paramBufferInfo.GetBufferSize())
	{
		paramsBuffer = g_renderAPI->CreateBuffer(paramBufferInfo, BUFFERUSAGE_UNIFORM | BUFFERUSAGE_COPY_DST, "SortParams");
		m_paramsBuffer = paramsBuffer;

		// create parameter list
		static thread_local Array<ParamsData> paramsDataList{ PP_SL };
		paramsDataList.clear();
		paramsDataList.reserve(paramsDataCount);

		for (uint mergeGroupSize = 2; mergeGroupSize <= N; mergeGroupSize <<= 1)
		{
			uint innerReminder = 0;
			for (uint stride = mergeGroupSize >> 1; stride >= 1; stride >>= 1)
			{
				const uint strideTrailingZeros = trailingZeroCnt(stride);
				const uint innerLastIdx = (mergeGroupSize >> strideTrailingZeros) - 1;

				ParamsData& data = paramsDataList.append();
				data.stride = stride;
				data.strideTrailingZeros = strideTrailingZeros;
				data.innerReminder = innerReminder;
				data.innerLastIdx = innerLastIdx;

				// Starting from the second iteration, inner index
				// should be odd to be the left one
				innerReminder = 1;
			}
		}

		// write params aligned
		for (int i = 0; i < paramsDataCount; ++i)
			cmdRecorder->WriteBuffer(paramsBuffer, &paramsDataList[i], sizeof(ParamsData), alignedParamSize * i);
	}

	IGPUBindGroupPtr sortDataGroup = g_renderAPI->CreateBindGroup(sortPipeline, Builder<BindGroupDesc>()
		.GroupIndex(0)
		.Buffer(StringIdConst24("keyData"), keys)
		.Buffer(StringIdConst24("values"), values)
		.End()
	);

	IGPUComputePassRecorderPtr computePassRecorder = cmdRecorder->BeginComputePass("Sort");
	computePassRecorder->SetPipeline(sortPipeline);
	computePassRecorder->SetBindGroup(0, sortDataGroup);

	constexpr int SORT_TILE_SIZE = 256;
	const uint x = (keysCount + SORT_TILE_SIZE - 1) - (keysCount - 1) % SORT_TILE_SIZE;
	ASSERT(x% SORT_TILE_SIZE == 0);

	const uint workgroupCountX = x / SORT_TILE_SIZE;
	for (int i = 0; i < paramsDataCount; ++i)
	{
		IGPUBindGroupPtr paramsBindGroup = g_renderAPI->CreateBindGroup(sortPipeline, Builder<BindGroupDesc>()
			.GroupIndex(1)
			.Buffer(StringIdConst24("params"), paramsBuffer, alignedParamSize * i, sizeof(ParamsData))
			.End()
		);
		computePassRecorder->SetBindGroup(1, paramsBindGroup);
		computePassRecorder->DispatchWorkgroups(workgroupCountX);
	}

	computePassRecorder->Complete();
}
