#pragma once

#include "renderers/ShaderAPI_defs.h"
#include "core/IPackFileReader.h"

enum EShaderModuleType
{
	SHADERMODULE_SPIRV,		// Vulkan & WGPU
	SHADERMODULE_DXBC,		// D3D10+
	SHADERMODULE_DXIL,		// D3D12
	SHADERMODULE_WGSL,		// WGPU only

	SHADERMODULE_TYPES,
};

enum ERWFlags : int
{
	RWFLAG_UNIFORM	= (1 << 0),
	RWFLAG_READ		= (1 << 1),
	RWFLAG_WRITE	= (1 << 2),
};

enum EBindingRangeType : int
{
	BINDING_RANGE_SRV = 0,
	BINDING_RANGE_UAV,
	BINDING_RANGE_CBV,
	BINDING_RANGE_SAMPLER
};

static const char* s_shaderModuleTypeExt[] = {
	".spv",
	".dxbc",
	".dxil",
	".wgsl",
};
static_assert(SHADERMODULE_TYPES == elementsOf(s_shaderModuleTypeExt), "BINDENTRY_TYPES doesn't match SHADERMODULE_TYPES count");

struct ShaderInfo
{
	struct Binding;
	struct Module;

	static uint		PackShaderModuleId(int queryStrHash, int vertexLayoutIdx, int kind, int entryPointStrHash);
	static bool		ParseShaderInfo(ShaderInfo& shaderInfo, IPackFileReaderPtr shaderPackFile, const KVSection& shaderInfoKvs, int& filesFound, bool parseBindings = true);
	void			ParseModuleBindings(const KVSection& bindingsSec, uint shaderModuleId, Module& moduleInfo, Map<uint, int>& usedBindingSlots);

	static uint		MakeBindingId(int descriptorSetIdx, int index);
	static void		UnpackBindingId(uint bindingIdx, int& descriptorSetIdx, int& index);
	static uint64	MakeShaderBindingId(uint shaderModuleId, uint bindingIdx);
	static uint64	MakeShaderBindingId(uint shaderModuleId, int descriptorSetIdx, int index);

	ShaderInfo() = default;
	ShaderInfo(ShaderInfo&& other) noexcept;
	ShaderInfo& operator=(ShaderInfo&& other) noexcept;

	bool			GetShaderQueryHash(ArrayCRef<EqString> findDefines, int& outHash) const;
	EqStringRef		GetShaderQueryStr(ArrayCRef<EqString> findDefines) const;

	ArrayCRef<uint>	GetBindingIds(const ShaderInfo::Module& module) const;

	struct VertLayout
	{
		EqString	name;
		int			nameHash{ 0 };
		int			aliasOf{ -1 };
	};

	struct Binding
	{
		int					nameId{ -1 };
		EBindEntryType		type{ BINDENTRY_BUFFER };
		int					rwFlags{ RWFLAG_UNIFORM };

		int					descriptorSetIdx{ -1 };
		int					index{ -1 };
		EBindingRangeType	rangeType{};
		int					registerIdx{ 0 };
	};

	struct Module
	{
		void*				rhiModule{ nullptr };
		EqString			entryPoint;
		int					fileIndex[SHADERMODULE_TYPES]{ -1 };
		EShaderKind			kind{ };
		int					bindingsStart{ 0 };
	};

	Array<Binding>			bindings{ PP_SL };
	Map<uint64, int>		bindingMap{ PP_SL };		// kind:descriptorSetIdx:index to bindings via MakeBindingIdx
	Array<uint>				bindingIds{ PP_SL };		// binding ids per module, start by Module::bindingsStart storing count, rest is ids

	EqString				shaderName;
	IPackFileReaderPtr		shaderPackFile{ nullptr };
	Array<VertLayout>		vertexLayouts{ PP_SL };
	Array<EqString>			defines{ PP_SL };
	Array<Module>			modules{ PP_SL };
	Map<uint, int>			modulesMap{ PP_SL };
	int						shaderKinds{ 0 };
};