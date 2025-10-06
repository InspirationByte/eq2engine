#include "core/core_common.h"
#include "utils/KeyValues.h"

#include "ShaderInfo.h"

uint ShaderInfo::PackShaderModuleId(int queryStrHash, int vertexLayoutIdx, int kind, int entryPointStrHash)
{
	uint hash = queryStrHash | (static_cast<uint>(vertexLayoutIdx) << StringId24Bits) | (static_cast<uint>(kind) << (StringId24Bits + 4));
	hash *= 31;
	hash += entryPointStrHash;
	return hash;
}

uint ShaderInfo::MakeBindingId(int descriptorSetIdx, int index)
{
	return static_cast<uint64>(descriptorSetIdx | (index << 8));
}

void ShaderInfo::UnpackBindingId(uint bindingIdx, int& descriptorSetIdx, int& index)
{
	descriptorSetIdx = bindingIdx & 255;
	index = bindingIdx >> 8;
}

uint64 ShaderInfo::MakeShaderBindingId(uint shaderModuleId, uint bindingIdx)
{
	return static_cast<uint64>(shaderModuleId) | (static_cast<uint64>(bindingIdx) << 32);
}

uint64 ShaderInfo::MakeShaderBindingId(uint shaderModuleId, int descriptorSetIdx, int index)
{
	return MakeShaderBindingId(shaderModuleId, MakeBindingId(descriptorSetIdx, index));
}

ShaderInfo::ShaderInfo(ShaderInfo&& other) noexcept
	: shaderName(std::move(other.shaderName))
	, shaderPackFile(std::move(other.shaderPackFile))
	, vertexLayouts(std::move(other.vertexLayouts))
	, defines(std::move(other.defines))
	, modules(std::move(other.modules))
	, modulesMap(std::move(other.modulesMap))
	, shaderKinds(other.shaderKinds)

{
	other.shaderPackFile = nullptr;
}

ShaderInfo& ShaderInfo::operator=(ShaderInfo&& other) noexcept
{
	shaderName = std::move(other.shaderName);
	shaderPackFile = std::move(other.shaderPackFile);
	vertexLayouts = std::move(other.vertexLayouts);
	defines = std::move(other.defines);
	modules = std::move(other.modules);
	modulesMap = std::move(other.modulesMap);
	shaderKinds = other.shaderKinds;
	other.shaderPackFile = nullptr;
	return *this;
}

EqStringRef ShaderInfo::GetShaderQueryStr(ArrayCRef<EqString> findDefines) const
{
	Array<int> defineIds(PP_SL);
	for (const EqString& define : findDefines)
	{
		const int defineId = arrayFindIndex(defines, define);
		if (defineId == -1)
			return nullptr;
		defineIds.append(defineId);
	}

	arraySort(defineIds, [](int a, int b) {
		return a - b;
		});

	EqString& queryStr = EqStringRef::GetTempString(nullptr, 0);
	for (int id : defineIds)
	{
		if (queryStr.Length())
			queryStr.Append("|");
		queryStr.Append(defines[id]);
	}
	return queryStr;
}

bool ShaderInfo::GetShaderQueryHash(ArrayCRef<EqString> findDefines, int& outHash) const
{
	outHash = StringId24(GetShaderQueryStr(findDefines), true);
	return true;
}


constexpr EqStringRef s_shaderKindVertexName = "Vertex";
constexpr EqStringRef s_shaderKindFragmentName = "Fragment";
constexpr EqStringRef s_shaderKindComputeName = "Compute";
constexpr EqStringRef s_DefaultVertexLayoutName = "Default";

static const char* s_bindingTypeNames[] = {
	"buffer",
	"sampler",
	"texture",
	"storagetexture",
};

static EBindEntryType GetBindingTypeByName(const char* name)
{
	for (int i = 0; i < elementsOf(s_bindingTypeNames); ++i)
	{
		if (!CString::Compare(s_bindingTypeNames[i], name))
			return (EBindEntryType)i;
	}
	return (EBindEntryType) - 1;
}

void ShaderInfo::ParseModuleBindings(const KVSection& bindingsSec, uint shaderModuleId, Module& moduleInfo, Map<uint, int>& usedBindingSlots)
{
	const int bindingCount = bindingsSec.KeyCount();
	moduleInfo.bindingsStart = bindingIds.numElem();

	// store binding count
	bindingIds.reserve(bindingIds.numElem() + bindingCount + 1);
	bindingIds.append(bindingCount);

	bindings.reserve(bindings.numElem() + bindingCount);
	for (const KVSection& bindingSec : bindingsSec.Keys())
	{
		Binding binding;

		int rangeTypeIdx;
		EqStringRef rwFlagsStr;
		EqStringRef typeName;
		bindingSec.GetValues(typeName, rwFlagsStr, binding.descriptorSetIdx, binding.index, rangeTypeIdx, binding.registerIdx);

		binding.type = GetBindingTypeByName(typeName);
		binding.name = bindingSec.GetName();
		binding.rangeType = static_cast<EBindingRangeType>(rangeTypeIdx);
		ASSERT(binding.type >= 0);

		if (rwFlagsStr == "readonly")
			binding.rwFlags = RWFLAG_READ;
		else if (rwFlagsStr == "writeonly")
			binding.rwFlags = RWFLAG_WRITE;
		else if (rwFlagsStr == "readwrite")
			binding.rwFlags = RWFLAG_READ | RWFLAG_WRITE;
		else if (rwFlagsStr == "uniform")
			binding.rwFlags = RWFLAG_UNIFORM;

		uint bindingId = StringId(binding.name);
		bindingId *= 31;
		bindingId += binding.rwFlags | (binding.type << 3) | (binding.rangeType << 5) | (binding.descriptorSetIdx << 8);
		bindingId *= 31;
		bindingId += binding.index | (binding.registerIdx << 16);

		auto it = usedBindingSlots.find(bindingId);
		int idx = -1;
		if (!it.atEnd())
		{
			idx = *it;
			const Binding& foundBinding = bindings[idx];
			ASSERT_MSG(foundBinding.name == binding.name, "bindingId hash collision");
			ASSERT_MSG(foundBinding.type == binding.type, "bindingId hash collision");
			ASSERT_MSG(foundBinding.descriptorSetIdx == binding.descriptorSetIdx, "bindingId hash collision");
			ASSERT_MSG(foundBinding.index == binding.index, "bindingId hash collision");
			ASSERT_MSG(foundBinding.rangeType == binding.rangeType, "bindingId hash collision");
			ASSERT_MSG(foundBinding.registerIdx == binding.registerIdx, "bindingId hash collision");
		}
		else
		{
			idx = bindings.append(binding);
			usedBindingSlots.insert(bindingId, idx);
		}

		const uint64 key = MakeShaderBindingId(shaderModuleId, binding.descriptorSetIdx, binding.index);
		bindingMap.insert(key, idx);
		bindingIds.append(MakeBindingId(binding.descriptorSetIdx, binding.index));
	}
}

ArrayCRef<uint>	ShaderInfo::GetBindingIds(const ShaderInfo::Module& module) const
{
	return ArrayCRef(&bindingIds[module.bindingsStart + 1], bindingIds[module.bindingsStart]);
}

bool ShaderInfo::ParseShaderInfo(ShaderInfo& shaderInfo, IPackFileReaderPtr shaderPackFile, const KVSection& shaderInfoKvs, int& filesFound, bool parseBindings)
{
	shaderInfo.shaderPackFile = shaderPackFile;
	shaderInfo.shaderName = shaderInfoKvs.GetName();

	const KVSection* defines = shaderInfoKvs["Defines"];
	if (defines)
	{
		shaderInfo.defines.reserve(defines->ValueCount());
		for (const EqStringRef def : defines->Values<EqStringRef>())
			shaderInfo.defines.append(def);
	}

	for (const KVSection& key : shaderInfoKvs.Get("VertexLayouts").Keys())
	{
		ShaderInfo::VertLayout& layout = shaderInfo.vertexLayouts.append();
		layout.name = key.GetName();
		if (layout.name != s_DefaultVertexLayoutName)
			layout.nameHash = StringId24(layout.name);
		
		if (!CString::CompareCaseIns(KV_GetValueString(&key, 0), "aliasOf"))
		{
			layout.aliasOf = arrayFindIndexF(shaderInfo.vertexLayouts, [&](const ShaderInfo::VertLayout& layout) {
				return layout.name == EqStringRef(KV_GetValueString(&key, 1));
			});
		}
	}

	auto getKind = [](const EqStringRef& kindStr) -> int {
		if (!kindStr.CompareCaseIns(s_shaderKindVertexName))
			return SHADERKIND_VERTEX;
		else if (!kindStr.CompareCaseIns(s_shaderKindFragmentName))
			return SHADERKIND_FRAGMENT;
		else if (!kindStr.CompareCaseIns(s_shaderKindComputeName))
			return SHADERKIND_COMPUTE;
		return 0;
	};

	auto getKindExt = [](int kind) -> char* {
		if (kind == SHADERKIND_VERTEX)
			return ".vert";
		if (kind == SHADERKIND_FRAGMENT)
			return ".frag";
		if (kind == SHADERKIND_COMPUTE)
			return ".comp";
		return nullptr;
	};

	Map<uint, int> usedBindingSlots(PP_SL);

	filesFound = 0;
	const KVSection* fileListSec = shaderInfoKvs["FileList"];
	for (const KVSection& itemSec : fileListSec->Keys("blob"))
	{
		int vertLayoutIdx = -1;
		EqStringRef kindStr;
		EqStringRef entryPointName;
		EqStringRef queryStr;
		if (itemSec.GetValues(vertLayoutIdx, kindStr, entryPointName, queryStr) < 4)
		{
			ASSERT_FAIL("Shader %s 'blob' does not have 4 values");
			break;
		}

		const int kind = getKind(kindStr);
		ASSERT_MSG(kind != 0, "Shader kind is not valid");

		const int queryStrHash = StringId24(queryStr, true);
		const int entryPointStrHash = StringId24(entryPointName);
		const uint shaderModuleId = ShaderInfo::PackShaderModuleId(queryStrHash, vertLayoutIdx, kind, entryPointStrHash);

		const int moduleIndex = shaderInfo.modules.numElem();
		{
			const EqString shaderFileName = EqString::Format("%s-%s%s", shaderInfo.vertexLayouts[vertLayoutIdx].name, queryStr, getKindExt(kind));
			
			ShaderInfo::Module& modInfo = shaderInfo.modules.append();
			for (int i = 0; i < SHADERMODULE_TYPES; ++i)
				modInfo.fileIndex[i] = shaderInfo.shaderPackFile->FindFileIndex(shaderFileName + s_shaderModuleTypeExt[i]);

			modInfo.kind = static_cast<EShaderKind>(kind);
			modInfo.entryPoint = entryPointName;

			// parse module pipeline layout
			if (parseBindings)
				shaderInfo.ParseModuleBindings(itemSec, shaderModuleId, modInfo, usedBindingSlots);
		}
		{

			auto exIt = shaderInfo.modulesMap.find(shaderModuleId);
			ASSERT_MSG(exIt.atEnd(), "%s-%s%s module already added at idx %d (check for hash collisions)", shaderInfo.shaderName.ToCString(), queryStr, kindStr.ToCString(), exIt.value());

			shaderInfo.modulesMap.insert(shaderModuleId, moduleIndex);
			shaderInfo.shaderKinds |= kind;
		}
		++filesFound;
	}

	// we need to validate references so collect refs in second pass
	int refIdx = 0;
	for (const KVSection& itemSec : fileListSec->Keys("ref"))
	{
		int vertLayoutIdx = -1;
		EqStringRef kindStr;
		EqStringRef entryPointName;
		EqStringRef queryStr;
		int refBlobIdx = -1;
		if (itemSec.GetValues(vertLayoutIdx, kindStr, entryPointName, queryStr, refBlobIdx) < 5)
		{
			ASSERT_FAIL("Shader %s 'ref' does not have 5 values (old shader version?)", shaderInfoKvs.GetName());
			break;
		}

		const int kind = getKind(kindStr);

		ASSERT_MSG(kind != 0, "Shader kind is not valid");
		const int queryStrHash = StringId24(queryStr, true);
		const int entryPointStrHash = StringId24(entryPointName);
		const uint shaderModuleId = ShaderInfo::PackShaderModuleId(queryStrHash, vertLayoutIdx, kind, entryPointStrHash);
		ASSERT_MSG(shaderInfo.modules[refBlobIdx].kind == static_cast<EShaderKind>(kind), "%s ref %d (%s-%s) points to invalid shader kind", shaderInfo.shaderName.ToCString(), refBlobIdx, kindStr.ToCString(), queryStr.ToCString());

		auto exIt = shaderInfo.modulesMap.find(shaderModuleId);
		if (!exIt.atEnd())
		{
			ASSERT_FAIL("%s %s-%s module reference already added at idx %d (check for hash collisions)", shaderInfo.shaderName.ToCString(), kindStr, queryStr, exIt.value());
		}

		shaderInfo.modulesMap.insert(shaderModuleId, refBlobIdx);
		++refIdx;
	}
	return true;
}