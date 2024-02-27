//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Dynamic Bit array
//////////////////////////////////////////////////////////////////////////////////

#pragma once

template <int p>
constexpr int getIntExp(int _x = 0) { return getIntExp<p / 2>(_x + 1); }

template <>
constexpr int getIntExp<0>(int _x) { return _x - 1; }

static constexpr int numBitsSet(uint x)
{
	x = x - ((x >> 1) & 0x55555555);
	x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
	x = (x + (x >> 4)) & 0x0F0F0F0F;
	x = x + (x >> 8);
	x = x + (x >> 16);
	return x & 0x0000003F;
}

class BitArray
{
	using STORAGE_TYPE = int;
public:
	BitArray(PPSourceLine sl, int bitCount = 64);
	BitArray(STORAGE_TYPE* storage, int bitCount);
	~BitArray();

	const bool				operator[](int index) const;
	BitArray&				operator=(const BitArray& other);

	// cleans all bits to zero
	void					clear();

	// resizes the list
	void					resize(int newBitCount);

	// returns total number of bits
	int						numBits() const;

	// returns total number of bits that are set to true
	int						numTrue() const;

	// returns total number of bits that are set to false
	int						numFalse() const;

	// set the bit value
	void					set(int index, bool value);

	// set bit range
	void					setRange(int startIndex, int count, bool value);

	void					setTrue(int index);
	void					setFalse(int index);

	STORAGE_TYPE*			ptr() { return m_pListPtr; }
	const STORAGE_TYPE*		ptr() const { return m_pListPtr; }

private:
	STORAGE_TYPE&			getV(int);
	const STORAGE_TYPE&		getV(int) const;

	STORAGE_TYPE*			m_pListPtr{ nullptr };
	int						m_nSize{ 0 };
	const PPSourceLine		m_sl;
	bool					m_ownData{ false };
};

inline BitArray::BitArray(PPSourceLine sl, int bitCount)
	: m_sl(sl)
{
	m_ownData = true;
	resize(bitCount);
}

inline BitArray::BitArray(STORAGE_TYPE* storage, int bitCount)
	: m_pListPtr(storage), m_nSize(bitCount)
{
}

inline BitArray::~BitArray()
{
	if(m_ownData)
		delete[] m_pListPtr;
}

inline BitArray& BitArray::operator=(const BitArray& other)
{
	resize(other.m_nSize);

	const int typeSize = m_nSize / sizeof(STORAGE_TYPE) + 1;
	if (typeSize)
	{
		for (int i = 0; i < typeSize; i++)
			m_pListPtr[i] = other.m_pListPtr[i];
	}

	return *this;
}

// cleans all bits to zero
inline void BitArray::clear()
{
	memset(m_pListPtr, 0, m_nSize / sizeof(STORAGE_TYPE) + 1);
}

// resizes the list
inline void BitArray::resize(int newBitCount)
{
	// not changing the elemCount, so just exit
	if (newBitCount == m_nSize)
		return;

	if(!m_ownData)
	{
		ASSERT_FAIL("BitArray is not resizable");
	}

	const int oldTypeSize = m_nSize / sizeof(STORAGE_TYPE) + 1;
	const int newTypeSize = newBitCount / sizeof(STORAGE_TYPE) + 1;

	STORAGE_TYPE* temp = m_pListPtr;

	// copy the old m_pListPtr into our new one
	m_pListPtr = PPNewSL(m_sl) STORAGE_TYPE[newTypeSize];
	m_nSize = newBitCount;

	if (temp)
	{
		for (int i = 0; i < oldTypeSize; i++)
			m_pListPtr[i] = temp[i];
		delete[] temp;
	}

	for (int i = max(oldTypeSize-1, 0); i < newTypeSize; ++i)
		m_pListPtr[i] = 0;
}

// returns number of bits
inline int BitArray::numBits() const
{
	return m_nSize;
}

// returns total number of bits that are set to true
inline int BitArray::numTrue() const
{
	int count = 0;
	const int typeSize = m_nSize / sizeof(STORAGE_TYPE) + 1;
	if (typeSize)
	{
		for (int i = 0; i < typeSize; i++)
			count += numBitsSet(static_cast<uint>(m_pListPtr[i]));
	}
	return count;
}

// returns total number of bits that are set to false
inline int BitArray::numFalse() const
{
	int count = 0;
	const int typeSize = m_nSize / sizeof(STORAGE_TYPE) + 1;
	if (typeSize)
	{
		for (int i = 0; i < typeSize; i++)
			count += numBitsSet(~static_cast<uint>(m_pListPtr[i]));
	}
	return count;
}

// set the bit value
inline void BitArray::set(int index, bool value)
{
	if (value) 
		setTrue(index);
	else 
		setFalse(index);
}

// set bit range
inline void BitArray::setRange(int startIndex, int count, bool value)
{
	const int endIndex = min(m_nSize, startIndex + count);
	for (int index = startIndex; index < endIndex; ++index)
		set(index, value);
}

inline BitArray::STORAGE_TYPE& BitArray::getV(int index)
{
	constexpr const int shiftVal = getIntExp<sizeof(STORAGE_TYPE) * 8>();
	return m_pListPtr[index >> shiftVal];
}

inline const BitArray::STORAGE_TYPE& BitArray::getV(int index) const
{
	constexpr const int shiftVal = getIntExp<sizeof(STORAGE_TYPE) * 8>();
	return m_pListPtr[index >> shiftVal];
}

inline void BitArray::setTrue(int index)
{
	const int bitIndex = index & (sizeof(STORAGE_TYPE) * 8 - 1);
	getV(index) |= (1 << bitIndex);
}

inline void BitArray::setFalse(int index)
{
	const int bitIndex = index & (sizeof(STORAGE_TYPE) * 8 - 1);
	getV(index) &= ~(1 << bitIndex);
}

inline const bool BitArray::operator[](int index) const
{
	const int bitIndex = index & (sizeof(STORAGE_TYPE) * 8 - 1);
	const STORAGE_TYPE set = getV(index);
	return getV(index) & (1 << bitIndex);
}