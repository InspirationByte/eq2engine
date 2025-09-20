#include <shaderc/shaderc.hpp>

#include "core/core_common.h"
#include "core/IFileSystem.h"

#include "ShaderIncluder.h"
#include "GLSLBoilerplate.h"
#include "HLSLBoilerplate.h"

ShaderIncluderImpl::ShaderIncluderImpl(ShaderInfo& shaderInfo, ArrayCRef<EqString> includePaths)
	: m_shaderInfo(shaderInfo)
	, m_includePaths(includePaths)
{
}

ShaderIncluderImpl::IncludeResult* ShaderIncluderImpl::GetInclude(const char* fileName, bool isRelativePath, const char* sourcePath)
{
	IncludeResult* result = nullptr;
	if (!CString::Compare(fileName, "ShaderCooker"))
	{
		const int strId = StringId("ShaderCooker");
		result = &(*m_shaderIncludes.insert(strId));

		++result->includeCount;

		result->includeContent.Open(FS_OPEN_READ | FS_OPEN_WRITE, nullptr, 8192);

		if (m_shaderInfo.sourceType == SHADERSOURCE_GLSL)
			result->includeContent.Print(s_boilerPlateStrGLSL);
		else if (m_shaderInfo.sourceType == SHADERSOURCE_HLSL)
			result->includeContent.Print(s_boilerPlateStrHLSL);

		// also add vertex layout defines
		for (int i = 0; i < m_shaderInfo.vertexLayouts.numElem(); ++i)
		{
			const ShaderInfo::VertLayout& layout = m_shaderInfo.vertexLayouts[i];
			const int vertexId = layout.aliasOf != -1 ? layout.aliasOf : i;
			result->includeContent.Print("\n#define VID_%s %d\n", layout.name.ToCString(), StringId24(m_shaderInfo.vertexLayouts[vertexId].name));
		}

		int vertexId = -1;
		for (int i = 0; i < m_shaderInfo.vertexLayouts.numElem(); ++i)
		{
			const ShaderInfo::VertLayout& layout = m_shaderInfo.vertexLayouts[i];
			if (layout.name == m_vertexLayoutName)
			{
				vertexId = i;
				break;
			}
		}
		if (vertexId != -1)
			result->includeContent.Print("\n#define CURRENT_VERTEX_ID %d\n", StringId24(m_shaderInfo.vertexLayouts[vertexId].name));

		result->includeName = fileName;
		result->isError = false;
		result->resultData.content = (const char*)result->includeContent.GetBasePointer();
		result->resultData.content_length = result->includeContent.Tell();
		result->resultData.source_name = result->includeName;
		result->resultData.source_name_length = result->includeName.Length();
	}
	else if (!CString::Compare(fileName, "VertexLayout"))
	{
		const EqString shaderSourceName = fnmPathCombine("VertexLayouts", m_vertexLayoutName + ".h");
		result = TryOpenIncludeFile(sourcePath, shaderSourceName);
		result->includeName = shaderSourceName;
	}
	else if (isRelativePath)
	{
		result = TryOpenIncludeFile(sourcePath, fileName);
	}

	return result;
}

ShaderIncluderImpl::IncludeResult* ShaderIncluderImpl::TryOpenIncludeFile(const char* reqSource, const char* fileName)
{
	IFileStreamPtr openFile = nullptr;

	// find already loaded file buffer
	{
		EqString fullPath;
		for (const EqString& incPath : m_includePaths)
		{
			fullPath = fnmPathCombine(incPath, fileName);
			const int strId = StringId(fullPath);
			auto foundIt = m_shaderIncludes.find(strId);
			if (foundIt)
			{
				++foundIt->includeCount;
				return &(*foundIt);
			}
		}

		if (!openFile)
		{
			fullPath = fnmPathCombine(reqSource, fileName);
			const int strId = StringId(fullPath);
			auto foundIt = m_shaderIncludes.find(strId);
			if (foundIt)
			{
				++foundIt->includeCount;
				return &(*foundIt);
			}
		}
	}

	// open new file
	EqString fullPath;
	{
		for (const EqString& incPath : m_includePaths)
		{
			fullPath = fnmPathCombine(incPath, fileName);
			openFile = g_fileSystem->Open(fullPath, FS_OPEN_READ, SP_ROOT);
			if (openFile)
				break;
		}

		if (!openFile)
		{
			fullPath = fnmPathCombine(reqSource, fileName);
			openFile = g_fileSystem->Open(fullPath, FS_OPEN_READ, SP_ROOT);
		}
	}

	// store result
	const int strId = StringId(fullPath);
	IncludeResult& result = *m_shaderIncludes.insert(strId);
	++result.includeCount;
	if (openFile)
	{
		result.includeName = fullPath;
		result.includeContent.Open(FS_OPEN_READ | FS_OPEN_WRITE);
		result.includeContent.AppendStream(openFile);

		const char _zero = 0;
		result.includeContent.WriteObj(_zero);
		result.includeContent.Seek(-1, FS_SEEK_CUR);
		result.isError = false;

		result.resultData.content = (const char*)result.includeContent.GetBasePointer();
		result.resultData.content_length = result.includeContent.Tell();
		result.resultData.source_name = result.includeName;
		result.resultData.source_name_length = result.includeName.Length();
		return &result;
	}

	result.includeContent.Open(FS_OPEN_READ | FS_OPEN_WRITE);
	result.includeContent.Print("Could not open %s", fileName);
	result.resultData.content = (const char*)result.includeContent.GetBasePointer();
	result.resultData.content_length = result.includeContent.GetSize();

	// leave source_name and source_name_length empty
	return &result;
}

void ShaderIncluderImpl::ReleaseInclude(IncludeResult* data)
{
	if (!data)
		return;

	for (auto it = m_shaderIncludes.begin(); it; ++it)
	{
		if (&it.value() == data)
		{
			--(*it).includeCount;
			if ((*it).includeCount == 0)
				m_shaderIncludes.remove(it);
			return;
		}
	}
}

//----------------------------------------------

ShadercIncluder::ShadercIncluder(ShaderInfo& shaderInfo, ArrayCRef<EqString> includePaths)
	: ShaderIncluderImpl(shaderInfo, includePaths)
{
}

void ShadercIncluder::ReleaseInclude(shaderc_include_result* data)
{
	IncludeResult* incRes = reinterpret_cast<IncludeResult*>(data);
	ShaderIncluderImpl::ReleaseInclude(incRes);
}

shaderc_include_result* ShadercIncluder::GetInclude(const char* requested_source, shaderc_include_type type, const char* requesting_source, size_t include_depth)
{
	IncludeResult* result = ShaderIncluderImpl::GetInclude(requested_source, type == shaderc_include_type_relative, fnmPathExtractPath(requesting_source));

	return result ? &result->resultData : nullptr;
}

//----------------------------------------------
#ifdef _WIN32

ShaderDXCIncluder::ShaderDXCIncluder(EqStringRef shaderSourceFullName, ComPtr<IDxcUtils> utils, ShaderInfo& shaderInfo, ArrayCRef<EqString> includePaths)
	: m_dxcUtils(utils)
	, m_shaderSourceFullName(shaderSourceFullName)
	, ShaderIncluderImpl(shaderInfo, includePaths)
{
}

ULONG ShaderDXCIncluder::AddRef()
{
	return 1;
}

ULONG ShaderDXCIncluder::Release()
{
	return 1;
}

HRESULT ShaderDXCIncluder::QueryInterface(REFIID riid, void** ppvObject)
{
	if (riid == __uuidof(IDxcIncludeHandler) || riid == __uuidof(IUnknown))
	{
		AddRef();
		*ppvObject = this;
		return S_OK;
	}

	*ppvObject = nullptr;
	return E_NOINTERFACE;
}

// return difference start offset
static EqStringRef fnmRemoveCommonPath(EqStringRef commonPath, EqStringRef fullPath)
{
	int len = 0;
	for (const char* a = commonPath, *b = fullPath; *a == *b && *a && *b; ++a, ++b)
		++len;

	if (len < commonPath.Length())
		return fullPath;

	return fullPath.Mid(len, fullPath.Length() - len).TrimChar(_CORRECT_PATH_SEPARATOR_STR _INCORRECT_PATH_SEPARATOR_STR);
}

HRESULT STDMETHODCALLTYPE ShaderDXCIncluder::LoadSource(LPCWSTR pFilename, IDxcBlob** ppIncludeSource)
{
	EqString mbcFilename;
	AnsiUnicodeConverter(mbcFilename, pFilename);
	fnmPathFixSeparators(mbcFilename);
	mbcFilename = mbcFilename.TrimChar(_CORRECT_PATH_SEPARATOR_STR ".");

	EqStringRef basePath = fnmPathStripName(mbcFilename);
	EqString fileName = fnmRemoveCommonPath(basePath, mbcFilename);

	IncludeResult* result = ShaderIncluderImpl::GetInclude(fileName, true, basePath);
	if (result->resultData.source_name == nullptr)
	{
		ReleaseInclude(result);
		return E_FAIL;
	}

	ComPtr<IDxcBlobEncoding> pEncoding;
	if (result->includeCount > 1)
	{
		// Return empty string blob if this file has been included before
		static const char nullStr[] = " ";
		m_dxcUtils->CreateBlobFromPinned(nullStr, ARRAYSIZE(nullStr), DXC_CP_ACP, pEncoding.GetAddressOf());
		*ppIncludeSource = pEncoding.Detach();
		return S_OK;
	}

	if (FAILED(m_dxcUtils->CreateBlobFromPinned(result->includeContent.GetBasePointer(), result->includeContent.GetSize(), DXC_CP_UTF8, pEncoding.GetAddressOf())))
	{
		ReleaseInclude(result);
		return E_FAIL;
	}

	*ppIncludeSource = pEncoding.Detach();

	return S_OK;
}

#endif // _WIN32