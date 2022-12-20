//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Texture loader helper utility
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#define TEXTURELOADER_INTERFACE_VERSION "TexLoader_001"

class ITexture;
typedef struct SamplerStateParam_s SamplerStateParam_t;

class ITextureLoader : public IEqCoreModule
{
public:
	bool			IsInitialized() const { return true; }
	const char*		GetInterfaceName() const { return TEXTURELOADER_INTERFACE_VERSION; }

	virtual ITexture*			LoadTextureFromFileSync(const char* pszFileName, const SamplerStateParam_t& samplerParams, int nFlags = 0) = 0;
	virtual Future<ITexture*>	LoadTextureFromFile(const char* pszFileName, const SamplerStateParam_t& samplerParams, int nFlags = 0) = 0;
};

INTERFACE_SINGLETON(ITextureLoader, CTextureLoader, TEXTURELOADER_INTERFACE_VERSION, g_texLoader)
