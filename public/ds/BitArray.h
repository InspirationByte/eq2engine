//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Dynamic Bit array
//////////////////////////////////////////////////////////////////////////////////

#pragma once
using BIT_STORAGE_TYPE = uint;

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

#ifdef __GNUC__
#define leadingZeroCnt(x)	__builtin_clz(x)
#define trailingZeroCnt(x)	__builtin_ctz(x)
#else
#pragma warning(push)
#pragma warning(disable:4146)

inline uint leadingZeroCnt(uint x)
{
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	return 32 - numBitsSet(x);
}

static uint trailingZeroCnt(uint x)
{
	return numBitsSet((x & -x) - 1u);
}

#pragma warning(pop)

#endif

static inline void bitsSet(int& value, int mask, bool on)		{ value = (value & ~mask) | (static_cast<int>(on) * mask); }
static inline void bitsSet(uint& value, uint mask, bool on)		{ value = (value & ~mask) | (static_cast<uint>(on) * mask); }

template <int bitStorageSize = sizeof(BIT_STORAGE_TYPE)>
constexpr int bitArray2Dword(const int numBits)
{
	constexpr int storageBits = bitStorageSize * 8;
	return (numBits + storageBits - 1) / storageBits;
}

template <int bitStorageSize = sizeof(BIT_STORAGE_TYPE)>
constexpr int bitArrayShift()
{
	return getIntExp<bitStorageSize * 8>();
}

template <int bitStorageSize = sizeof(BIT_STORAGE_TYPE)>
constexpr int bitArrayRemainder()
{
	return (bitStorageSize * 8 - 1);
}

class BitArrayImpl
{
public:
	using STORAGE_TYPE = BIT_STORAGE_TYPE;

	// cleans all bits to zero
	static void		clear(STORAGE_TYPE* bitArray, int bitCount) { reset(bitArray, bitCount, false); }

	// resets all bits to specified value
	static void		reset(STORAGE_TYPE* bitArray, int bitCount, bool value = false);

	static bool		isTrue(const STORAGE_TYPE* bitArray, int bitCount, int index);

	// returns total number of bits that are set to true
	static int		numTrue(const STORAGE_TYPE* bitArray, int bitCount);

	// returns total number of bits that are set to false
	static int		numFalse(const STORAGE_TYPE* bitArray, int bitCount);

	// set the bit value
	static void		set(STORAGE_TYPE* bitArray, int bitCount, int index, bool value);

	// set bit range
	static void		setRange(STORAGE_TYPE* bitArray, int bitCount, int startIndex, int count, bool value);

	static void		setTrue(STORAGE_TYPE* bitArray, int bitCount, int index);
	static void		setFalse(STORAGE_TYPE* bitArray, int bitCount, int index);
};

inline void	BitArrayImpl::reset(STORAGE_TYPE* bitArray, int bitCount, bool value)
{
	const int listSize = bitArray2Dword(bitCount);
	for(int i = 0; i < listSize; ++i)
		bitArray[i] = value ? COM_UINT_MAX : 0;
}

// returns total number of bits that are set to true
inline int BitArrayImpl::numTrue(const STORAGE_TYPE* bitArray, int bitCount)
{
	if (!bitCount)
		return 0;

	int count = 0;
	const int typeSize = bitArray2Dword(bitCount);

	constexpr int storageMask = sizeof(STORAGE_TYPE) * 8 - 1;
	const int remainder = bitCount & storageMask;
	if (remainder)
	{
		for (int i = 0; i < typeSize - 1; i++)
			count += numBitsSet(static_cast<uint>(bitArray[i]));

		const STORAGE_TYPE lastChunkMask = (static_cast<STORAGE_TYPE>(1) << remainder) - 1;
		count += numBitsSet(static_cast<uint>(bitArray[typeSize-1] & lastChunkMask));
	}
	else
	{
		for (int i = 0; i < typeSize; i++)
			count += numBitsSet(static_cast<uint>(bitArray[i]));
	}

	return count;
}

// returns total number of bits that are set to false
inline int BitArrayImpl::numFalse(const STORAGE_TYPE* bitArray, int bitCount)
{
	if (!bitCount)
		return 0;

	int count = 0;
	const int typeSize = bitArray2Dword(bitCount);

	constexpr int storageMask = sizeof(STORAGE_TYPE) * 8 - 1;
	const int remainder = bitCount & storageMask;
	if (remainder)
	{
		for (int i = 0; i < typeSize - 1; i++)
			count += numBitsSet(static_cast<uint>(~bitArray[i]));

		const STORAGE_TYPE lastChunkMask = (static_cast<STORAGE_TYPE>(1) << remainder) - 1;
		count += numBitsSet(static_cast<uint>(~bitArray[typeSize - 1] & lastChunkMask));
	}
	else
	{
		for (int i = 0; i < typeSize; i++)
			count += numBitsSet(static_cast<uint>(~bitArray[i]));
	}

	return count;
}

// set the bit value
inline void BitArrayImpl::set(STORAGE_TYPE* bitArray, int bitCount, int index, bool value)
{
	if (value) 
		setTrue(bitArray, bitCount, index);
	else 
		setFalse(bitArray, bitCount, index);
}

// set bit range
inline void BitArrayImpl::setRange(STORAGE_TYPE* bitArray, int bitCount, int startIndex, int count, bool value)
{
	const int endIndex = min(bitCount, startIndex + count);
#if 0
	// slow - for reference
	for (int index = startIndex; index < endIndex; ++index)
		set(bitArray, bitCount, index, value);
#else
	if (count <= 0)
		return;

	const int startWordIndex = startIndex >> bitArrayShift();
	const int endWordIndex = (endIndex - 1) >> bitArrayShift();

	const int firstBitIdx = startIndex & bitArrayRemainder();
	const int lastBitIdx = endIndex & bitArrayRemainder();

	const STORAGE_TYPE startMask = ~((1 << firstBitIdx) - 1);
	const STORAGE_TYPE endMask = ((1 << lastBitIdx) - 1);
	if (startWordIndex == endWordIndex)
	{
		if(value)
			bitArray[startWordIndex] |= startMask & endMask;
		else
			bitArray[startWordIndex] &= ~(startMask & endMask);
		return;
	}

	if (value)
	{
		bitArray[startWordIndex] |= startMask;
		bitArray[endWordIndex-1] |= endMask;
		for (int i = startWordIndex+1; i < endWordIndex-1; ++i)
			bitArray[i] = 0xffffffff;
	}
	else
	{
		bitArray[startWordIndex] &= ~startMask;
		bitArray[endWordIndex-1] &= ~endMask;
		for (int i = startWordIndex+1; i < endWordIndex-1; ++i)
			bitArray[i] = 0;
	}
#endif
}

inline bool	BitArrayImpl::isTrue(const STORAGE_TYPE* bitArray, int bitCount, int index)
{
	const int bitIndex = index & bitArrayRemainder();
	return bitArray[index >> bitArrayShift()] & (1 << bitIndex);
}

inline void BitArrayImpl::setTrue(STORAGE_TYPE* bitArray, int bitCount, int index)
{
	const int bitIndex = index & bitArrayRemainder();
	bitArray[index >> bitArrayShift()] |= (1 << bitIndex);
}

inline void BitArrayImpl::setFalse(STORAGE_TYPE* bitArray, int bitCount, int index)
{
	const int bitIndex = index & bitArrayRemainder();
	bitArray[index >> bitArrayShift()] &= ~(1 << bitIndex);
}

//--------------------------------------------------------

class BitArray
{
public:
	using STORAGE_TYPE = BIT_STORAGE_TYPE;

	BitArray(PPSourceLine sl, int bitCount = 64);
	BitArray(STORAGE_TYPE* storage, int bitCount);
	~BitArray();

	const bool				operator[](int index) const;
	BitArray&				operator=(const BitArray& other);

	// cleans all bits to zero
	void					clear() { reset(false); }

	// resets all bits to specified value
	void					reset(bool value = false);

	// resizes the list
	void					resize(int newBitCount, bool newBitsValue = false);

	// returns total number of bits
	int						numBits() const { return m_nSize; }

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

	const int typeSize = bitArray2Dword(m_nSize);
	if (typeSize)
	{
		for (int i = 0; i < typeSize; i++)
			m_pListPtr[i] = other.m_pListPtr[i];
	}

	return *this;
}

// resets all bits to specified value
inline void BitArray::reset(bool value)
{
	const int listSize = bitArray2Dword(m_nSize);
	for(int i = 0; i < listSize; ++i)
		m_pListPtr[i] = value ? COM_UINT_MAX : 0;
}

// resizes the list
inline void BitArray::resize(int newBitCount, bool newBitsValue)
{
	// not changing the elemCount, so just exit
	if (newBitCount == m_nSize)
		return;

	if(!m_ownData)
	{
		ASSERT_FAIL("BitArray is not resizable");
		return;
	}

	ASSERT_MSG(newBitCount >= 0, "newBitCount (%d) must be positive", newBitCount);

	const int oldTypeSize = bitArray2Dword(m_nSize);
	const int newTypeSize = bitArray2Dword(newBitCount);
	if(oldTypeSize != newTypeSize)
	{
		if(newTypeSize)
		{
			STORAGE_TYPE* temp = m_pListPtr;
			m_pListPtr = PPNewSL(m_sl) STORAGE_TYPE[newTypeSize];
			if (temp)
			{
				for (int i = 0; i < min(newTypeSize, oldTypeSize); ++i)
					m_pListPtr[i] = temp[i];
				delete[] temp;
			}

			// new data must be reset to zero
			for (int i = oldTypeSize; i < newTypeSize; ++i)
				m_pListPtr[i] = newBitsValue ? COM_UINT_MAX : 0;
		}
		else
		{
			delete[] m_pListPtr;
			m_pListPtr = nullptr;
		}
	}
	m_nSize = newBitCount;
}

// returns total number of bits that are set to true
inline int BitArray::numTrue() const
{
	return BitArrayImpl::numTrue(m_pListPtr, m_nSize);
}

// returns total number of bits that are set to false
inline int BitArray::numFalse() const
{
	return BitArrayImpl::numFalse(m_pListPtr, m_nSize);
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
	BitArrayImpl::setRange(m_pListPtr, m_nSize, startIndex, count, true);
}

inline void BitArray::setTrue(int index)
{
	BitArrayImpl::setTrue(m_pListPtr, m_nSize, index);
}

inline void BitArray::setFalse(int index)
{
	BitArrayImpl::setFalse(m_pListPtr, m_nSize, index);
}

inline const bool BitArray::operator[](int index) const
{
	return BitArrayImpl::isTrue(m_pListPtr, m_nSize, index);
}