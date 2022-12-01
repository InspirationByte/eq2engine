//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: WAVe source
//////////////////////////////////////////////////////////////////////////////////

#pragma once

constexpr const int SOUND_SOURCE_MAX_LOOP_REGIONS = 2;

class ISoundSource
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

	static ISoundSource*	CreateSound(const char *szFilename);
	static void				DestroySound(ISoundSource *pSound);

//----------------------------------------------------

	virtual int             GetSamples(void* out, int samplesToRead, int startOffset, bool loop) const = 0;
	virtual void*			GetDataPtr(int& dataSize) const = 0;

	virtual const Format&	GetFormat() const = 0;
	virtual const char*		GetFilename() const = 0;
	virtual int				GetSampleCount() const = 0;

	virtual int				GetLoopRegions(int* samplePos) const = 0;

	virtual bool			IsStreaming() const = 0;
private:
	virtual bool			Load(const char *szFilename) = 0;
	virtual void			Unload () = 0;
};
