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

	size_t	Read(void* buffer, size_t count);
	size_t	Write(const void* buffer, size_t count);
	size_t	Seek(size_t offset, ESeekPos pos);
	size_t	Tell() const;

	bool	Flush();

private:
	void*	m_fp;
	int		m_flags{ 0 };

	COSFile(const COSFile&) = delete;
	COSFile& operator=(const COSFile&) = delete;
};