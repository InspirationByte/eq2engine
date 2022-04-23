//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Token parser
//////////////////////////////////////////////////////////////////////////////////

#ifndef TOKENIZER_H
#define TOKENIZER_H

#include "core/platform/Platform.h"
#include "ds/Array.h"

typedef bool (*BOOLFUNC)(const char ch);

bool isWhiteSpace(const char ch);
bool isNumeric(const char ch);
bool isAlphabetical(const char ch);
bool isNewLine(const char ch);

struct TokBuffer {
	char *buffer;
	unsigned int bufferSize;
};

class Tokenizer {
public:
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

	//uint	curline;

	int currentBuffer;
	Array <TokBuffer> buffers;

	char *getBuffer(unsigned int size);
};

#endif // TOKENIZER_H
