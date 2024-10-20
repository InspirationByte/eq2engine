#include <gtest/gtest.h>
#include <gtest/gtest-spi.h>

#include "core/core_common.h"


constexpr EqStringRef s_StringTestStr1 = "Testing String";
constexpr EqStringRef s_StringTestStr2 = "No Magic Very Long Long long hell long string Value 1234567890 0xFEDABEEF";

constexpr EqStringRef s_StringTestStrPart1 = "The String";
constexpr EqStringRef s_StringTestStrPart2 = " And the Extraordinary Nuts";
constexpr EqStringRef s_StringTestStrPart3 = " Nuts of God";

constexpr EqStringRef s_StringTestStrTwoParts = "The String And the Extraordinary Nuts";

constexpr int s_StringTestInt = 0xFEDABEEF;

TEST(EQSTRING_TESTS, Empty)
{
	EqString testString;

	EXPECT_EQ(testString.Length(), 0);
	ASSERT_TRUE(testString.IsValid());
	EXPECT_EQ(testString.GetData(), nullptr);
	EXPECT_NE(testString.ToCString(), nullptr);
	EXPECT_EQ(*testString.ToCString(), 0);
}

TEST(EQSTRING_TESTS, WithContents)
{
	EqString testString(s_StringTestStr1);

	EXPECT_EQ(testString, s_StringTestStr1);
	ASSERT_TRUE(testString.IsValid());
	EXPECT_EQ(testString.Length(), s_StringTestStr1.Length());
	EXPECT_GE(testString.GetSize(), elementsOf(s_StringTestStr1));
}

TEST(EQSTRING_TESTS, WithLongContents)
{
	EqString testString(s_StringTestStr2);

	EXPECT_EQ(testString, s_StringTestStr2);
	ASSERT_TRUE(testString.IsValid());
	EXPECT_EQ(testString.Length(), s_StringTestStr2.Length());
	EXPECT_GE(testString.GetSize(), elementsOf(s_StringTestStr2));
}

TEST(EQSTRING_TESTS, ReassignShortToLong)
{
	EqString testString(s_StringTestStr1);

	testString = s_StringTestStr2;

	EXPECT_EQ(testString, s_StringTestStr2);
	ASSERT_TRUE(testString.IsValid());
	EXPECT_EQ(testString.Length(), s_StringTestStr2.Length());
	EXPECT_GE(testString.GetSize(), elementsOf(s_StringTestStr2));
}

TEST(EQSTRING_TESTS, ReassignLongToShort)
{
	EqString testString(s_StringTestStr2);

	testString = s_StringTestStr1;

	EXPECT_EQ(testString, s_StringTestStr1);
	ASSERT_TRUE(testString.IsValid());
	EXPECT_EQ(testString.Length(), s_StringTestStr1.Length());
	EXPECT_GE(testString.GetSize(), elementsOf(s_StringTestStr1));
}

TEST(EQSTRING_TESTS, AssignEmpty)
{
	EqString testString(s_StringTestStr2);

	testString = "";

	EXPECT_EQ(testString, EqStringRef(""));
	ASSERT_TRUE(testString.IsValid());
	EXPECT_EQ(testString.Length(), 0);
	EXPECT_GE(testString.GetSize(), 1);
}

TEST(EQSTRING_TESTS, EmptyAppend)
{
	EqString testString;

	testString.Append(s_StringTestStr1);

	EXPECT_EQ(testString, s_StringTestStr1);
	ASSERT_TRUE(testString.IsValid());
	EXPECT_EQ(testString.Length(), s_StringTestStr1.Length());
	EXPECT_GE(testString.GetSize(), elementsOf(s_StringTestStr1));
}

TEST(EQSTRING_TESTS, EmptyPrepend)
{
	EqString testString;

	testString.Insert(s_StringTestStr1, 0);

	EXPECT_EQ(testString, s_StringTestStr1);
	ASSERT_TRUE(testString.IsValid());
	EXPECT_EQ(testString.Length(), s_StringTestStr1.Length());
	EXPECT_GE(testString.GetSize(), elementsOf(s_StringTestStr1));
}

TEST(EQSTRING_TESTS, Append)
{
	EqString testString = s_StringTestStrPart1;

	testString.Append(s_StringTestStrPart2);

	EXPECT_EQ(testString, s_StringTestStrTwoParts);
	ASSERT_TRUE(testString.IsValid());
	EXPECT_EQ(testString.Length(), s_StringTestStrTwoParts.Length());
	EXPECT_GE(testString.GetSize(), elementsOf(s_StringTestStrTwoParts));
}

TEST(EQSTRING_TESTS, Prepend)
{
	EqString testString = s_StringTestStrPart2;

	testString.Insert(s_StringTestStrPart1, 0);

	EXPECT_EQ(testString, s_StringTestStrTwoParts);
	ASSERT_TRUE(testString.IsValid());
	EXPECT_EQ(testString.Length(), s_StringTestStrTwoParts.Length());
	EXPECT_GE(testString.GetSize(), elementsOf(s_StringTestStrTwoParts));
}

TEST(EQSTRING_TESTS, InsertLast)
{
	EqString testString = s_StringTestStrPart1;

	testString.Insert(s_StringTestStrPart2, testString.Length());

	EXPECT_EQ(testString, s_StringTestStrTwoParts);
	ASSERT_TRUE(testString.IsValid());
	EXPECT_EQ(testString.Length(), s_StringTestStrTwoParts.Length());
	EXPECT_GE(testString.GetSize(), elementsOf(s_StringTestStrTwoParts));
}

TEST(EQSTRING_TESTS, InsertLastMultipleTimes)
{
	EqString testString = s_StringTestStrPart1;

	for(int i = 0; i < 4; ++i)
		testString.Insert("A", testString.Length());

	EXPECT_EQ(testString, s_StringTestStrPart1 + "AAAA");
	ASSERT_TRUE(testString.IsValid());
}

TEST(EQSTRING_TESTS, AppendChars)
{
	EqString testString = s_StringTestStrPart1;

	testString.Append(' ');
	testString.Append('a');
	testString.Append('n');
	testString.Append('d');
	testString.Append(' ');
	testString.Append('C');

	std::string checkStr = (std::string(s_StringTestStrPart1) + " and C");
	EXPECT_EQ(testString, EqStringRef(checkStr.c_str()));
	ASSERT_TRUE(testString.IsValid());
	EXPECT_EQ(testString.Length(), checkStr.length());
	EXPECT_GE(testString.GetSize(), checkStr.length()+1);
}

TEST(EQSTRING_TESTS, InsertInMiddle)
{
	EqString testString = s_StringTestStrPart1;

	testString.Insert(s_StringTestStrPart2, 3);

	{
		std::string checkStr = ("The And the Extraordinary Nuts String");
		EXPECT_EQ(testString, EqStringRef(checkStr.c_str()));
		ASSERT_TRUE(testString.IsValid());
		EXPECT_EQ(testString.Length(), checkStr.length());
		EXPECT_GE(testString.GetSize(), checkStr.length() + 1);
	}

	testString.Insert(s_StringTestStrPart3, 3);

	{
		std::string checkStr = ("The Nuts of God And the Extraordinary Nuts String");
		EXPECT_EQ(testString, EqStringRef(checkStr.c_str()));
		ASSERT_TRUE(testString.IsValid());
		EXPECT_EQ(testString.Length(), checkStr.length());
		EXPECT_GE(testString.GetSize(), checkStr.length() + 1);
	}
}

TEST(EQSTRING_TESTS, RemoveChars)
{
	{
		EqString testString(s_StringTestStr1);

		testString.Remove(0, strlen("Testing "));

		std::string checkStr = ("String");
		EXPECT_EQ(testString, EqStringRef(checkStr.c_str()));
		ASSERT_TRUE(testString.IsValid());
		EXPECT_EQ(testString.Length(), checkStr.length());
		EXPECT_GE(testString.GetSize(), checkStr.length() + 1);
	}

	{
		EqString testString(s_StringTestStr2);

		testString.Remove(9, strlen("Very Long Long long hell long "));

		std::string checkStr = ("No Magic string Value 1234567890 0xFEDABEEF");

		EXPECT_EQ(testString, EqStringRef(checkStr.c_str()));
		ASSERT_TRUE(testString.IsValid());
		EXPECT_EQ(testString.Length(), checkStr.length());
		EXPECT_GE(testString.GetSize(), checkStr.length() + 1);
	}

	// TEST: remove last single char
	{
		EqString testString(s_StringTestStr1);

		testString.Remove(13, 1);

		std::string checkStr = ("Testing Strin");

		EXPECT_EQ(testString, EqStringRef(checkStr.c_str()));
		ASSERT_TRUE(testString.IsValid());
		EXPECT_EQ(testString.Length(), checkStr.length());
		EXPECT_GE(testString.GetSize(), checkStr.length() + 1);
	}

	// TEST: invalid input
	{
		EqString testString(s_StringTestStr1);
		EXPECT_FATAL_FAILURE([&]() {testString.Remove(-4, 8); }, "start");
		EXPECT_FATAL_FAILURE([&]() {testString.Remove(12, 8); }, "start + count");

		ASSERT_TRUE(testString.IsValid());
	}
}

TEST(EQSTRING_TESTS, Left)
{
	EqString testString(s_StringTestStr1);

	EqString leftPart = testString.Left(4);

	std::string checkStr = ("Test");
	EXPECT_EQ(leftPart, EqStringRef(checkStr.c_str()));
	ASSERT_TRUE(leftPart.IsValid());
	EXPECT_EQ(leftPart.Length(), checkStr.length());
	EXPECT_GE(leftPart.GetSize(), checkStr.length() + 1);
}

TEST(EQSTRING_TESTS, Right)
{
	EqString testString(s_StringTestStr1);

	EqString rightPart = testString.Right(6);

	std::string checkStr = ("String");
	EXPECT_EQ(rightPart, EqStringRef(checkStr.c_str()));
	ASSERT_TRUE(rightPart.IsValid());
	EXPECT_EQ(rightPart.Length(), checkStr.length());
	EXPECT_GE(rightPart.GetSize(), checkStr.length() + 1);
}

TEST(EQSTRING_TESTS, Mid)
{
	EqString testString(s_StringTestStrTwoParts);

	EqString midPart = testString.Mid(4, 6);

	std::string checkStr = ("String");
	EXPECT_EQ(midPart, EqStringRef(checkStr.c_str()));
	ASSERT_TRUE(midPart.IsValid());
	EXPECT_EQ(midPart.Length(), checkStr.length());
	EXPECT_GE(midPart.GetSize(), checkStr.length() + 1);
}

TEST(EQSTRING_TESTS, ReplaceChar)
{
	EqString testString(s_StringTestStrTwoParts);

	testString.ReplaceChar('S', 'x');
	testString.ReplaceChar('i', 'E');
	testString.ReplaceChar('E', 'i');
	testString.ReplaceChar('x', 'S');
	testString.ReplaceChar('e', 'b');
	testString.ReplaceChar('a', 'e');

	EqString checkPart = testString.Mid(4, 6);

	std::string checkStr = ("String");
	EXPECT_EQ(checkPart, EqStringRef(checkStr.c_str()));
	ASSERT_TRUE(checkPart.IsValid());
	EXPECT_EQ(checkPart.Length(), checkStr.length());
	EXPECT_GE(checkPart.GetSize(), checkStr.length() + 1);
}


TEST(EQSTRING_TESTS, ReplaceSubStr)
{
	// TEST: replace with string of same length
	{
		EqString testString(s_StringTestStrTwoParts);

		testString.ReplaceSubstr("String", "Classy");

		std::string checkStr = ("The Classy And the Extraordinary Nuts");
		EXPECT_EQ(testString, EqStringRef(checkStr.c_str()));
		ASSERT_TRUE(testString.IsValid());
		EXPECT_EQ(testString.Length(), checkStr.length());
		EXPECT_GE(testString.GetSize(), checkStr.length() + 1);
	}

	// TEST: replace with longer string
	{
		EqString testString(s_StringTestStrTwoParts);

		testString.ReplaceSubstr("Extraordinary", "BoomeryBazookahAmazing");

		std::string checkStr = ("The String And the BoomeryBazookahAmazing Nuts");
		EXPECT_EQ(testString, EqStringRef(checkStr.c_str()));
		ASSERT_TRUE(testString.IsValid());
		EXPECT_EQ(testString.Length(), checkStr.length());
		EXPECT_GE(testString.GetSize(), checkStr.length() + 1);
	}

	// TEST: replace with shorter string
	{
		EqString testString(s_StringTestStrTwoParts);

		testString.ReplaceSubstr("Extraordinary", "Deez");
		testString.ReplaceSubstr("the ", "", true);

		std::string checkStr = ("The String And Deez Nuts");
		EXPECT_EQ(testString, EqStringRef(checkStr.c_str()));
		ASSERT_TRUE(testString.IsValid());
		EXPECT_EQ(testString.Length(), checkStr.length());
		EXPECT_GE(testString.GetSize(), checkStr.length() + 1);
	}

	// TEST: replace strings in sequence
	{
		EqString testString(s_StringTestStr2);

		testString.ReplaceSubstr("No", "Yes", true, 0);

		int pos = 0;
		while(pos >= 0)
		{
			pos = testString.ReplaceSubstr("Long", "Short", false, pos);
		}

		testString.ReplaceSubstr("Yes", "No");
		pos = testString.ReplaceSubstr("1234567890", "", pos);

		std::string checkStr = ("No Magic Very Short Short Short hell Short string Value  0xFEDABEEF");
		EXPECT_EQ(testString, EqStringRef(checkStr.c_str()));
		ASSERT_TRUE(testString.IsValid());
		EXPECT_EQ(testString.Length(), checkStr.length());
		EXPECT_GE(testString.GetSize(), checkStr.length() + 1);
	}
}

TEST(EQSTRING_TESTS, TestComparisonOperators)
{
	EqString testString1 = s_StringTestStrTwoParts;
	EqString testString2 = s_StringTestStr1;

	EXPECT_EQ(testString1.Compare(s_StringTestStrTwoParts), 0);
	EXPECT_TRUE(testString1 == s_StringTestStrTwoParts);
	EXPECT_FALSE(testString1 != s_StringTestStrTwoParts);

	EXPECT_FALSE(testString1 == testString2);
}

TEST(EQSTRING_TESTS, TestAppendOperators)
{
	// C string right
	{
		EqString testStr = EqString(s_StringTestStr1) + " and Another";
		EXPECT_EQ(testStr, "Testing String and Another");
	}

	// C string left
	{
		EqString testStr = "Test and " + EqString(s_StringTestStr1);
		EXPECT_EQ(testStr, "Test and Testing String");
	}

	// Two String Refs
	{
		EqString testString1 = "Left part";
		EqString testString2 = " and Right part";

		EqString testStr = testString1 + testString2;
		EXPECT_EQ(testStr, "Left part and Right part");
	}
}

TEST(EQSTRINGREF_TESTS, Instantiate)
{
	EqStringRef defaultRef;
	EqStringRef nullRef = nullptr;
	EqStringRef strRef = "String";
	static constexpr EqStringRef cexprRef = "Extraordinary";

	EqString str = "Extraordinary";

	EqStringRef eqStrRef = str;

	const int t = cexprRef.Find("Dinary", false, 0);
	EXPECT_EQ(t, 7);
}

TEST(EQSTRINGREF_TESTS, TestComparisonOperators)
{
	EqString compareToTest = s_StringTestStr1;
	EqStringRef testString = s_StringTestStr1;

	EXPECT_TRUE(compareToTest == testString);
	EXPECT_TRUE(testString == "Testing String");
	EXPECT_EQ(testString.Compare(s_StringTestStr1), 0);
	EXPECT_TRUE(testString == s_StringTestStr1);
	EXPECT_FALSE(testString != s_StringTestStr1);
	EXPECT_EQ(testString, s_StringTestStr1);
}

TEST(EQSTRINGREF_TESTS, AppendOperators)
{
	// C string right
	{
		EqString testStr = s_StringTestStr1 + " and Another";
		EXPECT_EQ(testStr, "Testing String and Another");
	}

	// C string left
	{
		EqString testStr = "Test and " + s_StringTestStr1;
		EXPECT_EQ(testStr, "Test and Testing String");
	}

	// Two String Refs
	{
		constexpr EqStringRef testString1 = "Left part";
		constexpr EqStringRef testString2 = " and Right part";

		EqString testStr = testString1 + testString2;
		EXPECT_EQ(testStr, "Left part and Right part");
	}
}


TEST(EQSTRINGREF_TESTS, Hash)
{
	{
		const int hashA = StringToHash("Testing String");
		const int hashB = StringToHashConst("Testing String");
		EXPECT_EQ(hashA, hashB);
	}

	{
		const int hashA = StringToHash("Hashed String 2");
		const int hashB = StringToHashConst("Hashed String 2");
		EXPECT_EQ(hashA, hashB);
	}

	{
		const int hashA = StringToHash("String 3");
		const int hashB = StringToHashConst("String 3");
		EXPECT_EQ(hashA, hashB);
	}

	{
		const int hashA = StringToHash(" 3");
		const int hashB = StringToHashConst(" 2");
		EXPECT_NE(hashA, hashB);
	}
}

