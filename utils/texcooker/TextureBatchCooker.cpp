//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Atlas packer - main code
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IFileSystem.h"
#include "imaging/ImageLoader.h"
#include "imaging/PixWriter.h"
#include "utils/KeyValues.h"

constexpr EqStringRef s_atlasFileExt = "atl";
constexpr EqStringRef s_materialFileExt = "mat";

extern void ProcessAtlasFile(const char* atlasSrcFileName, const char* materialsPath);

/*

// Configuration structure

// batching configuration
BatchConfig
{
	// application to run
	application "RwgTex.exe";

	// %ARGS% - additional arguments applied by usage aliases
	// %INPUT_FILENAME% - input file name
	// %OUTPUT_FILEPATH% - output file path
	arguments "%ARGS%";

	compression "<CompressionName>"
	{
		application "otherConverter.exe";
		arguments "-etcpack %INPUT_FILENAME% %OUTPUT_FILEPATH%";

		usage default
		{
			sourcepath 	"./0_materials_src/";	// soruce folder
			sourceext	"tga";
			arguments "-etc1";
		}

		usage <UsageName>		// in material: basetexture "texturepath" "usage:<UsageName>"
		{
			sourcepath 	"./0_materials_src/";	// soruce folder
			sourceext	"tga";
			arguments "-etc1";
		}
	}
}

//--------------------------------------------------------------
// Targets down below

Targets
{
	"<TargetName>"
	{
		compression		"<CompressionName>";
		output			"./materials_pc";		// target output folder
	}
}
*/

static const EqString s_argumentsTag("%ARGS%");
static const EqString s_inputFileNameTag("%INPUT_FILENAME%");
static const EqString s_outputFileNameTag("%OUTPUT_FILENAME%");
static const EqString s_outputFilePathTag("%OUTPUT_FILEPATH%");
static const EqString s_engineDirTag("%ENGINE_DIR%");
static const EqString s_gameDirTag("%GAME_DIR%");

static const char* s_textureValueIdentifier = "usage:";

enum ETexConvStatus
{
	INIT_STATE = 0,
	CRC_LOADED,
	CONVERTED,
	SKIPPED
};

struct UsageProperties
{
	EqString usageName;

	EqString applicationName;
	EqString applicationArguments;
};

struct BatchConfig
{
	EqString		applicationName; // can be overridden by UsageProperties::applicationName
	EqString		applicationArgumentsTemplate;

	EqString		compressionApplicationArguments;

	UsageProperties defaultUsage{ "default" };
	Array<UsageProperties> usageList{ PP_SL };

	KVSection		crcSec;			// crc list loaded from disk
	KVSection		newCRCSec;		// crc list that will be saved
};

struct TargetProperties
{
	EqString	sourceMaterialPath;
	EqString	sourceImageExt;
	EqString	targetCompression;
	EqString	targetFolder;
};

// CRC pairs
struct TexInfo
{
	EqString			sourcePath;
	UsageProperties*	usage{ nullptr };
	uint32				crc32{ 0 };
	ETexConvStatus		status{ INIT_STATE };
};

class CTextureCooker
{
public:
	bool				Init(const char* confFileName, const char* targetName);
	void				Execute();

private:
	void				LoadBatchConfig(const KVSection* batchSec);
	bool				AddTexture(const EqString& texturePath, const EqString& imageUsage);
	void				LoadMaterialImages(const char* materialFileName);

	void				SearchFolderForMaterialsAndGetTextures(const char* wildcard);
	void				SearchFolderForAtlasesAndConvert(const char* wildcard);

	bool				HasMatchingCRC(uint32 crc);
	void				ProcessMaterial(const EqString& materialFileName);
	void				ProcessTexture(TexInfo& textureInfo);
	UsageProperties*	FindUsage(const char* usageName);

	BatchConfig			m_batchConfig;
	TargetProperties	m_targetProps;

	Array<TexInfo>		m_textureList{ PP_SL };
	Array<EqString>		m_materialList{ PP_SL };
};

//-----------------------------------------------------------------------

UsageProperties* CTextureCooker::FindUsage(const char* usageName)
{
	for (int i = 0; i < m_batchConfig.usageList.numElem(); i++)
	{
		UsageProperties& usage = m_batchConfig.usageList[i];
		if (!usage.usageName.CompareCaseIns(usageName))
			return &usage;
	}

	return &m_batchConfig.defaultUsage;
}


void CTextureCooker::LoadBatchConfig(const KVSection* batchSec)
{
	// retrieve application name and arguments
	{
		const char* appName = KV_GetValueString(batchSec->FindSection("application"), 0, nullptr);
		const char* appArguments = KV_GetValueString(batchSec->FindSection("arguments"), 0, nullptr);

		m_batchConfig.applicationName = appName;
		m_batchConfig.applicationArgumentsTemplate = appArguments;
	}

	const KVSection* compressionSec = nullptr;

	for (int i = 0; i < batchSec->keys.numElem(); i++)
	{
		const KVSection* sec = batchSec->keys[i];

		if (!stricmp(sec->name, "compression") && !stricmp(KV_GetValueString(sec, 0, "INVALID"), m_targetProps.targetCompression))
		{
			compressionSec = sec;
			break;
		}
	}

	if (!compressionSec)
	{
		MsgError("Unknown compression preset '%s', check your BatchConfig section\n", m_targetProps.targetCompression.ToCString());
		return;
	}

	m_batchConfig.applicationName = KV_GetValueString(compressionSec->FindSection("application"), 0, m_batchConfig.applicationName);
	m_batchConfig.compressionApplicationArguments = KV_GetValueString(compressionSec->FindSection("arguments"), 0, "");

	// load usages
	for (int i = 0; i < compressionSec->keys.numElem(); i++)
	{
		const KVSection* usageKey = compressionSec->keys[i];

		if (stricmp(usageKey->name, "usage"))
			continue;

		const char* usageName = KV_GetValueString(usageKey, 0, nullptr);

		if (!usageName)
		{
			MsgWarning("Usage name not specified (in it's value)\n");
			continue;
		}

		UsageProperties usage;
		usage.usageName = usageName;
		usage.applicationName = KV_GetValueString(usageKey->FindSection("application"), 0, m_batchConfig.applicationName);
		usage.applicationArguments = KV_GetValueString(usageKey->FindSection("arguments"), 0, "");

		if (!stricmp(usageName, "default"))
			m_batchConfig.defaultUsage = usage;
		else
			m_batchConfig.usageList.append(usage);
	}
}

bool CTextureCooker::AddTexture(const EqString& texturePath, const EqString& imageUsage)
{
	EqString filename;
	CombinePath(filename, m_targetProps.sourceMaterialPath, EqString::Format("%s.%s", texturePath.ToCString(), m_targetProps.sourceImageExt.ToCString()));

	if (!g_fileSystem->FileExist(filename))
	{
		MsgError("  - texture '%s' does not exists!\n", filename.ToCString());
		return false;
	}

	TexInfo& newInfo = m_textureList.append();
	
	newInfo.sourcePath = texturePath;
	newInfo.usage = FindUsage(imageUsage);

	if (newInfo.usage == &m_batchConfig.defaultUsage)
	{
		MsgWarning("%s: invalid usage '%s'\n", filename, imageUsage);
	}

	return true;
}

void CTextureCooker::LoadMaterialImages(const char* materialFileName)
{
	KeyValues kvs;
	if (!kvs.LoadFromFile(materialFileName, SP_ROOT))
		return;

	EqString localMaterialFileName = materialFileName + m_targetProps.sourceMaterialPath.Length();
	localMaterialFileName = localMaterialFileName.TrimChar(CORRECT_PATH_SEPARATOR).TrimChar(INCORRECT_PATH_SEPARATOR);

	if (kvs.GetRootSection()->KeyCount() == 0)
	{
		MsgError("'%s' is not valid material file\n", localMaterialFileName.ToCString());
		return;
	}

	const KVSection* kvMaterial = kvs.GetRootSection()->keys[0];
	if (!kvMaterial->IsSection())
	{
		MsgError("'%s' is not valid material file\n", localMaterialFileName.ToCString());
		return;
	}

	MsgInfo("Material: '%s'\n", localMaterialFileName.ToCString());
	m_materialList.append(localMaterialFileName);

	int textures = 0;

	for (int i = 0; i < kvMaterial->keys.numElem(); i++)
	{
		bool keyHasUsage = false;
		const KVSection* key = kvMaterial->keys[i];
		for (int j = 1; j < key->ValueCount(); j++)
		{
			EqString imageUsage(KV_GetValueString(key, j, ""));
			const int usageIdx = imageUsage.ReplaceSubstr(s_textureValueIdentifier, "");

			if (usageIdx == -1)
			{
				continue;
			}

			if (keyHasUsage)
			{
				MsgWarning("  - material %s texture '%s' has multiple usages! Only one is supported", localMaterialFileName.ToCString(), key->GetName());
				continue;
			}

			keyHasUsage = true;

			EqString texturePath = KV_GetValueString(key, 0, "");

			// has pattern for animated texture?
			int animCountStart = texturePath.Find("[");
			int animCountEnd = -1;

			if (animCountStart != -1 &&
				(animCountEnd = texturePath.Find("]", false, animCountStart)) != -1)
			{
				// trying to load animated texture
				EqString textureWildcard = texturePath.Left(animCountStart);
				EqString textureFrameCount = texturePath.Mid(animCountStart + 1, (animCountEnd - animCountStart) - 1);
				int numFrames = atoi(textureFrameCount);

				for (int i = 0; i < numFrames; i++)
				{
					EqString textureNameFrame = EqString::Format(textureWildcard.ToCString(), i);
					if(AddTexture(textureNameFrame, imageUsage))
						textures++;
				}
			}
			else
			{
				if(AddTexture(texturePath, imageUsage))
					textures++;
			}
		}
	}

	if (!textures)
	{
		MsgWarning("  - no textures added!\n");
		return;
	}
		
	// make folder structure and clone material file

}

void CTextureCooker::SearchFolderForAtlasesAndConvert(const char* wildcard)
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
			CombinePath(searchTemplate, searchFolder, fileName, "*");

			SearchFolderForAtlasesAndConvert(searchTemplate);
		}
		else if(fnmPathExtractExt(fileName) == s_atlasFileExt)
		{
			EqString fullAtlPath;
			CombinePath(fullAtlPath, searchFolder, fileName);
			
			ProcessAtlasFile(fullAtlPath, m_targetProps.sourceMaterialPath);
		}
	}
}

void CTextureCooker::SearchFolderForMaterialsAndGetTextures(const char* wildcard)
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
			CombinePath(searchTemplate, searchFolder, fileName, "*");

			SearchFolderForMaterialsAndGetTextures(searchTemplate);
		}
		else if(fnmPathExtractExt(fileName) == s_materialFileExt)
		{
			EqString fullMaterialPath;
			CombinePath(fullMaterialPath, searchFolder, fileName);
			LoadMaterialImages(fullMaterialPath);
		}
	}
}

bool CTextureCooker::HasMatchingCRC(uint32 crc)
{
	for (int i = 0; i < m_batchConfig.crcSec.keys.numElem(); i++)
	{
		uint32 checkCRC = strtoul(m_batchConfig.crcSec.keys[i]->name, nullptr, 10);

		if (checkCRC == crc)
			return true;
	}

	return false;
}

void CTextureCooker::ProcessMaterial(const EqString& materialFileName)
{
	const EqString atlasFileName = fnmPathStripExt(materialFileName) + ".atlas";

	EqString sourceMaterialFileName;
	CombinePath(sourceMaterialFileName, m_targetProps.sourceMaterialPath, materialFileName);

	EqString sourceAtlasFileName;
	CombinePath(sourceAtlasFileName, m_targetProps.sourceMaterialPath, atlasFileName);

	EqString targetMaterialFileName;
	CombinePath(targetMaterialFileName, m_targetProps.targetFolder, materialFileName);

	EqString targetAtlasFileName;
	CombinePath(targetAtlasFileName, m_targetProps.targetFolder, atlasFileName);

	// make target material file path
	g_fileSystem->MakeDir(fnmPathStripName(targetMaterialFileName), SP_ROOT);

	// copy material file
	if (!g_fileSystem->FileExist(targetMaterialFileName, SP_ROOT))
	{
		// save material file
		if (!g_fileSystem->FileCopy(sourceMaterialFileName, targetMaterialFileName, true, SP_ROOT))
		{
			MsgWarning("  - cannot copy material file!\n");
		}
	}

	// also copy atlas file
	if (!g_fileSystem->FileExist(targetAtlasFileName, SP_ROOT) &&
		g_fileSystem->FileExist(sourceAtlasFileName, SP_ROOT))
	{
		if (!g_fileSystem->FileCopy(sourceAtlasFileName, targetAtlasFileName, true, SP_ROOT))
		{
			MsgWarning("  - cannot copy atlas file!\n");
		}
	}
}

void CTextureCooker::ProcessTexture(TexInfo& textureInfo)
{
	// before this, create folders...
	EqString sourceFilename;
	CombinePath(sourceFilename, m_targetProps.sourceMaterialPath, EqString::Format("%s.%s", textureInfo.sourcePath.ToCString(), m_targetProps.sourceImageExt.ToCString()));
	
	EqString targetFilename;
	CombinePath(targetFilename, m_targetProps.targetFolder, fnmPathStripExt(textureInfo.sourcePath) + ".dds");
	
	const EqString targetFilePath = fnmPathStripName(targetFilename).TrimChar(CORRECT_PATH_SEPARATOR);

	// make image folder
	g_fileSystem->MakeDir(targetFilePath, SP_ROOT);

	EqString arguments(m_batchConfig.applicationArgumentsTemplate);
	arguments.ReplaceSubstr(s_argumentsTag, (m_batchConfig.compressionApplicationArguments + " " + textureInfo.usage->applicationArguments));
	arguments.ReplaceSubstr(s_inputFileNameTag, sourceFilename);
	arguments.ReplaceSubstr(s_outputFilePathTag, targetFilePath);

	// generate CRC from image file content and arguments it's going to be built
	uint32 srcCRC = g_fileSystem->GetFileCRC32(sourceFilename, SP_ROOT);
	CRC32_UpdateChecksum(srcCRC, arguments.ToCString(), arguments.Length());

	// store new CRC
	m_batchConfig.newCRCSec.SetKey(EqString::Format("%u", srcCRC), sourceFilename);

	// now check CRC from loaded file
	if (HasMatchingCRC(srcCRC))
	{
		if (g_fileSystem->FileExist(targetFilename, SP_ROOT))
		{
			MsgInfo("Skipping %s: %s...\n", textureInfo.usage->usageName.ToCString(), textureInfo.sourcePath.ToCString());
			textureInfo.status = SKIPPED;
			return;
		}
		else
		{
			MsgInfo("Re-generating '%s'\n", targetFilename.ToCString());
		}
	}

	textureInfo.status = CONVERTED;

	MsgInfo("Processing %s: %s\n", textureInfo.usage->usageName.ToCString(), textureInfo.sourcePath.ToCString());

	EqString cmdLine(EqString::Format("%s %s", m_batchConfig.applicationName.ToCString(), arguments.ToCString()));
	fnmPathFixSeparators(cmdLine);

	DevMsg(DEVMSG_CORE, "*RUN '%s'\n", cmdLine.GetData());
	int result = system(cmdLine.GetData());
	if (result != 0)
	{
		MsgError("Error running command\n");
	}
}

bool CTextureCooker::Init(const char* confFileName, const char* targetName)
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

		// source materials settings
		{
			const char* materialsSrc = KV_GetValueString(currentTarget->FindSection("SourcePath"), 0, nullptr);

			if (!materialsSrc)
			{
				MsgError("Target '%s' field 'SourcePath' folder is not specified!\n", targetName);
				return false;
			}

			const char* sourceImageExt = KV_GetValueString(currentTarget->FindSection("SourceExt"), 0, nullptr);

			if (!sourceImageExt)
			{
				MsgWarning("Target '%s' field 'SourceExt' is not specified, default to 'tga'\n", targetName);
				sourceImageExt = "tga";
			}

			m_targetProps.sourceMaterialPath = materialsSrc;
			m_targetProps.sourceImageExt = _Es(sourceImageExt).TrimChar('.');

			m_targetProps.sourceMaterialPath.ReplaceSubstr(s_engineDirTag, g_fileSystem->GetCurrentDataDirectory());
			m_targetProps.sourceMaterialPath.ReplaceSubstr(s_gameDirTag, g_fileSystem->GetCurrentGameDirectory());
		}

		// target settings
		{
			const char* targetCompression = KV_GetValueString(currentTarget->FindSection("compression"), 0, nullptr);
			const char* targetFolder = KV_GetValueString(currentTarget->FindSection("output"), 0, nullptr);

			if (!targetCompression)
			{
				MsgError("Target '%s' missing 'compression' value\n", targetName);
				return false;
			}

			if (!targetFolder)
			{
				MsgError("Target '%s' missing 'output' value\n", targetName);
				return false;
			}

			m_targetProps.targetCompression = targetCompression;
			m_targetProps.targetFolder = targetFolder;

			m_targetProps.targetFolder.ReplaceSubstr(s_engineDirTag, g_fileSystem->GetCurrentDataDirectory());
			m_targetProps.targetFolder.ReplaceSubstr(s_gameDirTag, g_fileSystem->GetCurrentGameDirectory());
		}
	}

	// load batch configuration
	{
		KVSection* batchConfig = kvs.FindSection("BatchConfig");
		if (!batchConfig)
		{
			MsgError("Missing 'BatchConfig' section in '%s'\n", confFileName);
			return false;
		}

		LoadBatchConfig(batchConfig);

		if (!m_batchConfig.applicationName.Length())
		{
			MsgError("No application specified in either batch config or compression setting!\n");
			return false;
		}
	}
	return true;
}

void CTextureCooker::Execute()
{
	// perform batch conversion
	Msg("Material source path: '%s'\n", m_targetProps.sourceMaterialPath.ToCString());

	EqString searchTemplate;
	CombinePath(searchTemplate, m_targetProps.sourceMaterialPath, "*");

	// convert atlas sources first
	SearchFolderForAtlasesAndConvert(searchTemplate);

	// walk up material files
	SearchFolderForMaterialsAndGetTextures(searchTemplate);

	Msg("Got %d textures\n", m_textureList.numElem());

	EqString crcFileName(EqString::Format("%s/cook_%s_crc.txt", m_targetProps.sourceMaterialPath.ToCString(), m_targetProps.targetCompression.ToCString()));

	// load CRC list, check for existing DDS files, and skip if necessary
	KV_LoadFromFile(crcFileName, SP_ROOT, &m_batchConfig.crcSec);

	// process material files
	// this makes target folders and copies materials
	for (int i = 0; i < m_materialList.numElem(); i++)
	{
		ProcessMaterial(m_materialList[i]);
	}

	// do conversion
	for (int i = 0; i < m_textureList.numElem(); i++)
	{
		TexInfo& tex = m_textureList[i];
		Msg("%d / %d...\n", i + 1, m_textureList.numElem());
		ProcessTexture(tex);
	}

	// save CRC list file
	IFilePtr pStream = g_fileSystem->Open(crcFileName, "wt", SP_ROOT);

	if (pStream)
	{
		KV_WriteToStream(pStream, &m_batchConfig.newCRCSec, 0, true);
	}
}

void CookTarget(const char* pszTargetName)
{
	CTextureCooker cooker;
	if (!cooker.Init("TextureCooker.CONFIG", pszTargetName))
		return;
	cooker.Execute();
}