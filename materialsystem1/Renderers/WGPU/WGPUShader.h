#pragma once

class IPackFileReader;
using IPackFileReaderPtr = CRefPtr<IPackFileReader>;

struct ShaderInfoWGPUImpl
{
	~ShaderInfoWGPUImpl();

	ShaderInfoWGPUImpl() = default;
	ShaderInfoWGPUImpl(ShaderInfoWGPUImpl&& other) noexcept;
	ShaderInfoWGPUImpl& operator=(ShaderInfoWGPUImpl&& other) noexcept;

	void Release();
	bool GetShaderQueryHash(ArrayCRef<EqString> findDefines, int& outHash) const;

	struct VertLayout
	{
		EqString	name;
		int			nameHash{ 0 };
		int			aliasOf{ -1 };
	};

	struct Module
	{
		WGPUShaderModule	rhiModule{ nullptr };
		EShaderKind			kind;
		int					fileIndex{ -1 };
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
	Map<int, EntryPoint>	entryPoints{ PP_SL };
	Map<uint, int>			modulesMap{ PP_SL };
	int						shaderKinds{ 0 };
};