#pragma once

#ifdef _WIN32
#include <d3dcompiler.h> // FXC
#include <dxcapi.h> // DXC

#include <wrl/client.h>
using Microsoft::WRL::ComPtr;
#endif

#include "ShaderInfo.h"

class ShaderIncluderImpl
{
public:
	struct IncludeResult
	{
		shaderc_include_result	resultData;
		EqString				includeName;
		CMemoryStream			includeContent{ PP_SL };
	};

	ShaderIncluderImpl(ShaderInfo& shaderInfo, ArrayCRef<EqString> includePaths);

	IncludeResult*			GetInclude(const char* fileName, bool isRelativePath, const char* includeFromName);
	void					ReleaseInclude(IncludeResult* data);

	bool					TryOpenIncludeFile(const char* reqSource, const char* fileName, IncludeResult* result);
	void					SetVertexLayout(const char* vertexLayoutName) { m_vertexLayoutName = vertexLayoutName; }

protected:
	Array<IncludeResult>	m_shaderIncludes{ PP_SL };
	Array<int>				m_freeSlots{ PP_SL };
	ArrayCRef<EqString>		m_includePaths;
	const ShaderInfo&		m_shaderInfo;
	EqString				m_vertexLayoutName;
};

// includer used for shaderc
class ShadercIncluder
	: public ShaderIncluderImpl
	, public shaderc::CompileOptions::IncluderInterface
{
public:
	ShadercIncluder(ShaderInfo& shaderInfo, ArrayCRef<EqString> includePaths);

	shaderc_include_result* GetInclude(const char* requested_source, shaderc_include_type type, const char* requesting_source, size_t include_depth) override;
	void ReleaseInclude(shaderc_include_result* data) override;
};

class ShaderDXCIncluder
	: public ShaderIncluderImpl
	, public IDxcIncludeHandler
{
public:
	ShaderDXCIncluder(EqStringRef shaderSourceFullName, IDxcLibrary* library, ShaderInfo& shaderInfo, ArrayCRef<EqString> includePaths);

	ULONG STDMETHODCALLTYPE AddRef() override;
	ULONG STDMETHODCALLTYPE Release() override;

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override;

	HRESULT STDMETHODCALLTYPE LoadSource(LPCWSTR pFilename, IDxcBlob** ppIncludeSource) override;

protected:
	IDxcLibrary*	m_dxcLibrary;
	EqStringRef		m_shaderSourceFullName;
};