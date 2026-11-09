//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2026
//////////////////////////////////////////////////////////////////////////////////
// Description: Font atlas generator - MSDF glyph atlases and eqfont2 files from TTF / OTF
//////////////////////////////////////////////////////////////////////////////////

#define STB_RECT_PACK_IMPLEMENTATION
#define STBRP_STATIC
#include <stb_rect_pack.h>
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include <stb_truetype.h>
#include <msdfgen/msdfgen.h>
#include <msdfgen/core/ShapeDistanceFinder.h>

#include "core/core_common.h"
#include "core/IFileSystem.h"
#include "utils/KeyValues.h"
#include "imaging/ImageLoader.h"
#include "texcooker_defs.h"

/*

// Font description file (.font) in the source materials folder

// atlas texture in the source materials folder (TGA, no extension); .fnt files reference it
texture		"ui/roboto_msdf";

type		mtsdf;		// mtsdf (default), msdf, sdf, psdf
size		48;			// atlas pixels per em
pxrange		8;			// distance range in atlas pixels
padding		2;			// extra pixels around each glyph box

// characters to include: values are charset files (key-values, UTF-8) relative to this file,
// the section adds entries inline; files and the section use the same keys
charset		"charset_eq2020.txt"
{
	chars	"№€";		// every character of the strings
	range	"А" "я";	// first and last, a single character or a number each
	code	0x2116;		// code points, decimal or 0x hex
}

// font <name in resources/fonts> <source TTF / OTF relative to this file>
// all fonts share one atlas
font	"roboto_msdf"		"ttf/Roboto-Regular.ttf";
font	"roboto_msdf_b"		"ttf/Roboto-Bold.ttf";

*/

static constexpr EqStringRef s_fontsOutputPath = "resources/fonts";
static constexpr EqStringRef s_fontFileExt = "fnt";
static constexpr EqStringRef s_fontsTextureUsage = "usage:fonts";

static constexpr double s_edgeColoringAngle = 3.0;
static constexpr int s_maxFontAtlasSize = 8192;

enum EFontAtlasType : int
{
	FONTATLAS_MTSDF = 0,
	FONTATLAS_MSDF,
	FONTATLAS_SDF,
	FONTATLAS_PSDF,
	FONTATLAS_TYPE_COUNT,
};

static constexpr EqStringRef s_fontAtlasTypeStr[] =
{
	"mtsdf",
	"msdf",
	"sdf",
	"psdf",
};

struct FontAtlasParams
{
	EFontAtlasType	type{ FONTATLAS_MTSDF };
	double			emSize{ 48.0 };
	double			pxRange{ 8.0 };
	int				padding{ 2 };
};

struct FontGlyph
{
	msdfgen::Shape*	shape{ nullptr };	// null for glyphs without contours (spaces)
	int				unicode{ 0 };
	int				glyphIndex{ 0 };
	double			advance{ 0.0 };

	// glyph box in atlas pixels relative to the glyph origin, Y up
	int				boxLeft{ 0 };
	int				boxBottom{ 0 };
	int				boxWidth{ 0 };
	int				boxHeight{ 0 };

	// atlas position of the box, top-left origin
	int				atlasX{ 0 };
	int				atlasY{ 0 };
};

struct FontKerningPair
{
	int				first{ 0 };
	int				second{ 0 };
	double			advance{ 0.0 };
};

struct FontFace
{
	EqString		name;
	EqString		sourceFileName;
	ubyte*			fileData{ nullptr };
	stbtt_fontinfo	info;
	double			emScale{ 1.0 };	// font units to ems

	double			ascender{ 0.0 };
	double			descender{ 0.0 };
	double			lineHeight{ 0.0 };

	Array<FontGlyph>		glyphs{ PP_SL };
	Array<FontKerningPair>	kerning{ PP_SL };
};

//-------------------------------------------------------------------------------------------------
// Charset parsing

static int DecodeUTF8Char(const char*& str)
{
	const ubyte first = static_cast<ubyte>(*str++);
	if (first < 0x80)
		return first;

	int extraBytes = 0;
	int codePoint = 0;
	if ((first & 0xE0) == 0xC0)
	{
		extraBytes = 1;
		codePoint = first & 0x1F;
	}
	else if ((first & 0xF0) == 0xE0)
	{
		extraBytes = 2;
		codePoint = first & 0x0F;
	}
	else if ((first & 0xF8) == 0xF0)
	{
		extraBytes = 3;
		codePoint = first & 0x07;
	}
	else
		return -1;

	for (int i = 0; i < extraBytes; ++i)
	{
		const ubyte next = static_cast<ubyte>(*str);
		if ((next & 0xC0) != 0x80)
			return -1;

		codePoint = (codePoint << 6) | (next & 0x3F);
		++str;
	}

	return codePoint;
}

static bool ParseCharsetNumber(const char* str, int& outValue)
{
	char* end = nullptr;
	const long value = strtol(str, &end, 0);
	if (end == str || *end)
		return false;

	outValue = static_cast<int>(value);
	return true;
}

// range bound: a single character or a number
static bool ParseCharsetRangeBound(const char* str, int& outValue)
{
	const char* next = str;
	const int codePoint = DecodeUTF8Char(next);
	if (codePoint > 0 && *next == 0)
	{
		outValue = codePoint;
		return true;
	}

	return ParseCharsetNumber(str, outValue);
}

// charset section keys:
//	chars	"<characters>" ...;		- every character of the strings
//	range	<first> <last>;			- bounds are single characters or numbers
//	code	<number> ...;			- code points, decimal or 0x hex
static bool ParseCharsetSection(const KVSection& charsetSec, const char* fileName, Array<int>& outCodePoints)
{
	for (int i = 0; i < charsetSec.KeyCount(); ++i)
	{
		const KVSection& key = charsetSec.KeyAt(i);
		const EqStringRef keyName = key.GetName();

		if (!keyName.CompareCaseIns("chars"))
		{
			for (int v = 0; v < key.ValueCount(); ++v)
			{
				const char* str = key.ValueAt(v).GetString();
				while (*str)
				{
					const int codePoint = DecodeUTF8Char(str);
					if (codePoint < 0)
					{
						MsgError("Charset '%s' line %d: invalid UTF-8 text\n", fileName, key.line);
						return false;
					}
					outCodePoints.append(codePoint);
				}
			}
		}
		else if (!keyName.CompareCaseIns("range"))
		{
			int first = 0;
			int last = 0;
			if (key.ValueCount() != 2
				|| !ParseCharsetRangeBound(key.ValueAt(0).GetString(), first)
				|| !ParseCharsetRangeBound(key.ValueAt(1).GetString(), last)
				|| last < first)
			{
				MsgError("Charset '%s' line %d: expected range <first> <last>, a character or a number each\n", fileName, key.line);
				return false;
			}

			for (int codePoint = first; codePoint <= last; ++codePoint)
				outCodePoints.append(codePoint);
		}
		else if (!keyName.CompareCaseIns("code"))
		{
			for (int v = 0; v < key.ValueCount(); ++v)
			{
				int codePoint = 0;
				if (!ParseCharsetNumber(key.ValueAt(v).GetString(), codePoint))
				{
					MsgError("Charset '%s' line %d: invalid code '%s'\n", fileName, key.line, key.ValueAt(v).GetString());
					return false;
				}
				outCodePoints.append(codePoint);
			}
		}
		else
		{
			MsgError("Charset '%s' line %d: unknown key '%s', expected chars, range or code\n", fileName, key.line, keyName.ToCString());
			return false;
		}
	}

	return true;
}

// charset values are charset files relative to the .font file, keys are inline charset entries
static bool LoadCharset(const KVSection& charsetSec, const char* fontSrcFileName, Array<int>& outCodePoints)
{
	const EqString fontDir = fnmPathStripName(fontSrcFileName);
	for (int v = 0; v < charsetSec.ValueCount(); ++v)
	{
		const EqString charsetFileName = fnmPathCombine(fontDir, charsetSec.ValueAt(v).GetString());

		KVSection charsetFile;
		if (!KV_LoadFromFile(charsetFileName, -1, charsetFile))
		{
			MsgError("Can't open charset '%s'\n", charsetFileName.ToCString());
			return false;
		}

		if (!ParseCharsetSection(charsetFile, charsetFileName, outCodePoints))
			return false;
	}

	if (!ParseCharsetSection(charsetSec, fontSrcFileName, outCodePoints))
		return false;

	// sort and remove duplicates
	arraySort(outCodePoints, [](const int a, const int b) {
		return a - b;
	});

	for (int i = outCodePoints.numElem() - 1; i > 0; --i)
	{
		if (outCodePoints[i] == outCodePoints[i - 1])
			outCodePoints.removeIndex(i);
	}

	if (!outCodePoints.numElem())
	{
		MsgError("Font '%s' has empty charset\n", fontSrcFileName);
		return false;
	}

	return true;
}

//-------------------------------------------------------------------------------------------------
// Glyph shapes

static msdfgen::Shape* LoadGlyphShape(const FontFace& face, int glyphIndex)
{
	stbtt_vertex* vertices = nullptr;
	const int vertexCount = stbtt_GetGlyphShape(&face.info, glyphIndex, &vertices);
	defer{
		stbtt_FreeShape(&face.info, vertices);
	};

	if (vertexCount <= 0)
		return nullptr;

	const double scale = face.emScale;
	auto toPoint = [scale](int x, int y) {
		return msdfgen::Point2(x * scale, y * scale);
	};

	msdfgen::Shape* shape = PPNew msdfgen::Shape();
	msdfgen::Contour* contour = nullptr;
	msdfgen::Point2 lastPoint;

	for (int i = 0; i < vertexCount; ++i)
	{
		const stbtt_vertex& vert = vertices[i];
		const msdfgen::Point2 point = toPoint(vert.x, vert.y);

		if (vert.type == STBTT_vmove)
		{
			contour = &shape->addContour();
			lastPoint = point;
			continue;
		}

		if (!contour)
			continue;

		switch (vert.type)
		{
		case STBTT_vline:
			if (point != lastPoint)
				contour->addEdge(msdfgen::EdgeHolder(lastPoint, point));
			break;
		case STBTT_vcurve:
			contour->addEdge(msdfgen::EdgeHolder(lastPoint, toPoint(vert.cx, vert.cy), point));
			break;
		case STBTT_vcubic:
			contour->addEdge(msdfgen::EdgeHolder(lastPoint, toPoint(vert.cx, vert.cy), toPoint(vert.cx1, vert.cy1), point));
			break;
		}
		lastPoint = point;
	}

	// drop empty contours
	for (int i = static_cast<int>(shape->contours.size()) - 1; i >= 0; --i)
	{
		if (shape->contours[i].edges.empty())
			shape->contours.erase(shape->contours.begin() + i);
	}

	if (shape->contours.empty())
	{
		delete shape;
		return nullptr;
	}

	shape->normalize();

	// a point outside of the shape must have negative distance, otherwise the contours are reversed
	const msdfgen::Shape::Bounds bounds = shape->getBounds();
	const msdfgen::Point2 outerPoint(bounds.l - (bounds.r - bounds.l) - 1.0, bounds.b - (bounds.t - bounds.b) - 1.0);
	if (msdfgen::SimpleTrueShapeDistanceFinder::oneShotDistance(*shape, outerPoint) > 0.0)
	{
		for (msdfgen::Contour& shapeContour : shape->contours)
			shapeContour.reverse();
	}

	return shape;
}

static void SetupGlyphBox(FontGlyph& glyph, const FontAtlasParams& params)
{
	const msdfgen::Shape::Bounds bounds = glyph.shape->getBounds();
	const double border = 0.5 * params.pxRange + params.padding;

	const int left = static_cast<int>(floor(bounds.l * params.emSize - border));
	const int bottom = static_cast<int>(floor(bounds.b * params.emSize - border));
	const int right = static_cast<int>(ceil(bounds.r * params.emSize + border));
	const int top = static_cast<int>(ceil(bounds.t * params.emSize + border));

	glyph.boxLeft = left;
	glyph.boxBottom = bottom;
	glyph.boxWidth = right - left;
	glyph.boxHeight = top - bottom;
}

static void GenerateGlyph(const FontGlyph& glyph, const FontAtlasParams& params, ubyte* atlasPixels, int atlasWidth)
{
	const msdfgen::Projection projection(
		msdfgen::Vector2(params.emSize),
		msdfgen::Vector2(-glyph.boxLeft / params.emSize, -glyph.boxBottom / params.emSize));
	const msdfgen::SDFTransformation transformation(projection, msdfgen::DistanceMapping(msdfgen::Range(params.pxRange / params.emSize)));

	const int width = glyph.boxWidth;
	const int height = glyph.boxHeight;

	// bitmap rows go up, atlas rows go down
	auto atlasPixel = [&](int x, int y) {
		return atlasPixels + ((glyph.atlasY + height - 1 - y) * atlasWidth + glyph.atlasX + x) * 4;
	};

	switch (params.type)
	{
	case FONTATLAS_MTSDF:
	{
		msdfgen::Bitmap<float, 4> bitmap(width, height);
		msdfgen::generateMTSDF(bitmap, *glyph.shape, transformation);

		for (int y = 0; y < height; ++y)
		{
			for (int x = 0; x < width; ++x)
			{
				const float* src = bitmap(x, y);
				ubyte* dst = atlasPixel(x, y);
				for (int c = 0; c < 4; ++c)
					dst[c] = msdfgen::pixelFloatToByte(src[c]);
			}
		}
		break;
	}
	case FONTATLAS_MSDF:
	{
		msdfgen::Bitmap<float, 3> bitmap(width, height);
		msdfgen::generateMSDF(bitmap, *glyph.shape, transformation);

		for (int y = 0; y < height; ++y)
		{
			for (int x = 0; x < width; ++x)
			{
				const float* src = bitmap(x, y);
				ubyte* dst = atlasPixel(x, y);
				for (int c = 0; c < 3; ++c)
					dst[c] = msdfgen::pixelFloatToByte(src[c]);
				dst[3] = 255;
			}
		}
		break;
	}
	case FONTATLAS_SDF:
	case FONTATLAS_PSDF:
	{
		msdfgen::Bitmap<float, 1> bitmap(width, height);
		if (params.type == FONTATLAS_SDF)
			msdfgen::generateSDF(bitmap, *glyph.shape, transformation);
		else
			msdfgen::generatePSDF(bitmap, *glyph.shape, transformation);

		// the font shader takes the median of RGB
		for (int y = 0; y < height; ++y)
		{
			for (int x = 0; x < width; ++x)
			{
				const ubyte value = msdfgen::pixelFloatToByte(*bitmap(x, y));
				ubyte* dst = atlasPixel(x, y);
				dst[0] = value;
				dst[1] = value;
				dst[2] = value;
				dst[3] = value;
			}
		}
		break;
	}
	default:
		ASSERT_FAIL("unknown font atlas type %d", params.type);
	}
}

//-------------------------------------------------------------------------------------------------
// Font faces

static bool LoadFontFace(FontFace& face, const Array<int>& codePoints, const FontAtlasParams& params)
{
	VSSize fileSize = 0;
	face.fileData = g_fileSystem->GetFileBuffer(face.sourceFileName, &fileSize, -1);
	if (!face.fileData)
	{
		MsgError("Can't open font '%s'\n", face.sourceFileName.ToCString());
		return false;
	}

	const int fontOffset = stbtt_GetFontOffsetForIndex(face.fileData, 0);
	if (fontOffset < 0 || !stbtt_InitFont(&face.info, face.fileData, fontOffset))
	{
		MsgError("'%s' is not a valid TrueType / OpenType font\n", face.sourceFileName.ToCString());
		return false;
	}

	face.emScale = stbtt_ScaleForMappingEmToPixels(&face.info, 1.0f);

	int ascent = 0, descent = 0, lineGap = 0;
	stbtt_GetFontVMetrics(&face.info, &ascent, &descent, &lineGap);
	face.ascender = ascent * face.emScale;
	face.descender = -descent * face.emScale;
	face.lineHeight = (ascent - descent + lineGap) * face.emScale;

	int missingGlyphs = 0;
	for (const int codePoint : codePoints)
	{
		const int glyphIndex = stbtt_FindGlyphIndex(&face.info, codePoint);
		if (glyphIndex == 0)
		{
			++missingGlyphs;
			continue;
		}

		int advanceWidth = 0, leftSideBearing = 0;
		stbtt_GetGlyphHMetrics(&face.info, glyphIndex, &advanceWidth, &leftSideBearing);

		FontGlyph& glyph = face.glyphs.append();
		glyph.unicode = codePoint;
		glyph.glyphIndex = glyphIndex;
		glyph.advance = advanceWidth * face.emScale;
		glyph.shape = LoadGlyphShape(face, glyphIndex);

		if (!glyph.shape)
			continue;

		if (params.type == FONTATLAS_MTSDF || params.type == FONTATLAS_MSDF)
			msdfgen::edgeColoringSimple(*glyph.shape, s_edgeColoringAngle, 0);

		SetupGlyphBox(glyph, params);
	}

	if (missingGlyphs)
		MsgWarning("Font '%s': %d characters of the charset are missing\n", face.name.ToCString(), missingGlyphs);

	// kerning pairs from GPOS or kern table
	for (const FontGlyph& first : face.glyphs)
	{
		for (const FontGlyph& second : face.glyphs)
		{
			const int kernAdvance = stbtt_GetGlyphKernAdvance(&face.info, first.glyphIndex, second.glyphIndex);
			if (kernAdvance == 0)
				continue;

			FontKerningPair& pair = face.kerning.append();
			pair.first = first.unicode;
			pair.second = second.unicode;
			pair.advance = kernAdvance * face.emScale;
		}
	}

	return face.glyphs.numElem() > 0;
}

static void FreeFontFace(FontFace& face)
{
	for (FontGlyph& glyph : face.glyphs)
	{
		delete glyph.shape;
		glyph.shape = nullptr;
	}

	PPFree(face.fileData);
	face.fileData = nullptr;
}

static bool WriteFontFile(const FontFace& face, const FontAtlasParams& params, const char* textureName)
{
	const EqString fontsPath = fnmPathCombine(g_fileSystem->GetCurrentGameDirectory(), s_fontsOutputPath);
	const EqString fontFileName = fnmPathApplyExt(fnmPathCombine(fontsPath, face.name), s_fontFileExt);

	const float emSize = static_cast<float>(params.emSize);

	// all metrics are in ems, atlas bounds are in atlas pixels
	KVSection kvs;
	KVSection& fontSec = kvs.CreateSection("eqfont2");
	fontSec.SetKey("type", s_fontAtlasTypeStr[params.type].ToCString());
	fontSec.SetKey("texture", textureName);
	fontSec.SetKey("size", emSize);
	fontSec.SetKey("distanceRange", static_cast<float>(params.pxRange));
	fontSec.SetKey("lineheight", static_cast<float>(face.lineHeight));
	fontSec.SetKey("ascender", static_cast<float>(face.ascender));
	fontSec.SetKey("descender", static_cast<float>(face.descender));

	// glyph <unicode> <advance> [<plane left top right bottom> <atlas left top right bottom>]
	for (const FontGlyph& glyph : face.glyphs)
	{
		KVSection& glyphKey = fontSec.CreateSection("glyph");
		glyphKey.AddValue(glyph.unicode);
		glyphKey.AddValue(static_cast<float>(glyph.advance));

		if (!glyph.shape)
			continue;

		// bounds go through the centers of the border texels, Y down from the baseline
		glyphKey.AddValue((glyph.boxLeft + 0.5f) / emSize);
		glyphKey.AddValue(-(glyph.boxBottom + glyph.boxHeight - 0.5f) / emSize);
		glyphKey.AddValue((glyph.boxLeft + glyph.boxWidth - 0.5f) / emSize);
		glyphKey.AddValue(-(glyph.boxBottom + 0.5f) / emSize);

		glyphKey.AddValue(glyph.atlasX + 0.5f);
		glyphKey.AddValue(glyph.atlasY + 0.5f);
		glyphKey.AddValue(glyph.atlasX + glyph.boxWidth - 0.5f);
		glyphKey.AddValue(glyph.atlasY + glyph.boxHeight - 0.5f);
	}

	// kern <first unicode> <second unicode> <advance>
	for (const FontKerningPair& pair : face.kerning)
	{
		KVSection& kernKey = fontSec.CreateSection("kern");
		kernKey.AddValue(pair.first);
		kernKey.AddValue(pair.second);
		kernKey.AddValue(static_cast<float>(pair.advance));
	}

	g_fileSystem->MakeDir(fnmPathStripName(fontFileName), SP_ROOT);

	IFileStreamPtr file = g_fileSystem->Open(fontFileName, FS_OPEN_WRITE, SP_ROOT);
	if (!file)
	{
		MsgError("Cannot save font file '%s'\n", fontFileName.ToCString());
		return false;
	}

	KeyValues::WriteText(file, kvs);

	Msg("  %s: %d glyphs, %d kerning pairs\n", fontFileName.ToCString(), face.glyphs.numElem(), face.kerning.numElem());
	return true;
}

//-------------------------------------------------------------------------------------------------
// Atlas

static bool PackFontAtlas(Array<FontFace>& faces, int& outWidth, int& outHeight)
{
	Array<stbrp_rect> rects(PP_SL);
	Array<FontGlyph*> rectGlyphs(PP_SL);

	int64 totalArea = 0;
	for (FontFace& face : faces)
	{
		for (FontGlyph& glyph : face.glyphs)
		{
			if (!glyph.shape)
				continue;

			stbrp_rect& rect = rects.append();
			rect.id = rectGlyphs.numElem();
			rect.w = glyph.boxWidth;
			rect.h = glyph.boxHeight;
			rect.x = 0;
			rect.y = 0;
			rect.was_packed = 0;

			rectGlyphs.append(&glyph);
			totalArea += glyph.boxWidth * glyph.boxHeight;
		}
	}

	if (!rects.numElem())
	{
		MsgError("No glyphs to pack\n");
		return false;
	}

	// smallest power of two rectangle, width is never less than height
	int width = 64;
	int height = 64;
	auto growSize = [&]() {
		if (width <= height)
			width *= 2;
		else
			height *= 2;
	};

	while (static_cast<int64>(width) * height < totalArea)
		growSize();

	Array<stbrp_node> nodes(PP_SL);
	for (;;)
	{
		if (width > s_maxFontAtlasSize)
		{
			MsgError("Glyphs do not fit into %dx%d atlas\n", s_maxFontAtlasSize, s_maxFontAtlasSize);
			return false;
		}

		nodes.setNum(width);

		stbrp_context context;
		stbrp_init_target(&context, width, height, nodes.ptr(), nodes.numElem());
		stbrp_setup_heuristic(&context, STBRP_HEURISTIC_Skyline_BL_sortHeight);
		if (stbrp_pack_rects(&context, rects.ptr(), rects.numElem()))
			break;

		growSize();
	}

	for (const stbrp_rect& rect : rects)
	{
		FontGlyph* glyph = rectGlyphs[rect.id];
		glyph->atlasX = rect.x;
		glyph->atlasY = rect.y;
	}

	outWidth = width;
	outHeight = height;
	return true;
}

static bool ParseFontAtlasParams(const KVSection& kvs, const char* fontSrcFileName, FontAtlasParams& params)
{
	const EqStringRef typeStr = KV_GetValueString(kvs.FindSection("type"), 0, s_fontAtlasTypeStr[FONTATLAS_MTSDF].ToCString());

	int type = 0;
	for (; type < FONTATLAS_TYPE_COUNT; ++type)
	{
		if (!s_fontAtlasTypeStr[type].CompareCaseIns(typeStr))
			break;
	}

	if (type == FONTATLAS_TYPE_COUNT)
	{
		MsgError("Font '%s': unknown atlas type '%s'\n", fontSrcFileName, typeStr.ToCString());
		return false;
	}

	params.type = static_cast<EFontAtlasType>(type);
	params.emSize = KV_GetValueFloat(kvs.FindSection("size"), 0, 48.0f);
	params.pxRange = KV_GetValueFloat(kvs.FindSection("pxrange"), 0, 8.0f);
	params.padding = KV_GetValueInt(kvs.FindSection("padding"), 0, 2);

	if (params.emSize < 1.0 || params.pxRange <= 0.0 || params.padding < 0)
	{
		MsgError("Font '%s': invalid size, pxrange or padding\n", fontSrcFileName);
		return false;
	}

	return true;
}

bool ProcessFontFile(const char* fontSrcFileName, const char* materialsPath, Array<EqString>& outTextureNames)
{
	KVSection kvs;
	if (!KV_LoadFromFile(fontSrcFileName, -1, kvs))
	{
		MsgError("Can't open '%s'\n", fontSrcFileName);
		return false;
	}

	MsgInfo("Font: '%s'\n", fontSrcFileName);

	const char* textureName = KV_GetValueString(kvs.FindSection("texture"), 0, nullptr);
	if (!textureName)
	{
		MsgError("Font '%s' missing 'texture'\n", fontSrcFileName);
		return false;
	}

	FontAtlasParams params;
	if (!ParseFontAtlasParams(kvs, fontSrcFileName, params))
		return false;

	const EqString fontDir = fnmPathStripName(fontSrcFileName);

	// charset
	Array<int> codePoints(PP_SL);
	{
		const KVSection* charsetSec = kvs.FindSection("charset");
		if (!charsetSec)
		{
			MsgError("Font '%s' missing 'charset'\n", fontSrcFileName);
			return false;
		}

		if (!LoadCharset(*charsetSec, fontSrcFileName, codePoints))
			return false;
	}

	// font faces
	Array<FontFace> faces(PP_SL);
	defer{
		for (FontFace& face : faces)
			FreeFontFace(face);
	};

	for (const KVSection& fontSec : kvs.Keys("font"))
	{
		EqStringRef fontName;
		EqStringRef fontSourceName;
		if (fontSec.GetValues(fontName, fontSourceName) < 2)
		{
			MsgError("Font '%s' line %d: expected font <name> <source file>\n", fontSrcFileName, fontSec.line);
			return false;
		}

		FontFace& face = faces.append();
		face.name = fontName;
		face.sourceFileName = fnmPathCombine(fontDir, fontSourceName);

		if (!LoadFontFace(face, codePoints, params))
			return false;
	}

	if (!faces.numElem())
	{
		MsgError("Font '%s' has no 'font' entries\n", fontSrcFileName);
		return false;
	}

	int atlasWidth = 0;
	int atlasHeight = 0;
	if (!PackFontAtlas(faces, atlasWidth, atlasHeight))
		return false;

	Msg("  generating %s atlas %dx%d...\n", s_fontAtlasTypeStr[params.type].ToCString(), atlasWidth, atlasHeight);

	CImage atlasImage;
	ubyte* atlasPixels = atlasImage.Create(FORMAT_RGBA8, atlasWidth, atlasHeight, 1, 1);
	memset(atlasPixels, 0, atlasImage.GetMipMappedSize(0, 1));

	for (const FontFace& face : faces)
	{
		for (const FontGlyph& glyph : face.glyphs)
		{
			if (glyph.shape)
				GenerateGlyph(glyph, params, atlasPixels, atlasWidth);
		}
	}

	const EqString imageFileName = fnmPathApplyExt(fnmPathCombine(materialsPath, textureName), s_sourceTextureFileExt);
	g_fileSystem->MakeDir(fnmPathStripName(imageFileName), SP_ROOT);

	if (!atlasImage.SaveImage(imageFileName, SP_ROOT))
	{
		MsgError("Cannot save font atlas texture '%s'\n", imageFileName.ToCString());
		return false;
	}

	for (const FontFace& face : faces)
		WriteFontFile(face, params, textureName);

	outTextureNames.append(textureName);
	return true;
}

//-------------------------------------------------------------------------------------------------
// Fonts material

void UpdateFontsMaterial(const char* materialsPath, const Array<EqString>& textureNames)
{
	const EqString materialFileName = fnmPathCombine(materialsPath, s_fontsMaterialFileName);

	KVSection kvs;
	if (g_fileSystem->FileExist(materialFileName, SP_ROOT))
		KV_LoadFromFile(materialFileName, SP_ROOT, kvs);

	if (!kvs.KeyCount())
		kvs.CreateSection("BaseUnlit");

	KVSection& shaderSec = kvs.KeyAt(0);
	bool changed = false;

	for (const EqString& textureName : textureNames)
	{
		bool found = false;
		for (KVSection& key : shaderSec.Keys())
		{
			EqStringRef texturePath;
			EqStringRef usage;
			if (key.GetValues(texturePath, usage) < 1 || texturePath.CompareCaseIns(textureName))
				continue;

			found = true;
			if (usage.CompareCaseIns(s_fontsTextureUsage))
			{
				key.SetValue(s_fontsTextureUsage, 1);
				changed = true;
			}
			break;
		}

		if (found)
			continue;

		const EqString keyName = fnmPathExtractName(textureName);
		KVSection& key = shaderSec.CreateSection(keyName);
		key.AddValue(textureName);
		key.AddValue(s_fontsTextureUsage);
		changed = true;
	}

	if (!changed)
		return;

	IFileStreamPtr file = g_fileSystem->Open(materialFileName, FS_OPEN_WRITE, SP_ROOT);
	if (!file)
	{
		MsgError("Cannot save '%s' (read-only?)\n", materialFileName.ToCString());
		return;
	}

	KeyValues::WriteText(file, kvs);
	MsgInfo("Updated '%s'\n", materialFileName.ToCString());
}
