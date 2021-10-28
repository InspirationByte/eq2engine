//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Atlas packer - main code
//////////////////////////////////////////////////////////////////////////////////

#include "core/DebugInterface.h"
#include "core/IFileSystem.h"

#include "utils/strtools.h"
#include "utils/KeyValues.h"

#include "utils/CRC32.h"

#include "imaging/ImageLoader.h"
#include "imaging/PixWriter.h"
#include "math/DkMath.h"

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
	arguments "-f -threads 4 %ARGS% -dds %INPUT_FILENAME% %OUTPUT_FILEPATH%";

	source_materials 	"./0_materials_src/";	// soruce folder
	source_image_ext		"tga";

	compression "<CompressionName>"
	{
		arguments "-etcpack";	//-pvrtex;

		usage default
		{
			arguments "-etc1";
		}

		usage <UsageName>		// in material: basetexture "texturepath" "usage:<UsageName>"
		{
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

static const char* s_textureValueIdentifier = "usage:";

struct UsageProperties_t
{
	EqString usageName;
	EqString applicationArguments;
};

struct BatchConfig_t
{
	EqString sourceMaterialPath;
	EqString sourceImageExt;

	EqString applicationName;
	EqString applicationArgumentsTemplate;

	EqString compressionApplicationArguments;

	UsageProperties_t defaultUsage{ "default" };
	DkList<UsageProperties_t> usageList;

	kvkeybase_t crcSec;			// crc list loaded from disk
	kvkeybase_t newCRCSec;		// crc list that will be saved
} g_batchConfig;

UsageProperties_t* FindUsage(const char* usageName)
{
	for (int i = 0; i < g_batchConfig.usageList.numElem(); i++)
	{
		UsageProperties_t& usage = g_batchConfig.usageList[i];
		if (!usage.usageName.CompareCaseIns(usageName))
			return &usage;
	}

	return &g_batchConfig.defaultUsage;
}

struct TargetProperties_t
{
	EqString targetCompression;
	EqString targetFolder;
} g_targetProps;

enum ETexConvStatus
{
	INIT_STATE = 0,
	CRC_LOADED,
	CONVERTED,
	SKIPPED
};

// CRC pairs
struct TexInfo_t
{
	TexInfo_t()
	{
		status = INIT_STATE;
		crc32 = 0;
	}

	TexInfo_t(const char* filename, const char* usageName)
	{
		sourcePath = filename;
		usage = FindUsage(usageName);

		if (usage == &g_batchConfig.defaultUsage)
		{
			MsgWarning("%s: invalid usage '%s'\n", filename, usageName);
		}
	}

	EqString			sourcePath;
	UsageProperties_t*	usage;
	uint32				crc32;
	ETexConvStatus		status;
};

DkList<TexInfo_t*> g_textureList;

//-----------------------------------------------------------------------

int MakePath(const char *path)
{
	/* Adapted from http://stackoverflow.com/a/2336245/119527 */
	const size_t len = strlen(path);
	char _path[MAX_PATH];
	char *p;

	errno = 0;

	/* Copy string so its mutable */
	if (len > sizeof(_path) - 1) {
		return -1;
	}

	strcpy(_path, path);

	/* Iterate the string */
	for (p = _path + 1; *p; p++) {
		if (*p == CORRECT_PATH_SEPARATOR) {
			/* Temporarily truncate */
			*p = '\0';

			g_fileSystem->MakeDir(_path, SP_ROOT);

			*p = CORRECT_PATH_SEPARATOR;
		}
	}

	g_fileSystem->MakeDir(_path, SP_ROOT);

	return 0;
}

//-----------------------------------------------------------------------

void LoadBatchConfig(kvkeybase_t* batchSec)
{
	// retrieve application name and arguments
	{
		const char* appName = KV_GetValueString(batchSec->FindKeyBase("application"), 0, nullptr);

		if (!appName)
		{
			MsgError("BatchConfig 'application' is not specified!\n");
			return;
		}

		const char* appArguments = KV_GetValueString(batchSec->FindKeyBase("arguments"), 0, nullptr);

		if (!appArguments)
		{
			MsgError("BatchConfig 'arguments' is not specified (application arguments)!\n");
			return;
		}

		g_batchConfig.applicationName = appName;
		g_batchConfig.applicationArgumentsTemplate = appArguments;
	}

	// source materials settings
	{
		const char* materialsSrc = KV_GetValueString(batchSec->FindKeyBase("source_materials"), 0, nullptr);

		if (!materialsSrc)
		{
			MsgError("BatchConfig 'source_materials' folder is not specified!\n");
			return;
		}

		const char* sourceImageExt = KV_GetValueString(batchSec->FindKeyBase("source_image_ext"), 0, nullptr);

		if (!sourceImageExt)
		{
			MsgWarning("BatchConfig 'source_image_ext' is not specified, default to 'tga'\n");
			sourceImageExt = "tga";
		}

		g_batchConfig.sourceMaterialPath = materialsSrc;
		g_batchConfig.sourceImageExt = _Es(sourceImageExt).TrimChar('.');
	}

	kvkeybase_t* compressionSec = nullptr;

	for (int i = 0; i < batchSec->keys.numElem(); i++)
	{
		kvkeybase_t* sec = batchSec->keys[i];

		if (!stricmp(sec->name, "compression") && !stricmp(KV_GetValueString(sec, 0, "INVALID"), g_targetProps.targetCompression.ToCString()))
		{
			compressionSec = sec;
			break;
		}
	}

	if (!compressionSec)
	{
		MsgError("Unknown compression preset '%s', check your BatchConfig section\n", g_targetProps.targetCompression.ToCString());
		return;
	}

	g_batchConfig.compressionApplicationArguments = KV_GetValueString(compressionSec->FindKeyBase("arguments"), 0, "");

	// load usages
	for (int i = 0; i < compressionSec->keys.numElem(); i++)
	{
		kvkeybase_t* usageKey = compressionSec->keys[i];

		if (stricmp(usageKey->name, "usage"))
			continue;

		const char* usageName = KV_GetValueString(usageKey, 0, nullptr);

		if (!usageName)
		{
			MsgWarning("Usage name not specified (in it's value)\n");
			continue;
		}

		UsageProperties_t usage;
		usage.usageName = usageName;
		usage.applicationArguments = KV_GetValueString(usageKey->FindKeyBase("arguments"), 0, "");

		if (!stricmp(usageName, "default"))
			g_batchConfig.defaultUsage = usage;
		else
			g_batchConfig.usageList.append(usage);
	}
}

void LoadMaterialImages(const char* materialFileName)
{
	KeyValues kvs;
	if (!kvs.LoadFromFile(materialFileName, SP_ROOT))
		return;

	if (kvs.GetRootSection()->KeyCount() == 0)
	{
		MsgError("'%s' is not valid material file\n", materialFileName);
		return;
	}

	kvkeybase_t* kvMaterial = kvs.GetRootSection()->keys[0];
	if (!kvMaterial->IsSection())
	{
		MsgError("'%s' is not valid material file\n", materialFileName);
		return;
	}

	MsgInfo("Material: '%s'\n", materialFileName);

	int textures = 0;

	bool matFileSaved = false;

	for (int i = 0; i < kvMaterial->keys.numElem(); i++)
	{
		kvkeybase_t* key = kvMaterial->keys[i];
		for (int j = 1; j < key->ValueCount(); j++)
		{
			EqString imageUsage(KV_GetValueString(key, j, ""));
			int usageIdx = imageUsage.ReplaceSubstr("usage:", "");

			if (usageIdx != -1)
			{
				const char* imageName = KV_GetValueString(key, 0, "");

				EqString filename;
				CombinePath(filename, 2, g_batchConfig.sourceMaterialPath.ToCString(), EqString::Format("%s.%s", imageName, g_batchConfig.sourceImageExt.ToCString()).ToCString());
				
				if (!g_fileSystem->FileExist(filename.ToCString()))
				{
					MsgError("  - texture '%s' does not exists!\n", filename.ToCString());
					continue;
				}

				if (!matFileSaved)
				{
					// make path
					EqString targetFilePath;
					CombinePath(targetFilePath, 2, g_targetProps.targetFolder.ToCString(), _Es(imageName).Path_Strip_Name().ToCString());
					
					MakePath(targetFilePath.ToCString());

					EqString materialFileName(targetFilePath.Path_Strip_Name() + _Es(materialFileName).Path_Strip_Path());

					// save material file
					kvs.SaveToFile(materialFileName.ToCString(), SP_ROOT);

					matFileSaved = true;
				}

				g_textureList.append(new TexInfo_t(imageName, imageUsage.ToCString()));
				textures++;
			}
		}
	}

	if (!textures)
	{
		MsgWarning("  - no usage for textures specified - no textures added!\n");
		return;
	}
		
	// make folder structure and clone material file

}

void SearchFolderForMaterialsAndGetTextures(const char* wildcard)
{
	EqString searchFolder(wildcard);
	searchFolder.ReplaceSubstr("*.*", "");

	DKFINDDATA* findData = nullptr;
	char* fileName = (char*)g_fileSystem->FindFirst(wildcard, &findData, SP_ROOT);

	if (fileName)
	{
		while (fileName = (char*)g_fileSystem->FindNext(findData))
		{
			if (g_fileSystem->FindIsDirectory(findData) && stricmp(fileName, ".") && stricmp(fileName, ".."))
			{
				EqString searchTemplate;
				CombinePath(searchTemplate, 3, searchFolder.ToCString(), fileName, "*.*");

				SearchFolderForMaterialsAndGetTextures(searchTemplate.ToCString());
			}
			else if (xstristr(fileName, ".mat"))
			{
				EqString fullMaterialPath;
				CombinePath(fullMaterialPath, 2, searchFolder.ToCString(), fileName);
				LoadMaterialImages(fullMaterialPath.ToCString());
			}
		}

		g_fileSystem->FindClose(findData);
	}
}

bool hasMatchingCRC(uint32 crc)
{
	for (int i = 0; i < g_batchConfig.crcSec.keys.numElem(); i++)
	{
		uint32 checkCRC = strtoul(g_batchConfig.crcSec.keys[i]->name, nullptr, 10);

		if (checkCRC == crc)
			return true;
	}

	return false;
}

void ProcessTexture(TexInfo_t* textureInfo)
{
	// before this, create folders...
	EqString sourceFilename;
	CombinePath(sourceFilename, 2, g_batchConfig.sourceMaterialPath.ToCString(), EqString::Format("%s.%s", textureInfo->sourcePath.ToCString(), g_batchConfig.sourceImageExt.ToCString()).ToCString());
	
	EqString targetFilename;
	CombinePath(targetFilename, 2, g_targetProps.targetFolder.ToCString(), (textureInfo->sourcePath.Path_Strip_Ext() + ".dds").ToCString());
	
	EqString targetFilePath;
	CombinePath(targetFilePath, 2, g_targetProps.targetFolder.ToCString(), textureInfo->sourcePath.Path_Strip_Name().ToCString());

	targetFilePath = targetFilePath.TrimChar(CORRECT_PATH_SEPARATOR);

	EqString arguments(g_batchConfig.applicationArgumentsTemplate);
	arguments.ReplaceSubstr(s_argumentsTag.ToCString(), textureInfo->usage->applicationArguments.ToCString());
	arguments.ReplaceSubstr(s_inputFileNameTag.ToCString(), sourceFilename.ToCString());
	arguments.ReplaceSubstr(s_outputFilePathTag.ToCString(), targetFilePath.ToCString());

	// generate CRC from image file content and arguments it's going to be built
	uint32 srcCRC = g_fileSystem->GetFileCRC32(sourceFilename.ToCString(), SP_ROOT) + CRC32_BlockChecksum(arguments.ToCString(), arguments.Length());

	// store new CRC
	g_batchConfig.newCRCSec.SetKey(EqString::Format("%u", srcCRC).ToCString(), sourceFilename.ToCString());

	// now check CRC from loaded file
	if (hasMatchingCRC(srcCRC))
	{
		if (g_fileSystem->FileExist(targetFilename.ToCString(), SP_ROOT))
		{
			MsgInfo("Skipping %s: %s...\n", textureInfo->usage->usageName.ToCString(), textureInfo->sourcePath.ToCString());
			textureInfo->status = SKIPPED;
			return;
		}
		else
		{
			MsgInfo("Re-generating '%s'!\n", targetFilename.ToCString());
		}
	}

	textureInfo->status = CONVERTED;

	MsgInfo("Processing %s: %s...\n", textureInfo->usage->usageName.ToCString(), textureInfo->sourcePath.ToCString());

	/*static const EqString s_argumentsTag("%ARGS%");
	static const EqString s_inputFileNameTag("%INPUT_FILENAME%");
	static const EqString s_outputFileNameTag("%OUTPUT_FILENAME%");
	static const EqString s_outputFilePathTag("%OUTPUT_FILEPATH%");*/

	EqString cmdLine(EqString::Format("%s %s %s", g_batchConfig.applicationName.ToCString(), g_batchConfig.compressionApplicationArguments.ToCString(), arguments.ToCString()));

	Msg("*RUN '%s'\n", cmdLine.GetData());
	system(cmdLine.GetData());
}

void CookMaterialsToTarget(const char* pszTargetName)
{
	// load all properties
	KeyValues kvs;

	if (!kvs.LoadFromFile("TextureCooker.CONFIG", SP_ROOT))
	{
		MsgError("Failed to load 'TextureCooker.CONFIG' file!\n");
		return;
	}

	// get the target properties
	{
		// load target info
		kvkeybase_t* targets = kvs.FindKeyBase("Targets");
		if (!targets)
		{
			MsgError("Missing 'Targets' section in 'TextureCooker.CONFIG'\n");
			return;
		}

		kvkeybase_t* currentTarget = targets->FindKeyBase(pszTargetName);

		if (!currentTarget)
		{
			MsgError("Cannot find target section '%s'\n", pszTargetName);
			return;
		}

		const char* targetCompression = KV_GetValueString(currentTarget->FindKeyBase("compression"), 0, nullptr);
		const char* targetFolder = KV_GetValueString(currentTarget->FindKeyBase("output"), 0, nullptr);

		if (!targetCompression)
		{
			MsgError("Target '%s' missing 'compression' value\n", pszTargetName);
			return;
		}

		if (!targetFolder)
		{
			MsgError("Target '%s' missing 'output' value\n", pszTargetName);
			return;
		}

		g_targetProps.targetCompression = targetCompression;
		g_targetProps.targetFolder = targetFolder;
	}

	// load batch configuration
	{
		kvkeybase_t* batchConfig = kvs.FindKeyBase("BatchConfig");
		if (!batchConfig)
		{
			MsgError("Missing 'BatchConfig' section in 'TextureCooker.CONFIG'\n");
			return;
		}

		LoadBatchConfig(batchConfig);
	}

	// perform batch conversion
	{
		EqString searchTemplate;
		CombinePath(searchTemplate, 2, g_batchConfig.sourceMaterialPath.ToCString(), "*.*");

		Msg("Material source path: '%s'\n", searchTemplate.ToCString());

		// walk up material files
		SearchFolderForMaterialsAndGetTextures( searchTemplate.ToCString() );

		Msg("Got %d textures\n", g_textureList.numElem());

		EqString crcFileName(EqString::Format("cook_%s_crc.txt", g_targetProps.targetCompression.ToCString()));

		// load CRC list, check for existing DDS files, and skip if necessary
		KV_LoadFromFile(crcFileName.ToCString(), SP_ROOT, &g_batchConfig.crcSec);

		// do conversion
		for (int i = 0; i < g_textureList.numElem(); i++)
		{
			TexInfo_t* tex = g_textureList[i];
			ProcessTexture(tex);

			delete tex;
		}

		// save CRC list file
		IFile* pStream = g_fileSystem->Open(crcFileName.ToCString(), "wt", SP_ROOT);

		if (pStream)
		{
			KV_WriteToStream(pStream, &g_batchConfig.newCRCSec, 0, true);
			g_fileSystem->Close(pStream);
		}
	}
}