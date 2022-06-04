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
		application "otherConverter.exe";
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

	EqString applicationName;
	EqString applicationArguments;
};

struct BatchConfig_t
{
	EqString sourceMaterialPath;
	EqString sourceImageExt;

	EqString applicationName; // can be overridden by UsageProperties_t::applicationName
	EqString applicationArgumentsTemplate;

	EqString compressionApplicationArguments;

	UsageProperties_t defaultUsage{ "default" };
	Array<UsageProperties_t> usageList{ PP_SL };

	KVSection crcSec;			// crc list loaded from disk
	KVSection newCRCSec;		// crc list that will be saved
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
	TexInfo_t() = default;

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
	UsageProperties_t* usage{ nullptr };
	uint32				crc32{ 0 };
	ETexConvStatus		status{ INIT_STATE };
};

Array<TexInfo_t*> g_textureList{ PP_SL };
Array<EqString> g_materialList{ PP_SL };

//-----------------------------------------------------------------------

void LoadBatchConfig(KVSection* batchSec)
{
	// retrieve application name and arguments
	{
		const char* appName = KV_GetValueString(batchSec->FindSection("application"), 0, nullptr);
		const char* appArguments = KV_GetValueString(batchSec->FindSection("arguments"), 0, nullptr);

		g_batchConfig.applicationName = appName;
		g_batchConfig.applicationArgumentsTemplate = appArguments;
	}

	// source materials settings
	{
		const char* materialsSrc = KV_GetValueString(batchSec->FindSection("source_materials"), 0, nullptr);

		if (!materialsSrc)
		{
			MsgError("BatchConfig 'source_materials' folder is not specified!\n");
			return;
		}

		const char* sourceImageExt = KV_GetValueString(batchSec->FindSection("source_image_ext"), 0, nullptr);

		if (!sourceImageExt)
		{
			MsgWarning("BatchConfig 'source_image_ext' is not specified, default to 'tga'\n");
			sourceImageExt = "tga";
		}

		g_batchConfig.sourceMaterialPath = materialsSrc;
		g_batchConfig.sourceImageExt = _Es(sourceImageExt).TrimChar('.');
	}

	KVSection* compressionSec = nullptr;

	for (int i = 0; i < batchSec->keys.numElem(); i++)
	{
		KVSection* sec = batchSec->keys[i];

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

	g_batchConfig.applicationName = KV_GetValueString(compressionSec->FindSection("application"), 0, g_batchConfig.applicationName.ToCString());
	g_batchConfig.compressionApplicationArguments = KV_GetValueString(compressionSec->FindSection("arguments"), 0, "");

	// load usages
	for (int i = 0; i < compressionSec->keys.numElem(); i++)
	{
		KVSection* usageKey = compressionSec->keys[i];

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
		usage.applicationName = KV_GetValueString(usageKey->FindSection("application"), 0, g_batchConfig.applicationName.ToCString());
		usage.applicationArguments = KV_GetValueString(usageKey->FindSection("arguments"), 0, "");

		if (!stricmp(usageName, "default"))
			g_batchConfig.defaultUsage = usage;
		else
			g_batchConfig.usageList.append(usage);
	}
}

bool AddTexture(const EqString& texturePath, const EqString& imageUsage)
{
	EqString filename;
	CombinePath(filename, 2, g_batchConfig.sourceMaterialPath.ToCString(), EqString::Format("%s.%s", texturePath.ToCString(), g_batchConfig.sourceImageExt.ToCString()).ToCString());

	if (!g_fileSystem->FileExist(filename.ToCString()))
	{
		MsgError("  - texture '%s' does not exists!\n", filename.ToCString());
		return false;
	}

	g_textureList.append(new TexInfo_t(texturePath.ToCString(), imageUsage.ToCString()));
	return true;
}

void LoadMaterialImages(const char* materialFileName)
{
	KeyValues kvs;
	if (!kvs.LoadFromFile(materialFileName, SP_ROOT))
		return;

	EqString localMaterialFileName = materialFileName + g_batchConfig.sourceMaterialPath.Length();
	localMaterialFileName = localMaterialFileName.TrimChar(CORRECT_PATH_SEPARATOR).TrimChar(INCORRECT_PATH_SEPARATOR);

	if (kvs.GetRootSection()->KeyCount() == 0)
	{
		MsgError("'%s' is not valid material file\n", localMaterialFileName.ToCString());
		return;
	}

	KVSection* kvMaterial = kvs.GetRootSection()->keys[0];
	if (!kvMaterial->IsSection())
	{
		MsgError("'%s' is not valid material file\n", localMaterialFileName.ToCString());
		return;
	}

	MsgInfo("Material: '%s'\n", localMaterialFileName.ToCString());
	g_materialList.append(localMaterialFileName);

	int textures = 0;

	for (int i = 0; i < kvMaterial->keys.numElem(); i++)
	{
		bool keyHasUsage = false;
		KVSection* key = kvMaterial->keys[i];
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
				int numFrames = atoi(textureFrameCount.ToCString());

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

void ProcessMaterial(const EqString& materialFileName)
{
	const EqString atlasFileName = _Es(materialFileName).Path_Strip_Ext() + ".atlas";

	EqString sourceMaterialFileName;
	CombinePath(sourceMaterialFileName, 2, g_batchConfig.sourceMaterialPath.ToCString(), materialFileName.ToCString());

	EqString sourceAtlasFileName;
	CombinePath(sourceAtlasFileName, 2, g_batchConfig.sourceMaterialPath.ToCString(), atlasFileName.ToCString());

	EqString targetMaterialFileName;
	CombinePath(targetMaterialFileName, 2, g_targetProps.targetFolder.ToCString(), materialFileName.ToCString());

	EqString targetAtlasFileName;
	CombinePath(targetAtlasFileName, 2, g_targetProps.targetFolder.ToCString(), atlasFileName.ToCString());

	// make target material file path
	g_fileSystem->MakeDir(targetMaterialFileName.Path_Strip_Name(), SP_ROOT);

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

void ProcessTexture(TexInfo_t* textureInfo)
{
	// before this, create folders...
	EqString sourceFilename;
	CombinePath(sourceFilename, 2, g_batchConfig.sourceMaterialPath.ToCString(), EqString::Format("%s.%s", textureInfo->sourcePath.ToCString(), g_batchConfig.sourceImageExt.ToCString()).ToCString());
	
	EqString targetFilename;
	CombinePath(targetFilename, 2, g_targetProps.targetFolder.ToCString(), (textureInfo->sourcePath.Path_Strip_Ext() + ".dds").ToCString());
	
	const EqString targetFilePath = targetFilename.Path_Strip_Name().TrimChar(CORRECT_PATH_SEPARATOR);

	// make image folder
	g_fileSystem->MakeDir(targetFilePath.ToCString(), SP_ROOT);

	EqString arguments(g_batchConfig.applicationArgumentsTemplate);
	arguments.ReplaceSubstr(s_argumentsTag.ToCString(), (g_batchConfig.compressionApplicationArguments + " " + textureInfo->usage->applicationArguments).ToCString());
	arguments.ReplaceSubstr(s_inputFileNameTag.ToCString(), sourceFilename.ToCString());
	arguments.ReplaceSubstr(s_outputFilePathTag.ToCString(), targetFilePath.ToCString());

	// generate CRC from image file content and arguments it's going to be built
	uint32 srcCRC = g_fileSystem->GetFileCRC32(sourceFilename.ToCString(), SP_ROOT);
	CRC32_UpdateChecksum(srcCRC, arguments.ToCString(), arguments.Length());

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
			MsgInfo("Re-generating '%s'\n", targetFilename.ToCString());
		}
	}

	textureInfo->status = CONVERTED;

	MsgInfo("Processing %s: %s\n", textureInfo->usage->usageName.ToCString(), textureInfo->sourcePath.ToCString());

	EqString cmdLine(EqString::Format("%s %s", g_batchConfig.applicationName.ToCString(), arguments.ToCString()));

	DevMsg(DEVMSG_CORE, "*RUN '%s'\n", cmdLine.GetData());
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
		KVSection* targets = kvs.FindSection("Targets");
		if (!targets)
		{
			MsgError("Missing 'Targets' section in 'TextureCooker.CONFIG'\n");
			return;
		}

		KVSection* currentTarget = targets->FindSection(pszTargetName);

		if (!currentTarget)
		{
			MsgError("Cannot find target section '%s'\n", pszTargetName);
			return;
		}

		const char* targetCompression = KV_GetValueString(currentTarget->FindSection("compression"), 0, nullptr);
		const char* targetFolder = KV_GetValueString(currentTarget->FindSection("output"), 0, nullptr);

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
		KVSection* batchConfig = kvs.FindSection("BatchConfig");
		if (!batchConfig)
		{
			MsgError("Missing 'BatchConfig' section in 'TextureCooker.CONFIG'\n");
			return;
		}

		LoadBatchConfig(batchConfig);

		if (!g_batchConfig.applicationName.Length()) 
		{
			MsgError("No application specified in either batch config or compression setting!\n");
			return;
		}
	}

	// perform batch conversion
	{
		Msg("Material source path: '%s'\n", g_batchConfig.sourceMaterialPath.ToCString());

		EqString searchTemplate;
		CombinePath(searchTemplate, 2, g_batchConfig.sourceMaterialPath.ToCString(), "*.*");

		// walk up material files
		SearchFolderForMaterialsAndGetTextures( searchTemplate.ToCString() );

		Msg("Got %d textures\n", g_textureList.numElem());

		EqString crcFileName(EqString::Format("cook_%s_crc.txt", g_targetProps.targetCompression.ToCString()));

		// load CRC list, check for existing DDS files, and skip if necessary
		KV_LoadFromFile(crcFileName.ToCString(), SP_ROOT, &g_batchConfig.crcSec);

		// process material files
		// this makes target folders and copies materials
		for(int i = 0; i < g_materialList.numElem(); i++)
		{
			ProcessMaterial(g_materialList[i]);
		}

		// do conversion
		for (int i = 0; i < g_textureList.numElem(); i++)
		{
			TexInfo_t* tex = g_textureList[i];
			Msg("%d / %d...\n", i+1, g_textureList.numElem());
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