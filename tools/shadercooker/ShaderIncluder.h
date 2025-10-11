#pragma once
#include "ShaderInfo.h"

class ShaderIncluderImpl
{
public:
	struct IncludeResult
	{
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
