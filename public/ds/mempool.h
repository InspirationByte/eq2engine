#pragma once

// based on https://github.com/moya-lang/Allocator

template <typename T, int CHUNK_ITEMS = 1024>
class MemoryPool
{
public:
	MemoryPool(const MemoryPool& memoryPool) = delete;
	MemoryPool operator =(const MemoryPool& memoryPool) = delete;

	MemoryPool(PPSourceLine sl)
		: m_sl(sl)
	{
	}

	MemoryPool(MemoryPool&& other) noexcept
		: m_sl(other.m_sl)
		, m_firstFreeBlock(other.m_firstFreeBlock)
		, m_firstBuffer(other.m_firstBuffer)
		, m_bufferedBlocks(other.m_bufferedBlocks)
	{
		other.m_firstFreeBlock = nullptr;
		other.m_firstBuffer = nullptr;
	}

	MemoryPool& operator=(MemoryPool&& other) noexcept
	{
		m_sl = other.m_sl;
		m_firstFreeBlock = other.m_firstFreeBlock;
		m_firstBuffer = other.m_firstBuffer;
		m_bufferedBlocks = other.m_bufferedBlocks;
		other.m_firstFreeBlock = nullptr;
		other.m_firstBuffer = nullptr;
		return *this;
	}

	virtual ~MemoryPool()
	{
		Buffer* buffer = m_firstBuffer;
		while (buffer)
		{
			Buffer* del = buffer;
			buffer = buffer->next;
			delete del;
		}
	}

	void clear()
	{
		Buffer* buffer = m_firstBuffer;
		while (buffer)
		{
			Buffer* del = buffer;
			buffer = buffer->next;
			delete del;
		}
		m_firstFreeBlock = nullptr;
		m_firstBuffer = nullptr;
		m_bufferedBlocks = CHUNK_ITEMS;
	}

	T* allocate()
	{
		if (m_firstFreeBlock)
		{
			Block* block = m_firstFreeBlock;
			m_firstFreeBlock = block->next;
			return reinterpret_cast<T*>(block);
		}

		if (m_bufferedBlocks >= CHUNK_ITEMS)
		{
			m_firstBuffer = PPNewSL(m_sl) Buffer(m_firstBuffer);
			m_bufferedBlocks = 0;
		}

		T* ptr = m_firstBuffer->getBlock(m_bufferedBlocks++);
		return ptr;
	}

	void deallocate(T* pointer)
	{
		Block* block = reinterpret_cast<Block*>(pointer);
		block->next = m_firstFreeBlock;
		m_firstFreeBlock = block;
	}

	const PPSourceLine getSL() const { return m_sl; }

private:
	// internal structure to link free item data
	struct Block
	{
		Block*			next{ nullptr };
	};

	static constexpr size_t BLOCK_SIZE = max(sizeof(T), sizeof(Block));

	struct Buffer
	{
		uint8			data[BLOCK_SIZE * CHUNK_ITEMS];
		Buffer* const	next{ nullptr };

		Buffer(Buffer* next) : next(next) {}
		T*				getBlock(int index) { return reinterpret_cast<T*>(&data[BLOCK_SIZE * index]); }
	};

	PPSourceLine 	m_sl;
	Block*			m_firstFreeBlock{ nullptr };
	Buffer*			m_firstBuffer{ nullptr };
	int				m_bufferedBlocks{ CHUNK_ITEMS };
};


// std-compatible allocator
template <class T, int CHUNK_ITEMS = 1024>
class PoolAllocator : private MemoryPool<T, CHUNK_ITEMS>
{
public:
	using size_type = size_t;
	using difference_type = ptrdiff_t ;
	using pointer = T*;
	using const_pointer = const T* ;
	using reference = T& ;
	using const_reference = const T& ;
	using value_type = T;

	template <class U>
	struct rebind
	{
		typedef PoolAllocator<U, CHUNK_ITEMS> other;
	};

	T* allocate(size_t n, const void* hint = 0)
	{
		if (n != 1 || hint)
			return nullptr;

		return MemoryPool<T, CHUNK_ITEMS>::allocate();
	}

	void deallocate(T* p, size_t n)
	{
		MemoryPool<T, CHUNK_ITEMS>::deallocate(p);
	}

	void construct(T* p, const T& val)
	{
		new (p) T(val);
	}

	void destroy(T* p)
	{
		p->~T();
	}
};