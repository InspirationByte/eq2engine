//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Token parser
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IFileSystem.h"
#include "Tokenizer.h"

bool Tokenizer::isWhiteSpace(const char ch)
{
	return (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n');
}

bool Tokenizer::isNumeric(const char ch)
{
	return (ch >= '0' && ch <= '9');
}

bool Tokenizer::isNumericSpecial(const char ch)
{
	return (ch >= '0' && ch <= '9') || ch == '-' || ch == '+' || ch == 'e' || ch == '.';
}

bool Tokenizer::isAlphabetical(const char ch)
{
	return ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_');
}

bool Tokenizer::isNewLine(const char ch)
{
	return (ch == '\r' || ch == '\n');
}


Tokenizer::Tokenizer(int nBuffers)
{
	buffers.setNum(nBuffers);
	reset();
}

Tokenizer::~Tokenizer()
{
	for (int i = 0; i < buffers.numElem(); i++)
	{
		if (buffers[i].buffer)
			delete[] buffers[i].buffer;
	}
	delete[] str;
}

void Tokenizer::setString(const char* string)
{
	length = strlen(string);

	// Increase capacity if necessary
	if (length >= capacity) {
		delete[] str;

		capacity = length + 1;
		str = PPNew char[capacity];
	}

	currentBuffer = 0;

	strcpy(str, string);

	reset();
}

bool Tokenizer::setFile(const char* fileName)
{
	delete[] str;
	str = nullptr;

	IFilePtr file = g_fileSystem->Open(fileName, "rb");
	if (file)
	{
		length = file->GetSize();

		str = PPNew char[(length + 1) * sizeof(char)];
		file->Read(str, length, 1);
		str[length] = '\0';

		reset();
		return true;
	}

	currentBuffer = 0;

	return false;
}

void Tokenizer::reset()
{
	end = 0;
}

bool Tokenizer::goToNext(BOOLFUNC isAlpha)
{
	start = end;

	while (start < length && isWhiteSpace(str[start]))
		start++;

	end = start + 1;

	if (start < length)
	{
		if (isNumeric(str[start]))
		{
			while (isNumericSpecial(str[end]))
				end++;
		}
		else if (isAlpha(str[start]))
		{
			while (isAlpha(str[end]) || isNumeric(str[end]))
				end++;
		}
		return true;
	}
	return false;
}

bool Tokenizer::goToNextLine()
{
	if (end < length)
	{
		start = end;

		while (end < length - 1 && !isNewLine(str[end]))
			end++;

		if (isNewLine(str[end + 1]) && str[end] != str[end + 1])
			end += 2;
		else
			end++;

		return true;
	}

	return false;
}


char* Tokenizer::next(BOOLFUNC isAlpha)
{
	if (goToNext(isAlpha))
	{
		int size = end - start;
		char* buffer = getBuffer(size + 1);
		strncpy(buffer, str + start, size);
		buffer[size] = '\0';
		return buffer;
	}
	return nullptr;
}

char* Tokenizer::nextAfterToken(const char* token, BOOLFUNC isAlpha)
{
	while (goToNext(isAlpha))
	{
		if (strncmp(str + start, token, end - start) == 0)
		{
			return next();
		}
	}

	return nullptr;
}

char* Tokenizer::nextLine()
{
	if (goToNextLine())
	{
		int size = end - start;
		char* buffer = getBuffer(size + 1);
		strncpy(buffer, str + start, size);
		buffer[size] = '\0';
		return buffer;
	}
	return nullptr;
}

char* Tokenizer::getBuffer(int size)
{
	currentBuffer++;
	if (currentBuffer >= buffers.numElem())
		currentBuffer = 0;

	if (size > buffers[currentBuffer].bufferSize)
	{
		delete buffers[currentBuffer].buffer;
		buffers[currentBuffer].buffer = PPNew char[buffers[currentBuffer].bufferSize = size];
	}

	return buffers[currentBuffer].buffer;
}