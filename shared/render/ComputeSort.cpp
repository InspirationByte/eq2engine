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
	m_prepareParamBufferPipeline = g_renderAPI->CreateComputePipeline(Builder<ComputePipelineDesc>()
		.ShaderName(BITONIC_MERGE_SORT_SHADERNAME)
		.ShaderLayoutId(StringIdConst24("PrepareParamBuffer"))
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

	const uint n = keysCount;
	uint N = 1;
	while (N < n)
		N *= 2;

	const int TILE_SIZE = 256;

	struct ParamsData
	{
		uint stride = 0;
		uint strideTrailingZeros = 0;
		uint innerReminder = 0;
		uint innerLastIdx = 0;

		uint indirectX = 1;
		uint _indirectY = 1;
		uint _indirectZ = 1;
		uint _padding = 0;
	};
	static thread_local Array<ParamsData>	paramsDataList{ PP_SL };

	paramsDataList.clear();
	paramsDataList.reserve(N);

	for (uint mergeGroupSize = 2; mergeGroupSize <= N; mergeGroupSize <<= 1)
	{
		uint innerReminder = 0;
		for (uint stride = mergeGroupSize >> 1; stride >= 1; stride >>= 1)
		{
			const uint strideTrailingZeros = trailingZeroCnt(stride);
			const uint innerLastIdx = (mergeGroupSize >> strideTrailingZeros) - 1;

			const uint x = (n + TILE_SIZE - 1) - (n - 1) % TILE_SIZE;
			ASSERT(x % TILE_SIZE == 0);

			ParamsData& data = paramsDataList.append();
			data.stride = stride;
			data.strideTrailingZeros = strideTrailingZeros;
			data.innerReminder = innerReminder;
			data.innerLastIdx = innerLastIdx;
			data.indirectX = x / TILE_SIZE;

			// Starting from the second iteration, inner index
			// should be odd to be the left one
			innerReminder = 1;
		}
	}

	const ShaderAPICapabilities& rhiCaps = g_renderAPI->GetCaps();

	const int alignedParamSize = max(sizeof(ParamsData), rhiCaps.minStorageBufferOffsetAlignment);

	const BufferInfo paramBufferInfo(alignedParamSize, paramsDataList.numElem());
	if(!m_paramsBuffer || m_paramsBuffer->GetSize() < paramBufferInfo.GetBufferSize())
		m_paramsBuffer = g_renderAPI->CreateBuffer(paramBufferInfo, BUFFERUSAGE_UNIFORM | BUFFERUSAGE_STORAGE | BUFFERUSAGE_INDIRECT, "SortParams");

	{
		struct SmolStruct
		{
			int steps;
			int paramCount;
		} smol;
		smol.steps = alignedParamSize / sizeof(ParamsData);
		smol.paramCount = paramsDataList.numElem();

		const BufferInfo tmpParamsBufferInfo(sizeof(ParamsData), paramsDataList.numElem() + 1);
		if(!m_tmpParamsBuffer || m_tmpParamsBuffer->GetSize() < tmpParamsBufferInfo.GetBufferSize())
			m_tmpParamsBuffer = g_renderAPI->CreateBuffer(tmpParamsBufferInfo, BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "SortTmpParams");

		cmdRecorder->WriteBuffer(m_tmpParamsBuffer, &smol, sizeof(smol), 0);
		cmdRecorder->WriteBuffer(m_tmpParamsBuffer, paramsDataList.ptr(), sizeof(ParamsData) * paramsDataList.numElem(), sizeof(smol));

		// m_paramsBuffer -> BUFFERUSAGE_STORAGE
		IGPUBindGroupPtr paramsGroup = g_renderAPI->CreateBindGroup(m_prepareParamBufferPipeline, Builder<BindGroupDesc>()
			.GroupIndex(0)
			.Buffer(StringIdConst24("srcParams"), m_tmpParamsBuffer)
			.Buffer(StringIdConst24("dstParams"), m_paramsBuffer)
			.End()
		);

		IGPUComputePassRecorderPtr computePassRecorder = cmdRecorder->BeginComputePass("PrepareParams");
		computePassRecorder->SetPipeline(m_prepareParamBufferPipeline);
		computePassRecorder->SetBindGroup(0, paramsGroup);

		constexpr int GROUP_SIZE = 16;
		computePassRecorder->DispatchWorkgroups(paramsDataList.numElem() / GROUP_SIZE + 1);
		computePassRecorder->Complete();
	}

	IGPUBindGroupPtr sortDataGroup = g_renderAPI->CreateBindGroup(sortPipeline, Builder<BindGroupDesc>()
		.GroupIndex(0)
		.Buffer(StringIdConst24("keyData"), keys)
		.Buffer(StringIdConst24("values"), values)
		.End()
	);

	// m_paramsBuffer -> BUFFERUSAGE_INDIRECT
	IGPUComputePassRecorderPtr computePassRecorder = cmdRecorder->BeginComputePass("Sort");
	computePassRecorder->SetPipeline(sortPipeline);
	computePassRecorder->SetBindGroup(0, sortDataGroup);

	for (int i = 0; i < paramsDataList.numElem(); ++i)
	{
		IGPUBindGroupPtr paramsBindGroup = g_renderAPI->CreateBindGroup(sortPipeline, Builder<BindGroupDesc>()
			.GroupIndex(1)
			.Buffer(StringIdConst24("params"), m_paramsBuffer, alignedParamSize * i, sizeof(ParamsData))
			.End()
		);
		computePassRecorder->SetBindGroup(1, paramsBindGroup);
		computePassRecorder->DispatchWorkgroupsIndirect(m_paramsBuffer, alignedParamSize * i + offsetOf(ParamsData, indirectX));
	}

	computePassRecorder->Complete();
}
