#pragma once

class COSFile
{
public:
	enum EFlags
	{
		READ		= (1 << 0),
		WRITE		= (1 << 1),
		APPEND		= (1 << 2),
		OPEN_EXIST	= (1 << 3),

		WITH_OFFSET	= (1 << 4),
	};

	enum class ESeekPos
	{
		SET,
		CURRENT,
		END,
	};

	COSFile();
	COSFile(COSFile&& r) noexcept;
	~COSFile();

	bool	Open(const char* fileName, int modeFlags = READ);
	void	Close();
	bool	IsOpen() const;

	int64	Read(void* buffer, int64 count);
	int64	ReadWithOffset(void* buffer, int64 count, int64 offset);
	int64	Write(const void* buffer, int64 count);
	int64	Seek(int64 offset, ESeekPos pos);
	int64	Tell() const;

	bool	Flush();

private:
	void*	m_fp;
	int		m_flags{ 0 };

	COSFile(const COSFile&) = delete;
	COSFile& operator=(const COSFile&) = delete;
};