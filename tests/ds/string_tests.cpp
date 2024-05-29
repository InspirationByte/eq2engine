#include <gtest/gtest.h>
#include <gtest/gtest-spi.h>

#include "core/core_common.h"


constexpr const char s_StringTestStr1[] = "Testing String";
constexpr const char s_StringTestStr2[] = "No Magic Very Long Long long hell long string Value 1234567890 0xFEDABEEF";

constexpr const char s_StringTestStrPart1[] = "The String";
constexpr const char s_StringTestStrPart2[] = " And the Extraordinary Nuts";
constexpr const char s_StringTestStrPart3[] = " Nuts of God";

constexpr const char s_StringTestStrTwoParts[] = "The String And the Extraordinary Nuts";

constexpr int s_StringTestInt = 0xFEDABEEF;

TEST(EQSTRING_TESTS, Empty)
{
	EqString testString;

	EXPECT_EQ(testString.Length(), 0);
	ASSERT_TRUE(testString.IsValid());
	EXPECT_NE(testString.GetData(), nullptr);
	EXPECT_EQ(*testString.GetData(), 0);
}

TEST(EQSTRING_TESTS, WithContents)
{
	EqString testString(s_StringTestStr1);

	EXPECT_EQ(testString, s_StringTestStr1);
	ASSERT_TRUE(testString.IsValid());
	EXPECT_EQ(testString.Length(), strlen(s_StringTestStr1));
	EXPECT_GE(testString.GetSize(), elementsOf(s_StringTestStr1));
}

TEST(EQSTRING_TESTS, WithLongContents)
{
	EqString testString(s_StringTestStr2);

	EXPECT_EQ(testString, s_StringTestStr2);
	ASSERT_TRUE(testString.IsValid());
	EXPECT_EQ(testString.Length(), strlen(s_StringTestStr2));
	EXPECT_GE(testString.GetSize(), elementsOf(s_StringTestStr2));
}

TEST(EQSTRING_TESTS, ReassignShortToLong)
{
	EqString testString(s_StringTestStr1);

	testString = s_StringTestStr2;

	EXPECT_EQ(testString, s_StringTestStr2);
	ASSERT_TRUE(testString.IsValid());
	EXPECT_EQ(testString.Length(), strlen(s_StringTestStr2));
	EXPECT_GE(testString.GetSize(), elementsOf(s_StringTestStr2));
}

TEST(EQSTRING_TESTS, ReassignLongToShort)
{
	EqString testString(s_StringTestStr2);

	testString = s_StringTestStr1;

	EXPECT_EQ(testString, s_StringTestStr1);
	ASSERT_TRUE(testString.IsValid());
	EXPECT_EQ(testString.Length(), strlen(s_StringTestStr1));
	EXPECT_GE(testString.GetSize(), elementsOf(s_StringTestStr1));
}

TEST(EQSTRING_TESTS, AssignEmpty)
{
	EqString testString(s_StringTestStr2);

	testString = "";

	EXPECT_EQ(testString, "");
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
	EXPECT_EQ(testString.Length(), strlen(s_StringTestStr1));
	EXPECT_GE(testString.GetSize(), elementsOf(s_StringTestStr1));
}

TEST(EQSTRING_TESTS, EmptyPrepend)
{
	EqString testString;

	testString.Insert(s_StringTestStr1, 0);

	EXPECT_EQ(testString, s_StringTestStr1);
	ASSERT_TRUE(testString.IsValid());
	EXPECT_EQ(testString.Length(), strlen(s_StringTestStr1));
	EXPECT_GE(testString.GetSize(), elementsOf(s_StringTestStr1));
}

TEST(EQSTRING_TESTS, Append)
{
	EqString testString = s_StringTestStrPart1;

	testString.Append(s_StringTestStrPart2);

	EXPECT_EQ(testString, s_StringTestStrTwoParts);
	ASSERT_TRUE(testString.IsValid());
	EXPECT_EQ(testString.Length(), strlen(s_StringTestStrTwoParts));
	EXPECT_GE(testString.GetSize(), elementsOf(s_StringTestStrTwoParts));
}

TEST(EQSTRING_TESTS, Prepend)
{
	EqString testString = s_StringTestStrPart2;

	testString.Insert(s_StringTestStrPart1, 0);

	EXPECT_EQ(testString, s_StringTestStrTwoParts);
	ASSERT_TRUE(testString.IsValid());
	EXPECT_EQ(testString.Length(), strlen(s_StringTestStrTwoParts));
	EXPECT_GE(testString.GetSize(), elementsOf(s_StringTestStrTwoParts));
}

TEST(EQSTRING_TESTS, InsertLast)
{
	EqString testString = s_StringTestStrPart1;

	testString.Insert(s_StringTestStrPart2, testString.Length());

	EXPECT_EQ(testString, s_StringTestStrTwoParts);
	ASSERT_TRUE(testString.IsValid());
	EXPECT_EQ(testString.Length(), strlen(s_StringTestStrTwoParts));
	EXPECT_GE(testString.GetSize(), elementsOf(s_StringTestStrTwoParts));
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
	EXPECT_EQ(testString, checkStr.c_str());
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
		EXPECT_EQ(testString, checkStr.c_str());
		ASSERT_TRUE(testString.IsValid());
		EXPECT_EQ(testString.Length(), checkStr.length());
		EXPECT_GE(testString.GetSize(), checkStr.length() + 1);
	}

	testString.Insert(s_StringTestStrPart3, 3);

	{
		std::string checkStr = ("The Nuts of God And the Extraordinary Nuts String");
		EXPECT_EQ(testString, checkStr.c_str());
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
		EXPECT_EQ(testString, checkStr.c_str());
		ASSERT_TRUE(testString.IsValid());
		EXPECT_EQ(testString.Length(), checkStr.length());
		EXPECT_GE(testString.GetSize(), checkStr.length() + 1);
	}

	{
		EqString testString(s_StringTestStr2);

		testString.Remove(9, strlen("Very Long Long long hell long "));

		std::string checkStr = ("No Magic string Value 1234567890 0xFEDABEEF");

		EXPECT_EQ(testString, checkStr.c_str());
		ASSERT_TRUE(testString.IsValid());
		EXPECT_EQ(testString.Length(), checkStr.length());
		EXPECT_GE(testString.GetSize(), checkStr.length() + 1);
	}

	// TEST: remove last single char
	{
		EqString testString(s_StringTestStr1);

		testString.Remove(13, 1);

		std::string checkStr = ("Testing Strin");

		EXPECT_EQ(testString, checkStr.c_str());
		ASSERT_TRUE(testString.IsValid());
		EXPECT_EQ(testString.Length(), checkStr.length());
		EXPECT_GE(testString.GetSize(), checkStr.length() + 1);
	}

	// TEST: invalid input
	{
		EqString testString(s_StringTestStr1);
		EXPECT_FATAL_FAILURE([&]() {testString.Remove(-4, 8); }, "nStart");
		EXPECT_FATAL_FAILURE([&]() {testString.Remove(12, 8); }, "nStart + nCount");

		ASSERT_TRUE(testString.IsValid());
	}
}

TEST(EQSTRING_TESTS, Left)
{
	EqString testString(s_StringTestStr1);

	EqString leftPart = testString.Left(4);

	std::string checkStr = ("Test");
	EXPECT_EQ(leftPart, checkStr.c_str());
	ASSERT_TRUE(leftPart.IsValid());
	EXPECT_EQ(leftPart.Length(), checkStr.length());
	EXPECT_GE(leftPart.GetSize(), checkStr.length() + 1);
}

TEST(EQSTRING_TESTS, Right)
{
	EqString testString(s_StringTestStr1);

	EqString rightPart = testString.Right(6);

	std::string checkStr = ("String");
	EXPECT_EQ(rightPart, checkStr.c_str());
	ASSERT_TRUE(rightPart.IsValid());
	EXPECT_EQ(rightPart.Length(), checkStr.length());
	EXPECT_GE(rightPart.GetSize(), checkStr.length() + 1);
}

TEST(EQSTRING_TESTS, Mid)
{
	EqString testString(s_StringTestStrTwoParts);

	EqString midPart = testString.Mid(4, 6);

	std::string checkStr = ("String");
	EXPECT_EQ(midPart, checkStr.c_str());
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
	EXPECT_EQ(checkPart, checkStr.c_str());
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
		EXPECT_EQ(testString, checkStr.c_str());
		ASSERT_TRUE(testString.IsValid());
		EXPECT_EQ(testString.Length(), checkStr.length());
		EXPECT_GE(testString.GetSize(), checkStr.length() + 1);
	}

	// TEST: replace with longer string
	{
		EqString testString(s_StringTestStrTwoParts);

		testString.ReplaceSubstr("Extraordinary", "BoomeryBazookahAmazing");

		std::string checkStr = ("The String And the BoomeryBazookahAmazing Nuts");
		EXPECT_EQ(testString, checkStr.c_str());
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
		EXPECT_EQ(testString, checkStr.c_str());
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
		EXPECT_EQ(testString, checkStr.c_str());
		ASSERT_TRUE(testString.IsValid());
		EXPECT_EQ(testString.Length(), checkStr.length());
		EXPECT_GE(testString.GetSize(), checkStr.length() + 1);
	}
}

TEST(EQSTRINGREF_TESTS, Instantiate)
{
	GTEST_SKIP() << "TODO: string ref tests";

	EqStringRef defaultRef;
	EqStringRef nullRef = nullptr;
	EqStringRef strRef = "String";
	static constexpr EqStringRef cexprRef = "Extraordinary";

	EqString str = "Extraordinary";

	EqStringRef eqStrRef = str;

	const int t = cexprRef.Find("Dinary", false, 0);

	EXPECT_EQ(t, 7);
	//EXPECT_EQ(str.Length(), 13);
}