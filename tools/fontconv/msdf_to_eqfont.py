#!/usr/bin/env python3
#////////////////////////////////////////////////////////////////////////////////
# Copyright (C) Inspiration Byte
# 2009-2026
#////////////////////////////////////////////////////////////////////////////////
# Description: converts msdf-atlas-gen JSON layout and PNG atlas to eqfont2 + DDS
#////////////////////////////////////////////////////////////////////////////////
#
# Usage:
#   msdf_to_eqfont.py <layout.json> <atlas.png> -texture <material texture name>
#                     -fntout <dir> [-ddsout <file.dds>] [-name <font file name>]
#
# - layout.json / atlas.png are produced by msdf-atlas-gen (-json, -imageout ... -format png).
# - A JSON with several fonts (msdf-atlas-gen -and) produces one .fnt per font variant; every
#   variant needs -fontname, it is used as the .fnt file name. All variants share the texture.
# - The atlas is written as uncompressed RGBA8 DDS: block compression breaks (M)SDF channels.

import argparse
import json
import os
import struct
import sys
import zlib

SUPPORTED_ATLAS_TYPES = ("sdf", "psdf", "msdf", "mtsdf")

#--------------------------------------------------------------------------------

def DecodePNG(fileName):
	with open(fileName, "rb") as f:
		data = f.read()

	if data[:8] != b"\x89PNG\r\n\x1a\n":
		raise RuntimeError("%s is not a PNG file" % fileName)

	pos = 8
	width = height = bitDepth = colorType = interlace = 0
	idat = bytearray()
	while pos < len(data):
		length, chunkType = struct.unpack(">I4s", data[pos:pos + 8])
		chunk = data[pos + 8:pos + 8 + length]
		pos += 12 + length
		if chunkType == b"IHDR":
			width, height, bitDepth, colorType, _, _, interlace = struct.unpack(">IIBBBBB", chunk)
		elif chunkType == b"IDAT":
			idat += chunk
		elif chunkType == b"IEND":
			break

	channelsByColorType = { 0: 1, 2: 3, 4: 2, 6: 4 }
	if bitDepth != 8 or interlace != 0 or colorType not in channelsByColorType:
		raise RuntimeError("%s: only 8-bit non-interlaced gray/RGB/RGBA PNG is supported" % fileName)

	channels = channelsByColorType[colorType]
	stride = width * channels
	raw = zlib.decompress(bytes(idat))

	pixels = bytearray(stride * height)
	prevRow = bytearray(stride)
	src = 0
	for y in range(height):
		filterType = raw[src]
		src += 1
		row = bytearray(raw[src:src + stride])
		src += stride

		if filterType == 1:
			for i in range(channels, stride):
				row[i] = (row[i] + row[i - channels]) & 0xFF
		elif filterType == 2:
			for i in range(stride):
				row[i] = (row[i] + prevRow[i]) & 0xFF
		elif filterType == 3:
			for i in range(stride):
				left = row[i - channels] if i >= channels else 0
				row[i] = (row[i] + ((left + prevRow[i]) >> 1)) & 0xFF
		elif filterType == 4:
			for i in range(stride):
				a = row[i - channels] if i >= channels else 0
				b = prevRow[i]
				c = prevRow[i - channels] if i >= channels else 0
				p = a + b - c
				pa = abs(p - a)
				pb = abs(p - b)
				pc = abs(p - c)
				pred = a if (pa <= pb and pa <= pc) else (b if pb <= pc else c)
				row[i] = (row[i] + pred) & 0xFF
		elif filterType != 0:
			raise RuntimeError("%s: invalid PNG filter %d" % (fileName, filterType))

		pixels[y * stride:(y + 1) * stride] = row
		prevRow = row

	# expand to RGBA
	if channels == 4:
		return width, height, bytes(pixels)

	rgba = bytearray(width * height * 4)
	for i in range(width * height):
		if channels == 1:
			v = pixels[i]
			rgba[i * 4:i * 4 + 4] = bytes((v, v, v, 255))
		elif channels == 2:
			v = pixels[i * 2]
			rgba[i * 4:i * 4 + 4] = bytes((v, v, v, pixels[i * 2 + 1]))
		else:
			rgba[i * 4:i * 4 + 3] = pixels[i * 3:i * 3 + 3]
			rgba[i * 4 + 3] = 255
	return width, height, bytes(rgba)

def WriteDDS(fileName, width, height, rgba):
	DDSD_CAPS = 0x1
	DDSD_HEIGHT = 0x2
	DDSD_WIDTH = 0x4
	DDSD_PITCH = 0x8
	DDSD_PIXELFORMAT = 0x1000
	DDPF_ALPHAPIXELS = 0x1
	DDPF_RGB = 0x40
	DDSCAPS_TEXTURE = 0x1000

	header = struct.pack("<4sIIIIIII44x", b"DDS ", 124,
		DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PITCH | DDSD_PIXELFORMAT,
		height, width, width * 4, 0, 0)

	# pixel format: R8G8B8A8 in memory order
	header += struct.pack("<II4sIIIII", 32, DDPF_RGB | DDPF_ALPHAPIXELS, b"\0\0\0\0", 32,
		0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000)
	header += struct.pack("<IIIII", DDSCAPS_TEXTURE, 0, 0, 0, 0)

	os.makedirs(os.path.dirname(os.path.abspath(fileName)), exist_ok=True)
	with open(fileName, "wb") as f:
		f.write(header)
		f.write(rgba)

#--------------------------------------------------------------------------------

def FormatFloat(value):
	return ("%.6f" % value).rstrip("0").rstrip(".") if value != 0 else "0"

def WriteFont(fileName, textureName, atlas, variant):
	yTop = atlas.get("yOrigin", "bottom") == "top"
	atlasHeight = atlas["height"]
	metrics = variant["metrics"]

	# eqfont2 uses Y down, origin at baseline
	ySign = 1.0 if yTop else -1.0

	lines = []
	lines.append("eqfont2")
	lines.append("{")
	lines.append("\ttype \"%s\";" % atlas["type"])
	lines.append("\ttexture \"%s\";" % textureName)
	lines.append("\tsize %s;" % FormatFloat(atlas["size"]))
	lines.append("\tdistanceRange %s;" % FormatFloat(atlas["distanceRange"]))
	lines.append("\tlineheight %s;" % FormatFloat(metrics["lineHeight"]))
	lines.append("\tascender %s;" % FormatFloat(abs(metrics["ascender"])))
	lines.append("\tdescender %s;" % FormatFloat(abs(metrics["descender"])))
	lines.append("\t// all metrics are in ems; atlas bounds are in atlas pixels")
	lines.append("\t// glyph <unicode> <advance> <plane left top right bottom> <atlas left top right bottom>")

	glyphCount = 0
	for glyph in variant["glyphs"]:
		if "unicode" not in glyph:
			continue

		values = [ str(glyph["unicode"]), FormatFloat(glyph["advance"]) ]

		plane = glyph.get("planeBounds")
		atlasBounds = glyph.get("atlasBounds")
		if plane and atlasBounds:
			planeTop = plane["top"] * ySign
			planeBottom = plane["bottom"] * ySign
			atlasTop = atlasBounds["top"] if yTop else atlasHeight - atlasBounds["top"]
			atlasBottom = atlasBounds["bottom"] if yTop else atlasHeight - atlasBounds["bottom"]
			values += [ FormatFloat(v) for v in (plane["left"], planeTop, plane["right"], planeBottom) ]
			values += [ FormatFloat(v) for v in (atlasBounds["left"], atlasTop, atlasBounds["right"], atlasBottom) ]

		lines.append("\tglyph %s;" % " ".join(values))
		glyphCount += 1

	kerning = variant.get("kerning", [])
	for pair in kerning:
		lines.append("\tkern %d %d %s;" % (pair["unicode1"], pair["unicode2"], FormatFloat(pair["advance"])))

	lines.append("}")

	os.makedirs(os.path.dirname(os.path.abspath(fileName)), exist_ok=True)
	with open(fileName, "w", newline="\n", encoding="utf-8") as f:
		f.write("\n".join(lines) + "\n")

	print("%s: %d glyphs, %d kerning pairs" % (fileName, glyphCount, len(kerning)))

#--------------------------------------------------------------------------------

def main():
	parser = argparse.ArgumentParser(description="Convert msdf-atlas-gen output to eqfont2 font files")
	parser.add_argument("json", help="msdf-atlas-gen JSON layout file")
	parser.add_argument("png", nargs="?", help="msdf-atlas-gen PNG atlas image (optional if -ddsout is not used)")
	parser.add_argument("-texture", required=True, help="texture name written to the font file (materials path, no extension)")
	parser.add_argument("-fntout", required=True, help="output folder for .fnt files")
	parser.add_argument("-ddsout", help="output .dds file of the atlas")
	parser.add_argument("-name", help="font file name for a single-font atlas (default: JSON name or file name)")
	args = parser.parse_args()

	with open(args.json, "r", encoding="utf-8") as f:
		layout = json.load(f)

	atlas = layout["atlas"]
	if atlas["type"] not in SUPPORTED_ATLAS_TYPES:
		print("Unsupported atlas type '%s', supported: %s" % (atlas["type"], ", ".join(SUPPORTED_ATLAS_TYPES)))
		return 1

	if "variants" in layout:
		for variant in layout["variants"]:
			if not variant.get("name"):
				print("Every font of a multi-font atlas needs -fontname in msdf-atlas-gen")
				return 1
			WriteFont(os.path.join(args.fntout, variant["name"] + ".fnt"), args.texture, atlas, variant)
	else:
		name = args.name or layout.get("name") or os.path.splitext(os.path.basename(args.json))[0]
		WriteFont(os.path.join(args.fntout, name + ".fnt"), args.texture, atlas, layout)

	if args.ddsout:
		if not args.png:
			print("PNG atlas is required to write DDS")
			return 1
		width, height, rgba = DecodePNG(args.png)
		if width != atlas["width"] or height != atlas["height"]:
			print("Atlas image size %dx%d does not match JSON %dx%d" % (width, height, atlas["width"], atlas["height"]))
			return 1
		WriteDDS(args.ddsout, width, height, rgba)
		print("%s: %dx%d RGBA8" % (args.ddsout, width, height))

	return 0

if __name__ == "__main__":
	sys.exit(main())
