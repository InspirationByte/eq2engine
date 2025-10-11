#include <slang.h>
#include <slang-com-helper.h>

#include "core/core_common.h"
#include "core/IFileSystem.h"

#include "ShaderIncluder.h"
#include "GLSLBoilerplate.h"
#include "HLSLBoilerplate.h"
#include "SlangBoilerplate.h"

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

		// append boilerplate
		if (m_shaderInfo.sourceType == SHADERSOURCE_SLANG)
			result->includeContent.Print(s_boilerPlateStrSlang);	// TODO
		else if (m_shaderInfo.sourceType == SHADERSOURCE_GLSL)
			result->includeContent.Print(s_boilerPlateStrGLSL);
		else if (m_shaderInfo.sourceType == SHADERSOURCE_HLSL)
			result->includeContent.Print(s_boilerPlateStrHLSL);

		result->includeName = fileName;
		result->isError = false;
	}
	else if (!CString::Compare(fileName, "VertexLayout"))
	{
		const EqString shaderSourceName = fnmPathCombine("VertexLayouts", m_vertexLayoutName + ".h");
		result = TryOpenIncludeFile(sourcePath, shaderSourceName);
		result->includeName = shaderSourceName;
	}
	else
	{
		result = TryOpenIncludeFile(isRelativePath ? sourcePath : "", fileName);
	}

	return result;
}

ShaderIncluderImpl::IncludeResult* ShaderIncluderImpl::TryOpenIncludeFile(const char* sourcePath, const char* fileName)
{
	IFileStreamPtr openFile = nullptr;

	// find already loaded file buffer
	{
		EqString fullPath;
		for (const EqString& incPath : m_includePaths)
		{
			{
				fullPath = fnmPathCombine(incPath, sourcePath, fileName);
				const int strId = StringId(fullPath);
				auto foundIt = m_shaderIncludes.find(strId);
				if (foundIt && !foundIt->isError)
				{
					++foundIt->includeCount;
					return &(*foundIt);
				}
			}
			{
				fullPath = fnmPathCombine(incPath, fileName);
				const int strId = StringId(fullPath);
				auto foundIt = m_shaderIncludes.find(strId);
				if (foundIt && !foundIt->isError)
				{
					++foundIt->includeCount;
					return &(*foundIt);
				}
			}
		}

		if (!openFile)
		{
			fullPath = fnmPathCombine(sourcePath, fileName);
			const int strId = StringId(fullPath);
			auto foundIt = m_shaderIncludes.find(strId);
			if (foundIt && !foundIt->isError)
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
			{
				fullPath = fnmPathCombine(incPath, sourcePath, fileName);
				openFile = g_fileSystem->Open(fullPath, FS_OPEN_READ, SP_ROOT);
				if (openFile)
					break;
			}
			{
				fullPath = fnmPathCombine(incPath, fileName);
				openFile = g_fileSystem->Open(fullPath, FS_OPEN_READ, SP_ROOT);
				if (openFile)
					break;
			}
		}

		if (!openFile)
		{
			fullPath = fnmPathCombine(sourcePath, fileName);
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
		result.includeContent.Print("\n");

		const char _zero = 0;
		result.includeContent.WriteObj(_zero);
		result.includeContent.Seek(-1, FS_SEEK_CUR);
		result.isError = false;
		return &result;
	}

	result.includeContent.Open(FS_OPEN_READ | FS_OPEN_WRITE);
	result.includeContent.Print("Could not open %s", fileName);

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

SlangFileSystemIncluder::SlangFileSystemIncluder(ShaderInfo& shaderInfo, ArrayCRef<EqString> includePaths)
	: ShaderIncluderImpl(shaderInfo, includePaths)
{
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

SLANG_NO_THROW SlangResult SLANG_MCALL SlangFileSystemIncluder::loadFile(char const* path, ISlangBlob** outBlob)
{
	EqString mbcFilename = path;
	fnmPathFixSeparators(mbcFilename);
	mbcFilename = mbcFilename.TrimChar(_CORRECT_PATH_SEPARATOR_STR ".");

	EqStringRef basePath = fnmPathStripName(mbcFilename);
	EqString fileName = fnmRemoveCommonPath(basePath, mbcFilename);

	IncludeResult* result = ShaderIncluderImpl::GetInclude(fileName, true, basePath);
	if (result->isError)
	{
		ReleaseInclude(result);
		return SLANG_E_NOT_FOUND;
	}

	ISlangBlob* blob = slang_createBlob(result->includeContent.GetBasePointer(), result->includeContent.Tell());
	if (!blob)
	{
		ReleaseInclude(result);
		return SLANG_E_NOT_FOUND;
	}

	*outBlob = blob;

	return SLANG_OK;
}
