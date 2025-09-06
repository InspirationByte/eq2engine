#pragma once

#include "renderers/IShaderAPI.h"
#include "core/IPackFileReader.h"

enum EShaderModuleType
{
	SHADERMODULE_SPIRV,
	SHADERMODULE_DXBC,
	SHADERMODULE_WGSL,		// WGPU only
};

struct ShaderInfo
{
	static uint PackShaderModuleId(int queryStrHash, int vertexLayoutIdx, int kind, int entryPointStrHash);
	static bool ParseShaderInfo(ShaderInfo& shaderInfo, IPackFileReaderPtr shaderPackFile, const KVSection& shaderInfoKvs, int& filesFound);

	ShaderInfo() = default;
	ShaderInfo(ShaderInfo&& other) noexcept;
	ShaderInfo& operator=(ShaderInfo&& other) noexcept;

	bool GetShaderQueryHash(ArrayCRef<EqString> findDefines, int& outHash) const;

	struct VertLayout
	{
		EqString	name;
		int			nameHash{ 0 };
		int			aliasOf{ -1 };
	};

	struct Module
	{
		void*				rhiModule{ nullptr };
		EShaderKind			kind;
		EqString			entryPoint;
		int					fileIndex{ -1 };
		EShaderModuleType	type{};
	};

	struct EntryPoint
	{
		EqString	name;
		int			kind{ -1 };
	};

	EqString				shaderName;
	IPackFileReaderPtr		shaderPackFile{ nullptr };
	IGPUPipelineLayoutPtr	pipelineLayout;				// needed for NVRHI
	Array<VertLayout>		vertexLayouts{ PP_SL };
	Array<EqString>			defines{ PP_SL };
	Array<Module>			modules{ PP_SL };
	Map<uint, int>			modulesMap{ PP_SL };
	int						shaderKinds{ 0 };
};