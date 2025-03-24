#include <ctype.h>

#include "core/core_common.h"
#include "Base64.h"

static constexpr EqStringRef s_base64CharMap =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwxyz"
	"0123456789+/";

inline bool IsBase64(unsigned char c)
{
	return isalnum(c) || (c == '+') || (c == '/');
}

EqString Base64Encode(ArrayCRef<ubyte> src)
{
	EqString ret;
	int i = 0;
	int srcIdx = 0;

	unsigned char charArray3[3];
	unsigned char charArray4[4];

	int inLen = src.numElem();
	while (inLen--)
	{
		charArray3[i++] = src[srcIdx++];
		if (i == 3)
		{
			charArray4[0] = (charArray3[0] & 0xfc) >> 2;
			charArray4[1] = ((charArray3[0] & 0x03) << 4) + ((charArray3[1] & 0xf0) >> 4);
			charArray4[2] = ((charArray3[1] & 0x0f) << 2) + ((charArray3[2] & 0xc0) >> 6);
			charArray4[3] = charArray3[2] & 0x3f;

			for (i = 0; i < 4; i++)
			{
				ret.Append(s_base64CharMap[charArray4[i]]);
			}
			i = 0;
		}
	}

	if (i)
	{
		for (int j = i; j < 3; j++)
			charArray3[j] = '\0';

		charArray4[0] = (charArray3[0] & 0xfc) >> 2;
		charArray4[1] = ((charArray3[0] & 0x03) << 4) + ((charArray3[1] & 0xf0) >> 4);
		charArray4[2] = ((charArray3[1] & 0x0f) << 2) + ((charArray3[2] & 0xc0) >> 6);
		charArray4[3] = charArray3[2] & 0x3f;

		for (int j = 0; j < i + 1; j++)
			ret.Append(s_base64CharMap[charArray4[j]]);

		while (i++ < 3)
			ret.Append('=');
	}

	return ret;
}


Array<ubyte> Base64Decode(EqStringRef src)
{
	size_t inLen = src.Length();
	int i = 0;
	int srcIdx = 0;

	unsigned char charArray4[4];
	unsigned char charArray3[3];

	Array<ubyte> ret(PP_SL);
	while (inLen-- && src[srcIdx] != '=' && IsBase64(src[srcIdx]))
	{
		charArray4[i++] = src[srcIdx++];
		if (i == 4)
		{
			for (i = 0; i < 4; i++)
				charArray4[i] = (unsigned char)s_base64CharMap.Find(charArray4[i]);

			charArray3[0] = (charArray4[0] << 2) + ((charArray4[1] & 0x30) >> 4);
			charArray3[1] = ((charArray4[1] & 0xf) << 4) + ((charArray4[2] & 0x3c) >> 2);
			charArray3[2] = ((charArray4[2] & 0x3) << 6) + charArray4[3];

			for (i = 0; i < 3; i++)
				ret.append(charArray3[i]);

			i = 0;
		}
	}

	if (i)
	{
		for (int j = i; j < 4; j++)
			charArray4[j] = 0;

		for (int j = 0; j < 4; j++)
			charArray4[j] = (unsigned char)s_base64CharMap.Find(charArray4[j]);

		charArray3[0] = (charArray4[0] << 2) + ((charArray4[1] & 0x30) >> 4);
		charArray3[1] = ((charArray4[1] & 0xf) << 4) + ((charArray4[2] & 0x3c) >> 2);
		charArray3[2] = ((charArray4[2] & 0x3) << 6) + charArray4[3];

		for (int j = 0; j < i - 1; j++)
			ret.append(charArray3[j]);
	}
	return ret;
}
