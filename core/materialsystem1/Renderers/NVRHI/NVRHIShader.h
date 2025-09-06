#pragma once

class IPackFileReader;
using IPackFileReaderPtr = CRefPtr<IPackFileReader>;

enum EShaderModuleType
{
	SHADERMODULE_SPIRV,
};

struct ShaderInfoNVRHIImpl
{
	~ShaderInfoNVRHIImpl();

	ShaderInfoNVRHIImpl() = default;
	ShaderInfoNVRHIImpl(ShaderInfoNVRHIImpl&& other) noexcept;
	ShaderInfoNVRHIImpl& operator=(ShaderInfoNVRHIImpl&& other) noexcept;

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
		nvrhi::ShaderHandle	rhiModule{ nullptr };
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

	IGPUPipelineLayoutPtr	pipelineLayout;
	Array<VertLayout>		vertexLayouts{ PP_SL };
	Array<EqString>			defines{ PP_SL };
	Array<Module>			modules{ PP_SL };
	Map<uint, int>			modulesMap{ PP_SL };
	int						shaderKinds{ 0 };
};