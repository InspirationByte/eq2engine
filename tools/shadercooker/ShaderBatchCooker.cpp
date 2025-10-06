//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2023
//////////////////////////////////////////////////////////////////////////////////
// Description: Shader module compiler
//////////////////////////////////////////////////////////////////////////////////

#include <shaderc/shaderc.hpp>	// TODO: remove some day
#include <slang.h>
#include <slang-com-ptr.h>
#include <slang-com-helper.h>

#include "core/core_common.h"
#include "core/IFileSystem.h"
#include "core/platform/eqjobmanager.h"
#include "ds/MemoryStream.h"
#include "utils/KeyValues.h"
#include "utils/CRC32.h"
#include "utils/Tokenizer.h"
#include "dpk/DPKFileWriter.h"

#include "ShaderInfo.h"
#include "ShaderIncluder.h"

struct ShaderPackageCompileData;

/*

Targets
{
	"<TargetName>"
	{
		sourcepath 		"EqBase/shadersSRC/";
		output			"EqBase/shaders";
		sourceext		".def";
	}
}
*/

static constexpr EqStringRef s_engineDirTag("%ENGINE_DIR%");
static constexpr EqStringRef s_gameDirTag("%GAME_DIR%");

//-----------------------------------------------------------------------

class CShaderCooker
{
public:
	CShaderCooker(CEqJobManager& jobMng)
		: m_jobMng(jobMng)
	{
	}

	bool				Init(const char* confFileName, const char* targetName);
	void				Execute();

	void				SetFilter(const char* filter) { m_filter = filter; }

private:
	struct CompileTargetData
	{
		EShaderModuleType	type;
		CMemoryStream		resultData{ PP_SL };
		int					targetIdx{ -1 };
	};

	void				SearchFolderForShaders(const char* wildcard);
	bool				HasMatchingCRC(uint32 crc);

	bool				ParseShaderInfo(const char* shaderDefFileName, const KVSection& shaderSection, bool isExt = false);
	bool				ParseShaderExtensionInfo(const char* shaderDefFileName, const KVSection& shaderSection);
	bool				ParsePackage(const char* shaderDefFileName, const KVSection& shaderSection);

	void				ParseFileList(ShaderInfo& shaderInfo, const KVSection* fileListSec);

	void				InitShaderVariants(ShaderInfo& shaderInfo, int baseVariant, const KVSection& section);
	void				ProcessShader(ShaderInfo& shaderInfo, SyncJob& syncJob);

	bool				CompileShaderShadercSpirV(ShaderPackageCompileData& compileData, int entryPointIdx, int vertLayoutIdx, EqStringRef queryStr, ArrayRef<CompileTargetData> targetData, Array<ShaderInfo::Binding>& bindings);
	bool				CompileShaderSlang(ShaderPackageCompileData& compileData, int entryPointIdx, int vertLayoutIdx, EqStringRef queryStr, ArrayRef<CompileTargetData> targetData, Array<ShaderInfo::Binding>& bindings);

	void				AddOrReferenceCompilationResult(ShaderInfo& shaderInfo, ShaderInfo::Result& outResult, CompileTargetData& targetData, int entryPointIdx, int vertLayoutIdx, EqStringRef queryStr);

	struct BatchConfig
	{
		KVSection	crcSec;			// crc list loaded from disk
		KVSection	newCRCSec;		// crc list that will be saved
	};

	struct TargetProperties
	{
		Array<EqString>	includePaths{ PP_SL };
		Array<EShaderModuleType> blobTypes{ PP_SL };
		EqString		sourceShaderPath;
		EqString		sourceShaderDescExt;
		EqString		targetFolder;
	};

	CEqJobManager&		m_jobMng;
	BatchConfig			m_batchConfig;
	TargetProperties	m_targetProps;

	EqString			m_filter;

	Array<ShaderInfo>	m_shaderList{ PP_SL };
};

//-----------------------------------------------------------------------

struct ShaderPackageCompileData
	: public RefCountedObject<ShaderPackageCompileData>
{
	ShaderPackageCompileData(ShaderInfo& shaderInfo, const char* targetFileName, const char* shaderSourceName, CMemoryStream&& shaderSourceString)
		: shaderInfo(shaderInfo)
		, targetFileName(targetFileName)
		, shaderSourceFullName(shaderSourceName)
		, shaderSourceString(std::move(shaderSourceString))
	{
	}

	ShaderInfo&			shaderInfo;
	EqString			targetFileName;
	EqString			shaderSourceFullName;
	CMemoryStream		shaderSourceString;
	Array<EqString>		switchDefines{ PP_SL };
	volatile uint		compileErrors{ false };
};

//-----------------------------------------------------------------------

template<typename Processor>
static void ProcessIncludesRecursively(const char* fileName, const char* source, int length, ShaderIncluderImpl& includer, Processor& processor, int depth = 0);

struct IncludeCRCProcessor
{
	ShaderInfo& shaderInfo;
	Set<uint>	crcProcessed{ PP_SL };

	void Process(const char* currentSource, ShaderIncluderImpl& includer, const EqString& name, bool relative, int depth)
	{
		if (name == "VertexLayout")
		{
			// add all possible vertex layouts defined by the shader
			for (ShaderInfo::VertLayout vertLayout : shaderInfo.vertexLayouts)
			{
				if (vertLayout.aliasOf != -1)
					continue;

				includer.SetVertexLayout(vertLayout.name);
				ShaderIncluderImpl::IncludeResult* includeResult = includer.GetInclude(name, relative, fnmPathStripName(currentSource));
				if (!includeResult->isError)
				{
					uint checkCrc;
					CRC32_InitChecksum(checkCrc);
					CRC32_UpdateChecksum(checkCrc, includeResult->includeName, includeResult->includeName.Length());
					CRC32_FinishChecksum(checkCrc);

					if (crcProcessed.find(checkCrc).atEnd())
					{
						const char* incText = (const char*)includeResult->includeContent.GetBasePointer();
						const int incLength = includeResult->includeContent.Tell();

						crcProcessed.insert(checkCrc);
						CRC32_UpdateChecksum(shaderInfo.crc32, incText, incLength);

						ProcessIncludesRecursively(includeResult->includeName, incText, incLength, includer, *this, depth + 1);
					}
					//else
					//{
					//	MsgWarning("Skip Include: %.*s\n", (uint)includeResult->source_name_length, includeResult->source_name);
					//}
				}
				else
				{
					MsgWarning("%s: dependent file '%s' not found, skipping checksum\n", currentSource, name.ToCString());
				}

				includer.ReleaseInclude(includeResult);
			}
			return;
		}

		ShaderIncluderImpl::IncludeResult* includeResult = includer.GetInclude(name, relative, fnmPathStripName(currentSource));
		if (includeResult && !includeResult->isError)
		{
			uint checkCrc;
			CRC32_InitChecksum(checkCrc);
			CRC32_UpdateChecksum(checkCrc, includeResult->includeName, includeResult->includeName.Length());
			CRC32_FinishChecksum(checkCrc);

			if (crcProcessed.find(checkCrc).atEnd())
			{
				const char* incText = (const char*)includeResult->includeContent.GetBasePointer();
				const int incLength = includeResult->includeContent.Tell();

				crcProcessed.insert(checkCrc);
				CRC32_UpdateChecksum(shaderInfo.crc32, incText, incLength);
				ProcessIncludesRecursively(includeResult->includeName, incText, incLength, includer, *this, depth + 1);
			}
			//else
			//{
			//	MsgWarning("Skip Include: %.*s\n", (uint)includeResult->source_name_length, includeResult->source_name);
			//}
		}
		else
		{
			MsgWarning("%s: dependent file '%s' not found, skipping checksum\n", currentSource, name.ToCString());
		}

		includer.ReleaseInclude(includeResult);
	}
};

static EqStringRef GetRWFlagsString(int rwFlags)
{
	if (rwFlags == 0)
		return "";
	if (rwFlags == RWFLAG_UNIFORM)
		return "uniform";
	if (rwFlags == (RWFLAG_READ | RWFLAG_WRITE))
		return "readwrite";
	return rwFlags == RWFLAG_READ ? "readonly" : "writeonly";
}

//-----------------------------------------------------------------------

void CShaderCooker::ParseFileList(ShaderInfo& shaderInfo, const KVSection* fileListSec)
{
	if (!fileListSec)
		return;

	EqString pathToFile;
	for (const KVSection& itemSec : fileListSec->Keys())
	{
		const EqStringRef fileName = itemSec.GetName();

		pathToFile = fnmPathCombine(m_targetProps.sourceShaderPath, fileName);
		if (!g_fileSystem->FileExist(pathToFile))
		{
			MsgWarning("Can't find file %s\n", pathToFile.ToCString());
			continue;
		}

		ShaderInfo::AddFile& addFile = shaderInfo.addedFiles.append();
		addFile.fileName = pathToFile;

		for (auto valueIt : itemSec.Values<EqStringRef>())
			addFile.values.append(valueIt);
	}
}

static void ParseVertexLayouts(ShaderInfo& shaderInfo, const KVSection* vertLayoutsSec)
{
	if (!vertLayoutsSec)
		return;

	// add vertex layouts
	for (const KVSection& layoutKey : vertLayoutsSec->Keys())
	{
		ShaderInfo::VertLayout& vertLayout = shaderInfo.vertexLayouts.append();
		vertLayout.name = layoutKey.GetName();

		EqStringRef attrib;
		EqStringRef aliasOfStr;
		if (!layoutKey.GetValues(attrib, aliasOfStr))
			continue;

		if (!attrib.CompareCaseIns("aliasOf"))
		{
			const int aliasLayoutIdx = arrayFindIndexF(shaderInfo.vertexLayouts, [aliasOfStr](const ShaderInfo::VertLayout& layout) {
				return layout.name == aliasOfStr;
			});
			if (aliasLayoutIdx == -1)
				MsgError("%s - vertex layout %s for 'aliasOf' not found\n", shaderInfo.name.ToCString(), aliasOfStr.ToCString());

			vertLayout.aliasOf = aliasLayoutIdx;
		}
		else if (!attrib.CompareCaseIns("excludeDefines"))
		{
			for (EqStringRef def : layoutKey.Values<EqStringRef>(1))
				vertLayout.excludeDefines.append(def);
		}
	}
}

bool CShaderCooker::ParsePackage(const char* shaderDefFileName, const KVSection& shaderSection)
{
	const KVSection* fileListSec = shaderSection["FileList"];
	if (!fileListSec)
	{
		MsgWarning("%s missing 'FileList' section\n", shaderDefFileName);
		return false;
	}

	ShaderInfo& shaderInfo = m_shaderList.append();
	shaderInfo.crc32 = g_fileSystem->GetFileCRC32(shaderDefFileName, SP_ROOT);
	shaderSection.GetValues(shaderInfo.name);
	shaderInfo.type = ShaderInfo::SHADER_PACKAGE;

	// process file list
	ParseFileList(shaderInfo, fileListSec);

	ParseVertexLayouts(shaderInfo, shaderSection["VertexLayouts"]);

	return true;
}

bool CShaderCooker::ParseShaderInfo(const char* shaderDefFileName, const KVSection& shaderSection, bool isExt)
{
	EqStringRef sourceText;
	shaderSection.Get("SourceText").GetValues(sourceText);

	EqStringRef sourceFileName;
	shaderSection.Get("SourceFile").GetValues(sourceFileName);

	if (!sourceFileName.Length() && !sourceText.Length())
	{
		MsgWarning("%s missing 'SourceFile' or 'SourceText'\n", shaderDefFileName);
		return false;
	}

	if (sourceFileName.Length() && sourceText.Length())
	{
		MsgWarning("%s containts both 'SourceFile' and 'SourceText', please use one\n", shaderDefFileName);
		return false;
	}

	const KVSection* kinds = shaderSection.FindSection("SourceKind");
	if (!kinds)
	{
		MsgWarning("%s missing 'SourceKind' section\n", shaderDefFileName);
		return false;
	}

	{
		int shaderKindsFound = 0;
		for (const KVSection& key : kinds->Keys())
		{
			EqStringRef kindStr(key.GetName());

			if (!kindStr.CompareCaseIns("Vertex"))
				shaderKindsFound |= SHADERKIND_VERTEX;
			else if (!kindStr.CompareCaseIns("Fragment"))
				shaderKindsFound |= SHADERKIND_FRAGMENT;
			else if (!kindStr.CompareCaseIns("Compute"))
				shaderKindsFound |= SHADERKIND_COMPUTE;
		}

		if ((shaderKindsFound & SHADERKIND_COMPUTE) == 0 && (shaderKindsFound & SHADERKIND_VERTEX) == 0)
		{
			MsgWarning("%s must have Vertex kind section if it doesn't serve Compute\n", shaderDefFileName);
			return false;
		}
	}

	ShaderInfo& shaderInfo = m_shaderList.append();
	shaderInfo.crc32 = g_fileSystem->GetFileCRC32(shaderDefFileName, SP_ROOT);
	shaderSection.GetValues(shaderInfo.name);
	shaderInfo.sourceFilename = sourceText ? shaderDefFileName : sourceFileName;
	shaderInfo.sourceText = sourceText;
	shaderInfo.type = isExt ? ShaderInfo::SHADER_EXT : ShaderInfo::SHADER_BASE;
	shaderSection.Get("DebugInfo").GetValues(shaderInfo.debugInfo);
	shaderSection.Get("SkipOptimize").GetValues(shaderInfo.skipOptimize);

	for (const KVSection& kindSec : kinds->Keys())
	{
		EqStringRef kindName(kindSec.GetName());
		EShaderKind kind = {};
		EqStringRef entryPointNamePrefix;
		if (!kindName.CompareCaseIns("Vertex"))
		{
			kind = SHADERKIND_VERTEX;
			//entryPointNamePrefix = "vs_";
		}
		else if (!kindName.CompareCaseIns("Fragment"))
		{
			kind = SHADERKIND_FRAGMENT;
			//entryPointNamePrefix = "ps_";
		}
		else if (!kindName.CompareCaseIns("Compute"))
		{
			kind = SHADERKIND_COMPUTE;
			//entryPointNamePrefix = "cs_";
		}

		// add main entry point as default
		if (!kindSec.KeyCount())
		{
			ShaderInfo::EntryPoint& entryPoint = shaderInfo.entryPoints.append();
			entryPoint.name = entryPointNamePrefix + "main";
			entryPoint.kind = kind;
			continue;
		}

		for (const KVSection& entryPointSec : kindSec.Keys("EntryPoint"))
		{
			ShaderInfo::EntryPoint& entryPoint = shaderInfo.entryPoints.append();
			entryPointSec.GetValues(entryPoint.name);
			entryPoint.kind = kind;
		}
	}

	EqStringRef shaderType;
	shaderSection.Get("SourceType").GetValues(shaderType);
	if (!shaderType.CompareCaseIns("hlsl"))
		shaderInfo.sourceType = SHADERSOURCE_HLSL;
	else if (!shaderType.CompareCaseIns("glsl"))
		shaderInfo.sourceType = SHADERSOURCE_GLSL;

	InitShaderVariants(shaderInfo, -1, shaderSection);

	// Add default vertex layout (For compute of VBOless)
	if (shaderInfo.vertexLayouts.isEmpty())
	{
		ShaderInfo::VertLayout& defaultVertexLayout = shaderInfo.vertexLayouts.append();
		defaultVertexLayout.name = "Default";
	}

	// count all shader variations
	int numSwitchableDefines = 0;
	for (int i = 0; i < shaderInfo.variants.numElem(); ++i)
	{
		const ShaderInfo::Variant* variant = &shaderInfo.variants[i];
		do
		{
			if (variant->baseVariant != -1)
			{
				numSwitchableDefines += variant->defines.numElem();
				variant = &shaderInfo.variants[variant->baseVariant];
			}
			else
				break;
		} while (variant);
	}
	int nonAliasVertLayouts = 0;
	for (const ShaderInfo::VertLayout& vertexLayout : shaderInfo.vertexLayouts)
	{
		if (vertexLayout.aliasOf == -1)
			++nonAliasVertLayouts;
	}

	shaderInfo.totalVariationCount = nonAliasVertLayouts * (1 << numSwitchableDefines);

	// process file list if exists
	ParseFileList(shaderInfo, shaderSection["FileList"]);

	return true;
}

bool CShaderCooker::ParseShaderExtensionInfo(const char* shaderDefFileName, const KVSection& shaderSection)
{
	EqStringRef sourceFileName;
	EqStringRef sourceShaderName;
	EqStringRef newShaderName;
	if (shaderSection.GetValues(sourceFileName, sourceShaderName, newShaderName) < 2)
	{
		MsgError("shaderExt params are sourceFilename, sourceShaderName, newShaderName (optional)");
		return false;
	}

	EqString extShaderDefFileName;
	for (const EqString& path : m_targetProps.includePaths)
	{
		extShaderDefFileName = fnmPathCombine(path, sourceFileName);
		if (g_fileSystem->FileExist(extShaderDefFileName, SP_ROOT))
			break;
	}

	KVSection baseShaderRoot;
	if (!KV_LoadFromFile(extShaderDefFileName, SP_ROOT, baseShaderRoot))
	{
		MsgWarning("%s: unknown shader file '%s', check include paths\n", shaderDefFileName, sourceFileName.ToCString());
		return false;
	}

	KVSection* shaderRoot = nullptr;
	for (KVSection& shdKey : baseShaderRoot.Keys("shader"))
	{
		EqStringRef baseShaderName;
		if (!shdKey.GetValues(baseShaderName))
			continue;
		
		if (baseShaderName == sourceShaderName)
		{
			shaderRoot = &shdKey;
			break;
		}
	}

	if (!shaderRoot)
	{
		MsgWarning("%s: can't find shader '%s' in %s\n", shaderDefFileName, KV_GetValueString(&shaderSection, 1), extShaderDefFileName.ToCString());
		return false;
	}

	if(newShaderName)
		shaderRoot->SetValue(newShaderName.ToCString(), 0);

	// merge sections softly
	for (const KVSection& section : shaderSection.Keys())
	{
		KVSection* baseSec = shaderRoot->FindSection(section.GetName());
		if (baseSec)
		{
			baseSec->Clear();
			section.CopyTo(*baseSec);
		}
	}

	return ParseShaderInfo(extShaderDefFileName, *shaderRoot, true);
}

void CShaderCooker::SearchFolderForShaders(const char* wildcard)
{
	EqString searchFolder(wildcard);
	searchFolder.ReplaceSubstr("*", "");

	CFileSystemFind fsFind(wildcard, SP_ROOT);
	EqString fullShaderPath;
	EqString searchTemplate;
	while (fsFind.Next())
	{
		const EqStringRef fileName = fsFind.GetPath();

		if (m_filter.Length() && fileName.Find(m_filter) == -1)
			continue;

		if (fsFind.IsDirectory() && fileName != EqStringRef(".") && fileName != EqStringRef(".."))
		{
			searchTemplate = fnmPathCombine(searchFolder, fileName, "*");
			SearchFolderForShaders(searchTemplate);
		}
		else if (fnmPathExtractExt(fileName) == m_targetProps.sourceShaderDescExt.LowerCase())
		{
			fullShaderPath = fnmPathCombine(searchFolder, fileName);

			KVSection rootSec;
			if (!KV_LoadFromFile(fullShaderPath, SP_ROOT, rootSec))
				continue;

			int shadersFound = 0;
			for (const KVSection& shdKey : rootSec.Keys("shader"))
			{
				if(ParseShaderInfo(fullShaderPath, shdKey))
					++shadersFound;
			}

			for (const KVSection& shdExtKey : rootSec.Keys("shaderExt"))
			{
				if(ParseShaderExtensionInfo(fullShaderPath, shdExtKey))
					++shadersFound;
			}

			for (const KVSection& packageKey : rootSec.Keys("package"))
			{
				if (ParsePackage(fullShaderPath, packageKey))
					++shadersFound;
			}

			if (!shadersFound)
			{
				MsgWarning("%s does not have 'shader', 'shaderExt' or 'package' section.\n", fullShaderPath.ToCString());
				continue;
			}
		}
	}
}

bool CShaderCooker::HasMatchingCRC(uint32 crc)
{
	for (KVSection& crcEntry : m_batchConfig.crcSec.Keys())
	{
		uint32 checkCRC = strtoul(crcEntry.GetName(), nullptr, 10);
		if (checkCRC == crc)
			return true;
	}

	return false;
}

void CShaderCooker::InitShaderVariants(ShaderInfo& shaderInfo, int baseVariantIdx, const KVSection& section)
{
	bool hasVariantsThisLevel = false;
	for (const KVSection& nestedSec : section.Keys())
	{
		if (!CString::CompareCaseIns(nestedSec.GetName(), "define")
		 || !CString::CompareCaseIns(nestedSec.GetName(), "VertexLayouts"))
		{
			hasVariantsThisLevel = true;
			break;
		}
	}

	auto getVariant = [&](int& thisVariantIndex) -> ShaderInfo::Variant& {
		if (hasVariantsThisLevel || baseVariantIdx == -1)
		{
			thisVariantIndex = shaderInfo.variants.numElem();
			ShaderInfo::Variant& variant = shaderInfo.variants.append();
			variant.baseVariant = baseVariantIdx;

			EqStringRef secValue;
			section.GetValues(secValue);
			variant.name = EqString::Format("%s%s%s", section.GetName(), secValue ? "_" : "", secValue.ToCString());

			return variant;
		}
		return shaderInfo.variants[baseVariantIdx];
	};

	int thisVariantIndex = baseVariantIdx;
	ShaderInfo::Variant& variant = getVariant(thisVariantIndex);

	// collect all defines and vertex layouts
	for (const KVSection& nestedSec : section.Keys())
	{
		if (!CString::CompareCaseIns(nestedSec.GetName(), "define"))
		{
			// TODO: define types
			variant.defines.append(KV_GetValueString(&nestedSec));
		}
		else if (!CString::CompareCaseIns(nestedSec.GetName(), "VertexLayouts"))
		{
			ParseVertexLayouts(shaderInfo, &nestedSec);
		}
		else if(!CString::CompareCaseIns(nestedSec.GetName(), "SkipCombo"))
		{
			// TODO: expression parser?
			ShaderInfo::SkipCombo& skipCombo = shaderInfo.skipCombos.append();
			for (const EqStringRef& def : nestedSec.Values<EqStringRef>())
				skipCombo.defines.append(def);
		}
		else if (!CString::CompareCaseIns(nestedSec.GetName(), "UniformLayout"))
		{
			// atm skip
		}
		else
			InitShaderVariants(shaderInfo, thisVariantIndex, nestedSec);
	}
}

template<typename Processor>
static void ProcessIncludesRecursively(const char* fileName, const char* source, int length, ShaderIncluderImpl& includer, Processor& processor, int depth)
{
	const char* srcBegin = source;
	const char* srcEnd = source + length;

	const char* nameStart = nullptr;
	for (const char* sp = srcBegin; sp < srcEnd;)
	{
		if (!nameStart)
		{
			if (*sp != '#')
			{
				++sp;
				continue;
			}

			const char* prefix = sp;

			if (strncmp(prefix + 1, "include", 7))
			{
				sp = prefix + 1;
				continue;
			}

			for (const char* ns = prefix + 8; ns < srcEnd; ++ns)
			{
				if (*ns == '"' || *ns == '<')
				{
					nameStart = ns + 1;
					break;
				}
			}
		}
		else
		{
			const char* nameEnd = nullptr;
			for (const char* ns = nameStart; ns < srcEnd; ++ns)
			{
				if (*ns == '"' || *ns == '>')
				{
					nameEnd = ns;
					break;
				}
			}
			if (!nameEnd)
			{
				MsgError("%s error: #include directive is not closed", fileName);
				return;
			}

			const EqString name(nameStart, nameEnd - nameStart);
			const bool relativePath = *nameEnd == '"';

			processor.Process(fileName, includer, name, relativePath, depth);
			nameStart = nullptr;
			sp = nameEnd;
		}
	}
}

// this build root signature
static void ParseShaderResourceBindings(Array<ShaderInfo::Binding>& bindings, EShaderKind shaderKind, const char* name, char* source, int length)
{
	if (CString::CompareCaseIns(name, "ShaderCooker") == 0)
		return;

	Tokenizer tokenizer(2);
	static const char BIND_MARKER[] = "__sc_bind__";

	char* bindBegin = CString::SubString((char*)source, BIND_MARKER);
	while (bindBegin)
	{
		tokenizer.setString(bindBegin);
		char* tok = tokenizer.next();

		ShaderInfo::Binding& newBinding = bindings.append();
		tok = tokenizer.next();
		if (*tok == '(')
		{
			tok = tokenizer.next();

			{
				newBinding.descriptorSetIdx = atoi(tok);
				tok = tokenizer.next();
				if (*tok != ',')
				{
					ASSERT_FAIL("%s* expects value after comma\n", BIND_MARKER, name);
					bindBegin = CString::SubString(bindBegin + sizeof(BIND_MARKER), BIND_MARKER);
					continue;
				}
				tok = tokenizer.next();
			}

			newBinding.index = atoi(tok);

			tok = tokenizer.next();

			// skip extra arguments (image formats etc)
			while (*tok != ')')
				tok = tokenizer.next();

			// replace bind decl with spaces to make valid source file again (see GLSLBoilerPlate/HLSLBoilerPlate)
			{
				const int tokPos = tokenizer.getPos();
				for (int i = 0; i <= tokPos; ++i)
					bindBegin[i] = ' ';
			}

			tok = tokenizer.next();

			// skip this pointless keyword
			if (!CString::Compare(tok, "uniform"))
				tok = tokenizer.next();

			if (!CString::Compare(tok, "readonly"))
			{
				newBinding.rwFlags = RWFLAG_READ;
				tok = tokenizer.next();
			}
			else if (!CString::Compare(tok, "writeonly"))
			{
				newBinding.rwFlags = RWFLAG_WRITE;
				tok = tokenizer.next();
			}

			// skip this pointless keyword
			if (!CString::Compare(tok, "uniform"))
				tok = tokenizer.next();

			if (!CString::Compare(tok, "image1D") || !CString::Compare(tok, "image2D") || !CString::Compare(tok, "image3D") || !CString::Compare(tok, "imageCube")
				|| !CString::Compare(tok, "image1DArray") || !CString::Compare(tok, "image2DArray") || !CString::Compare(tok, "image3DArray") || !CString::Compare(tok, "imageCubeArray"))
				newBinding.type = BINDENTRY_STORAGETEXTURE;
			else if (!CString::Compare(tok, "texture1D") || !CString::Compare(tok, "texture2D") || !CString::Compare(tok, "texture3D") || !CString::Compare(tok, "textureCube")
				|| !CString::Compare(tok, "texture1DArray") || !CString::Compare(tok, "texture2DArray") || !CString::Compare(tok, "texture3DArray") || !CString::Compare(tok, "textureCubeArray"))
				newBinding.type = BINDENTRY_TEXTURE;
			else if (!CString::Compare(tok, "sampler"))
				newBinding.type = BINDENTRY_SAMPLER;
			else if (!CString::Compare(tok, "buffer"))
				newBinding.type = BINDENTRY_BUFFER;
			else // everything else is a uniform buffer
			{
				newBinding.type = BINDENTRY_BUFFER;
				newBinding.rwFlags = RWFLAG_UNIFORM;
			}

			if(newBinding.type != BINDENTRY_BUFFER)
				tok = tokenizer.next();

			newBinding.name = tok;
		}

		bindBegin = CString::SubString(bindBegin + sizeof(BIND_MARKER), BIND_MARKER);
	}
}

static Threading::CEqMutex s_slangMutex;

bool CShaderCooker::CompileShaderSlang(ShaderPackageCompileData& compileData, int entryPointIdx, int vertLayoutIdx, EqStringRef queryStr, ArrayRef<CompileTargetData> targetData, Array<ShaderInfo::Binding>& bindings)
{
	using namespace Slang;
	using namespace Threading;

	ShaderInfo& shaderInfo = compileData.shaderInfo;
	const ShaderInfo::EntryPoint& entryPoint = shaderInfo.entryPoints[entryPointIdx];
	const ShaderInfo::VertLayout& vertexLayout = shaderInfo.vertexLayouts[vertLayoutIdx];

	thread_local ComPtr<slang::IGlobalSession> slangGlobalSes;
	if(!slangGlobalSes)
	{
		SlangGlobalSessionDesc slangGlobalSessionDesc;
		slangGlobalSessionDesc.enableGLSL = (shaderInfo.sourceType == SHADERSOURCE_GLSL);

		slang::createGlobalSession(&slangGlobalSessionDesc, slangGlobalSes.writeRef());
	}

	SlangFileSystemIncluder includer(shaderInfo, m_targetProps.includePaths);
	includer.SetVertexLayout(vertexLayout.name);
	includer.AddRef();

	ComPtr<SlangCompileRequest> slangCompileRequest;
	slangGlobalSes->createCompileRequest(slangCompileRequest.writeRef());

	slangCompileRequest->setFileSystem(&includer);
	slangCompileRequest->setAllowGLSLInput(shaderInfo.sourceType == SHADERSOURCE_GLSL);

	// NOTE: WGSL doesn't work
	slangCompileRequest->setMatrixLayoutMode(SLANG_MATRIX_LAYOUT_ROW_MAJOR);

	if (shaderInfo.debugInfo)
		slangCompileRequest->setDebugInfoLevel(SLANG_DEBUG_INFO_LEVEL_MAXIMAL);
	else
		slangCompileRequest->setDebugInfoLevel(SLANG_DEBUG_INFO_LEVEL_NONE);

	if (shaderInfo.skipOptimize)
		slangCompileRequest->setOptimizationLevel(SLANG_OPTIMIZATION_LEVEL_NONE);
	else
		slangCompileRequest->setOptimizationLevel(SLANG_OPTIMIZATION_LEVEL_MAXIMAL);

	for(auto& path : m_targetProps.includePaths)
		slangCompileRequest->addSearchPath(path);

	// add vertex layout defines
	for (int i = 0; i < shaderInfo.vertexLayouts.numElem(); ++i)
	{
		const ShaderInfo::VertLayout& layout = shaderInfo.vertexLayouts[i];
		const int vertexId = layout.aliasOf != -1 ? layout.aliasOf : i;

		slangCompileRequest->addPreprocessorDefine(EqString::Format("VID_%s", layout.name), EqString::Format("%u", StringId24(shaderInfo.vertexLayouts[vertexId].name)));
	}

	slangCompileRequest->addPreprocessorDefine("CURRENT_VERTEX_ID", EqString::Format("%u", StringId24(vertexLayout.name)));

	// currently using as hack for crashing extern struct
	slangCompileRequest->addPreprocessorDefine("VERTEX_LAYOUT", vertexLayout.name);
	
	for(CompileTargetData& tgtData : targetData)
	{
		SlangCompileTarget slangCompileTarget;
		EqStringRef profileName;
		switch (tgtData.type)
		{
		case SHADERMODULE_SPIRV:
			// TODO: configuration
			profileName = "spirv_1_3";
			slangCompileTarget = SLANG_SPIRV;
			break;
		case SHADERMODULE_DXBC:
			slangCompileTarget = SLANG_DXBC;
			break;
		case SHADERMODULE_DXIL:
			profileName = "sm_6_0";
			slangCompileTarget = SLANG_DXIL;
			break;
		case SHADERMODULE_WGSL:
			slangCompileTarget = SLANG_WGSL;
			break;
		default:
			ASSERT_FAIL("Unknown shader module type");
		}
		tgtData.targetIdx = slangCompileRequest->addCodeGenTarget(slangCompileTarget);
		if(profileName)
			slangCompileRequest->setTargetProfile(tgtData.targetIdx, slangGlobalSes->findProfile(profileName));
	}

	SlangSourceLanguage srcLang = SLANG_SOURCE_LANGUAGE_SLANG;
	if (shaderInfo.sourceType == SHADERSOURCE_GLSL)
		srcLang = SLANG_SOURCE_LANGUAGE_GLSL;
	else if (shaderInfo.sourceType == SHADERSOURCE_HLSL)
		srcLang = SLANG_SOURCE_LANGUAGE_HLSL;

	SlangStage entryPointStage;
	EqStringRef entryPointPrefix;
	if (entryPoint.kind == SHADERKIND_VERTEX)
	{
		slangCompileRequest->addPreprocessorDefine("VERTEX", nullptr);
		entryPointStage = SLANG_STAGE_VERTEX;
		//entryPointPrefix = "vs_";
	}
	else if (entryPoint.kind == SHADERKIND_FRAGMENT)
	{
		slangCompileRequest->addPreprocessorDefine("FRAGMENT", nullptr);
		entryPointStage = SLANG_STAGE_FRAGMENT;
		//entryPointPrefix = "ps_";
	}
	else if (entryPoint.kind == SHADERKIND_COMPUTE)
	{
		slangCompileRequest->addPreprocessorDefine("COMPUTE", nullptr);
		entryPointStage = SLANG_STAGE_COMPUTE;
		//entryPointPrefix = "cs_";
	}

	// add macros from query string
	if (queryStr)
	{
		BitArray enabledSwitches(PP_SL, compileData.switchDefines.numElem());

		char* macros = const_cast<char*>(queryStr.GetData());
		char* macrosEnd = macros + queryStr.Length();
		while (macros < macrosEnd)
		{
			char* next = strchr(macros, '|');
			if (!next)
				next = macrosEnd;

			const EqString str(macros, next - macros);
			const int foundSwitchIdx = arrayFindIndex(compileData.switchDefines, str);
			if (foundSwitchIdx != -1)
				enabledSwitches.setTrue(foundSwitchIdx);

			macros = next + 1;
		}

		for(int i = 0; i < compileData.switchDefines.numElem(); ++i)
			slangCompileRequest->addPreprocessorDefine(compileData.switchDefines[i], enabledSwitches[i] ? "1" : "0");
	}

	ComPtr<slang::IEntryPoint> slangEntryPoint;
	auto slangAddSourceFile = [&](const char* name, const char* path) {
		const int idx = slangCompileRequest->addTranslationUnit(srcLang, name);
		slangCompileRequest->addTranslationUnitSourceFile(idx, path);
	};

	slangAddSourceFile("ShaderCooker", "ShaderCooker");
	if (vertexLayout.name != "Default" && entryPoint.kind != SHADERKIND_COMPUTE)
	{
		slangAddSourceFile("IVertex", fnmPathCombine("VertexLayouts", "IVertex.h"));
		slangAddSourceFile("VertexLayout", fnmPathCombine("VertexLayouts", vertexLayout.name + ".h"));
	}

	{
		const char* srcStart = (const char*)compileData.shaderSourceString.GetBasePointer();
		const char* srcEnd = srcStart + compileData.shaderSourceString.Tell();
		const int idx = slangCompileRequest->addTranslationUnit(srcLang, "Main");
		slangCompileRequest->addTranslationUnitSourceStringSpan(idx, compileData.shaderSourceFullName, srcStart, srcEnd);
		slangCompileRequest->addEntryPoint(idx, entryPointPrefix + entryPoint.name, entryPointStage);
	}

	if (SLANG_FAILED(slangCompileRequest->compile()))
	{
		MsgError("Slang Failed compiling %s %s %s\n%s\n\n", shaderInfo.name.ToCString(), vertexLayout.name.ToCString(), queryStr.ToCString(), (const char*)slangCompileRequest->getDiagnosticOutput());

		Atomic::Store(compileData.compileErrors, true);
		return false;
	}

	if(0)
	{
		const char* diagStr = (const char*)slangCompileRequest->getDiagnosticOutput();
		if(diagStr && *diagStr)
		{
			MsgWarning(diagStr);
			MsgWarning("\n");
		}
	}

	// construct bindings for PipelineLayout
	{
		slang::ShaderReflection* shaderReflection = slang::ShaderReflection::get(slangCompileRequest);
		slang::VariableLayoutReflection* varLayoutReflection = shaderReflection->getGlobalParamsVarLayout();
		slang::TypeLayoutReflection* typeLayoutReflection = varLayoutReflection->getTypeLayout();

		auto slangBindingTypeToD3DRangeType = [](SlangBindingType bindingType, SlangResourceAccess accessType) {
			switch (bindingType & SLANG_BINDING_TYPE_BASE_MASK)
			{
			case SLANG_BINDING_TYPE_SAMPLER:
				return BINDING_RANGE_SAMPLER;
			case SLANG_BINDING_TYPE_TEXTURE:
				switch (accessType)
				{
					case SLANG_RESOURCE_ACCESS_READ_WRITE:
					case SLANG_RESOURCE_ACCESS_WRITE:
						return BINDING_RANGE_UAV;
					default:
						return BINDING_RANGE_SRV;
				}
			case SLANG_BINDING_TYPE_CONSTANT_BUFFER:
				return BINDING_RANGE_CBV;
			case SLANG_BINDING_TYPE_PARAMETER_BLOCK:
			case SLANG_BINDING_TYPE_TYPED_BUFFER:
			case SLANG_BINDING_TYPE_RAW_BUFFER:
				return BINDING_RANGE_UAV;
			default:
				return BINDING_RANGE_SRV;
			}
		};

		int bindingTypeCounter[8][SLANG_BINDING_TYPE_PUSH_CONSTANT + 1]{ 0 };

		const int paramCount = shaderReflection->getParameterCount();
		for (int i = 0; i < paramCount; i++)
		{
			auto param = shaderReflection->getParameterByIndex(i);
			auto paramName = param->getName();
			auto type = param->getType();

			const slang::BindingType bindingType = typeLayoutReflection->getBindingRangeType(i);
			const SlangResourceAccess accessType = type->getResourceAccess();
			const EBindingRangeType bindingRangeType = slangBindingTypeToD3DRangeType(static_cast<SlangBindingType>(bindingType), accessType);

			switch (type->getKind())
			{
			case slang::TypeReflection::Kind::Resource:
			{
				ShaderInfo::Binding& binding = bindings.append();
				binding.name = paramName;

				binding.descriptorSetIdx = param->getBindingSpace();
				binding.index = param->getBindingIndex();

				const int registerIndex = bindingTypeCounter[param->getBindingSpace()][static_cast<int>(bindingRangeType)]++;
				binding.rangeType = bindingRangeType;
				binding.registerIdx = registerIndex;

				switch (type->getResourceShape())
				{
				case SLANG_STRUCTURED_BUFFER:
				case SLANG_BYTE_ADDRESS_BUFFER:
					binding.type = BINDENTRY_BUFFER; break;
				default:
					binding.type = BINDENTRY_TEXTURE;
				}

				switch (type->getResourceAccess())
				{
				case SLANG_RESOURCE_ACCESS_READ_WRITE:
					binding.rwFlags = RWFLAG_READ | RWFLAG_WRITE; break;
				case SLANG_RESOURCE_ACCESS_READ:
					binding.rwFlags = RWFLAG_READ; break;
				case SLANG_RESOURCE_ACCESS_WRITE:
					binding.rwFlags = RWFLAG_WRITE; break;
				}

				if (binding.type == BINDENTRY_TEXTURE && binding.rwFlags != RWFLAG_UNIFORM)
					binding.type = BINDENTRY_STORAGETEXTURE;

				break;
			}
			case slang::TypeReflection::Kind::SamplerState:
			{
				ShaderInfo::Binding& binding = bindings.append();
				binding.name = paramName;
				binding.type = BINDENTRY_SAMPLER;
				binding.rwFlags = RWFLAG_UNIFORM;

				binding.descriptorSetIdx = param->getBindingSpace();
				binding.index = param->getBindingIndex();

				const int registerIndex = bindingTypeCounter[param->getBindingSpace()][static_cast<int>(bindingRangeType)]++;
				binding.rangeType = bindingRangeType;
				binding.registerIdx = registerIndex;
				break;
			}
			case slang::TypeReflection::Kind::ConstantBuffer:
			{
				ShaderInfo::Binding& binding = bindings.append();
				binding.name = paramName;
				binding.type = BINDENTRY_BUFFER;
				binding.rwFlags = RWFLAG_UNIFORM;

				binding.descriptorSetIdx = param->getBindingSpace();
				binding.index = param->getBindingIndex();

				const int registerIndex = bindingTypeCounter[param->getBindingSpace()][static_cast<int>(bindingRangeType)]++;
				binding.rangeType = bindingRangeType;
				binding.registerIdx = registerIndex;
				break;
			}
			}
		}

		for (const ShaderInfo::Binding& binding : bindings)
		{
			Msg("[vk_layout(%d,%d)] %s %s %s : register(%d, space%d)\n", binding.index, binding.descriptorSetIdx, GetRWFlagsString(binding.rwFlags).ToCString(), s_bindingTypeNames[binding.type], binding.name.ToCString(), binding.registerIdx, binding.descriptorSetIdx);
		}
	}

	for (CompileTargetData& tgtData : targetData)
	{
		ComPtr<ISlangBlob> slangTargetBlob;
		slangCompileRequest->getEntryPointCodeBlob(0, tgtData.targetIdx, slangTargetBlob.writeRef());

		CMemoryStream blobData(PP_SL);
		blobData.Open((uint8_t*)slangTargetBlob->getBufferPointer(), slangTargetBlob->getBufferSize());

		tgtData.resultData.Open(FS_OPEN_WRITE);
		tgtData.resultData.AppendStream(&blobData);
	}

	return true;
}

bool CShaderCooker::CompileShaderShadercSpirV(ShaderPackageCompileData& compileData, int entryPointIdx, int vertLayoutIdx, EqStringRef queryStr, ArrayRef<CompileTargetData> targetData, Array<ShaderInfo::Binding>& bindings)
{
	ASSERT(targetData[0].type == SHADERMODULE_SPIRV);

	ShaderInfo& shaderInfo = compileData.shaderInfo;
	const ShaderInfo::EntryPoint& entryPoint = shaderInfo.entryPoints[entryPointIdx];
	const ShaderInfo::VertLayout& vertexLayout = shaderInfo.vertexLayouts[vertLayoutIdx];

	EqStringRef kindMacroStr;
	EqStringRef entryPointPrefix;
	shaderc_shader_kind shaderCKind;
	if (entryPoint.kind == SHADERKIND_VERTEX)
	{
		kindMacroStr = "VERTEX";
		shaderCKind = shaderc_vertex_shader;
	}
	else if (entryPoint.kind == SHADERKIND_FRAGMENT)
	{
		kindMacroStr = "FRAGMENT";
		shaderCKind = shaderc_fragment_shader;
	}
	else if (entryPoint.kind == SHADERKIND_COMPUTE)
	{
		kindMacroStr = "COMPUTE";
		shaderCKind = shaderc_compute_shader;
	}
	
	shaderc::CompileOptions options;
	{
		std::unique_ptr<ShadercIncluder> includer = std::make_unique<ShadercIncluder>(shaderInfo, m_targetProps.includePaths);
		includer->SetVertexLayout(vertexLayout.name);
		options.SetIncluder(std::move(includer));
	}
	options.SetSourceLanguage(s_sourceLanguage[shaderInfo.sourceType]);
	options.AddMacroDefinition(kindMacroStr.ToCString());

	// add vertex layout defines
	for (int i = 0; i < shaderInfo.vertexLayouts.numElem(); ++i)
	{
		const ShaderInfo::VertLayout& layout = shaderInfo.vertexLayouts[i];
		const int vertexId = layout.aliasOf != -1 ? layout.aliasOf : i;

		options.AddMacroDefinition(EqString::Format("VID_%s", layout.name).ToCString(), EqString::Format("%u", StringId24(shaderInfo.vertexLayouts[vertexId].name)).ToCString());
	}
	options.AddMacroDefinition("CURRENT_VERTEX_ID", EqString::Format("%u", StringId24(vertexLayout.name)).ToCString());

	// add macros from query string
	if (queryStr)
	{
		char* macros = const_cast<char*>(queryStr.GetData());
		char* macrosEnd = macros + queryStr.Length();
		while (macros < macrosEnd)
		{
			char* next = strchr(macros, '|');
			if (!next)
				next = macrosEnd;

			options.AddMacroDefinition(macros, next - macros, nullptr, 0u);
			macros = next + 1;
		}
	}

	if (shaderInfo.skipOptimize)
		options.SetOptimizationLevel(shaderc_optimization_level_zero);
	else
		options.SetOptimizationLevel(shaderc_optimization_level_performance);

	if (shaderInfo.debugInfo)
		options.SetGenerateDebugInfo();

	if (shaderInfo.sourceType == SHADERSOURCE_GLSL)
	{
		options.SetForcedVersionProfile(450, shaderc_profile_none);
		options.SetTargetEnvironment(shaderc_target_env_webgpu, 0);
	}

	// first we need to pre-process our file and collect bindings for pipeline layouts
	shaderc::Compiler compiler;
	shaderc::PreprocessedSourceCompilationResult preprocessResult = compiler.PreprocessGlsl(
		(const char*)compileData.shaderSourceString.GetBasePointer(),
		compileData.shaderSourceString.GetSize(),
		shaderCKind,
		compileData.shaderSourceFullName,
		options);			

	const shaderc_compilation_status preprocessStatus = preprocessResult.GetCompilationStatus();
	if (preprocessStatus != shaderc_compilation_status_success)
	{
		MsgError("Failed pre-processing %s %s\n%s\n", vertexLayout.name.ToCString(), queryStr.ToCString(), preprocessResult.GetErrorMessage().c_str());
		if (preprocessStatus == shaderc_compilation_status_compilation_error)
			Atomic::Store(compileData.compileErrors, true);
		return false;
	}

	// we need to parse shader resources from pre-processed text
	ParseShaderResourceBindings(bindings, entryPoint.kind, compileData.shaderSourceFullName, const_cast<char*>(preprocessResult.begin()), preprocessResult.end() - preprocessResult.begin());

	shaderc::SpvCompilationResult spvCompilationResult = compiler.CompileGlslToSpv(
		preprocessResult.begin(),
		preprocessResult.end() - preprocessResult.begin(),
		shaderCKind,
		compileData.shaderSourceFullName,
		entryPoint.name,
		options
	);

	const shaderc_compilation_status compileStatus = spvCompilationResult.GetCompilationStatus();
	if (compileStatus != shaderc_compilation_status_success)
	{
		MsgError("ShaderC Failed compiling %s %s %s\n%s\n", shaderInfo.name.ToCString(), vertexLayout.name.ToCString(), queryStr.ToCString(), spvCompilationResult.GetErrorMessage().c_str());
		if (compileStatus == shaderc_compilation_status_compilation_error)
			Atomic::Store(compileData.compileErrors, true);
		return false;
	}
	
	CMemoryStream resultStream;
	resultStream.Open((const ubyte*)spvCompilationResult.begin(), (spvCompilationResult.end() - spvCompilationResult.begin()) * sizeof(spvCompilationResult.begin()[0]));

	targetData[0].resultData.Open(FS_OPEN_WRITE);
	targetData[0].resultData.AppendStream(&resultStream);

	return true;
};


static Threading::CEqMutex s_resultsMutex;

void CShaderCooker::AddOrReferenceCompilationResult(ShaderInfo& shaderInfo, ShaderInfo::Result& outResult, CompileTargetData& targetData, int entryPointIdx, int vertLayoutIdx, EqStringRef queryStr)
{
	using namespace Threading;

	const ShaderInfo::EntryPoint& entryPoint = shaderInfo.entryPoints[entryPointIdx];
	const ShaderInfo::VertLayout& vertexLayout = shaderInfo.vertexLayouts[vertLayoutIdx];

	uint32 resultCRC = 0;
	CRC32_InitChecksum(resultCRC); 
	CRC32_UpdateChecksum(resultCRC, &entryPoint.kind, sizeof(entryPoint.kind));
	CRC32_UpdateChecksum(resultCRC, vertexLayout.name.ToCString(), vertexLayout.name.Length());
	CRC32_UpdateChecksum(resultCRC, targetData.resultData.GetBasePointer(), targetData.resultData.GetSize());
	CRC32_FinishChecksum(resultCRC);

	{
		Threading::CScopedMutex m(s_resultsMutex);

		// Reference shaders if they have same output
		const int refIdx = arrayFindIndexF(shaderInfo.results, [&targetData, resultCRC](const ShaderInfo::Result& result) {
			return resultCRC == result.crc32[targetData.type];
		});

		if (refIdx == -1)
		{
			outResult.data[targetData.type].Open(FS_OPEN_WRITE | FS_OPEN_READ);
			targetData.resultData.WriteToStream(&outResult.data[targetData.type]);
			outResult.crc32[targetData.type] = resultCRC;
		}
		else
		{
			ASSERT_MSG(entryPoint.kind == shaderInfo.results[refIdx].kindFlag, "Referenced shader kind is invalid (checksum collision?)");
			outResult.refResult = refIdx;
		}
	}
}

void CShaderCooker::ProcessShader(ShaderInfo& shaderInfo, SyncJob& syncJob)
{
	using namespace Threading;

	const EqString targetFileName = fnmPathCombine(m_targetProps.targetFolder, EqString::Format("%s.shd", shaderInfo.name));

	CRefPtr<ShaderPackageCompileData> compileData;
	{
		CMemoryStream shaderSourceString{ PP_SL };
		EqString shaderSourceFullName;
		if (shaderInfo.type != ShaderInfo::SHADER_PACKAGE)
		{
			if (shaderInfo.sourceText.Length())
			{
				const int _zero = 0;
				shaderSourceString.Open(FS_OPEN_READ | FS_OPEN_WRITE);
				shaderSourceString.Write(shaderInfo.sourceText.GetData(), shaderInfo.sourceText.Length(), 1);
				// CRC is already computed for source

				// use .def file name
				shaderSourceFullName = shaderInfo.sourceFilename;
			}
			else if (shaderInfo.sourceFilename.Length())
			{
				if (shaderInfo.type == ShaderInfo::SHADER_EXT)
				{
					for (const EqString& path : m_targetProps.includePaths)
					{
						shaderSourceFullName = fnmPathCombine(path, shaderInfo.sourceFilename);
						if (g_fileSystem->FileExist(shaderSourceFullName, SP_ROOT))
							break;
					}
				}
				else
					shaderSourceFullName = fnmPathCombine(m_targetProps.sourceShaderPath, shaderInfo.sourceFilename);

				IFileStreamPtr file = g_fileSystem->Open(shaderSourceFullName, FS_OPEN_READ, SP_ROOT);
				if (!file)
				{
					MsgError("Unable to open source file for %s\n", shaderInfo.name.ToCString());
					return;
				}
				shaderSourceString.Open(FS_OPEN_READ | FS_OPEN_WRITE);
				shaderSourceString.AppendStream(file);

				// generate CRC from shader source file and append to the shader desc CRC
				CRC32_UpdateChecksum(shaderInfo.crc32, shaderSourceString.GetBasePointer(), shaderSourceString.GetSize());
			}
			else
			{
				ASSERT_FAIL("SourceFile or SourceText contains zero characters or programmer error");
				return;
			}
		}
		compileData = CRefPtr_new(ShaderPackageCompileData, shaderInfo, targetFileName, shaderSourceFullName, std::move(shaderSourceString));
	}

	// process all includes CRCs
	// also collect pipeline layouts
	{
		ShaderIncluderImpl includer(shaderInfo, m_targetProps.includePaths);
		IncludeCRCProcessor processor{ shaderInfo };

		EqStringRef sourceName = shaderInfo.sourceFilename.Length() ? shaderInfo.sourceFilename : shaderInfo.name;

		const char* incText = (const char*)compileData->shaderSourceString.GetBasePointer();
		const int incLength = compileData->shaderSourceString.Tell();

		ProcessIncludesRecursively(sourceName, incText, incLength, includer, processor);
	}

	// now check CRC from loaded file
	if (HasMatchingCRC(shaderInfo.crc32) && g_fileSystem->FileExist(targetFileName, SP_ROOT))
	{
		// store new CRC
		m_batchConfig.newCRCSec.SetKey(EqString::Format("%u", shaderInfo.crc32), shaderInfo.name);

		MsgInfo("Skipping shader '%s' (no changes made)\n", shaderInfo.name.ToCString());
		return;
	}

	FunctionJob* completeJob = PPNew FunctionJob("ShaderCompleteJob", [this, compileData](void*, int) {
		ShaderInfo& shaderInfo = compileData->shaderInfo;
		// Store files
		if (!compileData->compileErrors && (shaderInfo.results.numElem() || shaderInfo.addedFiles.numElem()))
		{
			CDPKFileWriter shaderPackFile("shaders", 4);
			if (!shaderPackFile.Begin(compileData->targetFileName))
			{
				MsgError("Unable to create pack file %s\n", compileData->targetFileName.ToCString());
				return;
			}

			// store new CRC
			m_batchConfig.newCRCSec.SetKey(EqString::Format("%u", shaderInfo.crc32), shaderInfo.name);

			// Store shader info
			KVSection shaderInfoKvs;
			shaderInfoKvs.SetName(shaderInfo.name);
			{
				KVSection& definesSec = shaderInfoKvs.CreateSection("Defines");
				for (EqString& defineStr : compileData->switchDefines)
					definesSec.AddValue(defineStr);
			}

			// store vertex layout info
			{
				KVSection& vertexLayoutsSec = shaderInfoKvs.CreateSection("VertexLayouts");
				for (ShaderInfo::VertLayout& vertLayout : shaderInfo.vertexLayouts)
				{
					KVSection& layoutSec = vertexLayoutsSec.CreateSection(vertLayout.name);
					if (vertLayout.aliasOf != -1)
					{
						layoutSec.AddValue("aliasOf");
						layoutSec.AddValue(shaderInfo.vertexLayouts[vertLayout.aliasOf].name);
					}
				}
			}

			KVSection& fileListSec = shaderInfoKvs.CreateSection("FileList");

			int shaderFileCount = 0;
			Array<int> referenceRemap(PP_SL);
			referenceRemap.setNum(shaderInfo.results.numElem());

			// Store shader outputs in separate files
			for (int i = 0; i < shaderInfo.results.numElem(); ++i)
			{
				const ShaderInfo::Result& result = shaderInfo.results[i];
				const ShaderInfo::VertLayout& layout = shaderInfo.vertexLayouts[result.vertLayoutIdx];

				if (result.refResult != -1)
					continue;

				EqString shaderFileName = EqString::Format("%s-%s", layout.name.ToCString(), result.queryStr.ToCString());

				KVSection& shaderBlobSec = fileListSec.CreateSection("blob");
				shaderBlobSec.AddValue(result.vertLayoutIdx);
			
				if (result.kindFlag == SHADERKIND_VERTEX)
				{
					shaderBlobSec.AddValue("Vertex");
					shaderFileName.Append(".vert");
				}
				else if (result.kindFlag == SHADERKIND_FRAGMENT)
				{
					shaderBlobSec.AddValue("Fragment");
					shaderFileName.Append(".frag");
				}
				else if (result.kindFlag == SHADERKIND_COMPUTE)
				{
					shaderBlobSec.AddValue("Compute");
					shaderFileName.Append(".comp");
				}

				shaderBlobSec.AddValue(shaderInfo.entryPoints[result.entryPointIdx].name);
				shaderBlobSec.AddValue(result.queryStr);

				for (ShaderInfo::Binding& binding : result.bindings)
				{
					KVSection& bindingSec = shaderBlobSec.CreateSection(binding.name.ToCString());
					bindingSec.AddValue(s_bindingTypeNames[binding.type]);
					bindingSec.AddValue(GetRWFlagsString(binding.rwFlags));
					bindingSec.AddValue(binding.descriptorSetIdx);
					bindingSec.AddValue(binding.index);
					bindingSec.AddValue(binding.rangeType);
					bindingSec.AddValue(binding.registerIdx);
				}

				// Write shader bytecode files
				if(result.data[SHADERMODULE_SPIRV].IsValid())
					shaderPackFile.Add(&result.data[SHADERMODULE_SPIRV], shaderFileName + s_shaderModuleTypeExt[SHADERMODULE_SPIRV]);
				if(result.data[SHADERMODULE_DXBC].IsValid())
					shaderPackFile.Add(&result.data[SHADERMODULE_DXBC], shaderFileName + s_shaderModuleTypeExt[SHADERMODULE_DXBC]);
				if(result.data[SHADERMODULE_DXIL].IsValid())
					shaderPackFile.Add(&result.data[SHADERMODULE_DXIL], shaderFileName + s_shaderModuleTypeExt[SHADERMODULE_DXIL]);
				if (result.data[SHADERMODULE_WGSL].IsValid())
					shaderPackFile.Add(&result.data[SHADERMODULE_WGSL], shaderFileName + s_shaderModuleTypeExt[SHADERMODULE_WGSL]);

				referenceRemap[i] = shaderFileCount++;
			}

			for (const ShaderInfo::Result& result : shaderInfo.results)
			{
				const ShaderInfo::VertLayout& layout = shaderInfo.vertexLayouts[result.vertLayoutIdx];

				if (result.refResult == -1)
					continue;

				ASSERT_MSG(result.refResult != -1, "Something went wrong, got empty shader and no reference id");
				ASSERT(shaderInfo.results[result.refResult].kindFlag == result.kindFlag);

				// Reference shader bytecode file
				KVSection& refSec = fileListSec.CreateSection("ref");
				refSec.AddValue(result.vertLayoutIdx);

				if (result.kindFlag == SHADERKIND_VERTEX)
					refSec.AddValue("Vertex");
				else if (result.kindFlag == SHADERKIND_FRAGMENT)
					refSec.AddValue("Fragment");
				else if (result.kindFlag == SHADERKIND_COMPUTE)
					refSec.AddValue("Compute");

				refSec.AddValue(shaderInfo.entryPoints[result.entryPointIdx].name);
				refSec.AddValue(result.queryStr);
				refSec.AddValue(referenceRemap[result.refResult]);
			}

			// put added files
			for (const ShaderInfo::AddFile& addFile : shaderInfo.addedFiles)
			{
				IFileStreamPtr filePtr = g_fileSystem->Open(addFile.fileName, FS_OPEN_READ, SP_ROOT);
				shaderPackFile.Add(filePtr, addFile.values.back());

				KVSection& fileSec = fileListSec.CreateSection(addFile.values[0]);
				for(int i = 1; i < addFile.values.numElem(); ++i)
					fileSec.AddValue(addFile.values[i]);
			}

			CMemoryStream shaderInfoData(PP_SL);
			shaderInfoData.Open(FS_OPEN_WRITE, nullptr, 8192);
			KeyValues::WriteBinary(&shaderInfoData, shaderInfoKvs);
			shaderPackFile.Add(&shaderInfoData, "ShaderInfo");

			shaderPackFile.End();
		}
	});
	completeJob->DeleteOnFinish();
	completeJob->InitJob();

	syncJob.AddWait(completeJob);

	if (shaderInfo.type != ShaderInfo::SHADER_PACKAGE)
	{
		// collect all defines into flat list
		for (const ShaderInfo::Variant& variant : shaderInfo.variants)
		{
			if(variant.baseVariant != -1)
				compileData->switchDefines.append(variant.defines);
		}

		MsgWarning("Processing shader %s (%d vertex layouts %d defines)\n", shaderInfo.name.ToCString(), shaderInfo.vertexLayouts.numElem(), compileData->switchDefines.numElem());

		const int totalVariantCount = 1 << compileData->switchDefines.numElem();

		// reserve variant count * (vertex + fragment)
		shaderInfo.results.reserve(shaderInfo.vertexLayouts.numElem() * totalVariantCount * 2);

		for (int vertLayoutIdx = 0; vertLayoutIdx < shaderInfo.vertexLayouts.numElem(); ++vertLayoutIdx)
		{
			if (compileData->compileErrors)
				break;

			const ShaderInfo::VertLayout& vertexLayout = shaderInfo.vertexLayouts[vertLayoutIdx];
			if (vertexLayout.aliasOf != -1)
			{
				// skip
				MsgInfo("   - ref vertex %s as %s\n", shaderInfo.vertexLayouts[vertexLayout.aliasOf].name.ToCString(), vertexLayout.name.ToCString());
				continue;
			}

			MsgWarning("   Compiling for vertex %s\n", vertexLayout.name.ToCString());
			for (int i = 0; i < totalVariantCount; ++i)
			{
				FunctionJob* compileVariantJob = PPNew FunctionJob(vertexLayout.name, [this, compileData, vertexLayout, i, vertLayoutIdx](void*, int) {
					if (compileData->compileErrors)
						return;

					ShaderInfo& shaderInfo = compileData->shaderInfo;

					EqString queryStr;
					for (int switchDef = 0; switchDef < compileData->switchDefines.numElem(); ++switchDef)
					{
						if (i & (1 << switchDef))
						{
							if (arrayFindIndex(vertexLayout.excludeDefines, compileData->switchDefines[switchDef]) != -1)
							{
								MsgWarning("Skipping %s %s\n", vertexLayout.name.ToCString(), compileData->switchDefines[switchDef].ToCString());
								return;
							}

							if (queryStr.Length())
								queryStr.Append("|");
							queryStr.Append(compileData->switchDefines[switchDef]);
						}
					}

					auto foundDefineLen = [](const char* str)
					{
						const char* p = str;
						while (!(*p == 0 || *p == '|'))
							++p;
						return p - str;
					};

					for (const ShaderInfo::SkipCombo& skip : shaderInfo.skipCombos)
					{
						if (skip.defines.isEmpty())
							continue;

						int foundCount = 0;
						for (const EqString& define : skip.defines)
						{
							const int foundIdx = queryStr.Find(define, true);
							if (foundIdx != -1 && foundDefineLen(queryStr.ToCString() + foundIdx) == define.Length())
							{
								++foundCount;
							}
						}
						if (foundCount == skip.defines.numElem())
							return;
					}

					for (int entryPointIdx = 0; entryPointIdx < shaderInfo.entryPoints.numElem(); ++entryPointIdx)
					{
						if (compileData->compileErrors)
							break;

						// store result
						s_resultsMutex.Lock();
						ShaderInfo::Result& result = shaderInfo.results.append();
						s_resultsMutex.Unlock();

						result.queryStr = queryStr;
						result.vertLayoutIdx = vertLayoutIdx;
						result.entryPointIdx = entryPointIdx;
						result.kindFlag = shaderInfo.entryPoints[entryPointIdx].kind;

						FixedArray<CompileTargetData, SHADERMODULE_TYPES> compileTargets;

						if (shaderInfo.sourceType == SHADERSOURCE_GLSL)
						{
							// Legacy source
							compileTargets.append({ SHADERMODULE_SPIRV });
							if (!CompileShaderShadercSpirV(compileData.Ref(), entryPointIdx, vertLayoutIdx, queryStr, compileTargets, result.bindings))
							{
								result.isError = true;
								break;
							}
						}
						else
						{
							for(EShaderModuleType blobType : m_targetProps.blobTypes)
								compileTargets.append({ blobType });

							// Slang can compile into SPIRV, DXIL, WGSL
							if (!CompileShaderSlang(compileData.Ref(), entryPointIdx, vertLayoutIdx, queryStr, compileTargets, result.bindings))
							{
								result.isError = true;
								break;
							}
						}

						for (CompileTargetData& tgtData : compileTargets)
							AddOrReferenceCompilationResult(shaderInfo, result, tgtData, entryPointIdx, vertLayoutIdx, queryStr);
					}

					//if(!stopCompilation)
					//	Msg("   - compiled variant '%s' (%d)\n", queryStr.ToCString(), nSwitch);
				});

				compileVariantJob->DeleteOnFinish();
				completeJob->AddWait(compileVariantJob);
				m_jobMng.InitStartJob(compileVariantJob);

				if (compileData->compileErrors)
					break;
			}
		}
	}

	m_jobMng.StartJob(completeJob);
}

bool CShaderCooker::Init(const char* confFileName, const char* targetName)
{
	// load all properties
	KVSection kvs;
	if (!KV_LoadFromFile(confFileName, SP_ROOT, kvs))
	{
		MsgError("Failed to load '%s' file!\n", confFileName);
		return false;
	}

	// get the target properties
	{
		// load target info
		const KVSection* targets = kvs["Targets"];
		if (!targets)
		{
			MsgError("Missing 'Targets' section in '%s'\n", confFileName);
			return false;
		}

		const KVSection* currentTarget = targets->FindSection(targetName);
		if (!currentTarget)
		{
			MsgError("Cannot find target section '%s'\n", targetName);
			return false;
		}

		// source shader settings
		{
			EqStringRef shadersSrc;
			if (!currentTarget->Get("SourcePath").GetValues(shadersSrc))
			{
				MsgError("Target '%s' SourcePath folder is not specified!\n", targetName);
				return false;
			}

			EqStringRef sourceFileExt;
			if (!currentTarget->Get("SourceExt").GetValues(sourceFileExt))
			{
				MsgWarning("Target '%s' SourceExt is not specified, default to 'tga'\n", targetName);
				sourceFileExt = "tga";
			}

			for (const KVSection& includePathKey : currentTarget->Keys("includePath"))
			{
				EqString includePath;
				includePathKey.GetValues(includePath);

				includePath.ReplaceSubstr(s_engineDirTag, g_fileSystem->GetCurrentDataDirectory());
				includePath.ReplaceSubstr(s_gameDirTag, g_fileSystem->GetCurrentGameDirectory());

				m_targetProps.includePaths.append(std::move(includePath));
			}

			for (auto val : currentTarget->Get("blobs").Values<EqStringRef>())
			{
				bool added = false;
				for (int type = 0; type < SHADERMODULE_TYPES; ++type)
				{
					if (!val.CompareCaseIns(s_shaderModuleTypeName[type]))
					{
						m_targetProps.blobTypes.append(static_cast<EShaderModuleType>(type));
						added = true;
						break;
					}
				}

				if(!added)
					MsgError("Target '%s' unknown blob type %s\n", targetName, val.ToCString());
			}

			if (m_targetProps.blobTypes.isEmpty())
			{
				MsgError("Target '%s' missing 'blobs' values or has invalid values\n", targetName);
				return false;
			}

			m_targetProps.sourceShaderPath = shadersSrc;
			m_targetProps.sourceShaderDescExt = sourceFileExt.TrimChar('.');

			m_targetProps.sourceShaderPath.ReplaceSubstr(s_engineDirTag, g_fileSystem->GetCurrentDataDirectory());
			m_targetProps.sourceShaderPath.ReplaceSubstr(s_gameDirTag, g_fileSystem->GetCurrentGameDirectory());
		}

		// target settings
		{
			EqStringRef targetFolder;
			if (!currentTarget->Get("output").GetValues(targetFolder))
			{
				MsgError("Target '%s' missing 'output' value\n", targetName);
				return false;
			}

			m_targetProps.targetFolder = targetFolder;

			m_targetProps.targetFolder.ReplaceSubstr(s_engineDirTag, g_fileSystem->GetCurrentDataDirectory());
			m_targetProps.targetFolder.ReplaceSubstr(s_gameDirTag, g_fileSystem->GetCurrentGameDirectory());

			g_fileSystem->MakeDir(m_targetProps.targetFolder, SP_ROOT);
		}
	}

	return true;
}

void CShaderCooker::Execute()
{
	// perform batch conversion
	Msg("Shader source path: '%s'\n", m_targetProps.sourceShaderPath.ToCString());

	const EqString searchTemplate = fnmPathCombine(m_targetProps.sourceShaderPath, "*");

	// walk up shader files
	SearchFolderForShaders(searchTemplate);

	int totalVariationCount = 0;
	for (ShaderInfo& shaderInfo : m_shaderList)
		totalVariationCount += shaderInfo.totalVariationCount;

	Msg("Got %d shaders %d variations total\n", m_shaderList.numElem(), totalVariationCount);

	EqString crcFileName(EqString::Format("%s/shaders_crc.txt", m_targetProps.targetFolder.ToCString()));

	// load CRC list, check for existing shader files, and skip if necessary
	KV_LoadFromFile(crcFileName, SP_ROOT, m_batchConfig.crcSec);

	// process shader files
	
	SyncJob syncJob("WaitForCompilationComplete");
	syncJob.InitSignal();
	for (ShaderInfo& shaderInfo : m_shaderList)
		ProcessShader(shaderInfo, syncJob);

	m_jobMng.InitStartJob(&syncJob);
	syncJob.GetSignal()->Wait();

	// save CRC list file
	IFileStreamPtr pStream = g_fileSystem->Open(crcFileName, FS_OPEN_WRITE, SP_ROOT);
	if (pStream)
		KeyValues::WriteText(pStream, m_batchConfig.newCRCSec);
}

void CookTarget(CEqJobManager& jobMng, const char* pszTargetName, const char* shaderNameFilter)
{
	CShaderCooker cooker(jobMng);
	if (!cooker.Init("ShaderCooker.CONFIG", pszTargetName))
		return;
	cooker.SetFilter(shaderNameFilter);
	cooker.Execute();
}