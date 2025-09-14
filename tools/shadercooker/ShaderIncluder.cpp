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

ShaderIncluderImpl::IncludeResult* ShaderIncluderImpl::GetInclude(const char* fileName, bool isRelativePath, const char* includeFromName)
{
	const EqString sourcePath = fnmPathExtractPath(includeFromName);

	IncludeResult* result = nullptr;
	if (m_freeSlots.numElem())
		result = &m_shaderIncludes[m_freeSlots.popBack()];
	else
		result = &m_shaderIncludes.append();

	if (isRelativePath)
	{
		if (!TryOpenIncludeFile(sourcePath, fileName, result))
			return result;
	}
	else if (!CString::Compare(fileName, "ShaderCooker"))
	{
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
	}
	else if (!CString::Compare(fileName, "VertexLayout"))
	{
		const EqString shaderSourceName = fnmPathCombine("VertexLayouts", m_vertexLayoutName + ".h");
		if (!TryOpenIncludeFile(sourcePath, shaderSourceName, result))
			return result;

		result->includeName = shaderSourceName;
	}

	result->resultData.content = (const char*)result->includeContent.GetBasePointer();
	result->resultData.content_length = result->includeContent.Tell();
	result->resultData.source_name = result->includeName;
	result->resultData.source_name_length = result->includeName.Length();
	return result;
}

bool ShaderIncluderImpl::TryOpenIncludeFile(const char* reqSource, const char* fileName, IncludeResult* result)
{
	IFileStreamPtr openFile = nullptr;

	EqString fullPath;
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

	if (!openFile)
	{
		result->includeContent.Open(FS_OPEN_READ | FS_OPEN_WRITE);
		result->includeContent.Print("Could not open %s", fileName);
		result->resultData.content = (const char*)result->includeContent.GetBasePointer();
		result->resultData.content_length = result->includeContent.GetSize();
		// leave source_name and source_name_length empty
		return false;
	}

	result->includeName = fullPath;
	result->includeContent.Open(FS_OPEN_READ | FS_OPEN_WRITE);
	result->includeContent.AppendStream(openFile);

	const char _zero = 0;
	result->includeContent.WriteObj(_zero);
	result->includeContent.Seek(-1, FS_SEEK_CUR);

	return true;
}

void ShaderIncluderImpl::ReleaseInclude(IncludeResult* data)
{
	if (!data)
		return;

	const int index = data - m_shaderIncludes.ptr();
	memset(&data->resultData, 0, sizeof(data->resultData));

	data->includeName.Clear();
	data->includeContent.Close();

	m_freeSlots.append(index);
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
	IncludeResult* result = ShaderIncluderImpl::GetInclude(requested_source, type == shaderc_include_type_relative, requesting_source);

	return result ? &result->resultData : nullptr;
}

//----------------------------------------------
#ifdef _WIN32

ShaderDXCIncluder::ShaderDXCIncluder(EqStringRef shaderSourceFullName, IDxcUtils* utils, ShaderInfo& shaderInfo, ArrayCRef<EqString> includePaths)
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

HRESULT STDMETHODCALLTYPE ShaderDXCIncluder::LoadSource(LPCWSTR pFilename, IDxcBlob** ppIncludeSource)
{
	EqString mbcFilename;
	AnsiUnicodeConverter(mbcFilename, pFilename);
	fnmPathFixSeparators(mbcFilename);
	mbcFilename = mbcFilename.TrimChar(_CORRECT_PATH_SEPARATOR_STR ".");
	const bool isRelativePath = m_shaderSourceFullName.Find(fnmPathStripName(mbcFilename)) == -1;

	IncludeResult* result = ShaderIncluderImpl::GetInclude(isRelativePath ? mbcFilename : fnmPathStripPath(mbcFilename), isRelativePath, mbcFilename);
	if (!result)
		return E_FAIL;

	if (result->resultData.source_name == nullptr)
	{
		ReleaseInclude(result);
		return E_FAIL;
	}

	IDxcBlobEncoding* textBlob;
	if (FAILED(m_dxcUtils->CreateBlobFromPinned(result->includeContent.GetBasePointer(), result->includeContent.GetSize(), DXC_CP_UTF8, &textBlob)))
	{
		ReleaseInclude(result);
		return E_FAIL;
	}

	*ppIncludeSource = textBlob;
	//ReleaseInclude(result);

	return S_OK;
}

#endif // _WIN32