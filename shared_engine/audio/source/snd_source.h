//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: WAVe source
//////////////////////////////////////////////////////////////////////////////////

#pragma once

constexpr const int SOUND_SOURCE_MAX_LOOP_REGIONS = 2;

class ISoundSource : public RefCountedObject<ISoundSource, RefCountedKeepPolicy>
{
public:
	enum EFormatType
	{
		FORMAT_INVALID = 0,
		FORMAT_PCM = 1
	};

	struct Format
	{
		int dataFormat;
		int channels;
		int bitwidth;
		int frequency;
	};

	virtual ~ISoundSource() {}

	static CRefPtr<ISoundSource>	CreateSound(const char *szFilename);

//----------------------------------------------------

	void					Ref_DeleteObject();

	int						GetNameHash() const { return m_nameHash; }
	const char*				GetFilename() const;
	void					SetFilename(const char* filename);

	virtual int             GetSamples(void* out, int samplesToRead, int startOffset, bool loop) const = 0;
	virtual void*			GetDataPtr(int& dataSize) const = 0;

	virtual const Format&	GetFormat() const = 0;
	virtual int				GetSampleCount() const = 0;

	virtual int				GetLoopRegions(int* samplePos) const = 0;

	virtual bool			IsStreaming() const = 0;
private:
	virtual bool			Load() = 0;
	virtual void			Unload() = 0;

	EqString				m_fileName;
	int						m_nameHash{ 0 };
};
