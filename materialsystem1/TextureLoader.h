//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Texture loader helper utility
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "materialsystem1/ITextureLoader.h"
struct MaterialsInitSettings;

class CTextureLoader : public ITextureLoader
{
public:
	void				Initialize(const char* texturePath, const char* textureSRCPath);

	ITexturePtr			LoadTextureFromFileSync(const char* pszFileName, const SamplerStateParams& samplerParams, int nFlags = 0);
	Future<ITexturePtr>	LoadTextureFromFile(const char* pszFileName, const SamplerStateParams& samplerParams, int nFlags = 0);

	EqString			m_texturePath;
	EqString			m_textureSRCPath;
};