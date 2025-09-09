//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Token parser
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "ds/IFileStream.h"
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

Tokenizer::Tokenizer(int bufferCount)
{
	m_buffers.setNum(bufferCount);
	reset();
}

Tokenizer::~Tokenizer()
{
	for (Buffer& buffer : m_buffers)
		SAFE_DELETE_ARRAY(buffer.data);

	SAFE_DELETE_ARRAY(m_str);
}

void Tokenizer::setString(const char* string, int length)
{
	m_length = length == -1 ? CString::Length(string) : length;

	// Increase capacity if necessary
	if (m_length >= m_capacity)
	{
		delete[] m_str;

		m_capacity = m_length + 1;
		m_str = PPNew char[m_capacity];
	}

	m_currentBuffer = 0;

	strncpy(m_str, string, m_length);
	m_str[m_length] = 0;

	reset();
}

bool Tokenizer::setFile(IFileStreamPtr file)
{
	SAFE_DELETE_ARRAY(m_str);
	if (!file)
	{
		m_currentBuffer = 0;
		return false;
	}

	m_length = file->GetSize();
	m_str = PPNew char[m_length + 1];

	file->Read(m_str, m_length, 1);
	m_str[m_length] = 0;

	reset();
	return true;
}

void Tokenizer::reset()
{
	m_end = 0;
}

bool Tokenizer::goToNext(TestFunc isAlpha)
{
	m_start = m_end;

	while (m_start < m_length && isWhiteSpace(m_str[m_start]))
		m_start++;

	m_end = m_start + 1;

	if (m_start < m_length)
	{
		if (isNumeric(m_str[m_start]))
		{
			while (isNumericSpecial(m_str[m_end]))
				m_end++;
		}
		else if (isAlpha(m_str[m_start]))
		{
			while (isAlpha(m_str[m_end]) || isNumeric(m_str[m_end]))
				m_end++;
		}
		return true;
	}
	return false;
}

bool Tokenizer::goToNextLine()
{
	const int length = m_length;
	if (m_end < length)
	{
		const char* str = m_str;

		m_start = m_end;
		while (m_end < length && !isNewLine(str[m_end]))
			m_end++;

		if (isNewLine(str[m_end + 1]) && str[m_end] != str[m_end + 1])
			m_end += 2;
		else
			m_end++;

		return true;
	}
	return false;
}


char* Tokenizer::next(TestFunc isAlpha)
{
	if (!goToNext(isAlpha))
		return nullptr;

	const int size = m_end - m_start;

	char* buffer = getBuffer(size + 1);
	strncpy(buffer, m_str + m_start, size);
	buffer[size] = 0;

	return buffer;
}

char* Tokenizer::nextAfterToken(const char* token, TestFunc isAlpha)
{
	while (goToNext(isAlpha))
	{
		if (strncmp(m_str + m_start, token, m_end - m_start) == 0)
			return next();
	}

	return nullptr;
}

char* Tokenizer::nextLine()
{
	if (!goToNextLine())
		return nullptr;

	const int size = m_end - m_start;

	char* buffer = getBuffer(size + 1);
	strncpy(buffer, m_str + m_start, size);
	buffer[size] = 0;

	return buffer;
}

char* Tokenizer::getBuffer(int size)
{
	m_currentBuffer = (m_currentBuffer + 1) % m_buffers.numElem();

	Buffer& buf = m_buffers[m_currentBuffer];
	if (size > buf.size)
	{
		if(buf.data)
			delete [] buf.data;

		buf.data = PPNew char[buf.size = size];
	}
	return buf.data;
}