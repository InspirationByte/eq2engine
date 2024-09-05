//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2023
//////////////////////////////////////////////////////////////////////////////////
// Description: Shader module compiler
//////////////////////////////////////////////////////////////////////////////////

#include <shaderc/shaderc.hpp>
#include "core/core_common.h"
#include "core/IFileSystem.h"
#include "core/platform/eqjobmanager.h"
#include "ds/MemoryStream.h"
#include "utils/KeyValues.h"
#include "dpk/DPKFileWriter.h"

#include "ShaderInfo.h"
#include "ShaderIncluder.h"

using namespace Threading;

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


//-------------------------------------

class CShaderCooker
{
public:
	CShaderCooker(CEqJobManager& jobMng)
		: m_jobMng(jobMng)
	{
	}

	bool				Init(const char* confFileName, const char* targetName);
	void				Execute();

private:
	void				SearchFolderForShaders(const char* wildcard);
	bool				HasMatchingCRC(uint32 crc);

	bool				ParseShaderInfo(const char* shaderDefFileName, const KVSection* shaderSection, bool isExt = false);
	bool				ParseShaderExtensionInfo(const char* shaderDefFileName, const KVSection* shaderSection);
	bool				ParsePackage(const char* shaderDefFileName, const KVSection* shaderSection);

	void				ParseFileList(ShaderInfo& shaderInfo, const KVSection* fileListSec);

	void				InitShaderVariants(ShaderInfo& shaderInfo, int baseVariant, const KVSection* section);
	void				ProcessShader(ShaderInfo& shaderInfo);

	struct BatchConfig
	{
		KVSection	crcSec;			// crc list loaded from disk
		KVSection	newCRCSec;		// crc list that will be saved
	};

	struct TargetProperties
	{
		Array<EqString>	includePaths{ PP_SL };
		EqString		sourceShaderPath;
		EqString		sourceShaderDescExt;
		EqString		targetFolder;
	};

	CEqJobManager&		m_jobMng;
	BatchConfig			m_batchConfig;
	TargetProperties	m_targetProps;

	Array<ShaderInfo>	m_shaderList{ PP_SL };
};

//-----------------------------------------------------------------------

void CShaderCooker::ParseFileList(ShaderInfo& shaderInfo, const KVSection* fileListSec)
{
	if (!fileListSec)
		return;

	for (const KVSection* itemSec : fileListSec->Keys())
	{
		const EqStringRef fileName = itemSec->GetName();

		EqString pathToFile;
		fnmPathCombine(pathToFile, m_targetProps.sourceShaderPath, fileName);
		if (!g_fileSystem->FileExist(pathToFile))
		{
			MsgWarning("Can't find file %s\n", pathToFile.ToCString());
			continue;
		}

		ShaderInfo::AddFile& addFile = shaderInfo.addedFiles.append();
		addFile.fileName = pathToFile;

		for (auto valueIt : itemSec->Values<EqStringRef>())
			addFile.values.append(valueIt);
	}
}

static void ParseVertexLayouts(ShaderInfo& shaderInfo, const KVSection* vertLayoutsSec)
{
	if (!vertLayoutsSec)
		return;

	// add vertex layouts
	for (const KVSection* layoutKey : vertLayoutsSec->Keys())
	{
		ShaderInfo::VertLayout& vertLayout = shaderInfo.vertexLayouts.append();
		vertLayout.name = layoutKey->GetName();

		if (!CString::CompareCaseIns(KV_GetValueString(layoutKey, 0), "aliasOf"))
		{
			EqStringRef aliasOfStr = KV_GetValueString(layoutKey, 1);
			const int aliasLayout = arrayFindIndexF(shaderInfo.vertexLayouts, [aliasOfStr](const ShaderInfo::VertLayout& layout) {
				return layout.name == aliasOfStr;
				});
			if (aliasLayout == -1)
				MsgError("%s - vertex layout %s for 'aliasOf' not found\n", shaderInfo.name.ToCString(), aliasOfStr.ToCString());

			vertLayout.aliasOf = aliasLayout;
		}
		else if (!CString::CompareCaseIns(KV_GetValueString(layoutKey, 0), "excludeDefines"))
		{
			for (KVValueIterator<EqString> it(layoutKey, 1); !it.atEnd(); ++it)
				vertLayout.excludeDefines.append(*it);
		}
	}
}

bool CShaderCooker::ParsePackage(const char* shaderDefFileName, const KVSection* shaderSection)
{
	const KVSection* fileListSec = (*shaderSection)["FileList"];
	if (!fileListSec)
	{
		MsgWarning("%s missing 'FileList' section\n", shaderDefFileName);
		return false;
	}

	ShaderInfo& shaderInfo = m_shaderList.append();
	shaderInfo.crc32 = g_fileSystem->GetFileCRC32(shaderDefFileName, SP_ROOT);
	shaderInfo.name = KV_GetValueString(shaderSection);
	shaderInfo.type = ShaderInfo::SHADER_PACKAGE;

	// process file list
	ParseFileList(shaderInfo, fileListSec);

	ParseVertexLayouts(shaderInfo, (*shaderSection)["VertexLayouts"]);

	return true;
}

bool CShaderCooker::ParseShaderInfo(const char* shaderDefFileName, const KVSection* shaderSection, bool isExt)
{
	const char* shaderFileName = KV_GetValueString(shaderSection->FindSection("SourceFile"), 0, nullptr);
	if (!shaderFileName)
	{
		MsgWarning("%s missing 'SourceFile'\n", shaderDefFileName);
		return false;
	}

	const KVSection* kinds = shaderSection->FindSection("SourceKind");
	if (!kinds)
	{
		MsgWarning("%s missing 'SourceKind' section\n", shaderDefFileName);
		return false;
	}

	{
		int shaderKindsFound = 0;
		for (const KVSection* key : kinds->Keys())
		{
			EqStringRef kindStr(key->name);

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
	shaderInfo.name = KV_GetValueString(shaderSection);
	shaderInfo.sourceFilename = shaderFileName;
	shaderInfo.type = isExt ? ShaderInfo::SHADER_EXT : ShaderInfo::SHADER_BASE;

	for (const KVSection* kindSec : kinds->Keys())
	{
		EqStringRef kindName(kindSec->GetName());
		int kind = 0;
		if (!kindName.CompareCaseIns("Vertex"))
			kind = SHADERKIND_VERTEX;
		else if (!kindName.CompareCaseIns("Fragment"))
			kind = SHADERKIND_FRAGMENT;
		else if (!kindName.CompareCaseIns("Compute"))
			kind = SHADERKIND_COMPUTE;

		// add main entry point as default
		if (!kindSec->KeyCount())
		{
			ShaderInfo::EntryPoint& entryPoint = shaderInfo.entryPoints.append();
			entryPoint.name = "main";
			entryPoint.kind = kind;
			continue;
		}

		for (const KVSection* entryPointSec : kindSec->Keys("EntryPoint"))
		{
			ShaderInfo::EntryPoint& entryPoint = shaderInfo.entryPoints.append();
			entryPoint.name = KV_GetValueString(entryPointSec);
			entryPoint.kind = kind;
		}
	}

	EqStringRef shaderType = KV_GetValueString(shaderSection->FindSection("SourceType"), 0, nullptr);
	if (!shaderType.CompareCaseIns("hlsl"))
		shaderInfo.sourceType = SHADERSOURCE_HLSL;
	else if (!shaderType.CompareCaseIns("glsl"))
		shaderInfo.sourceType = SHADERSOURCE_GLSL;
	//else if (!shaderType.CompareCaseIns("wgsl"))
	//	shaderInfo.sourceType = SHADERSOURCE_WGSL;

	InitShaderVariants(shaderInfo, -1, shaderSection);

	// Add default vertex layout (For compute of VBOless)
	if (shaderInfo.vertexLayouts.numElem() == 0)
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
	ParseFileList(shaderInfo, (*shaderSection)["FileList"]);

	return true;
}

bool CShaderCooker::ParseShaderExtensionInfo(const char* shaderDefFileName, const KVSection* shaderSection)
{
	EqStringRef sourceFileName;
	EqStringRef sourceShaderName;
	EqStringRef newShaderName;
	if (shaderSection->GetValues(sourceFileName, sourceShaderName, newShaderName) < 2)
	{
		MsgError("shaderExt params are sourceFilename, sourceShaderName, newShaderName (optional)");
		return false;
	}

	EqString extShaderDefFileName;
	for (const EqString& path : m_targetProps.includePaths)
	{
		fnmPathCombine(extShaderDefFileName, path, sourceFileName);
		if (g_fileSystem->FileExist(extShaderDefFileName, SP_ROOT))
			break;
	}

	KVSection baseShaderRoot;
	if (!KV_LoadFromFile(extShaderDefFileName, SP_ROOT, &baseShaderRoot))
	{
		MsgWarning("%s: unknown shader file '%s', check include paths\n", shaderDefFileName, sourceFileName.ToCString());
		return false;
	}

	KVSection* shaderRoot = nullptr;
	for (KVSection* shdKey : baseShaderRoot.Keys("shader"))
	{
		EqStringRef baseShaderName;
		if (!shdKey->GetValues(baseShaderName))
			continue;
		
		if (baseShaderName == sourceShaderName)
		{
			shaderRoot = shdKey;
			break;
		}
	}

	if (!shaderRoot)
	{
		MsgWarning("%s: can't find shader '%s' in %s\n", shaderDefFileName, KV_GetValueString(shaderSection, 1), extShaderDefFileName.ToCString());
		return false;
	}

	if(newShaderName)
		shaderRoot->SetValue(newShaderName.ToCString(), 0);

	// merge sections softly
	for (const KVSection* section : shaderSection->Keys())
	{
		KVSection* baseSec = shaderRoot->FindSection(section->GetName());
		if (baseSec)
		{
			baseSec->Cleanup();
			section->CopyTo(baseSec);
		}
	}

	return ParseShaderInfo(extShaderDefFileName, shaderRoot, true);
}

void CShaderCooker::SearchFolderForShaders(const char* wildcard)
{
	EqString searchFolder(wildcard);
	searchFolder.ReplaceSubstr("*", "");

	CFileSystemFind fsFind(wildcard, SP_ROOT);
	while (fsFind.Next())
	{
		EqString fileName = fsFind.GetPath();
		if (fsFind.IsDirectory() && fileName != EqStringRef(".") && fileName != EqStringRef(".."))
		{
			EqString searchTemplate;
			fnmPathCombine(searchTemplate, searchFolder, fileName, "*");

			SearchFolderForShaders(searchTemplate);
		}
		else if (fnmPathExtractExt(fileName) == m_targetProps.sourceShaderDescExt.LowerCase())
		{
			EqString fullShaderPath;
			fnmPathCombine(fullShaderPath, searchFolder, fileName);

			KVSection rootSec;
			if (!KV_LoadFromFile(fullShaderPath, SP_ROOT, &rootSec))
				continue;

			int shadersFound = 0;
			for (const KVSection* shdKey : rootSec.Keys("shader"))
			{
				if(ParseShaderInfo(fullShaderPath, shdKey))
					++shadersFound;
			}

			for (const KVSection* shdExtKey : rootSec.Keys("shaderExt"))
			{
				if(ParseShaderExtensionInfo(fullShaderPath, shdExtKey))
					++shadersFound;
			}

			for (const KVSection* packageKey : rootSec.Keys("package"))
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
	for (KVSection* crcEntry : m_batchConfig.crcSec.Keys())
	{
		uint32 checkCRC = strtoul(crcEntry->GetName(), nullptr, 10);
		if (checkCRC == crc)
			return true;
	}

	return false;
}

void CShaderCooker::InitShaderVariants(ShaderInfo& shaderInfo, int baseVariantIdx, const KVSection* section)
{
	bool hasVariantsThisLevel = false;
	for (const KVSection* nestedSec : section->Keys())
	{
		if (!CString::CompareCaseIns(nestedSec->GetName(), "define")
		 || !CString::CompareCaseIns(nestedSec->GetName(), "VertexLayouts"))
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

			const char* secValue = KV_GetValueString(section, 0, nullptr);
			variant.name = EqString::Format("%s%s%s", section->GetName(), secValue ? "_" : "", secValue);

			return variant;
		}
		return shaderInfo.variants[baseVariantIdx];
	};

	int thisVariantIndex = baseVariantIdx;
	ShaderInfo::Variant& variant = getVariant(thisVariantIndex);

	// collect all defines and vertex layouts
	for (const KVSection* nestedSec : section->Keys())
	{
		if (!CString::CompareCaseIns(nestedSec->GetName(), "define"))
		{
			// TODO: define types
			variant.defines.append(KV_GetValueString(nestedSec));
		}
		else if (!CString::CompareCaseIns(nestedSec->GetName(), "VertexLayouts"))
		{
			ParseVertexLayouts(shaderInfo, nestedSec);
		}
		else if(!CString::CompareCaseIns(nestedSec->GetName(), "SkipCombo"))
		{
			// TODO: expression parser?
			ShaderInfo::SkipCombo& skipCombo = shaderInfo.skipCombos.append();
			for (KVValueIterator<EqString> it(nestedSec); !it.atEnd(); ++it)
			{
				skipCombo.defines.append(*it);
			}
		}
		else if (!CString::CompareCaseIns(nestedSec->GetName(), "UniformLayout"))
		{
			// atm skip
		}
		else
			InitShaderVariants(shaderInfo, thisVariantIndex, nestedSec);
	}
}

void CShaderCooker::ProcessShader(ShaderInfo& shaderInfo)
{
	EqString targetFileName;
	fnmPathCombine(targetFileName, m_targetProps.targetFolder, EqString::Format("%s.shd", shaderInfo.name.ToCString()));

	uint32 srcCRC = shaderInfo.crc32;
	EqString shaderSourceName;
	CMemoryStream shaderSourceString(PP_SL);

	if (shaderInfo.type != ShaderInfo::SHADER_PACKAGE)
	{
		if (shaderInfo.type == ShaderInfo::SHADER_EXT)
		{
			for (const EqString& path : m_targetProps.includePaths)
			{
				fnmPathCombine(shaderSourceName, path, shaderInfo.sourceFilename);
				if (g_fileSystem->FileExist(shaderSourceName, SP_ROOT))
					break;
			}
		}
		else
			fnmPathCombine(shaderSourceName, m_targetProps.sourceShaderPath, shaderInfo.sourceFilename);

		{
			IFilePtr file = g_fileSystem->Open(shaderSourceName, "r", SP_ROOT);
			if (!file)
			{
				MsgError("Unable to open source file for %s\n", shaderInfo.name.ToCString());
				return;
			}
			shaderSourceString.Open(nullptr, VS_OPEN_READ | VS_OPEN_WRITE, file->GetSize());
			shaderSourceString.AppendStream(file);
		}

		// generate CRC from shader source file and append to the shader desc CRC
		CRC32_UpdateChecksum(srcCRC, shaderSourceString.GetBasePointer(), shaderSourceString.GetSize());
	}

	// now check CRC from loaded file
	if (HasMatchingCRC(srcCRC) && g_fileSystem->FileExist(targetFileName, SP_ROOT))
	{
		// store new CRC
		m_batchConfig.newCRCSec.SetKey(EqString::Format("%u", srcCRC), shaderInfo.sourceFilename);

		MsgInfo("Skipping shader '%s' (no changes made)\n", shaderInfo.name.ToCString());
		return;
	}

	bool compileErrors = false;
	Array<EqString> switchDefines(PP_SL);

	if (shaderInfo.type != ShaderInfo::SHADER_PACKAGE)
	{
		// collect all defines into flat list
		for (const ShaderInfo::Variant& variant : shaderInfo.variants)
		{
			if(variant.baseVariant != -1)
				switchDefines.append(variant.defines);
		}

		MsgWarning("Processing shader %s (%d vertex layouts %d defines)\n", shaderInfo.name.ToCString(), shaderInfo.vertexLayouts.numElem(), switchDefines.numElem());

		const int totalVariantCount = 1 << switchDefines.numElem();

		// reserve variant count * (vertex + fragment)
		shaderInfo.results.reserve(shaderInfo.vertexLayouts.numElem() * totalVariantCount * 2);

		ArrayCRef<EqString> includePaths(m_targetProps.includePaths);

		CEqMutex resultsMutex;
		for (int vertLayoutIdx = 0; vertLayoutIdx < shaderInfo.vertexLayouts.numElem(); ++vertLayoutIdx)
		{
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
				FunctionJob* funcJob = PPNew FunctionJob(vertexLayout.name, 
					[includePaths, &resultsMutex, &shaderSourceString, &shaderSourceName, &compileErrors, &shaderInfo, vertexLayout, &switchDefines, nSwitch = i, vertLayoutIdx](void*, int) {
					EqString queryStr;
					for (int j = 0; j < switchDefines.numElem(); ++j)
					{
						if (nSwitch & (1 << j))
						{
							if (arrayFindIndex(vertexLayout.excludeDefines, switchDefines[j]) != -1)
							{
								MsgWarning("Skipping %s %s\n", vertexLayout.name.ToCString(), switchDefines[j].ToCString());
								return;
							}

							if (queryStr.Length())
								queryStr.Append("|");
							queryStr.Append(switchDefines[j]);
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
						if (skip.defines.numElem() == 0)
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

					shaderc::Compiler compiler;
					auto fillMacros = [vertexLayout, &queryStr, &switchDefines, nSwitch](shaderc::CompileOptions& options) {
						for (int j = 0; j < switchDefines.numElem(); ++j)
						{
							if (nSwitch & (1 << j))
								options.AddMacroDefinition(switchDefines[j], switchDefines[j].Length(), nullptr, 0u);
						}
					};

					auto compileShaderKind = [&](int entryPointId, const ShaderInfo::EntryPoint& entryPoint)
					{
						EqStringRef kindMacroStr;
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

						std::unique_ptr<EqShaderIncluder> includer = std::make_unique<EqShaderIncluder>(shaderInfo, includePaths);
						includer->SetVertexLayout(vertexLayout.name);

						shaderc::CompileOptions options;
						options.SetSourceLanguage(s_sourceLanguage[shaderInfo.sourceType]);
						options.SetOptimizationLevel(shaderc_optimization_level_performance);
						options.SetIncluder(std::move(includer));
						options.AddMacroDefinition(kindMacroStr.ToCString());			
						options.SetTargetEnvironment(shaderc_target_env_webgpu, 0);
						//options.SetGenerateDebugInfo();

						fillMacros(options);
						shaderc::SpvCompilationResult compilationResult;

						if (shaderInfo.sourceType == SHADERSOURCE_GLSL)
						{
							options.SetForcedVersionProfile(450, shaderc_profile_none);

							compilationResult = compiler.CompileGlslToSpv(
								(const char*)shaderSourceString.GetBasePointer(),
								shaderSourceString.GetSize(),
								shaderCKind,
								shaderSourceName,
								entryPoint.name,
								options
							);
						}
						else if (shaderInfo.sourceType == SHADERSOURCE_HLSL)
						{
							compilationResult = compiler.CompileGlslToSpv(
								(const char*)shaderSourceString.GetBasePointer(),
								shaderSourceString.GetSize(),
								shaderCKind,
								shaderSourceName,
								entryPoint.name,
								options
							);
						}

						const shaderc_compilation_status compileStatus = compilationResult.GetCompilationStatus();

						if (compileStatus == shaderc_compilation_status_success)
						{
							uint32 resultCRC = 0;
							CRC32_InitChecksum(resultCRC);
							CRC32_UpdateChecksum(resultCRC, compilationResult.begin(), (compilationResult.end() - compilationResult.begin()) * sizeof(compilationResult.begin()[0]));

							CScopedMutex m(resultsMutex);

							// Reference shaders if they have same output
							// TODO: make it so defines are detected for Vertex or Fragment.
							const int refIdx = arrayFindIndexF(shaderInfo.results, [resultCRC](const ShaderInfo::Result& result) {
								return resultCRC == result.crc32;
							});

							// store result
							ShaderInfo::Result& result = shaderInfo.results.append();
							if (refIdx == -1)
							{
								result.data = std::move(compilationResult);
							}
							else
							{
								ASSERT_MSG(entryPoint.kind == shaderInfo.results[refIdx].kindFlag, "Referenced shader kind is invalid (checksum collision?)");
								result.refResult = refIdx;
							}

							result.queryStr = queryStr;
							result.vertLayoutIdx = vertLayoutIdx;
							result.entryPointId = entryPointId;
							result.kindFlag = entryPoint.kind;
							result.crc32 = resultCRC;
						}
						else
						{
							MsgError("Failed compiling %s %s\n%s\n", vertexLayout.name.ToCString(), queryStr.ToCString(), compilationResult.GetErrorMessage().c_str());
							if (compileStatus == shaderc_compilation_status_compilation_error)
								compileErrors = true;
						}
					};

					int entryPointIdx = 0;
					for (const ShaderInfo::EntryPoint& entryPoint : shaderInfo.entryPoints)
					{
						if (compileErrors)
							break;
						compileShaderKind(entryPointIdx, entryPoint);
						++entryPointIdx;
					}

					//if(!stopCompilation)
					//	Msg("   - compiled variant '%s' (%d)\n", queryStr.ToCString(), nSwitch);
				});

				funcJob->DeleteOnFinish();
				m_jobMng.InitStartJob(funcJob);

				if (compileErrors)
					break;
			}
		}
		m_jobMng.Wait();
	}

	// Store files
	if (!compileErrors && (shaderInfo.results.numElem() || shaderInfo.addedFiles.numElem()))
	{
		CDPKFileWriter shaderPackFile("shaders", 4);
		if (!shaderPackFile.Begin(targetFileName))
		{
			MsgError("Unable to create pack file %s\n", targetFileName.ToCString());
			return;
		}

		// store new CRC
		m_batchConfig.newCRCSec.SetKey(EqString::Format("%u", srcCRC), shaderInfo.sourceFilename);

		// Store shader info
		KVSection shaderInfoKvs;
		shaderInfoKvs.SetName(shaderInfo.name);
		{
			KVSection* definesSec = shaderInfoKvs.CreateSection("Defines");
			for (EqString& defineStr : switchDefines)
				definesSec->AddValue(defineStr);
		}

		// store vertex layout info
		{
			KVSection* vertexLayoutsSec = shaderInfoKvs.CreateSection("VertexLayouts");
			for (ShaderInfo::VertLayout& vertLayout : shaderInfo.vertexLayouts)
			{
				KVSection* layoutSec = vertexLayoutsSec->CreateSection(vertLayout.name);
				if (vertLayout.aliasOf != -1)
				{
					layoutSec->AddValue("aliasOf");
					layoutSec->AddValue(shaderInfo.vertexLayouts[vertLayout.aliasOf].name);
				}
			}
		}

		KVSection* fileListSec = shaderInfoKvs.CreateSection("FileList");

		int shaderFileCount = 0;
		Array<int> referenceRemap(PP_SL);
		referenceRemap.setNum(shaderInfo.results.numElem());

		// Store shader SPIR-V output in separate files
		for (int i = 0; i < shaderInfo.results.numElem(); ++i)
		{
			const ShaderInfo::Result& result = shaderInfo.results[i];
			const ShaderInfo::VertLayout& layout = shaderInfo.vertexLayouts[result.vertLayoutIdx];

			if (result.refResult != -1)
				continue;

			EqString shaderFileName = EqString::Format("%s-%s", layout.name.ToCString(), result.queryStr.ToCString());
			KVSection* spvSec = fileListSec->CreateSection("spv");
			spvSec->AddValue(result.vertLayoutIdx);
			
			if (result.kindFlag == SHADERKIND_VERTEX)
			{
				spvSec->AddValue("Vertex");
				shaderFileName.Append(".vert");
			}
			else if (result.kindFlag == SHADERKIND_FRAGMENT)
			{
				spvSec->AddValue("Fragment");
				shaderFileName.Append(".frag");
			}
			else if (result.kindFlag == SHADERKIND_COMPUTE)
			{
				spvSec->AddValue("Compute");
				shaderFileName.Append(".comp");
			}

			spvSec->AddValue(shaderInfo.entryPoints[result.entryPointId].name);
			spvSec->AddValue(result.queryStr);

			// Write shader bytecode file
			const uint32* shaderData = result.data.begin();
			const uint32 size = result.data.end() - shaderData;

			CMemoryStream readOnlyStream((ubyte*)shaderData, VS_OPEN_READ, size * sizeof(uint32), PP_SL);
			shaderPackFile.Add(&readOnlyStream, shaderFileName);

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
			KVSection* refSec = fileListSec->CreateSection("ref");
			refSec->AddValue(result.vertLayoutIdx);

			if (result.kindFlag == SHADERKIND_VERTEX)
				refSec->AddValue("Vertex");
			else if (result.kindFlag == SHADERKIND_FRAGMENT)
				refSec->AddValue("Fragment");
			else if (result.kindFlag == SHADERKIND_COMPUTE)
				refSec->AddValue("Compute");

			refSec->AddValue(shaderInfo.entryPoints[result.entryPointId].name);
			refSec->AddValue(result.queryStr);
			refSec->AddValue(referenceRemap[result.refResult]);
		}

		// put added files
		for (const ShaderInfo::AddFile& addFile : shaderInfo.addedFiles)
		{
			IFilePtr filePtr = g_fileSystem->Open(addFile.fileName, "r", SP_ROOT);
			shaderPackFile.Add(filePtr, addFile.values.back());

			KVSection* fileSec = fileListSec->CreateSection(addFile.values[0]);
			for(int i = 1; i < addFile.values.numElem(); ++i)
				fileSec->AddValue(addFile.values[i]);
		}

		CMemoryStream shaderInfoData(nullptr, VS_OPEN_WRITE, 8192, PP_SL);
		KV_WriteToStreamBinary(&shaderInfoData, &shaderInfoKvs);
		shaderPackFile.Add(&shaderInfoData, "ShaderInfo");

		shaderPackFile.End();
	}
}

bool CShaderCooker::Init(const char* confFileName, const char* targetName)
{
	// load all properties
	KeyValues kvs;

	if (!kvs.LoadFromFile(confFileName, SP_ROOT))
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
			const char* shadersSrc = KV_GetValueString(currentTarget->FindSection("SourcePath"), 0, nullptr);
			if (!shadersSrc)
			{
				MsgError("Target '%s' SourcePath folder is not specified!\n", targetName);
				return false;
			}

			const char* sourceImageExt = KV_GetValueString(currentTarget->FindSection("SourceExt"), 0, nullptr);

			if (!sourceImageExt)
			{
				MsgWarning("Target '%s' SourceExt is not specified, default to 'tga'\n", targetName);
				sourceImageExt = "tga";
			}

			for (const KVSection* includePathKey : currentTarget->Keys("includePath"))
			{
				EqString includePath = KV_GetValueString(includePathKey);
				includePath.ReplaceSubstr(s_engineDirTag, g_fileSystem->GetCurrentDataDirectory());
				includePath.ReplaceSubstr(s_gameDirTag, g_fileSystem->GetCurrentGameDirectory());

				m_targetProps.includePaths.append(std::move(includePath));
			}

			m_targetProps.sourceShaderPath = shadersSrc;
			m_targetProps.sourceShaderDescExt = _Es(sourceImageExt).TrimChar('.');

			m_targetProps.sourceShaderPath.ReplaceSubstr(s_engineDirTag, g_fileSystem->GetCurrentDataDirectory());
			m_targetProps.sourceShaderPath.ReplaceSubstr(s_gameDirTag, g_fileSystem->GetCurrentGameDirectory());
		}

		// target settings
		{
			const char* targetFolder = KV_GetValueString(currentTarget->FindSection("output"), 0, nullptr);
			if (!targetFolder)
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

	EqString searchTemplate;
	fnmPathCombine(searchTemplate, m_targetProps.sourceShaderPath, "*");

	// walk up shader files
	SearchFolderForShaders(searchTemplate);

	int totalVariationCount = 0;
	for (ShaderInfo& shaderInfo : m_shaderList)
		totalVariationCount += shaderInfo.totalVariationCount;

	Msg("Got %d shaders %d variations total\n", m_shaderList.numElem(), totalVariationCount);

	EqString crcFileName(EqString::Format("%s/shaders_crc.txt", m_targetProps.targetFolder.ToCString()));

	// load CRC list, check for existing shader files, and skip if necessary
	KV_LoadFromFile(crcFileName, SP_ROOT, &m_batchConfig.crcSec);

	// process shader files
	
	for (ShaderInfo& shaderInfo : m_shaderList)
	{	
		ProcessShader(shaderInfo);
	}

	// save CRC list file
	IFilePtr pStream = g_fileSystem->Open(crcFileName, "wt", SP_ROOT);
	if (pStream)
		KV_WriteToStream(pStream, &m_batchConfig.newCRCSec, 0, true);
}

void CookTarget(const char* pszTargetName, CEqJobManager& jobMng)
{
	CShaderCooker cooker(jobMng);
	if (!cooker.Init("ShaderCooker.CONFIG", pszTargetName))
		return;

	cooker.Execute();
}