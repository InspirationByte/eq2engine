#include <gtest/gtest.h>
#include <gtest/gtest-spi.h>

#include "core/core_common.h"
#include "utils/CRC32.h"


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

TEST(EQSTRING_TESTS, LeftRightMidChain)
{
	EqString testString(s_StringTestStrTwoParts);	// "The String And the Extraordinary Nuts"

	EqString thePart = testString.Right(34).Left(29).Mid(12, 3);

	std::string checkStr = ("the");
	EXPECT_EQ(thePart, EqStringRef(checkStr.c_str()));
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

TEST(EQSTRING_TESTS, TestMove)
{
	EqString str1 = "John";
	const char* strPtr = str1.ToCString();
	EqString str2 = std::move(str1);

	EXPECT_EQ(str1.Length(), 0);
	EXPECT_EQ(str1.GetData(), nullptr);
	EXPECT_EQ(str2, "John");
	EXPECT_EQ(reinterpret_cast<const void*>(str2.ToCString()), reinterpret_cast<const void*>(strPtr));
}

TEST(EQSTRING_TESTS, MoveAssign)
{
	// TEST: source has larger buffer, pointer is taken
	{
		EqString source = s_StringTestStr2;
		const char* sourcePtr = source.ToCString();
		EqString target = "abc";

		target = std::move(source);
		EXPECT_EQ(target, s_StringTestStr2);
		EXPECT_EQ(reinterpret_cast<const void*>(target.ToCString()), reinterpret_cast<const void*>(sourcePtr));
		EXPECT_EQ(source.Length(), 0);
		EXPECT_EQ(source.GetData(), nullptr);
	}

	// TEST: target buffer is large enough, contents are copied
	{
		EqString source = "abc";
		EqString target = s_StringTestStr2;
		const char* targetPtr = target.ToCString();

		target = std::move(source);
		EXPECT_EQ(target, "abc");
		ASSERT_TRUE(target.IsValid());
		EXPECT_EQ(reinterpret_cast<const void*>(target.ToCString()), reinterpret_cast<const void*>(targetPtr));
		EXPECT_EQ(source.Length(), 0);
		EXPECT_EQ(source.GetData(), nullptr);
	}
}

TEST(EQSTRING_TESTS, SelfAssign)
{
	EqString testString = s_StringTestStr1;
	EqString& sameString = testString;

	testString = sameString;
	EXPECT_EQ(testString, s_StringTestStr1);
	ASSERT_TRUE(testString.IsValid());
}

TEST(EQSTRING_TESTS, AssignNull)
{
	EqString testString = s_StringTestStr1;

	testString.Assign(static_cast<const char*>(nullptr));
	EXPECT_EQ(testString.Length(), 0);
	EXPECT_EQ(testString, "");
	ASSERT_TRUE(testString.IsValid());

	testString = s_StringTestStr1;
	testString.Assign(EqStringRef());
	EXPECT_EQ(testString.Length(), 0);
	ASSERT_TRUE(testString.IsValid());
}

TEST(EQSTRING_TESTS, AssignPart)
{
	EqString testString;

	testString.Assign("Testing String", 4);
	EXPECT_EQ(testString, "Test");
	EXPECT_EQ(testString.Length(), 4);

	EqString fromRef(EqStringRef("Testing String"), 8);
	EXPECT_EQ(fromRef, "String");

	EqString copyPart(EqString("Testing String"), 2, 3);
	EXPECT_EQ(copyPart, "sti");
}

TEST(EQSTRING_TESTS, AssignPartUpToEnd)
{
	// range which ends exactly at the end of source string
	EqString testString(EqStringRef("Testing"), 4, 3);
	EXPECT_EQ(testString, "ing");
	EXPECT_EQ(testString.Length(), 3);
	ASSERT_TRUE(testString.IsValid());
}

TEST(EQSTRING_TESTS, ClearAndEmpty)
{
	EqString testString = s_StringTestStr2;

	testString.Empty();
	EXPECT_EQ(testString.Length(), 0);
	EXPECT_EQ(testString, "");
	EXPECT_NE(testString.GetData(), nullptr);
	EXPECT_GT(testString.GetSize(), 0);
	ASSERT_TRUE(testString.IsValid());

	testString = s_StringTestStr1;
	testString.Clear();
	EXPECT_EQ(testString.Length(), 0);
	EXPECT_EQ(testString.GetSize(), 0);
	EXPECT_EQ(testString.GetData(), nullptr);
	EXPECT_EQ(*testString.ToCString(), 0);
	ASSERT_TRUE(testString.IsValid());

	// both are safe on never allocated string
	EqString emptyString;
	emptyString.Empty();
	emptyString.Clear();
	EXPECT_EQ(emptyString.Length(), 0);
}

TEST(EQSTRING_TESTS, IndexOperator)
{
	EqString testString = "abc";
	EXPECT_EQ(testString[0], 'a');
	EXPECT_EQ(testString[2], 'c');
	EXPECT_EQ(testString[3], 0);

	EqString emptyString;
	EXPECT_EQ(emptyString[0], 0);
}

TEST(EQSTRING_TESTS, ResizeShrink)
{
	EqString testString(s_StringTestStr2);
	ASSERT_GT(testString.Length(), 40);

	testString.Resize(35);
	EXPECT_EQ(testString.Length(), 35);
	ASSERT_TRUE(testString.IsValid());
	EXPECT_EQ(testString, s_StringTestStr2.Left(35));
}

TEST(EQSTRING_TESTS, ResizeGrowKeepsContents)
{
	EqString testString(s_StringTestStr1);

	testString.Resize(500);
	EXPECT_GE(testString.GetSize(), 500);
	EXPECT_EQ(testString, s_StringTestStr1);
	ASSERT_TRUE(testString.IsValid());

	testString.Resize(500, false);
	EXPECT_EQ(testString.Length(), 0);
	ASSERT_TRUE(testString.IsValid());
}

TEST(EQSTRING_TESTS, AppendCount)
{
	EqString testString = "Test";

	testString.Append("ing String", 3);
	EXPECT_EQ(testString, "Testing");

	// count larger than string length is clamped
	testString.Append("!", 10);
	EXPECT_EQ(testString, "Testing!");

	testString.Append("abc", 0);
	EXPECT_EQ(testString, "Testing!");
	ASSERT_TRUE(testString.IsValid());
}

TEST(EQSTRING_TESTS, AppendRefWithLength)
{
	EqString testString = "Test";

	testString.Append(EqStringRef("ing String", 3));
	EXPECT_EQ(testString, "Testing");
	ASSERT_TRUE(testString.IsValid());
}

TEST(EQSTRING_TESTS, AppendSelf)
{
	// grow past the initial buffer so reallocation happens
	EqString testString = "0123456789ABCDEF0123456789";

	testString.Append(testString);
	EXPECT_EQ(testString, "0123456789ABCDEF01234567890123456789ABCDEF0123456789");
	ASSERT_TRUE(testString.IsValid());
}

TEST(EQSTRING_TESTS, AppendSelfCString)
{
	EqString testString = "0123456789ABCDEF0123456789";

	testString.Append(testString.ToCString());
	EXPECT_EQ(testString, "0123456789ABCDEF01234567890123456789ABCDEF0123456789");
	ASSERT_TRUE(testString.IsValid());
}

TEST(EQSTRING_TESTS, InsertSelfCString)
{
	EqString testString = "0123456789ABCDEF0123456789";

	testString.Insert(testString.ToCString(), 0);
	EXPECT_EQ(testString, "0123456789ABCDEF01234567890123456789ABCDEF0123456789");
	ASSERT_TRUE(testString.IsValid());
}

TEST(EQSTRING_TESTS, InsertCount)
{
	EqString testString = "The Nuts";

	testString.Insert("Big Bad Wolf", 4, 4);
	EXPECT_EQ(testString, "The Big Nuts");

	testString.Insert(EqStringRef("Red Green", 4), 4);
	EXPECT_EQ(testString, "The Red Big Nuts");
	ASSERT_TRUE(testString.IsValid());
}

TEST(EQSTRING_TESTS, AppendFmt)
{
	EqString testString = "Value:";

	testString.AppendFmt(" %d %s", 42, "units");
	EXPECT_EQ(testString, "Value: 42 units");
	ASSERT_TRUE(testString.IsValid());

	testString.AppendFmt("%s", EqString("!"));
	EXPECT_EQ(testString, "Value: 42 units!");
}

TEST(EQSTRING_TESTS, AppendFmtLong)
{
	EqString longPart;
	for (int i = 0; i < 300; ++i)
		longPart.Append(static_cast<char>('a' + (i % 26)));

	EqString testString;
	for (int i = 0; i < 100; ++i)
		testString.Append(static_cast<char>('0' + (i % 10)));
	const EqString prefix = testString;

	testString.AppendFmt("%s", longPart);
	EXPECT_EQ(testString.Length(), 400);
	EXPECT_EQ(testString, prefix + longPart);
	ASSERT_TRUE(testString.IsValid());
}

TEST(EQSTRING_TESTS, Format)
{
	EXPECT_EQ(EqString::Format("%d-%s", 42, "abc"), "42-abc");
	EXPECT_EQ(EqString::Format("%s!", EqString("hi")), "hi!");
	EXPECT_EQ(EqString::Format("no args"), "no args");
	EXPECT_EQ(EqString::Format("").Length(), 0);
}

TEST(EQSTRING_TESTS, FormatLong)
{
	EqString longPart;
	for (int i = 0; i < 500; ++i)
		longPart.Append(static_cast<char>('a' + (i % 26)));

	const EqStringRef formatted = EqString::Format("[%s]", longPart);
	EXPECT_EQ(formatted.Length(), 502);
	EXPECT_EQ(EqString(formatted), "[" + longPart + "]");
}

static EqStringRef FormatVTest(const char* format, ...)
{
	va_list argptr;
	va_start(argptr, format);
	const EqStringRef result = EqString::FormatV(format, argptr);
	va_end(argptr);
	return result;
}

static EqWStringRef FormatVTestW(const wchar_t* format, ...)
{
	va_list argptr;
	va_start(argptr, format);
	const EqWStringRef result = EqWString::FormatV(format, argptr);
	va_end(argptr);
	return result;
}

static EqString MakeTestString(int length)
{
	EqString str;
	for (int i = 0; i < length; ++i)
		str.Append(static_cast<char>('a' + (i % 26)));
	return str;
}

static EqWString MakeTestWString(int length)
{
	EqWString str;
	for (int i = 0; i < length; ++i)
		str.Append(static_cast<wchar_t>(L'a' + (i % 26)));
	return str;
}

TEST(EQSTRING_TESTS, FormatV)
{
	EXPECT_EQ(FormatVTest("%d-%s", 42, "abc"), "42-abc");
	EXPECT_EQ(FormatVTest("100%%"), "100%");
	EXPECT_EQ(FormatVTest("[%s]", ""), "[]");
	EXPECT_EQ(FormatVTest("").Length(), 0);
	ASSERT_TRUE(FormatVTest("abc").IsValid());
}

TEST(EQSTRING_TESTS, FormatVLongMultipleArgs)
{
	// result does not fit initial buffer, so arguments are formatted twice
	const EqString partA = MakeTestString(300);
	const EqString partB = MakeTestString(200);

	const EqStringRef result = FormatVTest("%s|%d|%s|%c", partA.ToCString(), 12345, partB.ToCString(), 'Z');
	EXPECT_EQ(result.Length(), 300 + 1 + 5 + 1 + 200 + 2);
	EXPECT_EQ(EqString(result), partA + "|12345|" + partB + "|Z");
}

TEST(EQSTRING_TESTS, FormatVLengthBoundaries)
{
	// cover lengths around initial buffer size and its growth steps
	for (int length = 0; length < 400; ++length)
	{
		const EqString part = MakeTestString(length);
		const EqStringRef result = FormatVTest("%s", part.ToCString());

		ASSERT_EQ(result.Length(), length) << "length " << length;
		ASSERT_EQ(result, part.Ref()) << "length " << length;
	}
}

TEST(EQSTRING_TESTS, FormatNested)
{
	// arguments are temporary strings themselves
	const EqStringRef result = EqString::Format("%s/%s", EqString::Format("a%d", 1), EqString::Format("b%d", 2));
	EXPECT_EQ(result, "a1/b2");

	const EqString longPart = MakeTestString(300);
	const EqStringRef longResult = EqString::Format("<%s>", EqString::Format("%s", longPart));
	EXPECT_EQ(EqString(longResult), "<" + longPart + ">");
}

TEST(EQSTRING_TESTS, FormatResultsAreIndependent)
{
	const EqStringRef first = EqString::Format("first %d", 1);
	const EqStringRef second = EqString::Format("second %d", 2);

	EXPECT_EQ(first, "first 1");
	EXPECT_EQ(second, "second 2");
	EXPECT_NE(first.GetData(), second.GetData());
}

TEST(EQSTRING_TESTS, AppendFmtAfterFormat)
{
	EqString testString = EqString::Format("%d", 1);
	testString.AppendFmt("-%d", 2);
	testString.AppendFmt("-%s", "three");

	EXPECT_EQ(testString, "1-2-three");
	ASSERT_TRUE(testString.IsValid());
}

TEST(EQSTRING_TESTS, AppendFmtAfterFormatLong)
{
	const EqString partA = MakeTestString(150);
	const EqString partB = MakeTestString(250);

	EqString testString = EqString::Format("%s", partA);
	testString.AppendFmt("|%s|", partB);

	EXPECT_EQ(testString.Length(), 150 + 250 + 2);
	EXPECT_EQ(testString, partA + "|" + partB + "|");
	ASSERT_TRUE(testString.IsValid());
}

TEST(EQSTRING_TESTS, AppendFmtFormatArgument)
{
	EqString testString = "value:";
	testString.AppendFmt(" %s", EqString::Format("x%d", 5));
	EXPECT_EQ(testString, "value: x5");

	const EqString longPart = MakeTestString(300);
	testString.AppendFmt(" %s", EqString::Format("<%s>", longPart));
	EXPECT_EQ(testString, "value: x5 <" + longPart + ">");
	ASSERT_TRUE(testString.IsValid());
}

TEST(EQSTRING_TESTS, AppendFmtLengthBoundaries)
{
	// existing contents of various lengths with appended parts crossing buffer growth steps
	for (int prefixLength = 0; prefixLength < 100; prefixLength += 7)
	{
		const EqString prefix = MakeTestString(prefixLength);
		for (int length = 0; length < 300; ++length)
		{
			const EqString part = MakeTestString(length);

			EqString testString = EqString::Format("%s", prefix);
			testString.AppendFmt("%s", part);

			ASSERT_EQ(testString.Length(), prefixLength + length) << "prefix " << prefixLength << " length " << length;
			ASSERT_EQ(testString, prefix + part) << "prefix " << prefixLength << " length " << length;
			ASSERT_TRUE(testString.IsValid()) << "prefix " << prefixLength << " length " << length;
		}
	}
}

TEST(EQSTRING_TESTS, AppendFmtSelfArgument)
{
	// argument points into the string being extended
	EqString testString = MakeTestString(100);
	const EqString original = testString;

	testString.AppendFmt("|%s", testString);
	EXPECT_EQ(testString.Length(), 201);
	EXPECT_EQ(testString, original + "|" + original);
	ASSERT_TRUE(testString.IsValid());
}

TEST(EQWSTRING_TESTS, AppendFmtAfterFormat)
{
	EqWString testString = EqWString::Format(L"%d", 1);
	testString.AppendFmt(L"-%d", 2);
	testString.AppendFmt(L"-%ls", L"three");

	EXPECT_EQ(testString.Compare(L"1-2-three"), 0);
	ASSERT_TRUE(testString.IsValid());
}

TEST(EQWSTRING_TESTS, FormatV)
{
	EXPECT_EQ(FormatVTestW(L"%d-%ls", 42, L"abc").Compare(L"42-abc"), 0);
	EXPECT_EQ(FormatVTestW(L"").Length(), 0);
	ASSERT_TRUE(FormatVTestW(L"abc").IsValid());
}

TEST(EQWSTRING_TESTS, FormatVLong)
{
	const EqWString longPart = MakeTestWString(500);

	const EqWStringRef result = FormatVTestW(L"%ls", longPart.ToCString());
	EXPECT_EQ(result.Length(), 500);
	EXPECT_EQ(result.Compare(longPart), 0);
}

TEST(EQWSTRING_TESTS, FormatVLengthBoundaries)
{
	for (int length = 0; length < 400; ++length)
	{
		const EqWString part = MakeTestWString(length);
		const EqWStringRef result = FormatVTestW(L"%ls", part.ToCString());

		ASSERT_EQ(result.Length(), length) << "length " << length;
		ASSERT_EQ(result.Compare(part), 0) << "length " << length;
	}
}

TEST(EQWSTRING_TESTS, AppendFmtLong)
{
	const EqWString longPart = MakeTestWString(300);

	EqWString testString = MakeTestWString(100);
	const EqWString prefix = testString;

	testString.AppendFmt(L"%ls", longPart.ToCString());
	EXPECT_EQ(testString.Length(), 400);
	EXPECT_EQ(testString.Left(100).Compare(prefix), 0);
	EXPECT_EQ(testString.Right(300).Compare(longPart), 0);
	ASSERT_TRUE(testString.IsValid());
}

TEST(EQSTRING_TESTS, CompareCaseIns)
{
	EqString testString = "Testing String";

	EXPECT_EQ(testString.CompareCaseIns("tESTING sTRING"), 0);
	EXPECT_NE(testString.Compare("tESTING sTRING"), 0);
	EXPECT_LT(testString.CompareCaseIns("TESTING STRINGS"), 0);
	EXPECT_GT(testString.CompareCaseIns("TESTING"), 0);
}

TEST(EQSTRING_TESTS, Find)
{
	EqString testString = "Hello hello HELLO";

	// search is case insensitive by default
	EXPECT_EQ(testString.Find("hello"), 0);
	EXPECT_EQ(testString.Find("hello", true), 6);
	EXPECT_EQ(testString.Find("HELLO", true), 12);
	EXPECT_EQ(testString.Find("hello", false, 1), 6);
	EXPECT_EQ(testString.Find("hello", true, 7), -1);
	EXPECT_EQ(testString.Find("world"), -1);
	EXPECT_EQ(testString.Find("hello", false, 100), -1);
	EXPECT_EQ(testString.Find("hello", false, -1), -1);
}

TEST(EQSTRING_TESTS, FindChar)
{
	EqString testString = "abcabc";

	EXPECT_EQ(testString.Find('c'), 2);
	EXPECT_EQ(testString.Find('c', true, 3), 5);
	EXPECT_EQ(testString.Find('C', true), -1);
	EXPECT_EQ(testString.Find('C'), 2);
	EXPECT_EQ(testString.Find('z'), -1);
}

TEST(EQSTRING_TESTS, LowerUpperCase)
{
	EqString testString = "MiXeD Case 123";

	EXPECT_EQ(testString.LowerCase(), "mixed case 123");
	EXPECT_EQ(testString.UpperCase(), "MIXED CASE 123");

	// source is not changed
	EXPECT_EQ(testString, "MiXeD Case 123");
}

TEST(EQSTRING_TESTS, ReplaceSubstrCase)
{
	// TEST: case sensitive skips other case
	{
		EqString testString = "Hello hello";
		EXPECT_EQ(testString.ReplaceSubstr("hello", "bye", true), 6);
		EXPECT_EQ(testString, "Hello bye");
	}

	// TEST: case insensitive takes first match
	{
		EqString testString = "Hello hello";
		EXPECT_EQ(testString.ReplaceSubstr("hello", "bye"), 0);
		EXPECT_EQ(testString, "bye hello");
	}

	// TEST: not found leaves string as is
	{
		EqString testString = "Hello hello";
		EXPECT_EQ(testString.ReplaceSubstr("world", "bye"), -1);
		EXPECT_EQ(testString, "Hello hello");
	}
}

TEST(EQSTRING_TESTS, ReplaceSubstrContainingFind)
{
	EqString testString = "aXa";

	// returned position is the start of the replaced part
	EXPECT_EQ(testString.ReplaceSubstr("a", "aa"), 0);
	EXPECT_EQ(testString, "aaXa");

	EXPECT_EQ(testString.ReplaceSubstr("a", "aa", false, 3), 3);
	EXPECT_EQ(testString, "aaXaa");
	ASSERT_TRUE(testString.IsValid());
}

TEST(EQSTRING_TESTS, Trim)
{
	EqString testString = "  \tTesting String \n ";

	EXPECT_EQ(testString.TrimSpaces(), "Testing String");
	EXPECT_EQ(EqString("--abc--").TrimChar('-'), "abc");
	EXPECT_EQ(EqString("+-abc-+").TrimChar("-+"), "abc");
}

TEST(EQSTRING_TESTS, Serialize)
{
	CMemoryStream stream(PP_SL);
	ASSERT_TRUE(stream.Open(FS_OPEN_READ | FS_OPEN_WRITE));

	const EqString first = s_StringTestStr2;
	const EqString empty;
	const EqString last = s_StringTestStr1;

	VSWrite(&stream, first);
	VSWrite(&stream, empty);
	VSWrite(&stream, last);

	stream.Seek(0, FS_SEEK_SET);

	EqString readFirst = "garbage";
	EqString readEmpty = "garbage";
	EqString readLast;
	EXPECT_EQ(VSRead(&stream, readFirst), 1);
	EXPECT_EQ(VSRead(&stream, readEmpty), 1);
	EXPECT_EQ(VSRead(&stream, readLast), 1);

	EXPECT_EQ(readFirst, first);
	EXPECT_EQ(readEmpty.Length(), 0);
	EXPECT_EQ(readLast, last);
	ASSERT_TRUE(readFirst.IsValid());
	ASSERT_TRUE(readEmpty.IsValid());
	ASSERT_TRUE(readLast.IsValid());
}

TEST(EQSTRINGREF_TESTS, Empty)
{
	EqStringRef testString;

	EXPECT_EQ(testString.Length(), 0);
	ASSERT_FALSE(testString);
	ASSERT_FALSE(testString.IsValid());
	EXPECT_EQ(testString.GetData(), nullptr);
	EXPECT_EQ(testString.ToCString(), nullptr);
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

TEST(EQSTRINGREF_TESTS, Hash24)
{
	{
		const int hashA = StringId24("Testing String");
		const int hashB = StringIdConst24("Testing String");
		EXPECT_EQ(hashA, hashB);
	}

	{
		const int hashA = StringId24("Hashed String 2");
		const int hashB = StringIdConst24("Hashed String 2");
		EXPECT_EQ(hashA, hashB);
	}

	{
		const int hashA = StringId24("String 3");
		const int hashB = StringIdConst24("String 3");
		EXPECT_EQ(hashA, hashB);
	}

	{
		const int hashA = StringId24(" 3");
		const int hashB = StringIdConst24(" 2");
		EXPECT_NE(hashA, hashB);
	}
}

TEST(EQSTRINGREF_TESTS, HashCRC32)
{
	{
		const uint hashA = StringId("Testing String");
		const uint hashB = StringIdConst("Testing String");
		EXPECT_EQ(hashA, hashB);
	}

	{
		const uint hashA = StringId("Hashed String 2");
		const uint hashB = StringIdConst("Hashed String 2");
		EXPECT_EQ(hashA, hashB);
	}

	{
		const uint hashA = StringId("String 3");
		const uint hashB = StringIdConst("String 3");
		EXPECT_EQ(hashA, hashB);
	}

	{
		const uint hashA = StringId(" 3");
		const uint hashB = StringIdConst(" 2");
		EXPECT_NE(hashA, hashB);
	}
}

TEST(EQSTRINGREF_TESTS, Length)
{
	EXPECT_EQ(EqStringRef("abc").Length(), 3);
	EXPECT_EQ(EqStringRef("").Length(), 0);
	EXPECT_EQ(EqStringRef("abcdef", 3).Length(), 3);
	EXPECT_EQ(EqStringRef("abcdef", -1).Length(), 6);

	// cached length stays the same on repeated calls
	EqStringRef testString = s_StringTestStr2;
	EXPECT_EQ(testString.Length(), testString.Length());
}

TEST(EQSTRINGREF_TESTS, IndexOperator)
{
	EqStringRef testString = "abc";

	EXPECT_EQ(testString[0], 'a');
	EXPECT_EQ(testString[2], 'c');
	EXPECT_EQ(testString[3], 0);
}

TEST(EQSTRINGREF_TESTS, CompareWithLength)
{
	const EqStringRef testString("Testing String", 4);

	EXPECT_EQ(testString.Compare("Test"), 0);
	EXPECT_TRUE(testString == "Test");
	EXPECT_FALSE(testString == "Testing String");
}

TEST(EQSTRINGREF_TESTS, FindWithLength)
{
	const EqStringRef testString("Testing String", 4);

	EXPECT_EQ(testString.Find("est"), 1);
	EXPECT_EQ(testString.Find("ing"), -1);
	EXPECT_EQ(testString.Find("String"), -1);
}

TEST(EQSTRINGREF_TESTS, CompareInvalid)
{
	const EqStringRef invalidA;
	const EqStringRef invalidB;
	const EqStringRef valid = "abc";

	EXPECT_EQ(invalidA.Compare(invalidB), 0);
	EXPECT_NE(invalidA.Compare(valid), 0);
	EXPECT_NE(valid.Compare(invalidA), 0);
	EXPECT_TRUE(invalidA == invalidB);
}

TEST(EQSTRINGREF_TESTS, CompareOrdering)
{
	const EqStringRef testString = "abc";

	EXPECT_LT(testString.Compare("abd"), 0);
	EXPECT_GT(testString.Compare("abb"), 0);
	EXPECT_EQ(testString.CompareCaseIns("ABC"), 0);
	EXPECT_LT(testString.CompareCaseIns("ABD"), 0);
	EXPECT_GT(testString.CompareCaseIns("ABB"), 0);
}

TEST(EQSTRINGREF_TESTS, MatchingChars)
{
	// separate buffers so reading past terminators would not hit the same memory
	const EqString strA = "Testing String";
	const EqString strB = "Testing Strand";
	const EqString strC = "Testing String";

	EXPECT_EQ(strA.Ref().GetMathingChars(strB), 11);
	EXPECT_EQ(strA.Ref().GetMathingChars("testing"), 0);
	EXPECT_EQ(strA.Ref().GetMathingCharsCaseIns("tESTING sTRAND"), 11);
	EXPECT_EQ(strA.Ref().GetMathingChars(EqStringRef()), 0);

	EXPECT_EQ(strA.Ref().GetMathingChars(strC), strA.Length());
	EXPECT_EQ(strA.Ref().GetMathingCharsCaseIns("TESTING STRING"), strA.Length());
}

TEST(EQSTRINGREF_TESTS, Find)
{
	constexpr EqStringRef testString = "Hello hello HELLO";

	EXPECT_EQ(testString.Find("hello"), 0);
	EXPECT_EQ(testString.Find("hello", true), 6);
	EXPECT_EQ(testString.Find("hello", false, 1), 6);
	EXPECT_EQ(testString.Find("world"), -1);
	EXPECT_EQ(testString.Find(EqStringRef()), -1);
	EXPECT_EQ(EqStringRef().Find("hello"), -1);
}

TEST(EQSTRINGREF_TESTS, FindChar)
{
	constexpr EqStringRef testString = "abcabc";

	EXPECT_EQ(testString.Find('c'), 2);
	EXPECT_EQ(testString.Find('c', true, 3), 5);
	EXPECT_EQ(testString.Find('C', true), -1);
	EXPECT_EQ(testString.Find('z'), -1);
}

TEST(EQSTRINGREF_TESTS, LeftRightMid)
{
	constexpr EqStringRef testString = "Testing String";

	EXPECT_EQ(testString.Left(4), "Test");
	EXPECT_EQ(testString.Right(6), "String");
	EXPECT_EQ(testString.Mid(4, 3), "ing");

	// TEST: boundaries
	EXPECT_EQ(testString.Left(0), "");
	EXPECT_EQ(testString.Right(0), "");
	EXPECT_EQ(testString.Mid(14, 0), "");
	EXPECT_EQ(testString.Left(14), testString);
	EXPECT_EQ(testString.Right(14), testString);
	EXPECT_EQ(testString.Mid(0, 14), testString);

	// TEST: invalid ref gives empty string
	EXPECT_EQ(EqStringRef().Left(2), "");
	EXPECT_EQ(EqStringRef().Mid(1, 2), "");
}

TEST(EQSTRINGREF_TESTS, LeftRightOutOfRange)
{
	constexpr EqStringRef testString = "Testing";

	// both clamp to the whole string
	EXPECT_EQ(testString.Right(100), testString);
	EXPECT_EQ(testString.Left(100), testString);
}

TEST(EQSTRINGREF_TESTS, MidOutOfRange)
{
	constexpr EqStringRef testString = "Testing";

	EXPECT_FATAL_FAILURE([&]() { EXPECT_EQ(testString.Mid(5, 5), ""); }, "nStart + nCount <= Length()");
	EXPECT_FATAL_FAILURE([&]() { EXPECT_EQ(testString.Mid(-1, 2), ""); }, "nStart >= 0");
}

TEST(EQSTRINGREF_TESTS, LowerUpperCase)
{
	constexpr EqStringRef testString = "MiXeD Case 123";

	EXPECT_EQ(testString.LowerCase(), "mixed case 123");
	EXPECT_EQ(testString.UpperCase(), "MIXED CASE 123");
	EXPECT_EQ(EqStringRef("").LowerCase(), "");
}

TEST(EQSTRINGREF_TESTS, TempStringsStayValid)
{
	const EqStringRef lower = EqStringRef("ABC").LowerCase();

	// following temporary string producers must not overwrite the previous result right away
	for (int i = 0; i < 16; ++i)
		EqStringRef("Testing String").Mid(i % 8, 4);

	EXPECT_EQ(lower, "abc");
}

TEST(EQSTRINGREF_TESTS, TrimSpaces)
{
	constexpr EqStringRef testString = "  \tTesting String \n ";

	EXPECT_EQ(testString.TrimSpaces(), "Testing String");
	EXPECT_EQ(EqStringRef("Testing").TrimSpaces(), "Testing");
	EXPECT_EQ(EqStringRef("   ").TrimSpaces(), "");
	EXPECT_EQ(EqStringRef("").TrimSpaces(), "");
	EXPECT_EQ(EqStringRef().TrimSpaces(), "");
}

TEST(EQSTRINGREF_TESTS, TrimSpacesOneSide)
{
	constexpr EqStringRef testString = "  abc  ";

	EXPECT_EQ(testString.TrimSpaces(true, false), "abc  ");
	EXPECT_EQ(testString.TrimSpaces(false, true), "  abc");
	EXPECT_EQ(testString.TrimSpaces(false, false), testString);
}

TEST(EQSTRINGREF_TESTS, TrimChar)
{
	EXPECT_EQ(EqStringRef("--abc--").TrimChar('-'), "abc");
	EXPECT_EQ(EqStringRef("--abc--").TrimChar('-', true, false), "abc--");
	EXPECT_EQ(EqStringRef("--abc--").TrimChar('-', false, true), "--abc");
	EXPECT_EQ(EqStringRef("+-abc-+").TrimChar("-+"), "abc");
	EXPECT_EQ(EqStringRef("a-b").TrimChar('-'), "a-b");
	EXPECT_EQ(EqStringRef("----").TrimChar('-'), "");
	EXPECT_EQ(EqStringRef().TrimChar('-'), "");
}

TEST(EQSTRINGREF_TESTS, EatWhiteSpaces)
{
	EXPECT_EQ(EqStringRef(" a b\tc ").EatWhiteSpaces(), "abc");
	EXPECT_EQ(EqStringRef("abc").EatWhiteSpaces(), "abc");
	EXPECT_EQ(EqStringRef("   ").EatWhiteSpaces(), "");
}

TEST(EQSTRINGREF_TESTS, Hash)
{
	EXPECT_EQ(StringId("Testing String"), StringId(EqString("Testing String")));
	EXPECT_NE(StringId("Testing String"), StringId("testing string"));
	EXPECT_EQ(StringId24("Testing String"), StringId24(EqString("Testing String")));
	EXPECT_EQ(StringId("Testing String", true), StringId("tESTING sTRING", true));
	EXPECT_EQ(StringId24("Testing String", true), StringId24("tESTING sTRING", true));
}

//-------------------------------------------------------------------------------------------------

TEST(EQWSTRING_TESTS, Basics)
{
	EqWString testString = L"Hello";

	EXPECT_EQ(testString.Length(), 5);
	ASSERT_TRUE(testString.IsValid());

	testString.Append(L" World");
	EXPECT_EQ(testString.Compare(L"Hello World"), 0);
	EXPECT_EQ(testString.Length(), 11);

	testString.Insert(L"Big ", 6);
	EXPECT_EQ(testString.Compare(L"Hello Big World"), 0);

	testString.Remove(0, 6);
	EXPECT_EQ(testString.Compare(L"Big World"), 0);

	testString.ReplaceSubstr(L"Big", L"Small");
	EXPECT_EQ(testString.Compare(L"Small World"), 0);
	EXPECT_EQ(testString.Find(L"world"), 6);
	EXPECT_EQ(testString.Find(L"world", true), -1);

	EXPECT_EQ(testString.Mid(6, 5).Compare(L"World"), 0);
	ASSERT_TRUE(testString.IsValid());
}

TEST(EQWSTRING_TESTS, Format)
{
	EXPECT_EQ(EqWString::Format(L"%d-%ls", 42, L"abc").Compare(L"42-abc"), 0);
}

TEST(EQWSTRING_TESTS, CompareCaseIns)
{
	const EqWString testString = L"abc";

	EXPECT_EQ(testString.CompareCaseIns(L"ABC"), 0);
	EXPECT_LT(testString.CompareCaseIns(L"ABD"), 0);
	EXPECT_GT(testString.CompareCaseIns(L"ABB"), 0);
}

TEST(EQWSTRING_TESTS, NonAsciiCase)
{
	// A/O with diaeresis, Cyrillic A/YA; code units are spelled out because
	// non-ASCII wide literals are not encoded reliably with current compiler settings
	const wchar_t upperChars[] = { 0x00C4, 0x00D6, 0x0410, 0x042F, 0 };
	const wchar_t lowerChars[] = { 0x00E4, 0x00F6, 0x0430, 0x044F, 0 };
	const EqWString upper = upperChars;
	const EqWString lower = lowerChars;

	EXPECT_EQ(upper.LowerCase().Compare(lower), 0);
	EXPECT_EQ(lower.UpperCase().Compare(upper), 0);
	EXPECT_EQ(upper.CompareCaseIns(lower), 0);
	EXPECT_EQ(upper.Find(lower.Right(2)), 2);
}

TEST(STRING_UTILS, FilenameUtils)
{
	const char* testPathStringExt = "abc/ideal/filename.test";
	const char* testPathStringNoExt = "noexts_only/filenameNoExt";

	const char* testPathStringExtStripped = "abc/ideal/filename";

	EXPECT_TRUE(fnmPathHasExt(testPathStringExt));
	EXPECT_FALSE(fnmPathHasExt(testPathStringNoExt));

	EXPECT_TRUE(fnmPathApplyExt(testPathStringNoExt, "ext") == "noexts_only/filenameNoExt.ext");
	EXPECT_TRUE(fnmPathApplyExt(testPathStringExt, "newext") == "abc/ideal/filename.newext");

	EXPECT_TRUE(fnmPathStripExt(testPathStringExt) == testPathStringExtStripped);
	EXPECT_TRUE(fnmPathStripExt(testPathStringNoExt) == testPathStringNoExt);
	EXPECT_TRUE(fnmPathStripName(testPathStringExt) == "abc/ideal");
	EXPECT_TRUE(fnmPathStripPath(testPathStringExt) == "filename.test");

	EXPECT_TRUE(fnmPathCombine("test", "newpath") == ("test" _CORRECT_PATH_SEPARATOR_STR "newpath"));
}

