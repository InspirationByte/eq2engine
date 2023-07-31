//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Token parser
//////////////////////////////////////////////////////////////////////////////////

#pragma once

class Tokenizer
{
public:
	static bool isWhiteSpace(const char ch);
	static bool isNumeric(const char ch);
	static bool isAlphabetical(const char ch);
	static bool isNewLine(const char ch);
	static bool isNumericSpecial(const char ch);

	typedef bool (*BOOLFUNC)(const char ch);

	Tokenizer(int nBuffers = 1);
	~Tokenizer();

	void	setString(const char* string);
	bool	setFile(const char* fileName);
	void	reset();

	bool	goToNext(BOOLFUNC isAlpha = isAlphabetical);
	bool	goToNextLine();
	char*	next(BOOLFUNC isAlpha = isAlphabetical);
	char*	nextAfterToken(const char* token, BOOLFUNC isAlpha = isAlphabetical);
	char*	nextLine();

	//uint	getCurLine();
private:
	char*	str{ nullptr };
	int		length{ 0 };
	int		start{ 0 };
	int		end{ 0 };
	int		capacity{ 0 };

	int currentBuffer{ 0 };

	struct TokBuffer
	{
		char* buffer{ nullptr };
		int bufferSize{ 0 };
	};

	Array<TokBuffer> buffers{ PP_SL };

	char* getBuffer(int size);
};