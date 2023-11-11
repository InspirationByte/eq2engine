#pragma once

struct ShaderInfoWGPUImpl
{
	void Release();
	bool GetShaderQueryHash(const Array<EqString>& findDefines, int& outHash) const;

	struct VertLayout
	{
		EqString	name;
		int			aliasOf{ -1 };
	};

	Array<VertLayout>		vertexLayouts{ PP_SL };
	Array<EqString>			defines{ PP_SL };
	Array<WGPUShaderModule>	rhiModules{ PP_SL };
	Map<uint, int>			modulesMap{ PP_SL };
	int						shaderKinds{ 0 };
};