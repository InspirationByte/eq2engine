#include <shaderc/shaderc.hpp>

#include "core/core_common.h"
#include "core/IFileSystem.h"

#include "ShaderIncluder.h"
#include "GLSLBoilerplate.h"

EqShaderIncluder::EqShaderIncluder(ShaderInfo& shaderInfo, ArrayCRef<EqString> includePaths)
	: m_shaderInfo(shaderInfo), m_includePaths(includePaths)
{
}

shaderc_include_result* EqShaderIncluder::GetInclude(
	const char* requested_source, shaderc_include_type type,
	const char* requesting_source, size_t include_depth)
{
	const EqString sourcePath = fnmPathExtractPath(requesting_source);

	IncludeResult* result = nullptr;
	if (m_freeSlots.numElem())
		result = &m_shaderIncludes[m_freeSlots.popBack()];
	else
		result = &m_shaderIncludes.append();

	if (type == shaderc_include_type_relative)
	{
		if (!TryOpenIncludeFile(sourcePath, requested_source, result))
			return &result->resultData;
	}
	else
	{
		if (!CString::Compare(requested_source, "ShaderCooker"))
		{
			result->includeContent.Open(nullptr, VS_OPEN_READ | VS_OPEN_WRITE, 8192);
			result->includeContent.Print(s_boilerPlateStrGLSL);

			// also add vertex layout defines
			for (int i = 0; i < m_shaderInfo.vertexLayouts.numElem(); ++i)
			{
				const ShaderInfo::VertLayout& layout = m_shaderInfo.vertexLayouts[i];
				const int vertexId = layout.aliasOf != -1 ? layout.aliasOf : i;
				result->includeContent.Print("\n#define VID_%s %d\n", layout.name.ToCString(), vertexId);
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
			result->includeContent.Print("\n#define CURRENT_VERTEX_ID %d\n", vertexId);

			result->includeName = requested_source;
		}
		else if (!CString::Compare(requested_source, "VertexLayout"))
		{
			EqString shaderSourceName;
			fnmPathCombine(shaderSourceName, "VertexLayouts", m_vertexLayoutName + ".h");

			if (!TryOpenIncludeFile(sourcePath, shaderSourceName, result))
				return &result->resultData;

			result->includeName = shaderSourceName;
		}
	}
	result->resultData.content = (const char*)result->includeContent.GetBasePointer();
	result->resultData.content_length = result->includeContent.GetSize();
	result->resultData.source_name = result->includeName;
	result->resultData.source_name_length = result->includeName.Length();
	return &result->resultData;
}

bool EqShaderIncluder::TryOpenIncludeFile(const char* reqSource, const char* fileName, IncludeResult* result)
{
	IFilePtr openFile = nullptr;

	EqString fullPath;
	for (const EqString& incPath : m_includePaths)
	{
		fnmPathCombine(fullPath, incPath, fileName);
		openFile = g_fileSystem->Open(fullPath, "r", SP_ROOT);
		if (openFile)
			break;
	}

	if (!openFile)
	{
		fnmPathCombine(fullPath, reqSource, fileName);
		openFile = g_fileSystem->Open(fullPath, "r", SP_ROOT);
	}

	if (!openFile)
	{
		result->includeContent.Open(nullptr, VS_OPEN_READ | VS_OPEN_WRITE, 8192);
		result->includeContent.Print("Could not open %s", reqSource);
		result->resultData.content = (const char*)result->includeContent.GetBasePointer();
		result->resultData.content_length = result->includeContent.GetSize();
		// leave source_name and source_name_length empty
		return false;
	}

	result->includeName = fullPath;
	result->includeContent.Open(nullptr, VS_OPEN_READ | VS_OPEN_WRITE, openFile->GetSize());
	result->includeContent.AppendStream(openFile);
	return true;
}

// Handles shaderc_include_result_release_fn callbacks.
void EqShaderIncluder::ReleaseInclude(shaderc_include_result* data)
{
	IncludeResult* incRes = reinterpret_cast<IncludeResult*>(data);
	const int index = incRes - m_shaderIncludes.ptr();
	memset(data, 0, sizeof(*data));

	incRes->includeName.Clear();
	incRes->includeContent.Close();

	m_freeSlots.append(index);
}

void EqShaderIncluder::SetVertexLayout(const char* vertexLayoutName)
{
	m_vertexLayoutName = vertexLayoutName;
}