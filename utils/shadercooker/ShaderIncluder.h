#pragma once

#include "ShaderInfo.h"

class EqShaderIncluder: public shaderc::CompileOptions::IncluderInterface
{
public:
	EqShaderIncluder(ShaderInfo& shaderInfo, ArrayCRef<EqString> includePaths);

	shaderc_include_result* GetInclude(const char* requested_source, shaderc_include_type type, const char* requesting_source, size_t include_depth);
	void ReleaseInclude(shaderc_include_result* data);

	void SetVertexLayout(const char* vertexLayoutName);

private:
	IFilePtr TryOpenIncludeFile(const char* reqSource, const char* fileName);

	struct IncludeResult
	{
		shaderc_include_result	resultData;
		EqString				includeName;
		CMemoryStream			includeContent{ PP_SL };
	};

	ArrayCRef<EqString>			m_includePaths;
	FixedArray<IncludeResult, 32>	m_shaderIncludes;
	FixedArray<int, 32>		m_freeSlots;
	const ShaderInfo&		m_shaderInfo;
	EqString				m_vertexLayoutName;
};