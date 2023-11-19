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
	const EqString sourcePath = EqString(requesting_source).Path_Extract_Path();

	IncludeResult* result = nullptr;
	if (m_freeSlots.numElem())
		result = &m_shaderIncludes[m_freeSlots.popBack()];
	else
		result = &m_shaderIncludes.append();

	if (type == shaderc_include_type_relative)
	{
		IFilePtr file = TryOpenIncludeFile(sourcePath, requested_source);
		if (!file)
		{
			result->includeContent.Open(nullptr, VS_OPEN_READ | VS_OPEN_WRITE, 8192);
			result->includeContent.Print("Could not open %s", requested_source);
			result->resultData.content = (const char*)result->includeContent.GetBasePointer();
			result->resultData.content_length = result->includeContent.GetSize();
			return &result->resultData;
		}
		result->includeName = requested_source;
		result->includeContent.Open(nullptr, VS_OPEN_READ | VS_OPEN_WRITE, file->GetSize());
		result->includeContent.AppendStream(file);
	}
	else
	{
		result->includeContent.Close();
		result->includeContent.Open(nullptr, VS_OPEN_READ | VS_OPEN_WRITE, 8192);

		if (!strcmp(requested_source, "ShaderCooker"))
		{
			result->includeContent.Print(s_boilerPlateStrGLSL);

			// also add vertex layout defines
			for (int i = 0; i < m_shaderInfo.vertexLayouts.numElem(); ++i)
			{
				const ShaderInfo::VertLayout& layout = m_shaderInfo.vertexLayouts[i];
				const int vertexId = layout.aliasOf != -1 ? layout.aliasOf : i;
				result->includeContent.Print("\n#define VID_%s %d\n", layout.name.ToCString(), vertexId);
			}

			result->includeName = requested_source;
		}
		else if (!strcmp(requested_source, "VertexLayout"))
		{
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

			EqString shaderSourceName;
			CombinePath(shaderSourceName, "VertexLayouts", m_vertexLayoutName + ".h");

			IFilePtr file = TryOpenIncludeFile(sourcePath, shaderSourceName);
			if (!file)
			{
				result->includeContent.Print("Cannot open %s", shaderSourceName.ToCString());
				result->resultData.content = (const char*)result->includeContent.GetBasePointer();
				result->resultData.content_length = result->includeContent.GetSize();
				return &result->resultData;
			}

			result->includeName = shaderSourceName;
			result->includeContent.Close();
			result->includeContent.Open(nullptr, VS_OPEN_READ | VS_OPEN_WRITE, file->GetSize());
			result->includeContent.Print("\n#define CURRENT_VERTEX_ID %d\n", vertexId);
			result->includeContent.AppendStream(file);
		}
	}
	result->resultData.content = (const char*)result->includeContent.GetBasePointer();
	result->resultData.content_length = result->includeContent.GetSize();
	result->resultData.source_name = result->includeName;
	result->resultData.source_name_length = result->includeName.Length();
	return &result->resultData;
}

IFilePtr EqShaderIncluder::TryOpenIncludeFile(const char* reqSource, const char* fileName)
{
	EqString fullPath;
	for (const EqString& incPath : m_includePaths)
	{
		CombinePath(fullPath, incPath, fileName);
		IFilePtr file = g_fileSystem->Open(fullPath, "r", SP_ROOT);
		if (file)
			return file;
	}

	CombinePath(fullPath, reqSource, fileName);
	return g_fileSystem->Open(fullPath, "r", SP_ROOT);
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