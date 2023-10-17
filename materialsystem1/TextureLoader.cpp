/////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Texture loader helper utility
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/ConVar.h"
#include "core/IConsoleCommands.h"
#include "core/IFileSystem.h"

#include "imaging/ImageLoader.h"

#include "materialsystem1/IMaterialSystem.h"
#include "materialsystem1/renderers/IShaderAPI.h"
#include "TextureLoader.h"

#define TEXTURE_DEFAULT_EXTENSION		".dds"
#define TEXTURE_SECONDARY_EXTENSION		".tga"
#define TEXTURE_ANIMATED_EXTENSION		".ati"			// ATI - Animated Texture Index file

DECLARE_CVAR(r_reportTextureLoading, "0", "Echo textrue loading", 0);
DECLARE_CVAR(r_skipTextureLoading, "0", nullptr, CV_CHEAT);
DECLARE_CVAR(r_noMip, "0", nullptr, CV_CHEAT);

static void AnimGetImagesForTextureName(Array<EqString>& textureNames, const char* pszFileName)
{
	EqString texturePath(pszFileName);
	texturePath.Path_FixSlashes();

	// build valid texture paths
	EqString texturePathExt = texturePath + EqString(TEXTURE_DEFAULT_EXTENSION);
	EqString textureAnimPathExt = texturePath + EqString(TEXTURE_ANIMATED_EXTENSION);

	texturePathExt.Path_FixSlashes();
	textureAnimPathExt.Path_FixSlashes();

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

		if (r_reportTextureLoading.GetBool())
			Msg("Loading animated %d animated textures (%s)\n", numFrames, textureWildcard.ToCString());

		for (int i = 0; i < numFrames; i++)
		{
			EqString textureNameFrame = EqString::Format(textureWildcard.ToCString(), i);
			textureNames.append(textureNameFrame);
		}
	}
	else
	{
		// try loading older Animated Texture Index file
		EqString textureAnimPathExt = texturePath + EqString(TEXTURE_ANIMATED_EXTENSION);
		textureAnimPathExt.Path_FixSlashes();

		char* animScriptBuffer = (char*)g_fileSystem->GetFileBuffer(textureAnimPathExt);
		if (animScriptBuffer)
		{
			Array<EqString> frameFilenames(PP_SL);
			xstrsplit(animScriptBuffer, "\n", frameFilenames);
			for (int i = 0; i < frameFilenames.numElem(); i++)
			{
				// delete carriage return character if any
				EqString animFrameFilename = frameFilenames[i].TrimChar('\r', true, true);
				textureNames.append(animFrameFilename);
			}

			PPFree(animScriptBuffer);
		}
		else
		{
			textureNames.append(texturePath);
		}
	}
}

void CTextureLoader::Initialize(const char* texturePath, const char* textureSRCPath)
{
	m_texturePath = texturePath;
	m_textureSRCPath = textureSRCPath;

	m_texturePath.Path_FixSlashes();
	m_textureSRCPath.Path_FixSlashes();
}

ITexturePtr CTextureLoader::LoadTextureFromFileSync(const char* pszFileName, const SamplerStateParams& samplerParams, int nFlags)
{
	HOOK_TO_CVAR(r_allowSourceTextures);

	bool isJustCreated = false;
	ITexturePtr texture = g_renderAPI->FindOrCreateTexture(pszFileName, isJustCreated);

	if (!texture)
		return (nFlags & TEXFLAG_NULL_ON_ERROR) ? nullptr : g_renderAPI->GetErrorTexture();

	if (!isJustCreated)
		return texture;

	if (r_skipTextureLoading.GetBool())
	{
		if (nFlags & TEXFLAG_NULL_ON_ERROR)
			texture = nullptr;
		else
			texture->GenerateErrorTexture(nFlags);

		return texture;
	}

	PROF_EVENT("Load Texture from file");

	Array<EqString> textureNames(PP_SL);
	AnimGetImagesForTextureName(textureNames, pszFileName);

	EqString texNameStr(pszFileName);
	texNameStr.Path_FixSlashes();

	Array<CImage::PTR_T> imgList(PP_SL);

	// load frames
	for (int i = 0; i < textureNames.numElem(); i++)
	{
		CImage::PTR_T img = CRefPtr_new(CImage);

		EqString texturePathExt;
		CombinePath(texturePathExt, m_texturePath.ToCString(), textureNames[i].ToCString());
		bool isLoaded = img->LoadDDS(texturePathExt + TEXTURE_DEFAULT_EXTENSION, 0);

		if (!isLoaded && r_allowSourceTextures->GetBool())
		{
			CombinePath(texturePathExt, m_textureSRCPath.ToCString(), textureNames[i].ToCString());
			isLoaded = img->LoadTGA(texturePathExt + TEXTURE_SECONDARY_EXTENSION);
		}

		img->SetName(textureNames[i].ToCString());

		if (r_noMip.GetBool())
			img->RemoveMipMaps(0, 1);

		if (isLoaded)
		{
			const ShaderAPICaps& caps = g_renderAPI->GetCaps();

			if(!caps.textureFormatsSupported[img->GetFormat()])
			{
				MsgWarning("Texture has unsupported format: %s\n", texturePathExt.ToCString());
				continue;
			}

			if (g_renderAPI->GetShaderAPIClass() == SHADERAPI_DIRECT3D9)
			{
				if (img->GetFormat() == FORMAT_RGB8 || img->GetFormat() == FORMAT_RGBA8)
					img->SwapChannels(0, 2); // convert to BGR

				// Convert if needed and upload datas
				if (img->GetFormat() == FORMAT_RGB8) // as the D3DFMT_X8R8G8B8 used
					img->Convert(FORMAT_RGBA8);
			}

			imgList.append(img);

			if (r_reportTextureLoading.GetBool())
				MsgInfo("Texture loaded: %s\n", texturePathExt.ToCString());
		}
		else
		{
			MsgError("Can't open texture \"%s\"\n", texturePathExt.ToCString());
		}
	}

	const ShaderAPICaps& caps = g_renderAPI->GetCaps();
	caps.maxTextureAnisotropicLevel;

	// initialize texture
	if (!imgList.numElem() || !texture->Init(samplerParams, imgList, nFlags | TEXFLAG_PROGRESSIVE_LODS))
	{
		if (nFlags & TEXFLAG_NULL_ON_ERROR)
			texture = nullptr;
		else
			texture->GenerateErrorTexture(nFlags);
	}

	return texture;
}

Future<ITexturePtr> CTextureLoader::LoadTextureFromFile(const char* pszFileName, const SamplerStateParams& samplerParams, int nFlags)
{
	PROF_EVENT("Load Texture from file");

	// TODO: stream lods gradually

	return Future<ITexturePtr>::Failure(-1, "None");
}