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
	bool				IsInitialized() const { return m_texturePath.Length() > 0; }

	void				Initialize(const char* texturePath, const char* textureSRCPath);

	ITexturePtr			LoadTextureFromFileSync(const char* pszFileName, const SamplerStateParams& samplerParams, int nFlags = 0, const char* requestedBy = nullptr);
	Future<ITexturePtr>	LoadTextureFromFile(const char* pszFileName, const SamplerStateParams& samplerParams, int nFlags = 0, const char* requestedBy = nullptr);

	const char*			GetTexturePath() const { return m_texturePath; }
	const char*			GetTextureSRCPath() const { return m_textureSRCPath; }

	EqString			m_texturePath;
	EqString			m_textureSRCPath;
};