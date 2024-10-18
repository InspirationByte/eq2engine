//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
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
	if(name)						\
	{								\
		CFont* lfont = PPNew CFont();	\
		if( lfont->LoadFont( name ) )\
			v = lfont;				\
		else						\
			delete lfont;			\
	}


eqFontStyleInfo_t::~eqFontStyleInfo_t()
{
	SAFE_DELETE(regularFont);
	SAFE_DELETE(boldFont);
	SAFE_DELETE(italicFont);
	SAFE_DELETE(boldItalicFont);
}

//---------------------------------------------------------------------

CEqFontCache::CEqFontCache()
{
	g_eqCore->RegisterInterface(this);
}

CEqFontCache::~CEqFontCache()
{
	g_eqCore->UnregisterInterface<CEqFontCache>();
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
	for(const KVSection* fontSec : sec->Keys())
	{
		if( !fontSec->IsSection() )
		{
			if(!fontSec->name.Compare("#include"))
			{
				const char* incFileName = KV_GetValueString(fontSec,0, nullptr);

				if(incFileName)
					LoadFontDescriptionFile( incFileName );
				else
					MsgError("'%s' #include error: no filename specified\n", filename);
			}

			continue;
		}

		const int nameHash = StringId24(fontSec->GetName());
		if (m_fonts.contains(nameHash))
		{
			MsgWarning("Font %s already loaded, skipping\n", fontSec->GetName());
			continue;
		}

		eqFontFamily_t& familyEntry = m_fonts[nameHash];
		familyEntry.name = fontSec->name;

		int styleErrorCounter = 0;

		// enum font sizes
		for(const KVSection* styleTable : fontSec->Keys())
		{
			const int entrySize = atoi(styleTable->GetName());

			// find a styles, reg, bld, itl, or bolditalic
			EqStringRef regularFileName;
			EqStringRef boldFileName;
			EqStringRef italicFileName;
			EqStringRef boldItalicFileName;

			if (!styleTable->Get("reg").GetValues(regularFileName))
			{
				MsgError("Font desc '%s' (size %d) missing reg (regular) font entry\n", familyEntry.name.ToCString(), entrySize);
				continue;
			}
			styleTable->Get("bld").GetValues(boldFileName);
			styleTable->Get("itl").GetValues(italicFileName);
			styleTable->Get("b+i").GetValues(boldItalicFileName);
			
			// first we loading a regular font
			CFont* regFont = PPNew CFont();

			if( !regFont->LoadFont(regularFileName) )
			{
				MsgError("Failed to load font style '%s' (regular entry) in '%s'\n", regularFileName.ToCString(), familyEntry.name.ToCString());

				delete regFont;
				styleErrorCounter++;
				continue;
			}

			// now alloc
			eqFontStyleInfo_t& fontStyleInfo = familyEntry.sizeTable.append();
			fontStyleInfo.size = entrySize;
			fontStyleInfo.regularFont = regFont;
			
			FONT_LOADSTYLE(fontStyleInfo.boldFont, boldFileName);
			FONT_LOADSTYLE(fontStyleInfo.italicFont, italicFileName);
			FONT_LOADSTYLE(fontStyleInfo.boldItalicFont, boldItalicFileName);
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
			arraySort(familyEntry.sizeTable, [](const eqFontStyleInfo_t & a, const eqFontStyleInfo_t & b)
			{
				return a.size - b.size;
			});
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

	m_sdfMaterial = g_matSystem->CreateMaterial("_sdfRegular", &sdfFontParams);
	m_fontParams = m_sdfMaterial->GetMaterialVar("FontParams", "[0.94 0.06, 0, 1]");
	m_fontBaseColor = m_sdfMaterial->GetMaterialVar("FontBaseColor", "[1 1 1 1]");

	m_shadowParams = m_sdfMaterial->GetMaterialVar("ShadowParams", "[0.94 0.95 0, 1]");
	m_shadowColor = m_sdfMaterial->GetMaterialVar("ShadowColor", "[0 0 0 1]");
	m_shadowOffset = m_sdfMaterial->GetMaterialVar("ShadowOffset", "[0 0]");

	m_sdfMaterial->LoadShaderAndTextures();

	return true;	
}

void CEqFontCache::Shutdown()
{
	m_fonts.clear(true);
	m_defaultFont = nullptr;
	m_sdfMaterial = nullptr;
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
	const int nameHash = StringId24(name);
	auto it = m_fonts.find(nameHash);
	if (it.atEnd())
		return m_defaultFont;

	return &(*it);
}