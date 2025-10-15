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
	static bool		ParseShaderInfo(ShaderInfo& shaderInfo, IPackFileReaderPtr shaderPackFile, const KVSection& shaderInfoKvs, int& filesFound);
	void			ParseModuleBindings(const KVSection& bindingsSec, uint shaderModuleId, Module& moduleInfo, Map<uint, int>& usedBindingSlots);
	void			ParseVertexAttribs(const KVSection& vertexSec, uint shaderModuleId, Module& moduleInfo, Map<uint, int>& usedVertexAttribs);

	ShaderInfo() = default;
	ShaderInfo(ShaderInfo&& other) noexcept;
	ShaderInfo& operator=(ShaderInfo&& other) noexcept;

	bool			GetShaderQueryHash(ArrayCRef<EqString> findDefines, int& outHash) const;
	EqStringRef		GetShaderQueryStr(ArrayCRef<EqString> findDefines) const;

	ArrayCRef<int>	GetBindingIds(const ShaderInfo::Module& module) const;
	ArrayCRef<int>	GetVertexAttribIds(const ShaderInfo::Module& module) const;

	struct VertLayout
	{
		EqString	name;
		int			nameHash{ 0 };
		int			aliasOf{ -1 };
	};

	struct VertexAttrib
	{
#ifdef DEBUG_SHADER_BINDINGS
		EqString	name;
#endif
		EqString	semantic;
		int			nameId{ 0 };
		int			location{ -1 };
	};

	struct Binding
	{
#ifdef DEBUG_SHADER_BINDINGS
		EqString			name;
#endif
		int					nameId{ 0 };
		EBindEntryType		type{ BINDENTRY_BUFFER };
		int					rwFlags{ RWFLAG_UNIFORM };

		int					descriptorSetIdx{ -1 };
		int					index{ -1 };
		EBindingRangeType	rangeType{};
		int					registerIdx{ 0 };
	};

	struct Module
	{
		BitArray			usedBindings{ PP_SL };
		void*				rhiModule{ nullptr };
		EqString			entryPoint;
		int					fileIndex[SHADERMODULE_TYPES]{ -1 };
		EShaderKind			kind{ };
		int					bindingsStart{ -1 };
		int					vertexAttribsStart{ -1 };
		uint				id{ 0 };
	};

	Array<Binding>			bindings{ PP_SL };
	Array<int>				bindingIds{ PP_SL };		// binding ids per module, start by Module::bindingsStart storing count, rest is ids

	Array<VertexAttrib>		vertexAttribs{ PP_SL };
	Array<int>				vertexAttribIds{ PP_SL };	// vertex attrib ids per module, start by Module::vertexAttribsStart storing count, rest is ids

	EqString				shaderName;
	IPackFileReaderPtr		shaderPackFile{ nullptr };
	Array<VertLayout>		vertexLayouts{ PP_SL };
	Array<EqString>			defines{ PP_SL };
	Array<Module>			modules{ PP_SL };
	Map<uint, int>			modulesMap{ PP_SL };
	int						shaderKinds{ 0 };
};