#pragma once

#include "ShaderInfo.h"

// includer used for shaderc
class ShadercIncluder: public shaderc::CompileOptions::IncluderInterface
{
public:
	ShadercIncluder(ShaderInfo& shaderInfo, ArrayCRef<EqString> includePaths);

	shaderc_include_result* GetInclude(const char* requested_source, shaderc_include_type type, const char* requesting_source, size_t include_depth);
	void ReleaseInclude(shaderc_include_result* data);

	void SetVertexLayout(const char* vertexLayoutName);

private:
	struct IncludeResult
	{
		shaderc_include_result	resultData;
		EqString				includeName;
		CMemoryStream			includeContent{ PP_SL };
	};

	bool TryOpenIncludeFile(const char* reqSource, const char* fileName, IncludeResult* result);

	Array<IncludeResult>	m_shaderIncludes{ PP_SL };
	Array<int>				m_freeSlots{ PP_SL };
	ArrayCRef<EqString>		m_includePaths;
	const ShaderInfo&		m_shaderInfo;
	EqString				m_vertexLayoutName;
};