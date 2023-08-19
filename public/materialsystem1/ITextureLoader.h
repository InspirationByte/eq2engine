//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Texture loader helper utility
//////////////////////////////////////////////////////////////////////////////////

#pragma once

class ITexture;
using ITexturePtr = CRefPtr<ITexture>;

struct SamplerStateParams;

class ITextureLoader : public IEqCoreModule
{
public:
	CORE_INTERFACE("E2_TexLoader_001")

	bool			IsInitialized() const { return true; }

	virtual ITexturePtr			LoadTextureFromFileSync(const char* pszFileName, const SamplerStateParams& samplerParams, int nFlags = 0) = 0;
	virtual Future<ITexturePtr>	LoadTextureFromFile(const char* pszFileName, const SamplerStateParams& samplerParams, int nFlags = 0) = 0;
};

INTERFACE_SINGLETON(ITextureLoader, CTextureLoader, g_texLoader)
