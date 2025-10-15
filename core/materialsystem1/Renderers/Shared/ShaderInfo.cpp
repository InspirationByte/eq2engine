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

#ifdef DEBUG_SHADER_BINDINGS
		binding.name = bindingSec.GetName();
#endif

		binding.type = GetBindingTypeByName(typeName);
		binding.nameId = StringId24(bindingSec.GetName());
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

		uint bindingId = binding.nameId;
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
			ASSERT_MSG(foundBinding.nameId == binding.nameId, "bindingId hash collision");
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

		bindingIds.append(idx);
	}
}


void ShaderInfo::ParseVertexAttribs(const KVSection& vertexSec, uint shaderModuleId, Module& moduleInfo, Map<uint, int>& usedVertexAttribs)
{
	const int attribCount = vertexSec.KeyCount();
	if (attribCount == 0)
		return;
	moduleInfo.vertexAttribsStart = vertexAttribIds.numElem();

	// store attrib count
	vertexAttribIds.reserve(vertexAttribIds.numElem() + attribCount + 1);
	vertexAttribIds.append(attribCount);

	vertexAttribs.reserve(vertexAttribs.numElem() + attribCount);
	for (const KVSection& attribSec : vertexSec.Keys())
	{
		VertexAttrib attrib;

		attribSec.GetValues(attrib.location, attrib.semantic);
#ifdef DEBUG_SHADER_BINDINGS
		attrib.name = attribSec.GetName();
#endif
		attrib.nameId = StringId24(attribSec.GetName());

		uint attribId = attrib.nameId | (attrib.location << 24);
		attribId *= 31;
		attribId += StringId24(attrib.semantic);

		auto it = usedVertexAttribs.find(attribId);
		int idx = -1;
		if (!it.atEnd())
		{
			idx = *it;
			const VertexAttrib& foundAttrib = vertexAttribs[idx];
			ASSERT_MSG(foundAttrib.nameId == attrib.nameId, "attribId hash collision");
			ASSERT_MSG(foundAttrib.semantic == attrib.semantic, "attribId hash collision");
			ASSERT_MSG(foundAttrib.location == attrib.location, "attribId hash collision");
		}
		else
		{
			idx = vertexAttribs.append(attrib);
			usedVertexAttribs.insert(attribId, idx);
		}

		vertexAttribIds.append(idx);
	}
}

ArrayCRef<int> ShaderInfo::GetBindingIds(const ShaderInfo::Module& module) const
{
	if (module.bindingsStart < 0)
		return nullptr;
	return ArrayCRef(&bindingIds[module.bindingsStart + 1], bindingIds[module.bindingsStart]);
}

ArrayCRef<int> ShaderInfo::GetVertexAttribIds(const ShaderInfo::Module& module) const
{
	if (module.vertexAttribsStart < 0)
		return nullptr;
	return ArrayCRef(&vertexAttribIds[module.vertexAttribsStart + 1], vertexAttribIds[module.vertexAttribsStart]);
}

bool ShaderInfo::ParseShaderInfo(ShaderInfo& shaderInfo, IPackFileReaderPtr shaderPackFile, const KVSection& shaderInfoKvs, int& filesFound)
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
	Map<uint, int> usedVertexAttribs(PP_SL);

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
			modInfo.id = shaderModuleId;

			shaderInfo.ParseModuleBindings(itemSec.Get("Bindings"), shaderModuleId, modInfo, usedBindingSlots);
			if(kind == SHADERKIND_VERTEX)
				shaderInfo.ParseVertexAttribs(itemSec.Get("Vertex"), shaderModuleId, modInfo, usedVertexAttribs);
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