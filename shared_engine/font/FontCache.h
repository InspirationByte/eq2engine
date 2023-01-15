//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
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

namespace eqFontsInternal
{
struct eqFontStyleInfo_t
{
	eqFontStyleInfo_t() : 
		regularFont(nullptr), boldFont(nullptr), italicFont(nullptr), boldItalicFont(nullptr)
	{
	}

	~eqFontStyleInfo_t();

	int						size;	// size in pixels

	IEqFont*				regularFont;

	IEqFont*				boldFont;
	IEqFont*				italicFont;
	IEqFont*				boldItalicFont;
};
}

struct eqFontFamily_t
{
	EqString										name;		// TODO: use string hashes
	Array<eqFontsInternal::eqFontStyleInfo_t>		sizeTable{ PP_SL };

	IEqFont* FindBestSize(int bestSize, int styleFlags = TEXT_STYLE_REGULAR) const;
};

//-------------------------------------------------------------------------------------

class CEqFontCache : public IEqFontCache
{
	friend class CFont;
public:
	CEqFontCache();
	~CEqFontCache();

	bool					IsInitialized() const {return true;}
	const char*				GetInterfaceName() const { return FONTCACHE_INTERFACE_VERSION; }

	bool					Init();
	void					Shutdown();

	bool					LoadFontDescriptionFile(const char* filename);

	void					ReloadFonts();

	// finds font
	IEqFont*				GetFont(const char* name, int bestSize, int styleFlags = TEXT_STYLE_REGULAR, bool defaultIfNotFound = true) const;
	eqFontFamily_t*			GetFamily(const char* name) const;

protected:

	Map<int, eqFontFamily_t>	m_fonts{ PP_SL };
	eqFontFamily_t*				m_defaultFont{ nullptr };

	IMaterialPtr				m_sdfMaterial;
	MatVarProxy					m_fontParams;
};
