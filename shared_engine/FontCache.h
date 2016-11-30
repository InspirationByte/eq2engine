//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium engine font cache
//////////////////////////////////////////////////////////////////////////////////

#ifndef FONTCACHE_H
#define FONTCACHE_H

#include "IFontCache.h"

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
	DkList<eqFontsInternal::eqFontStyleInfo_t*>		sizeTable;

	IEqFont*	FindBestSize(int bestSize, int styleFlags = TEXT_STYLE_REGULAR);
};

//-------------------------------------------------------------------------------------

class CEqFontCache : public IEqFontCache
{
	friend class CFont;
public:
	CEqFontCache();
	~CEqFontCache();

	bool					IsInitialized() const {return true;}
	const char*				GetInterfaceName() const { return ""; }

	bool					Init();
	void					Shutdown();

	void					ReloadFonts();

	// finds font
	IEqFont*				GetFont(const char* name, int bestSize, int styleFlags = TEXT_STYLE_REGULAR, bool defaultIfNotFound = true) const;
	eqFontFamily_t*			GetFamily(const char* name) const;

	IMaterial*				GetSDFMaterial() const {return m_sdfRegular;}

protected:

	bool					LoadFontDescriptionFile( const char* filename );

	DkList<eqFontFamily_t*>	m_fonts;
	eqFontFamily_t*			m_defaultFont;

	IMaterial*				m_sdfRegular;
};

#endif // FONTCACHE_H