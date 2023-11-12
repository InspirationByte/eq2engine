//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2023
//////////////////////////////////////////////////////////////////////////////////
// Description: Shader module compiler
//////////////////////////////////////////////////////////////////////////////////

#include <shaderc/shaderc.hpp>
#include "core/core_common.h"
#include "core/IFileSystem.h"
#include "core/IEqParallelJobs.h"
#include "ds/MemoryStream.h"
#include "utils/KeyValues.h"
#include "dpk/DPKFileWriter.h"

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

enum EShaderKindFlags
{
	SHADERKIND_VERTEX	= (1 << 0),
	SHADERKIND_FRAGMENT	= (1 << 1),
	SHADERKIND_COMPUTE	= (1 << 2),
};

enum EShaderConvStatus
{
	SHADERCONV_INIT = 0,
	SHADERCONV_CRC_LOADED,
	SHADERCONV_COMPILED,
	SHADERCONV_FAILED,
	SHADERCONV_SKIPPED
};

enum EShaderVariantType
{
	SHADERVARIANT_RENDERPASS = 0,	// separate render pass
	SHADERVARIANT_MATVAR,			// definition
};

enum EShaderSourceType
{
	SHADERSOURCE_UNDEFINED,
	SHADERSOURCE_HLSL,
	SHADERSOURCE_GLSL,
	//SHADERSOURCE_WGSL,
};

static shaderc_source_language s_sourceLanguage[] = {
	shaderc_source_language_hlsl,
	shaderc_source_language_glsl,
};

struct ShaderInfo
{
	struct VertLayout
	{
		EqString name;
		int aliasOf{ -1 };
	};
	struct Variant
	{
		EqString			name;
		EShaderVariantType	type{ SHADERVARIANT_RENDERPASS };
		int					baseVariant{ -1 };
		Array<EqString>		defines{ PP_SL };
	};

	struct Result
	{
		shaderc::SpvCompilationResult data;
		EqString		queryStr;
		int				refResult{ -1 };
		int				vertLayoutIdx{ -1 };
		int				kindFlag{ -1 };
		uint32			crc32{ 0 };
	};
	Array<Result>		results{ PP_SL };
	Array<VertLayout>	vertexLayouts{ PP_SL };
	Array<Variant>		variants{ PP_SL };

	EqString			name;

	EqString			sourceFilename;
	EShaderSourceType	sourceType;

	uint32				crc32{ 0 };
	int					totalVariationCount{ 0 };
	
	int					kind{ 0 };	// EShaderKindFlags
	EShaderConvStatus	status{ SHADERCONV_INIT };
};

static const char s_boilerPlateStrGLSL[] = R"(

// functions
#define     frac        fract
#define     lerp        mix
#define     saturate(x) clamp(x, 0.0, 1.0)
#define     mul(a,b)    ((a)*(b))

float fmod(float x, float y) { return x - y * floor(x / y); }

#ifdef VERTEX
#	define clip(x)	(x)
#	define ddx(x)	(x)
#	define ddy(x)	(x)
#endif

#ifdef FRAGMENT
#	define clip(x)	if((x) < 0.0) discard
#	define ddx    	dFdx
#	define ddy    	dFdy
#endif

// See BaseShader and shaders layouts
#define MATERIAL_BINDING( N )	layout(set = 1, binding = N) 
#define MATSYS_BINDING( N )		layout(set = 0, binding = N) 

)";

class EqShaderIncluder: public shaderc::CompileOptions::IncluderInterface
{
public:
	virtual shaderc_include_result* GetInclude(
		const char* requested_source, shaderc_include_type type,
		const char* requesting_source, size_t include_depth)
	{
		const EqString sourcePath = EqString(requesting_source).Path_Extract_Path();
		EqString shaderSourceName = requested_source;

		IncludeResult* result = nullptr;
		if (m_freeSlots.numElem())
			result = &m_shaderIncludes[m_freeSlots.popBack()];
		else
			result = &m_shaderIncludes.append();

		if (type == shaderc_include_type_relative)
		{
			CombinePath(shaderSourceName, sourcePath, requested_source);

			IFilePtr file = g_fileSystem->Open(shaderSourceName, "r", SP_ROOT);
			if (!file)
			{
				result->includeContent.Open(nullptr, VS_OPEN_READ | VS_OPEN_WRITE, 8192);
				result->includeContent.Print("Could not open %s", shaderSourceName.ToCString());
				result->resultData.content = (const char*)result->includeContent.GetBasePointer();
				result->resultData.content_length = result->includeContent.GetSize();
				return &result->resultData;
			}
			result->includeName = shaderSourceName;
			result->includeContent.Open(nullptr, VS_OPEN_READ | VS_OPEN_WRITE, file->GetSize());
			result->includeContent.AppendStream(file);
		}
		else
		{
			result->includeContent.Close();
			result->includeContent.Open(nullptr, VS_OPEN_READ | VS_OPEN_WRITE, 8192);

			if (!shaderSourceName.CompareCaseIns("ShaderCooker"))
			{
				result->includeContent.Print(s_boilerPlateStrGLSL);
				result->includeName = shaderSourceName;
			}
			else if (!shaderSourceName.CompareCaseIns("VertexLayout"))
			{
				CombinePath(shaderSourceName, sourcePath, "VertexLayouts", m_vertexLayoutName + ".h");
				IFilePtr file = g_fileSystem->Open(shaderSourceName, "r", SP_ROOT);
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
				result->includeContent.AppendStream(file);
			}
		}
		result->resultData.content = (const char*)result->includeContent.GetBasePointer();
		result->resultData.content_length = result->includeContent.GetSize();
		result->resultData.source_name = result->includeName;
		result->resultData.source_name_length = result->includeName.Length();
		return &result->resultData;
	}

	// Handles shaderc_include_result_release_fn callbacks.
	void ReleaseInclude(shaderc_include_result* data)
	{
		IncludeResult* incRes = reinterpret_cast<IncludeResult*>(data);
		const int index = incRes - m_shaderIncludes.ptr();
		memset(data, 0, sizeof(*data));

		incRes->includeName.Clear();
		incRes->includeContent.Close();

		m_freeSlots.append(index);
	}

	void SetShaderInfo(const ShaderInfo& shaderInfo)
	{
		m_shaderInfo = &shaderInfo;
	}

	void SetVertexLayout(const char* vertexLayoutName)
	{
		m_vertexLayoutName = vertexLayoutName;
	}

private:

	struct IncludeResult
	{
		shaderc_include_result	resultData;
		EqString				includeName;
		CMemoryStream			includeContent{ PP_SL };
	};

	FixedArray<IncludeResult, 32>	m_shaderIncludes;
	FixedArray<int, 32>		m_freeSlots;
	const ShaderInfo*		m_shaderInfo{ nullptr };
	EqString				m_vertexLayoutName;
};

//-------------------------------------

class CShaderCooker
{
public:

	bool				Init(const char* confFileName, const char* targetName);
	void				Execute();
private:
	void				SearchFolderForShaders(const char* wildcard);
	bool				HasMatchingCRC(uint32 crc);

	void				InitShaderVariants(ShaderInfo& shaderInfo, int baseVariant, const KVSection* section);
	void				ProcessShader(ShaderInfo& shaderInfo);

	struct BatchConfig
	{
		KVSection	crcSec;			// crc list loaded from disk
		KVSection	newCRCSec;		// crc list that will be saved
	};

	struct TargetProperties
	{
		EqString	sourceShaderPath;
		EqString	sourceShaderDescExt;
		EqString	targetFolder;
	};

	BatchConfig			m_batchConfig;
	TargetProperties	m_targetProps;

	Array<ShaderInfo>	m_shaderList{ PP_SL };
};

//-----------------------------------------------------------------------

void CShaderCooker::SearchFolderForShaders(const char* wildcard)
{
	EqString searchFolder(wildcard);
	searchFolder.ReplaceSubstr("*", "");

	CFileSystemFind fsFind(wildcard, SP_ROOT);
	while (fsFind.Next())
	{
		EqString fileName = fsFind.GetPath();
		if (fsFind.IsDirectory() && fileName != "." && fileName != "..")
		{
			EqString searchTemplate;
			CombinePath(searchTemplate, searchFolder, fileName, "*");

			SearchFolderForShaders(searchTemplate);
		}
		else if (fileName.Path_Extract_Ext().LowerCase() == m_targetProps.sourceShaderDescExt.LowerCase())
		{
			EqString fullShaderPath;
			CombinePath(fullShaderPath, searchFolder, fileName);

			KVSection rootSec;
			if (!KV_LoadFromFile(fullShaderPath, SP_ROOT, &rootSec))
				continue;

			const KVSection* shaderSection = rootSec.FindSection("shader");
			if (!shaderSection)
			{
				MsgWarning("%s does not describe shader or section 'shader' is missing.", fullShaderPath.ToCString());
				continue;
			}

			const char* shaderFileName = KV_GetValueString(shaderSection->FindSection("SourceFile"), 0, nullptr);
			if (!shaderFileName)
			{
				MsgWarning("%s missing 'SourceFile'", fullShaderPath.ToCString());
				continue;
			}

			const KVSection* kinds = shaderSection->FindSection("SourceKind");
			if (!kinds)
			{
				MsgWarning("%s missing 'SourceKind' section", fullShaderPath.ToCString());
				continue;
			}

			int shaderKind = 0;
			for (KVKeyIterator keysIter(kinds); !keysIter.atEnd(); ++keysIter)
			{
				if (!stricmp(keysIter, "Vertex"))
					shaderKind |= SHADERKIND_VERTEX;
				else if (!stricmp(keysIter, "Fragment"))
					shaderKind |= SHADERKIND_FRAGMENT;
				else if (!stricmp(keysIter, "Compute"))
					shaderKind |= SHADERKIND_COMPUTE;
			}

			if ((shaderKind & SHADERKIND_COMPUTE) == 0 && (shaderKind & SHADERKIND_VERTEX) == 0)
			{
				MsgWarning("%s must have Vertex kind section if it doesn't serve Compute", fullShaderPath.ToCString());
				continue;
			}

			ShaderInfo& shaderInfo = m_shaderList.append();
			shaderInfo.crc32 = g_fileSystem->GetFileCRC32(fullShaderPath, SP_ROOT);
			shaderInfo.name = KV_GetValueString(shaderSection);
			shaderInfo.sourceFilename = shaderFileName;
			shaderInfo.kind = shaderKind;

			const char* shaderType = KV_GetValueString(shaderSection->FindSection("SourceType"), 0, nullptr);
			if (!stricmp(shaderType, "hlsl"))
				shaderInfo.sourceType = SHADERSOURCE_HLSL;
			else if (!stricmp(shaderType, "glsl"))
				shaderInfo.sourceType = SHADERSOURCE_GLSL;
			//else if (!stricmp(shaderType, "wgsl"))
			//	shaderInfo.sourceType = SHADERSOURCE_WGSL;

			InitShaderVariants(shaderInfo, -1, shaderSection);

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
		}
	}
}

bool CShaderCooker::HasMatchingCRC(uint32 crc)
{
	for (int i = 0; i < m_batchConfig.crcSec.keys.numElem(); i++)
	{
		uint32 checkCRC = strtoul(m_batchConfig.crcSec.keys[i]->name, nullptr, 10);

		if (checkCRC == crc)
			return true;
	}

	return false;
}

void CShaderCooker::InitShaderVariants(ShaderInfo& shaderInfo, int baseVariantIdx, const KVSection* section)
{
	bool hasVariantsThisLevel = false;
	for (const KVSection* nestedSec : section->keys)
	{
		if (!stricmp(nestedSec->GetName(), "define")
			|| !stricmp(nestedSec->GetName(), "VertexLayouts"))
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
	for (const KVSection* nestedSec : section->keys)
	{
		if (!stricmp(nestedSec->GetName(), "define"))
		{
			// TODO: define types
			variant.defines.append(KV_GetValueString(nestedSec));
		}
		else if (!stricmp(nestedSec->GetName(), "VertexLayouts"))
		{
			// add vertex layouts
			for (const KVSection* layoutKey : nestedSec->keys)
			{
				int aliasLayout = -1;
				if (!stricmp(KV_GetValueString(layoutKey, 0), "aliasOf"))
				{
					const char* aliasOfStr = KV_GetValueString(layoutKey, 1);
					aliasLayout = arrayFindIndexF(shaderInfo.vertexLayouts, [aliasOfStr](const ShaderInfo::VertLayout& layout) {
						return layout.name == aliasOfStr;
					});
					if (aliasLayout == -1)
					{
						MsgError("%s - vertex layout %s for 'aliasOf' not found\n", shaderInfo.name.ToCString(), aliasOfStr);
					}
				}
				shaderInfo.vertexLayouts.append({ layoutKey->GetName(), aliasLayout });
			}
		}
		else if (!stricmp(nestedSec->GetName(), "UniformLayout"))
		{
			// atm skip
		}
		else
			InitShaderVariants(shaderInfo, thisVariantIndex, nestedSec);
	}
}

void CShaderCooker::ProcessShader(ShaderInfo& shaderInfo)
{
	EqString shaderSourceName;
	CombinePath(shaderSourceName, m_targetProps.sourceShaderPath.Path_Extract_Path().ToCString(), shaderInfo.sourceFilename.ToCString());

	CMemoryStream shaderSourceString(PP_SL);
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
	uint32 srcCRC = shaderInfo.crc32;
	CRC32_UpdateChecksum(srcCRC, shaderSourceString.GetBasePointer(), shaderSourceString.GetSize());

	// store new CRC
	m_batchConfig.newCRCSec.SetKey(EqString::Format("%u", srcCRC), shaderInfo.sourceFilename);

	EqString targetFileName;
	CombinePath(targetFileName, m_targetProps.targetFolder, EqString::Format("%s.shd", shaderInfo.name.ToCString()));

	// now check CRC from loaded file
	if (HasMatchingCRC(srcCRC))
	{
		// check if  output exists
		if (g_fileSystem->FileExist(targetFileName, SP_ROOT))
		{
			MsgInfo("Skipping shader '%s' (no changes made)\n", shaderInfo.name.ToCString());
			return;
		}
	}

	if (shaderInfo.vertexLayouts.numElem() == 0)
	{
		MsgError("Shader '%s' has no vertex layouts defined, skipping...\n", shaderInfo.name.ToCString());
		return;
	}

	// collect all defines into flat list
	int numDefines = 0;
	Array<EqString> switchDefines(PP_SL);

	for (const ShaderInfo::Variant& variant : shaderInfo.variants)
	{
		if(variant.baseVariant != -1)
			switchDefines.append(variant.defines);
	}

	MsgWarning("Processing shader %s (%d vertex layouts %d defines)\n", shaderInfo.name.ToCString(), shaderInfo.vertexLayouts.numElem(), switchDefines.numElem());

	const int totalVariantCount = 1 << switchDefines.numElem();

	// reserve variant count * (vertex + fragment)
	shaderInfo.results.reserve(shaderInfo.vertexLayouts.numElem() * totalVariantCount * 2);

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

		bool stopCompilation = false;
		for (int i = 0; i < totalVariantCount; ++i)
		{
			g_parallelJobs->AddJob(JOB_TYPE_ANY, 
				[&resultsMutex, &shaderSourceString, &shaderSourceName, &stopCompilation, &shaderInfo, vertexLayout, &switchDefines, nSwitch = i, vertLayoutIdx](void*, int) {
				EqString queryStr;
				for (int j = 0; j < switchDefines.numElem(); ++j)
				{
					if (nSwitch & (1 << j))
					{
						if (queryStr.Length())
							queryStr.Append("|");
						queryStr.Append(switchDefines[j]);
					}
				}

				shaderc::Compiler compiler;
				auto fillMacros = [&queryStr, &switchDefines, nSwitch](shaderc::CompileOptions& options) {
					for (int j = 0; j < switchDefines.numElem(); ++j)
					{
						if (nSwitch & (1 << j))
							options.AddMacroDefinition(switchDefines[j], switchDefines[j].Length(), nullptr, 0u);
					}
				};

				auto compileShaderKind = [&](const char* kindStr, int kindFlag, shaderc_shader_kind shaderCKind)
				{
					std::unique_ptr<EqShaderIncluder> includer = std::make_unique<EqShaderIncluder>();
					includer->SetShaderInfo(shaderInfo);
					includer->SetVertexLayout(vertexLayout.name);

					shaderc::CompileOptions options;
					options.SetSourceLanguage(s_sourceLanguage[shaderInfo.sourceType]);
					options.SetOptimizationLevel(shaderc_optimization_level_performance);
					options.SetIncluder(std::move(includer));
					options.AddMacroDefinition(kindStr);
					options.SetForcedVersionProfile(450, shaderc_profile_none);
					options.SetTargetEnvironment(shaderc_target_env_webgpu, shaderc_env_version_webgpu);
					fillMacros(options);

					shaderc::SpvCompilationResult compilationResult = compiler.CompileGlslToSpv((const char*)shaderSourceString.GetBasePointer(), shaderSourceString.GetSize(), shaderCKind, shaderSourceName, options);
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
							ASSERT_MSG(kindFlag == shaderInfo.results[refIdx].kindFlag, "Referenced shader kind is invalid");
							result.refResult = refIdx;
						}

						result.queryStr = queryStr;
						result.vertLayoutIdx = vertLayoutIdx;
						result.kindFlag = kindFlag;
						result.crc32 = resultCRC;
					}
					else
					{
						MsgError("Failed compiling %s\n%s\n", queryStr.ToCString(), compilationResult.GetErrorMessage().c_str());
						if (compileStatus == shaderc_compilation_status_compilation_error)
							stopCompilation = true;
					}
				};

				if (!stopCompilation && (shaderInfo.kind & SHADERKIND_VERTEX))
					compileShaderKind("VERTEX", SHADERKIND_VERTEX, shaderc_vertex_shader);

				if (!stopCompilation && (shaderInfo.kind & SHADERKIND_FRAGMENT))
					compileShaderKind("FRAGMENT", SHADERKIND_FRAGMENT, shaderc_fragment_shader);

				if (!stopCompilation && (shaderInfo.kind & SHADERKIND_COMPUTE))
					compileShaderKind("COMPUTE", SHADERKIND_COMPUTE, shaderc_compute_shader);

				//if(!stopCompilation)
				//	Msg("   - compiled variant '%s' (%d)\n", queryStr.ToCString(), nSwitch);
			});
			if (stopCompilation)
				break;
		}

		g_parallelJobs->Submit();

		//if (stopCompilation)
		//	break;
	}
	g_parallelJobs->Wait();

	// Store files
	if (shaderInfo.results.numElem())
	{
		CDPKFileWriter shaderPackFile("shaders", 4);
		if (!shaderPackFile.Begin(targetFileName))
		{
			MsgError("Unable to create pack file %s\n", targetFileName.ToCString());
			return;
		}

		// Store shader info
		KVSection shaderInfoKvs;
		{
			KVSection* definesSec = shaderInfoKvs.CreateSection("Defines");
			for (EqString& defineStr : switchDefines)
				definesSec->AddValue(defineStr);
		}

		// store shader kinds
		{
			KVSection* definesSec = shaderInfoKvs.CreateSection("ShaderKinds");
			if(shaderInfo.kind & SHADERKIND_VERTEX)
				definesSec->AddValue("Vertex");

			if (shaderInfo.kind & SHADERKIND_FRAGMENT)
				definesSec->AddValue("Fragment");

			if (shaderInfo.kind & SHADERKIND_COMPUTE)
				definesSec->AddValue("Compute");
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

		KVSection* shadersListSec = shaderInfoKvs.CreateSection("FileList");

		// Store shader SPIR-V output in separate files
		for (const ShaderInfo::Result& result : shaderInfo.results)
		{
			const uint32* shaderData = result.data.begin();
			const uint32 size = result.data.end() - shaderData;
			
			const ShaderInfo::VertLayout& layout = shaderInfo.vertexLayouts[result.vertLayoutIdx];

			EqString shaderFileName = EqString::Format("%s-%s", layout.name.ToCString(), result.queryStr.ToCString());
			if (result.kindFlag == SHADERKIND_VERTEX)
				shaderFileName.Append(".vert");
			else if (result.kindFlag == SHADERKIND_FRAGMENT)
				shaderFileName.Append(".frag");
			else if (result.kindFlag == SHADERKIND_COMPUTE)
				shaderFileName.Append(".comp");

			if (size == 0)
			{
				ASSERT_MSG(result.refResult != -1, "Something went wrong, got empty shader and no reference id");

				// Reference shader bytecode file
				{
					KVSection* refSec = shadersListSec->CreateSection("ref");
					refSec->AddValue(result.vertLayoutIdx);

					if (result.kindFlag == SHADERKIND_VERTEX)
						refSec->AddValue("Vertex");
					else if (result.kindFlag == SHADERKIND_FRAGMENT)
						refSec->AddValue("Fragment");
					else if (result.kindFlag == SHADERKIND_COMPUTE)
						refSec->AddValue("Compute");

					refSec->AddValue(result.queryStr);
					refSec->AddValue(result.refResult);
				}

				continue;
			}

			{
				KVSection* spvSec = shadersListSec->CreateSection("spv");
				spvSec->AddValue(result.vertLayoutIdx);

				if (result.kindFlag == SHADERKIND_VERTEX)
					spvSec->AddValue("Vertex");
				else if (result.kindFlag == SHADERKIND_FRAGMENT)
					spvSec->AddValue("Fragment");
				else if (result.kindFlag == SHADERKIND_COMPUTE)
					spvSec->AddValue("Compute");

				spvSec->AddValue(result.queryStr);
				spvSec->AddValue(shaderFileName);
			}

			// Write shader bytecode file
			CMemoryStream readOnlyStream((ubyte*)shaderData, VS_OPEN_READ, size * sizeof(uint32), PP_SL);
			shaderPackFile.Add(&readOnlyStream, shaderFileName);
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
		const KVSection* targets = kvs.FindSection("Targets");
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

			m_targetProps.sourceShaderPath = shadersSrc;
			m_targetProps.sourceShaderDescExt = _Es(sourceImageExt).TrimChar('.');
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
		}
	}

	return true;
}

void CShaderCooker::Execute()
{
	// perform batch conversion
	Msg("Shader source path: '%s'\n", m_targetProps.sourceShaderPath.ToCString());

	EqString searchTemplate;
	CombinePath(searchTemplate, m_targetProps.sourceShaderPath.ToCString(), "*");

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

void CookTarget(const char* pszTargetName)
{
	CShaderCooker cooker;
	if (!cooker.Init("ShaderCooker.CONFIG", pszTargetName))
		return;

	cooker.Execute();
}