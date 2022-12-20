//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Texture loader helper utility
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "materialsystem1/ITextureLoader.h"

class CTextureLoader : public ITextureLoader
{
public:
	ITexture*			LoadTextureFromFileSync(const char* pszFileName, const SamplerStateParam_t& samplerParams, int nFlags = 0);
	Future<ITexture*>	LoadTextureFromFile(const char* pszFileName, const SamplerStateParam_t& samplerParams, int nFlags = 0);
};