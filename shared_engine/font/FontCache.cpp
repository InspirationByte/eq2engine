//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium engine font cache
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IDkCore.h"
#include "utils/KeyValues.h"

#include "FontCache.h"
#include "Font.h"

#include "materialsystem1/IMaterialSystem.h"

#define FONTBUFFER_MAX (16384)

using namespace eqFontsInternal;

static CEqFontCache s_fontCache;
IEqFontCache* g_fontCache = &s_fontCache;

#define FONT_DEFAULT_LIST_FILENAME "resources/fonts.res"

#define FONT_LOADSTYLE(v, name)		\
	if(name != nullptr)				\
	{								\
		CFont* lfont = PPNew CFont();	\
		if( lfont->LoadFont( name ) )\
			v = lfont;				\
		else						\
			delete lfont;			\
	}

int compareFontSizes( const eqFontStyleInfo_t &a, const eqFontStyleInfo_t &b )
{
	return a.size - b.size;
}

eqFontStyleInfo_t::~eqFontStyleInfo_t()
{
	delete regularFont;
	delete boldFont;
	delete italicFont;
	delete boldItalicFont;
}

//---------------------------------------------------------------------

CEqFontCache::CEqFontCache()
{
	g_eqCore->RegisterInterface(FONTCACHE_INTERFACE_VERSION, this);
}

CEqFontCache::~CEqFontCache()
{
	g_eqCore->UnregisterInterface(FONTCACHE_INTERFACE_VERSION);
}

bool CEqFontCache::LoadFontDescriptionFile( const char* filename )
{
	// load font resource list in mods directory
	KeyValues kvs;
	if(!kvs.LoadFromFile(filename, -1))
	{
		MsgError("ERROR: Cannot open font description file \"%s\"!\n", filename);
		return false;
	}

	KVSection* sec = kvs.GetRootSection();

	// enum font names
	for(int i = 0; i < sec->keys.numElem(); i++)
	{
		KVSection* fontSec = sec->keys[i];

		if( !fontSec->IsSection() )
		{
			if(!strcmp(fontSec->name, "#include"))
			{
				const char* incFileName = KV_GetValueString(fontSec,0, nullptr);

				if(incFileName)
					LoadFontDescriptionFile( incFileName );
				else
					MsgError("'%s' #include error: no filename specified\n", filename);
			}

			continue;
		}

		eqFontFamily_t& familyEntry = m_fonts.append();
		familyEntry.name = fontSec->name;

		int styleErrorCounter = 0;

		// enum font sizes
		for(int j = 0; j < fontSec->keys.numElem(); j++)
		{
			KVSection* styleTable = fontSec->keys[j];
			int entrySize = atoi(styleTable->name);

			// find a styles, reg, bld, itl, or bolditalic
			KVSection* regular = styleTable->FindSection("reg");
			KVSection* bold = styleTable->FindSection("bld");
			KVSection* italic = styleTable->FindSection("itl");
			KVSection* bolditalic = styleTable->FindSection("b+i");
			
			// first we loading a regular font
			CFont* regFont = PPNew CFont();

			if( !regFont->LoadFont( KV_GetValueString(regular, 0, "") ) )
			{
				MsgError("Failed to load font style '%s' (regular entry) in '%s'\n", KV_GetValueString(regular, 0, ""), familyEntry.name.ToCString());

				delete regFont;
				styleErrorCounter++;
				continue;
			}

			// now alloc
			eqFontStyleInfo_t& fontStyleInfo = familyEntry.sizeTable.append();
			fontStyleInfo.size = entrySize;
			fontStyleInfo.regularFont = regFont;
			
			FONT_LOADSTYLE(fontStyleInfo.boldFont, KV_GetValueString(bold, 0, nullptr));
			FONT_LOADSTYLE(fontStyleInfo.italicFont, KV_GetValueString(italic, 0, nullptr));
			FONT_LOADSTYLE(fontStyleInfo.boldItalicFont, KV_GetValueString(bolditalic, 0, nullptr));
		}

		if( familyEntry.sizeTable.numElem() == 0 )
		{
			if(styleErrorCounter == 0)
				MsgWarning("Warning: Font family '%s' has empty size/style table\n", familyEntry.name.ToCString());
			else
				MsgWarning("Warning: Font family '%s' style/size table is empty and has errors while loading\n", familyEntry.name.ToCString());

			continue;
		}
		else
		{
			// sort the fonts by size
			familyEntry.sizeTable.sort( compareFontSizes );
		}

		// any font description file may redefine default font
		if(!familyEntry.name.CompareCaseIns("default"))
			m_defaultFont = &familyEntry;
	}

	return true;	
}

bool CEqFontCache::Init()
{
	if( !LoadFontDescriptionFile( FONT_DEFAULT_LIST_FILENAME ) )
		return false;

	if(!m_defaultFont)
		MsgError("ERROR: No default font specified in '%s'!\n", FONT_DEFAULT_LIST_FILENAME);

	KVSection sdfFontParams;
	sdfFontParams.SetName("SDFFont");
	sdfFontParams.SetKey("basetexture", "$basetexture");

	m_sdfMaterial = materials->CreateMaterial("_sdfRegular", &sdfFontParams);
	m_fontParams = m_sdfMaterial->GetMaterialVar("FontParams", "[0.94 0.06, 0, 1]");

	m_sdfMaterial->LoadShaderAndTextures();

	return true;	
}

void CEqFontCache::Shutdown()
{
	m_fonts.clear();
	m_defaultFont = nullptr;

	m_sdfMaterial = nullptr;
	m_fontParams = nullptr;
	m_fontParams = nullptr;
}

void CEqFontCache::ReloadFonts()
{
	ASSERT_FAIL("Please implement CEqFontCache::ReloadFonts() !!!");
}

IEqFont* eqFontFamily_t::FindBestSize( int bestSize, int styleFlags ) const
{
	const eqFontStyleInfo_t* bestSizeStyleInfo = nullptr;

	for(int i = 0; i < sizeTable.numElem(); i++)
	{
		const eqFontStyleInfo_t& styleInfo = sizeTable[i];

		if(bestSizeStyleInfo == nullptr)
			bestSizeStyleInfo = &styleInfo;

		// find the best size-fitting style of the font
		if(bestSize >= styleInfo.size)
			bestSizeStyleInfo = &styleInfo;
	}

	if(bestSizeStyleInfo == nullptr)
		return nullptr;

	// best size found, find a best style
	if( (styleFlags & TEXT_STYLE_BOLD) && (styleFlags & TEXT_STYLE_ITALIC) &&
		bestSizeStyleInfo->boldItalicFont != nullptr)
	{
		return bestSizeStyleInfo->boldItalicFont;
	}
	else if( (styleFlags & TEXT_STYLE_BOLD) &&
		bestSizeStyleInfo->boldFont != nullptr)
	{
		// bold has higher priority
		return bestSizeStyleInfo->boldFont;
	}
	else if( (styleFlags & TEXT_STYLE_ITALIC) &&
		bestSizeStyleInfo->italicFont != nullptr)
	{
		return bestSizeStyleInfo->italicFont;
	}
	
	return bestSizeStyleInfo->regularFont;
}

// finds font
IEqFont* CEqFontCache::GetFont(const char* name, int bestSize, int styleFlags, bool defaultIfNotFound) const
{
	const eqFontFamily_t* family = GetFamily(name);

	if(!family)
	{
		if(defaultIfNotFound)
			return GetFont("default", bestSize, styleFlags, false);
		else
			return nullptr;
	}

	return family->FindBestSize(bestSize, styleFlags);
}

eqFontFamily_t* CEqFontCache::GetFamily(const char* name) const
{
	for(int i = 0; i < m_fonts.numElem(); i++)
	{
		const eqFontFamily_t& family = m_fonts[i];

		if( !family.name.Compare(name) )
		{
			return const_cast<eqFontFamily_t*>(&family);
		}
	}

	return m_defaultFont;
}