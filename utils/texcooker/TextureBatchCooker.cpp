//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Atlas packer - main code
//////////////////////////////////////////////////////////////////////////////////

#include "DebugInterface.h"

#include "imaging/ImageLoader.h"
#include "imaging/PixWriter.h"
#include "math/DkMath.h"

#include "utils/strtools.h"
#include "utils/KeyValues.h"

#include "utils/CRC32.h"

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

		if (!stricmp(sec->name, "compression") && !stricmp(KV_GetValueString(sec, 0, "INVALID"), g_targetProps.targetCompression.c_str()))
		{
			compressionSec = sec;
			break;
		}
	}

	if (!compressionSec)
	{
		MsgError("Unknown compression preset '%s', check your BatchConfig section\n", g_targetProps.targetCompression.c_str());
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

				EqString filename( CombinePath(2, g_batchConfig.sourceMaterialPath.c_str(), varargs("%s.%s", imageName,  g_batchConfig.sourceImageExt.c_str())) );

				if (!g_fileSystem->FileExist(filename.c_str()))
				{
					MsgError("  - texture '%s' does not exists!\n", filename.c_str());
					continue;
				}

				if (!matFileSaved)
				{
					// make path
					EqString targetFilePath(CombinePath(2, g_targetProps.targetFolder.c_str(), _Es(imageName).Path_Strip_Name().c_str()));
					MakePath(targetFilePath.c_str());

					EqString materialFileName(targetFilePath.Path_Strip_Name() + _Es(materialFileName).Path_Strip_Path());

					// save material file
					kvs.SaveToFile(materialFileName.c_str(), SP_ROOT);

					matFileSaved = true;
				}

				g_textureList.append(new TexInfo_t(imageName, imageUsage.c_str()));
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
				EqString searchTemplate(CombinePath(3, searchFolder.c_str(), fileName, "*.*"));

				SearchFolderForMaterialsAndGetTextures(searchTemplate.c_str());
			}
			else if (xstristr(fileName, ".mat"))
			{
				EqString fullMaterialPath(CombinePath(2, searchFolder.c_str(), fileName));
				LoadMaterialImages(fullMaterialPath.c_str());
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
	EqString sourceFilename(CombinePath(2, g_batchConfig.sourceMaterialPath.c_str(), varargs("%s.%s", textureInfo->sourcePath.c_str(), g_batchConfig.sourceImageExt.c_str())) );
	EqString targetFilename(CombinePath(2, g_targetProps.targetFolder.c_str(), textureInfo->sourcePath.Path_Strip_Ext() + ".dds" ));
	EqString targetFilePath(CombinePath(2, g_targetProps.targetFolder.c_str(), textureInfo->sourcePath.Path_Strip_Name().c_str() ));

	targetFilePath = targetFilePath.TrimChar(CORRECT_PATH_SEPARATOR);

	EqString arguments(g_batchConfig.applicationArgumentsTemplate);
	arguments.ReplaceSubstr(s_argumentsTag.c_str(), textureInfo->usage->applicationArguments.c_str());
	arguments.ReplaceSubstr(s_inputFileNameTag.c_str(), sourceFilename.c_str());
	arguments.ReplaceSubstr(s_outputFilePathTag.c_str(), targetFilePath.c_str());

	// generate CRC from image file content and arguments it's going to be built
	uint32 srcCRC = g_fileSystem->GetFileCRC32(sourceFilename.c_str(), SP_ROOT) + CRC32_BlockChecksum(arguments.c_str(), arguments.Length());

	// store new CRC
	g_batchConfig.newCRCSec.SetKey(varargs("%u", srcCRC), sourceFilename.c_str());

	// now check CRC from loaded file
	if (hasMatchingCRC(srcCRC) && g_fileSystem->FileExist(targetFilename.c_str(), SP_ROOT))
	{
		MsgInfo("Skipping %s: %s...\n", textureInfo->usage->usageName.c_str(), textureInfo->sourcePath.c_str());
		textureInfo->status = SKIPPED;
		return;
	}

	textureInfo->status = CONVERTED;

	MsgInfo("Processing %s: %s...\n", textureInfo->usage->usageName.c_str(), textureInfo->sourcePath.c_str());

	/*static const EqString s_argumentsTag("%ARGS%");
	static const EqString s_inputFileNameTag("%INPUT_FILENAME%");
	static const EqString s_outputFileNameTag("%OUTPUT_FILENAME%");
	static const EqString s_outputFilePathTag("%OUTPUT_FILEPATH%");*/

	EqString cmdLine(varargs("%s %s %s", g_batchConfig.applicationName.c_str(), g_batchConfig.compressionApplicationArguments.c_str(), arguments.c_str()));

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
		EqString searchTemplate(CombinePath(2, g_batchConfig.sourceMaterialPath.c_str(), "*.*"));

		Msg("Material source path: '%s'\n", searchTemplate.c_str());

		// walk up material files
		SearchFolderForMaterialsAndGetTextures( searchTemplate.c_str() );

		Msg("Got %d textures\n", g_textureList.numElem());

		EqString crcFileName(varargs("cook_%s_crc.txt", g_targetProps.targetCompression.c_str()));

		// load CRC list, check for existing DDS files, and skip if necessary
		KV_LoadFromFile(crcFileName.c_str(), SP_ROOT, &g_batchConfig.crcSec);

		// do conversion
		for (int i = 0; i < g_textureList.numElem(); i++)
		{
			TexInfo_t* tex = g_textureList[i];
			ProcessTexture(tex);

			delete tex;
		}

		// save CRC list file
		IFile* pStream = g_fileSystem->Open(crcFileName.c_str(), "wt", SP_ROOT);

		if (pStream)
		{
			KV_WriteToStream(pStream, &g_batchConfig.newCRCSec, 0, true);
			g_fileSystem->Close(pStream);
		}
	}
}