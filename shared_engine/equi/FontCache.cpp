//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium engine font cache
//////////////////////////////////////////////////////////////////////////////////

#include "FontCache.h"
#include "Font.h"

#define FONTBUFFER_MAX (16384)

using namespace eqFontsInternal;

static CEqFontCache s_fontCache;
IEqFontCache* g_fontCache = &s_fontCache;

#define FONT_DEFAULT_LIST_FILENAME "resources/fonts.res"

#define FONT_LOADSTYLE(v, name)		\
	if(name != NULL)				\
	{								\
		CFont* lfont = new CFont();	\
		if( lfont->LoadFont( name ) )\
			v = lfont;				\
		else						\
			delete lfont;			\
	}

int compareFontSizes( eqFontStyleInfo_t* const &a, eqFontStyleInfo_t* const &b )
{
	return a->size - b->size;
}

eqFontStyleInfo_t::~eqFontStyleInfo_t()
{
	delete regularFont;
	delete boldFont;
	delete italicFont;
	delete boldItalicFont;
}

//---------------------------------------------------------------------

CEqFontCache::CEqFontCache() : 
	m_defaultFont(nullptr),
	m_sdfMaterial(nullptr),
	m_sdfRange(nullptr)
{
	GetCore()->RegisterInterface(FONTCACHE_INTERFACE_VERSION, this);
}

CEqFontCache::~CEqFontCache()
{
	GetCore()->UnregisterInterface(FONTCACHE_INTERFACE_VERSION);
}

bool CEqFontCache::LoadFontDescriptionFile( const char* filename )
{
	// load font resource list in mods directory
	KeyValues kvs;
	if(!kvs.LoadFromFile(filename, -1))
	{
		MsgError("ERROR: Cannot open font description file \"%s\"!", filename);
		return false;
	}

	kvkeybase_t* sec = kvs.GetRootSection();

	// enum font names
	for(int i = 0; i < sec->keys.numElem(); i++)
	{
		kvkeybase_t* fontSec = sec->keys[i];

		if( !fontSec->IsSection() )
		{
			if(!strcmp(fontSec->name, "#include"))
			{
				const char* incFileName = KV_GetValueString(fontSec,0,NULL);

				if(incFileName)
					LoadFontDescriptionFile( incFileName );
				else
					MsgError("'%s' #include error: no filename specified\n", filename);
			}

			continue;
		}

		char* fontName = fontSec->name;

		eqFontFamily_t* familyEntry = new eqFontFamily_t;
		familyEntry->name = fontName;

		int styleErrorCounter = 0;

		// enum font sizes
		for(int j = 0; j < fontSec->keys.numElem(); j++)
		{
			kvkeybase_t* styleTable = fontSec->keys[j];
			int entrySize = atoi(styleTable->name);

			// find a styles, reg, bld, itl, or bolditalic
			kvkeybase_t* regular = styleTable->FindKeyBase("reg");
			kvkeybase_t* bold = styleTable->FindKeyBase("bld");
			kvkeybase_t* italic = styleTable->FindKeyBase("itl");
			kvkeybase_t* bolditalic = styleTable->FindKeyBase("b+i");
			
			// first we loading a regular font
			CFont* regFont = new CFont();

			if( !regFont->LoadFont( KV_GetValueString(regular, 0, "") ) )
			{
				MsgError("Failed to load font style '%s' (regular entry) in '%s'\n", KV_GetValueString(regular, 0, ""), fontName);

				delete regFont;
				styleErrorCounter++;
				continue;
			}

			// now alloc
			eqFontStyleInfo_t* fontStyleInfo = new eqFontStyleInfo_t;
			fontStyleInfo->size = entrySize;

			
			fontStyleInfo->regularFont = regFont;
			
			FONT_LOADSTYLE(fontStyleInfo->boldFont, KV_GetValueString(bold, 0, NULL));
			FONT_LOADSTYLE(fontStyleInfo->italicFont, KV_GetValueString(italic, 0, NULL));
			FONT_LOADSTYLE(fontStyleInfo->boldItalicFont, KV_GetValueString(bolditalic, 0, NULL));
			
			// add style/size entry to table
			familyEntry->sizeTable.append( fontStyleInfo );
		}

		if( familyEntry->sizeTable.numElem() == 0 )
		{
			if(styleErrorCounter == 0)
				MsgWarning("Warning: Font family '%s' has empty size/style table\n", fontName);
			else
				MsgWarning("Warning: Font family '%s' style/size table is empty and has errors while loading\n", fontName);

			delete familyEntry;
			continue;
		}
		else
		{
			// sort the fonts by size
			familyEntry->sizeTable.sort( compareFontSizes );
		}

		// any font description file may redefine default font
		if(!strcmp(fontName, "default"))
			m_defaultFont = familyEntry;

		m_fonts.append(familyEntry);
	}

	return true;	
}

bool CEqFontCache::Init()
{
	if( !LoadFontDescriptionFile( FONT_DEFAULT_LIST_FILENAME ) )
		return false;

	if(!m_defaultFont)
		MsgError("ERROR: No default font specified in '%s'!\n", FONT_DEFAULT_LIST_FILENAME);

	kvkeybase_t sdfFontParams;
	sdfFontParams.SetName("SDFFont");
	sdfFontParams.SetKey("basetexture", "$basetexture");

	m_sdfMaterial = materials->CreateMaterial("_sdfRegular", &sdfFontParams);
	m_sdfMaterial->Ref_Grab();

	m_sdfRange = m_sdfMaterial->GetMaterialVar("range", "[0.94 0.06]");

	m_sdfMaterial->LoadShaderAndTextures();

	return true;	
}

void CEqFontCache::Shutdown()
{
	for(int i = 0; i < m_fonts.numElem(); i++)
	{
		eqFontFamily_t* family = m_fonts[i];

		for(int j = 0; j < family->sizeTable.numElem(); j++)
			delete family->sizeTable[j];

		delete family;
	}

	m_fonts.clear();
	m_defaultFont = nullptr;

	materials->FreeMaterial( m_sdfMaterial );
	m_sdfMaterial = nullptr;
	m_sdfRange = nullptr;
}

void CEqFontCache::ReloadFonts()
{
	ASSERTMSG(false, "Please implement CEqFontCache::ReloadFonts() !!!");
}

IEqFont* eqFontFamily_t::FindBestSize( int bestSize, int styleFlags )
{
	eqFontStyleInfo_t* bestSizeStyleInfo = nullptr;

	for(int i = 0; i < sizeTable.numElem(); i++)
	{
		eqFontStyleInfo_t* styleInfo = sizeTable[i];

		if(bestSizeStyleInfo == nullptr)
			bestSizeStyleInfo = styleInfo;

		// find the best size-fitting style of the font
#pragma fixme("better size picking handling???")
		if(bestSize >= styleInfo->size)
			bestSizeStyleInfo = styleInfo;
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
	eqFontFamily_t* family = GetFamily(name);

	if(!family)
	{
		if(defaultIfNotFound)
			return GetFont("default", bestSize, styleFlags);
		else
			return NULL;
	}

	return family->FindBestSize(bestSize, styleFlags);
}

eqFontFamily_t* CEqFontCache::GetFamily(const char* name) const
{
	for(int i = 0; i < m_fonts.numElem(); i++)
	{
		eqFontFamily_t* family = m_fonts[i];

		if( !family->name.Compare(name) )
		{
			return family;
		}
	}

	return m_defaultFont;
}