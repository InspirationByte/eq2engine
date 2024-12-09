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

DECLARE_CVAR(r_reportTextureLoading, "0", "Echo textrue loading", 0);
DECLARE_CVAR(r_skipTextureLoading, "0", nullptr, CV_CHEAT);
DECLARE_CVAR(r_noMip, "0", nullptr, CV_CHEAT);

static void AnimGetImagesForTextureName(Array<EqString>& textureNames, const char* pszFileName)
{
	textureNames.clear();

	EqString texturePath(pszFileName);
	fnmPathFixSeparators(texturePath);

	// has pattern for animated texture?
	int animCountStart = texturePath.Find("[");
	int animCountEnd = -1;

	if (animCountStart != -1 && (animCountEnd = texturePath.Find("]", false, animCountStart)) != -1)
	{
		// trying to load animated texture
		EqString textureWildcard = texturePath.Left(animCountStart);
		EqString textureFrameCount = texturePath.Mid(animCountStart + 1, (animCountEnd - animCountStart) - 1);
		const int numFrames = atoi(textureFrameCount);

		if (r_reportTextureLoading.GetBool())
			Msg("Loading animated %d animated textures (%s)\n", numFrames, textureWildcard.ToCString());

		textureNames.reserve(numFrames);
		for (int i = 0; i < numFrames; i++)
			textureNames.append(EqString::Format(textureWildcard, i));
	}
	else
	{
		// try loading older Animated Texture Index file
		char* animScriptBuffer = (char*)g_fileSystem->GetFileBuffer(fnmPathApplyExt(texturePath, TEXTURE_ANIMATED_EXTENSION));
		if (animScriptBuffer)
		{
			Array<EqString> frameFilenames(PP_SL);
			StringSplit(animScriptBuffer, "\n", frameFilenames);
			for (const EqString& name : frameFilenames)
			{
				// delete carriage return character if any
				textureNames.append(name.TrimChar('\r', true, true));
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

	fnmPathFixSeparators(m_texturePath);
	fnmPathFixSeparators(m_textureSRCPath);
}

ITexturePtr CTextureLoader::LoadTextureFromFileSync(const char* pszFileName, const SamplerStateParams& samplerParams, int flags, const char* requestedBy)
{
	const bool nullOnError = (flags & TEXFLAG_LOAD_NULL_ON_ERROR);
	flags &= ~TEXFLAG_LOAD_NULL_ON_ERROR;

	HOOK_TO_CVAR(r_allowSourceTextures);

	bool isJustCreated = false;
	ITexturePtr texture = g_renderAPI->FindOrCreateTexture(pszFileName, isJustCreated);
	if (!texture)
	{
		return nullOnError ? nullptr : g_matSystem->GetErrorCheckerboardTexture((flags & TEXFLAG_CUBEMAP) ? TEXDIMENSION_CUBE : TEXDIMENSION_2D);
	}

	if (!isJustCreated)
		return texture;

	DevMsg(DEVMSG_RENDER, "Loading texture file %s\n", pszFileName);

	auto HandleError = [&texture, flags, nullOnError]() {
		if (nullOnError)
			texture = nullptr;
		else
			texture->GenerateErrorTexture(flags);
	};

	if (r_skipTextureLoading.GetBool())
	{
		HandleError();
		return texture;
	}

	PROF_EVENT("Load Texture from file");

	const ShaderAPICapabilities& caps = g_renderAPI->GetCaps();

	thread_local Array<EqString> textureNames(PP_SL);
	AnimGetImagesForTextureName(textureNames, pszFileName);

	Array<CImage::PTR_T> imgFrames(PP_SL);

	// load frames
	for (const EqString& texName : textureNames)
	{
		CImage::PTR_T img = CRefPtr_new(CImage);

		EqString texturePathExt;
		fnmPathCombine(texturePathExt, m_texturePath, texName);
		bool isLoaded = img->Load(fnmPathApplyExt(texturePathExt, TEXTURE_DEFAULT_EXTENSION), 0);
		if (!isLoaded && r_allowSourceTextures->GetBool())
		{
			fnmPathCombine(texturePathExt, m_textureSRCPath, texName);
			isLoaded = img->Load(fnmPathApplyExt(texturePathExt, TEXTURE_SECONDARY_EXTENSION));
		}

		img->SetName(texName);

		if (r_noMip.GetBool())
			img->RemoveMipMaps(0, 1);

		if (isLoaded)
		{
			if (!caps.IsSupportedTextureFormat(img->GetFormat()))
			{
				MsgWarning("%s: Texture %s unsupported format %d\n", requestedBy, texturePathExt.ToCString(), img->GetFormat());
				continue;
			}

			imgFrames.append(img);

			if (r_reportTextureLoading.GetBool())
				MsgInfo("%s: Texture loaded: %s\n", requestedBy, texturePathExt.ToCString());
		}
		else
		{
			MsgError("%s: Can't open texture \"%s\"\n", requestedBy, texturePathExt.ToCString());
		}
	}

	if (!imgFrames.numElem())
	{
		HandleError();
		return texture;
	}

	CImage::PTR_T textureImg = imgFrames.front();
	if (imgFrames.numElem() > 1)
	{
		MsgWarning("%s: animated texture definitions are deprecated, consider using texture array for faster loading.\n", requestedBy);

		CImage::PTR_T firstImg = textureImg;
		ASSERT_MSG(firstImg->GetArraySize() == 1, "Texture arrays are not supported when animation table is used.");

		textureImg = CRefPtr_new(CImage);
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
			ASSERT_MSG(	firstImg->GetFormat() == frameImg->GetFormat() &&
						firstImg->GetWidth() == frameImg->GetWidth() &&
						firstImg->GetHeight() == frameImg->GetHeight() &&
						firstImg->GetMipMapCount() == frameImg->GetMipMapCount(), "%s: animated textures must share same format, size and mipmap count", requestedBy);

			memcpy(textureData, frameImg->GetPixels(), stride);
			textureData += stride;
		}
	}

	// initialize texture
	if (!texture->Init(textureImg, samplerParams, (flags & ~TEXFLAG_LOAD_NULL_ON_ERROR) | TEXFLAG_PROGRESSIVE_LODS))
		HandleError();

	return texture;
}

Future<ITexturePtr> CTextureLoader::LoadTextureFromFile(const char* pszFileName, const SamplerStateParams& samplerParams, int nFlags, const char* requestedBy)
{
	PROF_EVENT("Load Texture from file");

	// TODO: stream lods gradually

	return Future<ITexturePtr>::Failure(-1, "None");
}