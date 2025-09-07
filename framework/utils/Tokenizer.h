//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Token parser
//////////////////////////////////////////////////////////////////////////////////

#pragma once

class IFileStream;
using IFileStreamPtr = CRefPtr<IFileStream>;

class Tokenizer
{
public:
	static bool isWhiteSpace(const char ch);
	static bool isNumeric(const char ch);
	static bool isAlphabetical(const char ch);
	static bool isNewLine(const char ch);
	static bool isNumericSpecial(const char ch);

	using TestFunc = bool(const char ch);

	Tokenizer(int bufferCount = 1);
	~Tokenizer();

	void			setString(const char* string);
	bool			setFile(IFileStreamPtr file);
	void			reset();

	bool			goToNext(TestFunc isAlpha = isAlphabetical);
	bool			goToNextLine();

	char*			next(TestFunc isAlpha = isAlphabetical);
	char*			nextAfterToken(const char* token, TestFunc isAlpha = isAlphabetical);
	char*			nextLine();

private:
	char*			getBuffer(int size);

	struct Buffer
	{
		char*	data{ nullptr };
		int		size{ 0 };
	};
	char*			m_str{ nullptr };

	Array<Buffer>	m_buffers{ PP_SL };
	int				m_currentBuffer{ 0 };

	int				m_length{ 0 };
	int				m_start{ 0 };
	int				m_end{ 0 };
	int				m_capacity{ 0 };
};