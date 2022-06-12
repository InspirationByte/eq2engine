//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Token parser
//////////////////////////////////////////////////////////////////////////////////

#pragma once

bool isWhiteSpace(const char ch);
bool isNumeric(const char ch);
bool isAlphabetical(const char ch);
bool isNewLine(const char ch);

class Tokenizer 
{
public:
	typedef bool (*BOOLFUNC)(const char ch);

	Tokenizer(unsigned int nBuffers = 1);
	~Tokenizer();

	void	setString(const char *string);
	bool	setFile(const char *fileName);
	void	reset();

	bool	goToNext(BOOLFUNC isAlpha = isAlphabetical);
	bool	goToNextLine();
	char*	next(BOOLFUNC isAlpha = isAlphabetical);
	char*	nextAfterToken(const char *token, BOOLFUNC isAlpha = isAlphabetical);
	char*	nextLine();

	//uint	getCurLine();
private:
	char*	str;
	uint	length;
	uint	start, end;
	uint	capacity;

	int currentBuffer;

	struct TokBuffer
	{
		char* buffer;
		unsigned int bufferSize;
	};

	Array<TokBuffer> buffers{ PP_SL };

	char *getBuffer(unsigned int size);
};