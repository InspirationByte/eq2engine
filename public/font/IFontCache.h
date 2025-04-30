//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium engine font cache
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "IFont.h"

//-------------------------------------------------------------------------------------
// equilibrium font cache interface
//-------------------------------------------------------------------------------------

class IEqFontCache : public IEqCoreModule
{
public:
	CORE_INTERFACE("E2_FontCache_003")

	virtual					~IEqFontCache() {}

	virtual bool			Init() = 0;
	virtual void			Shutdown() = 0;

	virtual bool			LoadFontDescriptionFile(const char* filename) = 0;
	virtual void			ReloadFonts() = 0;

	// finds font
	virtual IEqFont*		GetFont(const char* name, int bestSize, int styleFlags = TEXT_STYLE_REGULAR, bool defaultIfNotFound = true) const = 0;
};

extern IEqFontCache* g_fontCache;