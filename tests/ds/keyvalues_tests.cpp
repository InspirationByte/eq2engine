#include <gtest/gtest.h>
#include <gtest/gtest-spi.h>

#include "core/core_common.h"
#include "utils/KeyValues.h"

struct EmbeddedDef
{
	DEFINE_KEYVALUES_DESC_TYPE()

	int embeddedDescValue{ 32777 };
};

BEGIN_KEYVALUES_DESC(EmbeddedDef)
	KV_DESC_FIELD(embeddedDescValue)
END_KEYVALUES_DESC()

struct GeomInstanceDef
{
	DEFINE_KEYVALUES_DESC_TYPE()
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
END_KEYVALUES_DESC()

//-----------------------------------------------

TEST(KEYVALUES_TESTS, DescParser)
{
	KVSection section;
	section.SetKey("model", "weapons/w_har77.egf")
		.SetKey("castShadow", false)
		.SetKey("staticShadowmap", true);

	KVSection& embeddedSec = *section.CreateSection("embedded");
	embeddedSec.SetKey("embeddedDescValue", 555888);

	KVSection& bodyGroupsSec = *section.CreateSection("bodyGroups");
	bodyGroupsSec.AddValue("body1");
	bodyGroupsSec.AddValue("hat");
	bodyGroupsSec.AddValue("case");

	KVSection& arrayOfEmbeddedSec = *section.CreateSection("arrayOfEmbedded");
	{
		KVSection& embeddedSec = *arrayOfEmbeddedSec.CreateSection("embed1");
		embeddedSec.SetKey("embeddedDescValue", 111);
	}
	{
		KVSection& embeddedSec = *arrayOfEmbeddedSec.CreateSection("embed2");
		embeddedSec.SetKey("embeddedDescValue", 333);
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
	EXPECT_EQ(testDesc.arrayOfEmbedded[0].embeddedDescValue, 111);
	EXPECT_EQ(testDesc.arrayOfEmbedded[1].embeddedDescValue, 333);
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
		embeddedSec.SetKey("embeddedDescValue", 555888);

		KVSection& bodyGroupsSec = *section.CreateSection("bodyGroups");
		bodyGroupsSec.AddValue("body1");
		bodyGroupsSec.AddValue("hat");
		bodyGroupsSec.AddValue("case");

		KVSection& arrayOfEmbeddedSec = *section.CreateSection("arrayOfEmbedded");
		{
			KVSection& embeddedSec = *arrayOfEmbeddedSec.CreateSection("embed1");
			embeddedSec.SetKey("embeddedDescValue", 111);
		}
		{
			KVSection& embeddedSec = *arrayOfEmbeddedSec.CreateSection("embed2");
			embeddedSec.SetKey("embeddedDescValue", 333);
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
		EXPECT_EQ(testDesc.arrayOfEmbedded[0].embeddedDescValue, 111);
		EXPECT_EQ(testDesc.arrayOfEmbedded[1].embeddedDescValue, 333);
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
		EXPECT_EQ(testDesc.arrayOfEmbedded[0].embeddedDescValue, 111);
		EXPECT_EQ(testDesc.arrayOfEmbedded[1].embeddedDescValue, 333);
	}


}