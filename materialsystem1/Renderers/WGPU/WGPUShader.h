#pragma once

struct ShaderInfoWGPUImpl
{
	~ShaderInfoWGPUImpl();

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

	EqString			shaderName;
	IFilePackageReader* shaderPackFile{ nullptr };
	Array<VertLayout>	vertexLayouts{ PP_SL };
	Array<EqString>		defines{ PP_SL };
	Array<Module>		modules{ PP_SL };
	Map<uint, int>		modulesMap{ PP_SL };
	int					shaderKinds{ 0 };
};