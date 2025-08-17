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
#include "utils/CRC32.h"
#include "texcooker_defs.h"

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

constexpr EqStringRef s_textureValueIdentifier = "usage:";

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
	bool				isArray{ false };
};

class CTextureCooker
{
public:
	bool				Init(const char* confFileName, const char* targetName);
	void				Execute();

private:
	void				LoadBatchConfig(const KVSection* batchSec);
	bool				AddTexture(const EqString& texturePath, const EqString& imageUsage, bool isArray = false);
	bool				CreateArrayImageFile(const Array<EqString>& textureNames, const char* outputFileName, const char* imageUsage);
	void				LoadMaterialImages(const KVSection& kvMaterial);

	void				SearchFolderForMaterialsAndGetTextures(const char* wildcard);
	void				SearchFolderForAtlasesAndConvert(const char* wildcard);

	bool				HasMatchingCRC(uint32 crc);
	void				ProcessMaterial(const EqString& materialFileName);
	void				ProcessTexture(TexInfo& textureInfo);
	UsageProperties*	FindUsage(const char* usageName);

	BatchConfig			m_batchConfig;
	TargetProperties	m_targetProps;

	Array<TexInfo>		m_textureList{ PP_SL };
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
	for (const KVSection& sec : batchSec->Keys("compression"))
	{
		if (!m_targetProps.targetCompression.CompareCaseIns(KV_GetValueString(&sec, 0, "INVALID")))
		{
			compressionSec = &sec;
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
	for (const KVSection& usageKey : compressionSec->Keys("usage"))
	{
		EqStringRef usageName;
		if (!usageKey.GetValues(usageName))
		{
			MsgWarning("Usage name not specified (in it's value)\n");
			continue;
		}

		UsageProperties usage;
		usage.usageName = usageName;
		usage.applicationName = KV_GetValueString(usageKey.FindSection("application"), 0, m_batchConfig.applicationName);
		usage.applicationArguments = KV_GetValueString(usageKey.FindSection("arguments"), 0, "");

		if (!usageName.CompareCaseIns("default"))
			m_batchConfig.defaultUsage = usage;
		else
			m_batchConfig.usageList.append(usage);
	}
}

bool CTextureCooker::AddTexture(const EqString& texturePath, const EqString& imageUsage, bool isArray)
{
	EqString filename = fnmPathCombine(m_targetProps.sourceMaterialPath, texturePath);
	if (!fnmPathHasExt(filename))
		filename = fnmPathApplyExt(filename, m_targetProps.sourceImageExt);

	if (!g_fileSystem->FileExist(filename))
	{
		MsgError("  - texture '%s' does not exists!\n", filename.ToCString());
		return false;
	}

	TexInfo& newInfo = m_textureList.append();
	newInfo.sourcePath = texturePath;
	newInfo.usage = FindUsage(imageUsage);
	newInfo.isArray = isArray;

	if (newInfo.usage == &m_batchConfig.defaultUsage)
	{
		MsgWarning("%s: invalid usage '%s'\n", filename.ToCString(), imageUsage.ToCString());
	}

	return true;
}

bool CTextureCooker::CreateArrayImageFile(const Array<EqString>& textureNames, const char* outputFileName, const char* imageUsage)
{
	Array<CImage::PTR_T> imgFrames(PP_SL);

	// load frames
	EqString texturePathExt;
	for (const EqString& texName : textureNames)
	{
		CImage::PTR_T img = CRefPtr_new(CImage);

		texturePathExt = fnmPathCombine(m_targetProps.sourceMaterialPath, texName);
		if (img->Load(fnmPathApplyExt(texturePathExt, m_targetProps.sourceImageExt), 0))
		{
			imgFrames.append(img);
		}
		else
		{
			MsgError("Can't open texture \"%s\"\n", texturePathExt.ToCString());
		}
	}

	if (!imgFrames.numElem())
		return false;

	const CImage::PTR_T firstImg = imgFrames.front();
	ASSERT_MSG(firstImg->GetArraySize() == 1, "Texture arrays are not supported when animation table is used.");

	CImage::PTR_T textureImg = CRefPtr_new(CImage);
	ubyte* textureData = textureImg->Create(
		firstImg->GetFormat(),
		firstImg->GetWidth(),
		firstImg->GetHeight(),
		firstImg->GetDepth(),
		firstImg->GetMipMapCount(),
		imgFrames.numElem()
	);

	const int stride = firstImg->GetMipMappedSize(0, firstImg->GetMipMapCount());
	for (CImage* frameImg : imgFrames)
	{
		ASSERT_MSG(firstImg->GetFormat() == frameImg->GetFormat() &&
			firstImg->GetWidth() == frameImg->GetWidth() &&
			firstImg->GetHeight() == frameImg->GetHeight(), "Animated textures must share same format and size");

		memcpy(textureData, frameImg->GetPixels(), stride);
		textureData += stride;
	}

	// We need to save to DDS file with DX10 header
	EqString outFileName = fnmPathCombine(m_targetProps.sourceMaterialPath, outputFileName);
	outFileName = fnmPathApplyExt(outFileName, "dds");

	IFileStreamPtr file = g_fileSystem->Open(outFileName, FS_OPEN_WRITE, SP_ROOT);
	if (!textureImg->SaveDDS(file))
	{
		MsgError("Error while saving '%s'\n", outFileName.ToCString());
		return false;
	}

	AddTexture(fnmPathApplyExt(outputFileName, "dds"), imageUsage, true);
	return true;
}

void CTextureCooker::LoadMaterialImages(const KVSection& kvMaterial)
{
	int textures = 0;
	for (KVSection& key : kvMaterial.Keys())
	{
		EqStringRef texturePath, usageVal;
		if (key.GetValues(texturePath, usageVal) < 2)
			continue;

		EqString imageUsage = usageVal;
		const int usageIdx = imageUsage.ReplaceSubstr(s_textureValueIdentifier, "");
		if (usageIdx == -1)
			continue;

		// has pattern for animated texture?
		int animCountStart = texturePath.Find("[");
		int animCountEnd = -1;
		if (animCountStart != -1 && (animCountEnd = texturePath.Find("]", false, animCountStart)) != -1)
		{
			// load texture array
			EqString textureWildcard = texturePath.Left(animCountStart);
			EqString textureFrameCount = texturePath.Mid(animCountStart + 1, (animCountEnd - animCountStart) - 1);
			int numFrames = atoi(textureFrameCount);

			Array<EqString> textureNames(PP_SL);
			for (int i = 0; i < numFrames; i++)
				textureNames.append(EqString::Format(textureWildcard, i));

			EqString newTextureName = EqString::Format(textureWildcard, numFrames);
			if (CreateArrayImageFile(textureNames, newTextureName, imageUsage))
				textures++;

			// change wildcard to the generated array image
			key.SetValue(newTextureName, 0);
		}
		else
		{
			if(AddTexture(texturePath, imageUsage))
				textures++;
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
		EqStringRef fileName = fsFind.GetPath();
		if (fsFind.IsDirectory() && fileName != EqStringRef(".") && fileName != EqStringRef(".."))
		{
			const EqString searchTemplate = fnmPathCombine(searchFolder, fileName, "*");
			SearchFolderForAtlasesAndConvert(searchTemplate);
		}
		else if(fnmPathExtractExt(fileName) == s_atlasFileExt)
		{
			const EqString fullAtlPath = fnmPathCombine(searchFolder, fileName);
			ProcessAtlasFile(fullAtlPath, m_targetProps.sourceMaterialPath);
		}
	}
}

void CTextureCooker::SearchFolderForMaterialsAndGetTextures(const char* wildcard)
{
	EqString searchFolder(wildcard);
	searchFolder.ReplaceSubstr("*", "");

	Msg("Search MAT wildcard: %s\n", wildcard);

	CFileSystemFind fsFind(wildcard, SP_ROOT);
	while (fsFind.Next())
	{
		EqStringRef fileName = fsFind.GetPath();
		if (fsFind.IsDirectory() && fileName != EqStringRef(".") && fileName != EqStringRef(".."))
		{
			const EqString searchTemplate = fnmPathCombine(searchFolder, fileName, "*");
			SearchFolderForMaterialsAndGetTextures(searchTemplate);
		}
		else if(fnmPathExtractExt(fileName) == s_materialFileExt)
		{
			const EqString fullMaterialPath = fnmPathCombine(searchFolder, fileName);
			ProcessMaterial(fullMaterialPath);
		}
	}
}

bool CTextureCooker::HasMatchingCRC(uint32 crc)
{
	for (const KVSection& crcSec : m_batchConfig.crcSec.Keys())
	{
		uint32 checkCRC = strtoul(crcSec.GetName(), nullptr, 10);

		if (checkCRC == crc)
			return true;
	}

	return false;
}

void CTextureCooker::ProcessMaterial(const EqString& materialFileName)
{
	// try to load source material file
	KVSection kvs;
	if (!KV_LoadFromFile(materialFileName, SP_ROOT, kvs))
		return;

	EqString localMaterialFileName = materialFileName + m_targetProps.sourceMaterialPath.Length();
	localMaterialFileName = localMaterialFileName.TrimChar(CORRECT_PATH_SEPARATOR).TrimChar(INCORRECT_PATH_SEPARATOR);
	if (kvs.KeyCount() == 0)
	{
		MsgError("'%s' is not valid material file\n", localMaterialFileName.ToCString());
		return;
	}

	const KVSection* kvMaterial = *kvs.Begin();
	if (!kvMaterial->IsSection())
	{
		MsgError("'%s' is not valid material file\n", localMaterialFileName.ToCString());
		return;
	}

	MsgInfo("Material: '%s'\n", localMaterialFileName.ToCString()); 

	LoadMaterialImages(*kvMaterial);

	const EqString atlasFileName = fnmPathApplyExt(localMaterialFileName, s_materialAtlasFileExt);
	const EqString sourceAtlasFileName = fnmPathCombine(m_targetProps.sourceMaterialPath, atlasFileName);
	const EqString targetMaterialFileName = fnmPathCombine(m_targetProps.targetFolder, localMaterialFileName);
	const EqString targetAtlasFileName = fnmPathCombine(m_targetProps.targetFolder, atlasFileName);

	// make target material file path
	g_fileSystem->MakeDir(fnmPathStripName(targetMaterialFileName), SP_ROOT);

	// save material file
	IFileStreamPtr matFile = g_fileSystem->Open(targetMaterialFileName, FS_OPEN_WRITE, SP_ROOT);
	if (matFile)
		KeyValues::WriteText(matFile, kvs);
	else
		MsgError("Cannot save material file '%s'\n", targetMaterialFileName.ToCString());

	// also copy atlas file
	if (g_fileSystem->FileExist(sourceAtlasFileName, SP_ROOT))
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
	EqString sourceFilename = fnmPathCombine(m_targetProps.sourceMaterialPath, textureInfo.sourcePath);
	
	if (!fnmPathHasExt(sourceFilename))
		sourceFilename = fnmPathApplyExt(sourceFilename, m_targetProps.sourceImageExt);

	const EqString targetFilename = fnmPathCombine(m_targetProps.targetFolder, fnmPathApplyExt(textureInfo.sourcePath, "dds"));
	const EqString targetFilePath = fnmPathStripName(targetFilename).TrimChar(CORRECT_PATH_SEPARATOR);

	// make image folder
	g_fileSystem->MakeDir(targetFilePath, SP_ROOT);

	EqString arguments(m_batchConfig.applicationArgumentsTemplate);
	arguments.ReplaceSubstr(s_argumentsTag, (m_batchConfig.compressionApplicationArguments + " " + textureInfo.usage->applicationArguments));
	arguments.ReplaceSubstr(s_inputFileNameTag, g_fileSystem->GetAbsolutePath(SP_ROOT, sourceFilename));
	arguments.ReplaceSubstr(s_outputFilePathTag, g_fileSystem->GetAbsolutePath(SP_ROOT, targetFilePath));

	// generate CRC from image file content and arguments it's going to be built
	uint32 srcCRC = g_fileSystem->GetFileCRC32(sourceFilename, SP_ROOT);
	CRC32_UpdateChecksum(srcCRC, arguments, arguments.Length());

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
			MsgInfo("Re-generating %s: %s...\n", textureInfo.usage->usageName.ToCString(), targetFilename.ToCString());
		}
	}
	else
		MsgInfo("Processing %s: %s...\n", textureInfo.usage->usageName.ToCString(), textureInfo.sourcePath.ToCString());

	textureInfo.status = CONVERTED;

	EqString cmdLine(EqString::Format("%s %s", g_fileSystem->GetAbsolutePath(SP_ROOT, m_batchConfig.applicationName), arguments));
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
		const KVSection* batchConfig = kvs["BatchConfig"];
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

	const EqString searchTemplate = fnmPathCombine(m_targetProps.sourceMaterialPath, "*");

	// convert atlas sources first
	SearchFolderForAtlasesAndConvert(searchTemplate);

	// walk up material files
	SearchFolderForMaterialsAndGetTextures(searchTemplate);

	Msg("Got %d textures\n", m_textureList.numElem());

	EqString crcFileName(EqString::Format("%s/cook_%s_crc.txt", m_targetProps.sourceMaterialPath.ToCString(), m_targetProps.targetCompression.ToCString()));

	// load CRC list, check for existing DDS files, and skip if necessary
	KV_LoadFromFile(crcFileName, SP_ROOT, m_batchConfig.crcSec);

	// do conversion
	for (int i = 0; i < m_textureList.numElem(); i++)
	{
		TexInfo& tex = m_textureList[i];
		Msg("[%d / %d] ", i + 1, m_textureList.numElem());
		ProcessTexture(tex);
	}

	// save CRC list file
	IFileStreamPtr pStream = g_fileSystem->Open(crcFileName, FS_OPEN_WRITE, SP_ROOT);
	if (pStream)
		KeyValues::WriteText(pStream, m_batchConfig.newCRCSec);
}

void CookTarget(const char* pszTargetName)
{
	CTextureCooker cooker;
	if (!cooker.Init("TextureCooker.CONFIG", pszTargetName))
		return;
	cooker.Execute();
}