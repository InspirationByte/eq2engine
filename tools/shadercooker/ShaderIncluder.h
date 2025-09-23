#pragma once
#include "ShaderInfo.h"

class ShaderIncluderImpl
{
public:
	struct IncludeResult
	{
		shaderc_include_result	resultData;
		EqString				includeName;
		CMemoryStream			includeContent{ PP_SL };
		int						includeCount{ 0 };
		bool					isError{ true };
	};

	ShaderIncluderImpl(ShaderInfo& shaderInfo, ArrayCRef<EqString> includePaths);

	IncludeResult*			GetInclude(const char* fileName, bool isRelativePath, const char* includeFromName);
	void					ReleaseInclude(IncludeResult* data);

	IncludeResult*			TryOpenIncludeFile(const char* reqSource, const char* fileName);
	void					SetVertexLayout(const char* vertexLayoutName) { m_vertexLayoutName = vertexLayoutName; }

protected:
	Map<uint, IncludeResult>	m_shaderIncludes{ PP_SL };
	ArrayCRef<EqString>			m_includePaths;
	const ShaderInfo&			m_shaderInfo;
	EqString					m_vertexLayoutName;
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

class SlangFileSystemIncluder
	: public ShaderIncluderImpl
	, public ISlangFileSystem
{
public:
	SlangFileSystemIncluder(ShaderInfo& shaderInfo, ArrayCRef<EqString> includePaths);

	SLANG_NO_THROW SlangResult SLANG_MCALL loadFile(char const* path, ISlangBlob** outBlob) override;

	SLANG_FORCE_INLINE ISlangUnknown* getInterface(const Slang::Guid& guid)
	{
		if (guid == ISlangUnknown::getTypeGuid() ||
			guid == ISlangFileSystem::getTypeGuid())
		{
			return static_cast<ISlangFileSystem*>(this);
		}
		if (guid == ISlangCastable::getTypeGuid())
		{
			return static_cast<ISlangCastable*>(this);
		}

		return nullptr;
	}

	SLANG_FORCE_INLINE SLANG_NO_THROW void* SLANG_MCALL castAs(const SlangUUID& guid)
	{
		if (auto intf = getInterface(guid))
		{
			return intf;
		}
		return nullptr;
	}

	SLANG_IUNKNOWN_ALL
	int m_refCount = 0;
};


#if 0
class ShaderDXCIncluder
	: public ShaderIncluderImpl
	, public IDxcIncludeHandler
{
public:
	ShaderDXCIncluder(EqStringRef shaderSourceFullName, ComPtr<IDxcUtils> utils, ShaderInfo& shaderInfo, ArrayCRef<EqString> includePaths);

	ULONG STDMETHODCALLTYPE AddRef() override;
	ULONG STDMETHODCALLTYPE Release() override;

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override;

	HRESULT STDMETHODCALLTYPE LoadSource(LPCWSTR pFilename, IDxcBlob** ppIncludeSource) override;

protected:
	ComPtr<IDxcUtils>	m_dxcUtils;
	EqStringRef			m_shaderSourceFullName;
};
#endif // _WIN32