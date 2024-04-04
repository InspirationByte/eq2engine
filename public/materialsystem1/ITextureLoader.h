//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Texture loader helper utility
//////////////////////////////////////////////////////////////////////////////////

#pragma once

static constexpr const char* TEXTURE_DEFAULT_EXTENSION = ".dds";
static constexpr const char* TEXTURE_SECONDARY_EXTENSION = ".tga";
static constexpr const char* TEXTURE_ANIMATED_EXTENSION = ".ati";			// ATI - Animated Texture Index file

class ITexture;
using ITexturePtr = CRefPtr<ITexture>;

struct SamplerStateParams;

class ITextureLoader : public IEqCoreModule
{
public:
	CORE_INTERFACE("E2_TexLoader_002")

	virtual ITexturePtr			LoadTextureFromFileSync(const char* pszFileName, const SamplerStateParams& samplerParams, int nFlags = 0, const char* requestedBy = nullptr) = 0;
	virtual Future<ITexturePtr>	LoadTextureFromFile(const char* pszFileName, const SamplerStateParams& samplerParams, int nFlags = 0, const char* requestedBy = nullptr) = 0;

	virtual const char*			GetTexturePath() const = 0;
	virtual const char*			GetTextureSRCPath() const = 0;
};

INTERFACE_SINGLETON(ITextureLoader, CTextureLoader, g_texLoader)
