//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium engine font cache
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "font/IFontCache.h"
#include "materialsystem1/IMaterialVar.h"

class IMaterial;
class ITexture;

using IMaterialPtr = CRefPtr<IMaterial>;
//-------------------------------------------------------------------------------------

class CEqFontCache : public IEqFontCache
{
	friend class CFont;
public:
	CEqFontCache();
	~CEqFontCache();

	bool			IsInitialized() const { return true; }

	bool			Init();
	void			Shutdown();

	bool			LoadFontDescriptionFile(const char* filename);
	void			ReloadFonts();

	// finds font
	IEqFont*		GetFont(const char* name, int bestSize, int styleFlags = TEXT_STYLE_REGULAR, bool defaultIfNotFound = true) const;

protected:

	struct Style
	{
		~Style();

		IEqFont*	regularFont{ nullptr };
		IEqFont*	boldFont{ nullptr };
		IEqFont*	italicFont{ nullptr };
		IEqFont*	boldItalicFont{ nullptr };
		int			size{ 0 };	// size in pixels
	};

	struct FontFamily
	{
		EqString		name;
		Array<Style>	sizeTable{ PP_SL };

		IEqFont* FindBestSize(int bestSize, int styleFlags = TEXT_STYLE_REGULAR) const;
	};


	FontFamily*				GetFamily(const char* name) const;

	Map<int, FontFamily>	m_fonts{ PP_SL };
	FontFamily*				m_defaultFont{ nullptr };

	IMaterialPtr		m_sdfMaterial;

	MatVec4Proxy		m_fontBaseColor;
	MatVec4Proxy		m_fontParams;

	MatVec4Proxy		m_shadowColor;
	MatVec4Proxy		m_shadowParams;
	MatVec2Proxy		m_shadowOffset;
};
