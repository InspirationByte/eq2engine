#pragma once

#include "renderers/IShaderAPI.h"
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

struct ShaderInfo
{
	static uint PackShaderModuleId(int queryStrHash, int vertexLayoutIdx, int kind, int entryPointStrHash);
	static bool ParseShaderInfo(ShaderInfo& shaderInfo, IPackFileReaderPtr shaderPackFile, const KVSection& shaderInfoKvs, int& filesFound);

	ShaderInfo() = default;
	ShaderInfo(ShaderInfo&& other) noexcept;
	ShaderInfo& operator=(ShaderInfo&& other) noexcept;

	bool GetShaderQueryHash(ArrayCRef<EqString> findDefines, int& outHash) const;
	EqStringRef GetShaderQueryStr(ArrayCRef<EqString> findDefines) const;

	struct VertLayout
	{
		EqString	name;
		int			nameHash{ 0 };
		int			aliasOf{ -1 };
	};

	struct Binding
	{
		EqString		name{ 0 };
		int				bindGroupId{ -1 };
		int				index{ 0 };
		int				rwFlags{ RWFLAG_READ | RWFLAG_WRITE };
		EBindEntryType	type{};
	};

	struct Module
	{
		void*					rhiModule{ nullptr };
		EShaderKind				kind;
		EqString				entryPoint;
		int						fileIndex[SHADERMODULE_TYPES]{ -1 };
		Array<Binding>			bindings{ PP_SL };
		IGPUPipelineLayoutPtr	pipelineLayout;				// needed for NVRHI
	};

	struct EntryPoint
	{
		EqString	name;
		int			kind{ -1 };
	};

	EqString				shaderName;
	IPackFileReaderPtr		shaderPackFile{ nullptr };
	Array<VertLayout>		vertexLayouts{ PP_SL };
	Array<EqString>			defines{ PP_SL };
	Array<Module>			modules{ PP_SL };
	Map<uint, int>			modulesMap{ PP_SL };
	int						shaderKinds{ 0 };
};