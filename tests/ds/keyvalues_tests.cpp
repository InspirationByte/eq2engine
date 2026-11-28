#include <gtest/gtest.h>
#include <gtest/gtest-spi.h>

#include "core/core_common.h"
#include "utils/KeyValues.h"

enum ETestFlags : int
{
	TEST_FLAGS_NONE,

	TEST_FLAG1 = (1 << 0),
	TEST_FLAG2 = (1 << 1),
	TEST_FLAG3 = (1 << 2),
	TEST_FLAG4 = (1 << 3),
};

BEGIN_KEYVALUES_FLAGS_DESC(ETestFlags)
	KV_FLAG_DESC(ETestFlags::TEST_FLAG1, "flag1")
	KV_FLAG_DESC(ETestFlags::TEST_FLAG2, "flag2")
	KV_FLAG_DESC(ETestFlags::TEST_FLAG3, "flag3")
	KV_FLAG_DESC(ETestFlags::TEST_FLAG4, "flag4")
END_KEYVALUES_FLAGS_DESC

struct EmbeddedDef
{
	DEFINE_KEYVALUES_DESC();
	int embeddedFlags{ TEST_FLAGS_NONE };
};

BEGIN_KEYVALUES_DESC(EmbeddedDef)
	KV_DESC_FLAGS(embeddedFlags, ETestFlags)
END_KEYVALUES_DESC

struct GeomInstanceDef
{
	DEFINE_KEYVALUES_DESC();
	Array<EqString>		bodyGroups{ PP_SL };
	Array<EmbeddedDef>	arrayOfEmbedded{ PP_SL };
	EqString		model;
	EmbeddedDef		embedded;
	bool			staticShadowmap{ false };
	bool			castShadow{ true };
};

BEGIN_KEYVALUES_DESC(GeomInstanceDef)
	KV_DESC_FIELD(model)
	KV_DESC_FIELD(staticShadowmap)
	KV_DESC_FIELD(castShadow)
	KV_DESC_EMBEDDED(embedded)
	KV_DESC_ARRAY_FIELD(bodyGroups)
	KV_DESC_ARRAY_EMBEDDED(arrayOfEmbedded)
END_KEYVALUES_DESC

//-----------------------------------------------

TEST(KEYVALUES_TESTS, DescParser)
{
	KVSection section;
	section.SetKey("model", "weapons/w_har77.egf")
		.SetKey("castShadow", false)
		.SetKey("staticShadowmap", true);

	KVSection& embeddedSec = section.CreateSection("embedded");
	{
		KVSection& flagsSec = embeddedSec.CreateSection("embeddedFlags");
		flagsSec.AddValue("flag1");
		flagsSec.AddValue("flag3");
	}

	KVSection& bodyGroupsSec = section.CreateSection("bodyGroups");
	bodyGroupsSec.AddValue("body1");
	bodyGroupsSec.AddValue("hat");
	bodyGroupsSec.AddValue("case");

	KVSection& arrayOfEmbeddedSec = section.CreateSection("arrayOfEmbedded");
	{
		KVSection& embeddedSec = arrayOfEmbeddedSec.CreateSection("embed1");
		{
			KVSection& flagsSec = embeddedSec.CreateSection("embeddedFlags");
			flagsSec.AddValue("flag1");
			flagsSec.AddValue("flag4");
		}
	}
	{
		KVSection& embeddedSec = arrayOfEmbeddedSec.CreateSection("embed2");
		{
			KVSection& flagsSec = embeddedSec.CreateSection("embeddedFlags");
			flagsSec.AddValue("flag2");
		}
	}

	GeomInstanceDef testDesc;
	KeyValues::ParseDesc(testDesc, section);

	EXPECT_EQ(testDesc.castShadow, false);
	EXPECT_EQ(testDesc.staticShadowmap, true);
	EXPECT_EQ(testDesc.model, "weapons/w_har77.egf");
	EXPECT_EQ(testDesc.bodyGroups.numElem(), 3);
	EXPECT_EQ(testDesc.bodyGroups[1], "hat");
	EXPECT_EQ(testDesc.bodyGroups[2], "case");
	EXPECT_EQ(testDesc.arrayOfEmbedded.numElem(), 2);
	EXPECT_EQ(testDesc.arrayOfEmbedded[0].embeddedFlags, TEST_FLAG1 | TEST_FLAG4);
	EXPECT_EQ(testDesc.arrayOfEmbedded[1].embeddedFlags, TEST_FLAG2);
}

TEST(KEYVALUES_TESTS, SerializeDeserialize)
{
	CMemoryStream memStreamText(PP_SL);
	CMemoryStream memStreamBin(PP_SL);
	memStreamText.Open(FS_OPEN_WRITE);
	memStreamBin.Open(FS_OPEN_WRITE);
	{
		KVSection section;
		section.SetKey("model", "weapons/w_har77.egf")
			.SetKey("castShadow", false)
			.SetKey("staticShadowmap", true);

		KVSection& embeddedSec = section.CreateSection("embedded");
		{
			KVSection& flagsSec = embeddedSec.CreateSection("embeddedFlags");
			flagsSec.AddValue("flag1");
			flagsSec.AddValue("flag3");
		}

		KVSection& bodyGroupsSec = section.CreateSection("bodyGroups");
		bodyGroupsSec.AddValue("body1");
		bodyGroupsSec.AddValue("hat");
		bodyGroupsSec.AddValue("case");

		KVSection& arrayOfEmbeddedSec = section.CreateSection("arrayOfEmbedded");
		{
			KVSection& embeddedSec = arrayOfEmbeddedSec.CreateSection("embed1");
			{
				KVSection& flagsSec = embeddedSec.CreateSection("embeddedFlags");
				flagsSec.AddValue("flag1");
				flagsSec.AddValue("flag4");
			}
		}
		{
			KVSection& embeddedSec = arrayOfEmbeddedSec.CreateSection("embed2");
			{
				KVSection& flagsSec = embeddedSec.CreateSection("embeddedFlags");
				flagsSec.AddValue("flag2");
			}
		}

		KeyValues::WriteText(&memStreamText, section);
		KeyValues::WriteBinary(&memStreamBin, section);
	}

	// TEST: Deserialize text stream
	{
		KVSection deserSection;
		KeyValues::Parse(&memStreamText, deserSection);

		GeomInstanceDef testDesc;
		KeyValues::ParseDesc(testDesc, deserSection);

		EXPECT_EQ(testDesc.castShadow, false);
		EXPECT_EQ(testDesc.staticShadowmap, true);
		EXPECT_EQ(testDesc.model, "weapons/w_har77.egf");
		EXPECT_EQ(testDesc.bodyGroups.numElem(), 3);
		EXPECT_EQ(testDesc.bodyGroups[1], "hat");
		EXPECT_EQ(testDesc.bodyGroups[2], "case");
		EXPECT_EQ(testDesc.arrayOfEmbedded.numElem(), 2);
		EXPECT_EQ(testDesc.arrayOfEmbedded[0].embeddedFlags, TEST_FLAG1 | TEST_FLAG4);
		EXPECT_EQ(testDesc.arrayOfEmbedded[1].embeddedFlags, TEST_FLAG2);
	}

	// TEST: Deserialize binary stream
	{
		KVSection deserSection;
		KeyValues::Parse(&memStreamBin, deserSection);

		GeomInstanceDef testDesc;
		KeyValues::ParseDesc(testDesc, deserSection);

		EXPECT_EQ(testDesc.castShadow, false);
		EXPECT_EQ(testDesc.staticShadowmap, true);
		EXPECT_EQ(testDesc.model, "weapons/w_har77.egf");
		EXPECT_EQ(testDesc.bodyGroups.numElem(), 3);
		EXPECT_EQ(testDesc.bodyGroups[1], "hat");
		EXPECT_EQ(testDesc.bodyGroups[2], "case");
		EXPECT_EQ(testDesc.arrayOfEmbedded.numElem(), 2);
		EXPECT_EQ(testDesc.arrayOfEmbedded[0].embeddedFlags, TEST_FLAG1 | TEST_FLAG4);
		EXPECT_EQ(testDesc.arrayOfEmbedded[1].embeddedFlags, TEST_FLAG2);
	}


}
//-----------------------------------------------

static void FillBinaryTestSection(KVSection& section)
{
	section.SetKey("name", "value with spaces");
	section.SetKey("count", 12);
	section.SetKey("scale", 0.5f);
	section.SetKey("enabled", true);
	section.SetKey("position", Vector3D(1.0f, 2.0f, 3.0f));

	KVSection& nested = section.CreateSection("item", "first");
	nested.SetKey("value", 1);
	section.CreateSection("item", "second").SetKey("value", 2);
	section.CreateSection("empty");
}

TEST(KEYVALUES_TESTS, BinaryRoundTrip)
{
	KVSection section;
	FillBinaryTestSection(section);

	CMemoryStream stream(PP_SL);
	stream.Open(FS_OPEN_READ | FS_OPEN_WRITE, nullptr, 0);
	KeyValues::WriteBinary(&stream, section);
	stream.Seek(0, FS_SEEK_SET);

	KVSection readSection;
	ASSERT_TRUE(KeyValues::ParseBinary(&stream, readSection));

	EqString name;
	int count = 0;
	float scale = 0.0f;
	bool enabled = false;
	Vector3D position = vec3_zero;
	readSection.Get("name").GetValues(name);
	readSection.Get("count").GetValues(count);
	readSection.Get("scale").GetValues(scale);
	readSection.Get("enabled").GetValues(enabled);
	readSection.Get("position").GetValues(position);

	EXPECT_EQ(name, "value with spaces");
	EXPECT_EQ(count, 12);
	EXPECT_FLOAT_EQ(scale, 0.5f);
	EXPECT_TRUE(enabled);
	EXPECT_FLOAT_EQ(position.x, 1.0f);
	EXPECT_FLOAT_EQ(position.y, 2.0f);
	EXPECT_FLOAT_EQ(position.z, 3.0f);

	// repeated keys keep their order
	int itemCount = 0;
	for (const KVSection& item : readSection.Keys("item"))
	{
		int value = 0;
		EqString itemName;
		item.Get("value").GetValues(value);
		item.GetValues(itemName);

		EXPECT_EQ(value, itemCount + 1);
		EXPECT_EQ(itemName, itemCount == 0 ? "first" : "second");
		++itemCount;
	}
	EXPECT_EQ(itemCount, 2);
	EXPECT_NE(readSection.FindSection("empty"), nullptr);
}

TEST(KEYVALUES_TESTS, BinaryEqualBytes)
{
	// equal sections give equal bytes, the value headers have no garbage
	KVSection sectionA;
	KVSection sectionB;
	FillBinaryTestSection(sectionA);
	FillBinaryTestSection(sectionB);

	CMemoryStream streamA(PP_SL);
	CMemoryStream streamB(PP_SL);
	streamA.Open(FS_OPEN_READ | FS_OPEN_WRITE, nullptr, 0);
	streamB.Open(FS_OPEN_READ | FS_OPEN_WRITE, nullptr, 0);
	KeyValues::WriteBinary(&streamA, sectionA);
	KeyValues::WriteBinary(&streamB, sectionB);

	ASSERT_EQ(streamA.GetSize(), streamB.GetSize());
	EXPECT_EQ(memcmp(streamA.GetBasePointer(), streamB.GetBasePointer(), streamA.GetSize()), 0);
}

TEST(KEYVALUES_TESTS, BinaryTruncated)
{
	KVSection section;
	FillBinaryTestSection(section);

	CMemoryStream stream(PP_SL);
	stream.Open(FS_OPEN_READ | FS_OPEN_WRITE, nullptr, 0);
	KeyValues::WriteBinary(&stream, section);

	// every cut of the data fails without reading past its end
	for (VSSize size = 0; size < stream.GetSize(); ++size)
	{
		CMemoryStream cut(PP_SL);
		cut.Open(FS_OPEN_READ | FS_OPEN_WRITE, nullptr, size);
		cut.Write(stream.GetBasePointer(), 1, size);
		cut.Seek(0, FS_SEEK_SET);

		KVSection readSection;
		EXPECT_FALSE(KeyValues::ParseBinary(&cut, readSection)) << "size " << size;
	}
}

TEST(KEYVALUES_TESTS, BinaryCorrupt)
{
	KVSection section;
	section.SetKey("name", "value");

	CMemoryStream stream(PP_SL);
	stream.Open(FS_OPEN_READ | FS_OPEN_WRITE, nullptr, 0);
	KeyValues::WriteBinary(&stream, section);

	// root header (16 bytes, empty name), the "name" key header (16 bytes) and name (4 bytes),
	// then its value header: type, string length
	const int keyCountOffset = 16 + 8;
	const int stringLengthOffset = 16 + 16 + 4 + 4;
	const int hugeValue = 0x7fffffff;

	for (const int offset : { keyCountOffset, stringLengthOffset })
	{
		CMemoryStream corrupt(PP_SL);
		corrupt.Open(FS_OPEN_READ | FS_OPEN_WRITE, nullptr, stream.GetSize());
		corrupt.Write(stream.GetBasePointer(), 1, stream.GetSize());
		memcpy(corrupt.GetBasePointer() + offset, &hugeValue, sizeof(hugeValue));
		corrupt.Seek(0, FS_SEEK_SET);

		KVSection readSection;
		EXPECT_FALSE(KeyValues::ParseBinary(&corrupt, readSection)) << "offset " << offset;
	}
}
