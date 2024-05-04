using System;

namespace EqLocalizationTool
{
	public class KeyValues
	{
		enum EParserMode
		{
			MODE_DEFAULT = 0,
			MODE_SKIP_COMMENT_SINGLELINE,
			MODE_SKIP_COMMENT_MULTILINE,

			MODE_OPEN_STRING,
			MODE_QUOTED_STRING,

			MODE_PARSE_ERROR_BREAK
		};

		public enum EKVTokenState
		{
			KV_PARSE_ERROR = -1,
			KV_PARSE_RESUME = 0,
			KV_PARSE_SKIP,

			KV_PARSE_BREAK_TOKEN,   // for unquoted strings
		};

		// section beginning / ending
		const char KV_SECTION_BEGIN = '{';
        const char KV_SECTION_END = '}';

		// string symbols
		const char KV_STRING_BEGIN_END = '\"';
        const char KV_STRING_NEWLINE = '\n';
        const char KV_STRING_CARRIAGERETURN = '\r';

        // commnent symbols
        const char KV_COMMENT_SYMBOL = '/';
        const char KV_RANGECOMMENT_BEGIN_END = '*';
        const char KV_BREAK = ';';

        const char KV_ESCAPE_SYMBOL = '\\';

		// KV3 stuff, not currently used
		const char KV_ARRAY_BEGIN = '[';
		const char KV_ARRAY_END = ']';
		const char KV_ARRAY_SEPARATOR = ',';
		const char KV_TYPE_VALUESYMBOL = ':';

		static bool IsKVWhitespace(char c)
		{
            return (char.IsWhiteSpace(c) || (c) == KV_STRING_NEWLINE || (c) == KV_STRING_CARRIAGERETURN);
		}

		public class TokenInfo
        {
            public TokenInfo(int line, char[] data,int curDataIdx)
            {
                this.line = line;
                this.data = data;
                this.curDataIdx = curDataIdx;
			}

			public int line;
            public char[] data;
			public int curDataIdx;
		}

        public static string KV_ReadProcessString(string sourceStr)
        {
            return sourceStr
                    .Replace("\\\"", "\"")
                    .Replace("\\t", "\t")
                    .Replace("\\n", Environment.NewLine)
					.Replace("\\r", "\r");
        }

		public static bool KV_Tokenizer(string fileName, char[] data, Func<TokenInfo, string, object[], EKVTokenState> tokenFunc)
        {
		    EParserMode mode = EParserMode.MODE_DEFAULT;
			int curLine = 1;
			int lastModeStartLine = 0;

			int sectionDepth = 0;

            int currLetterIdx = 0;
            int firstLetterIdx = -1;

			for (; currLetterIdx < data.Length; ++currLetterIdx)
			{
				char c = data[currLetterIdx];

				if (c == KV_STRING_NEWLINE)
					++curLine;

				if (c == 0)
					break;

				// check for beginning of the string
				switch (mode)
		        {
			        case EParserMode.MODE_DEFAULT:
			        {
				        EKVTokenState state = tokenFunc(new TokenInfo(curLine, data, currLetterIdx), "c", new object[] {});

				        if (IsKVWhitespace(c))
					        break;

				        if (state == EKVTokenState.KV_PARSE_ERROR)
				        {
					        Console.Error.WriteLine($"'{fileName}' ({curLine}): unexpected '{c}' while parsing");
					        lastModeStartLine = curLine;
					        mode = EParserMode.MODE_PARSE_ERROR_BREAK;
					        break;
				        }

				        // while in skip mode we don't parse anything and let parser on top of it work
				        if (state == EKVTokenState.KV_PARSE_SKIP)
					        continue;

				        if (c == KV_COMMENT_SYMBOL)
				        {
					        // we got comment symbol again
					        if (data[currLetterIdx+1] == KV_COMMENT_SYMBOL && mode != EParserMode.MODE_SKIP_COMMENT_MULTILINE)
					        {
						        mode = EParserMode.MODE_SKIP_COMMENT_SINGLELINE;
						        lastModeStartLine = curLine;
						        continue;
					        }
					        else if (data[currLetterIdx+1] == KV_RANGECOMMENT_BEGIN_END && mode != EParserMode.MODE_SKIP_COMMENT_SINGLELINE)
					        {
						        mode = EParserMode.MODE_SKIP_COMMENT_MULTILINE;
						        lastModeStartLine = curLine;
						        ++currLetterIdx; // also skip next char
						        continue;
					        }
					        break;
				        }
				        else if (c == KV_SECTION_BEGIN)
				        {
					        ++sectionDepth;
					        state = tokenFunc(new TokenInfo(curLine, data, currLetterIdx), "s+", new object[] { sectionDepth } );

					        if (state == EKVTokenState.KV_PARSE_ERROR)
					        {
						        Console.Error.WriteLine($"'{fileName}' ({curLine}): unexpected '{c}' while parsing");
						        lastModeStartLine = curLine;
						        mode = EParserMode.MODE_PARSE_ERROR_BREAK;
						        break;
					        }
				        }
				        else if (c == KV_SECTION_END)
				        {
					        if (sectionDepth <= 0)
					        {
						        Console.Error.WriteLine($"'{fileName}' ({curLine}): unexpected '{c}' while parsing");
						        lastModeStartLine = curLine;
						        mode = EParserMode.MODE_PARSE_ERROR_BREAK;
						        break;
					        }
					        --sectionDepth;

							tokenFunc(new TokenInfo(curLine, data, currLetterIdx), "s-", new object[] { sectionDepth } );
				        }
				        else if (c == KV_STRING_BEGIN_END)
				        {
					        mode = EParserMode.MODE_QUOTED_STRING;
					        lastModeStartLine = curLine;
					        firstLetterIdx = currLetterIdx + 1; // exclude quote
				        }
				        else if (c == KV_BREAK || c == 0)
				        {
					        tokenFunc(new TokenInfo(curLine, data, currLetterIdx), "b", new object[] { });
				        }
				        else if(!IsKVWhitespace(c))
				        {
					        mode = EParserMode.MODE_OPEN_STRING; // by default we always start from string
					        lastModeStartLine = curLine;
					        firstLetterIdx = currLetterIdx;
				        }
				        break;
			        }
			        case EParserMode.MODE_SKIP_COMMENT_SINGLELINE:
			        {
				        if (c == KV_STRING_NEWLINE)
					        mode = EParserMode.MODE_DEFAULT;
				        break;
			        }
			        case EParserMode.MODE_SKIP_COMMENT_MULTILINE:
			        {
				        if (c == KV_RANGECOMMENT_BEGIN_END && data[currLetterIdx+1] == KV_COMMENT_SYMBOL)
				        {
					        mode = EParserMode.MODE_DEFAULT;
					        ++currLetterIdx; // also skip next char
				        }
				        break;
			        }
			        case EParserMode.MODE_OPEN_STRING:
			        {
				        // trigger close on any symbol
				        if (tokenFunc(new TokenInfo(curLine, data, currLetterIdx), "u", new object[] {}) == EKVTokenState.KV_PARSE_BREAK_TOKEN || IsKVWhitespace(c) || c == KV_BREAK || c == KV_COMMENT_SYMBOL || c == KV_SECTION_BEGIN)
				        {
					        mode = EParserMode.MODE_DEFAULT;
				        }

				        if (mode != EParserMode.MODE_OPEN_STRING)
				        {
                            string sourceString = new string(data, firstLetterIdx, currLetterIdx - firstLetterIdx);

							tokenFunc(new TokenInfo(curLine, data, currLetterIdx), "t", new object[] { sourceString });

					        --currLetterIdx; // open string - get back and parse character again
				        }

				        break;
			        }
			        case EParserMode.MODE_QUOTED_STRING:
			        {
				        // only trigger when on end
				        if (c == KV_STRING_BEGIN_END && data[currLetterIdx-1] != KV_ESCAPE_SYMBOL)
				        {
					        mode = EParserMode.MODE_DEFAULT;
				        }

				        if (mode != EParserMode.MODE_QUOTED_STRING)
				        {
                            string sourceString = new string(data, firstLetterIdx, currLetterIdx - firstLetterIdx);
							string processedString = KV_ReadProcessString(sourceString);

							tokenFunc(new TokenInfo(curLine, data, currLetterIdx), "t", new object[] {processedString});
						}
						break;
			        }
		        }

		        if (mode == EParserMode.MODE_PARSE_ERROR_BREAK)
			        break;
			}

			switch (mode)
			{
				case EParserMode.MODE_SKIP_COMMENT_MULTILINE:
					Console.Error.WriteLine($"'{fileName}' ({lastModeStartLine}): EOF unexpected (missing '*/')");
					break;
				case EParserMode.MODE_QUOTED_STRING:
					Console.Error.WriteLine($"'{fileName}' ({lastModeStartLine}): EOF unexpected (missing '\"')");
                    break;
			}

			return (mode == EParserMode.MODE_DEFAULT);
		}

	}
}
