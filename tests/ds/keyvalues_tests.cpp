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

BEGIN_KEYVALUES_FLAGS_DESC(ETestFlagsDesc)
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
	KV_DESC_FLAGS(embeddedFlags, ETestFlagsDesc)
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

	KVSection& embeddedSec = *section.CreateSection("embedded");
	{
		KVSection& flagsSec = *embeddedSec.CreateSection("embeddedFlags");
		flagsSec.AddValue("flag1");
		flagsSec.AddValue("flag3");
	}

	KVSection& bodyGroupsSec = *section.CreateSection("bodyGroups");
	bodyGroupsSec.AddValue("body1");
	bodyGroupsSec.AddValue("hat");
	bodyGroupsSec.AddValue("case");

	KVSection& arrayOfEmbeddedSec = *section.CreateSection("arrayOfEmbedded");
	{
		KVSection& embeddedSec = *arrayOfEmbeddedSec.CreateSection("embed1");
		{
			KVSection& flagsSec = *embeddedSec.CreateSection("embeddedFlags");
			flagsSec.AddValue("flag1");
			flagsSec.AddValue("flag4");
		}
	}
	{
		KVSection& embeddedSec = *arrayOfEmbeddedSec.CreateSection("embed2");
		{
			KVSection& flagsSec = *embeddedSec.CreateSection("embeddedFlags");
			flagsSec.AddValue("flag2");
		}
	}

	GeomInstanceDef testDesc;
	KV_ParseDesc(testDesc, section);

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
	CMemoryStream memStreamText(nullptr, VS_OPEN_WRITE, 1024, PP_SL);
	CMemoryStream memStreamBin(nullptr, VS_OPEN_WRITE, 1024, PP_SL);
	{
		KVSection section;
		section.SetKey("model", "weapons/w_har77.egf")
			.SetKey("castShadow", false)
			.SetKey("staticShadowmap", true);

		KVSection& embeddedSec = *section.CreateSection("embedded");
		{
			KVSection& flagsSec = *embeddedSec.CreateSection("embeddedFlags");
			flagsSec.AddValue("flag1");
			flagsSec.AddValue("flag3");
		}

		KVSection& bodyGroupsSec = *section.CreateSection("bodyGroups");
		bodyGroupsSec.AddValue("body1");
		bodyGroupsSec.AddValue("hat");
		bodyGroupsSec.AddValue("case");

		KVSection& arrayOfEmbeddedSec = *section.CreateSection("arrayOfEmbedded");
		{
			KVSection& embeddedSec = *arrayOfEmbeddedSec.CreateSection("embed1");
			{
				KVSection& flagsSec = *embeddedSec.CreateSection("embeddedFlags");
				flagsSec.AddValue("flag1");
				flagsSec.AddValue("flag4");
			}
		}
		{
			KVSection& embeddedSec = *arrayOfEmbeddedSec.CreateSection("embed2");
			{
				KVSection& flagsSec = *embeddedSec.CreateSection("embeddedFlags");
				flagsSec.AddValue("flag2");
			}
		}

		KV_WriteToStream(&memStreamText, &section);
		KV_WriteToStreamBinary(&memStreamBin, &section);
	}

	// TEST: Deserialize text stream
	{
		KVSection deserSection;
		KV_LoadFromStream(&memStreamText, &deserSection);

		GeomInstanceDef testDesc;
		KV_ParseDesc(testDesc, deserSection);

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
		KV_LoadFromStream(&memStreamBin, &deserSection);

		GeomInstanceDef testDesc;
		KV_ParseDesc(testDesc, deserSection);

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