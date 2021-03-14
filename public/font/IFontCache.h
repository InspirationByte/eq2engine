//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium engine font cache
//////////////////////////////////////////////////////////////////////////////////

#ifndef IFONTCACHE_H
#define IFONTCACHE_H

#include "IFont.h"
#include "core/InterfaceManager.h"

#define FONTCACHE_INTERFACE_VERSION		"ENGINE_FontCache_002"

struct eqFontFamily_t;

//-------------------------------------------------------------------------------------
// equilibrium font cache interface
//-------------------------------------------------------------------------------------

class IEqFontCache : public IEqCoreModule
{
public:
	virtual						~IEqFontCache() {}

	virtual bool				Init() = 0;
	virtual void				Shutdown() = 0;

	virtual void				ReloadFonts() = 0;

	// finds font
	virtual IEqFont*			GetFont(const char* name, int bestSize, int styleFlags = TEXT_STYLE_REGULAR, bool defaultIfNotFound = true) const = 0;
	virtual eqFontFamily_t*		GetFamily(const char* name) const = 0;
};

extern IEqFontCache* g_fontCache;

#endif // IFONTCACHE_H