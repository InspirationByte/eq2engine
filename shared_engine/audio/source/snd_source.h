//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: WAVe source
//////////////////////////////////////////////////////////////////////////////////

#ifndef SND_SOURCE_H
#define SND_SOURCE_H

#include "core/dktypes.h"

typedef struct soundFormat_s
{
	int format;
	int channels;
	int bitwidth;
	int frequency;
} soundFormat_t;

class ISoundSource
{
public:
	virtual ~ISoundSource() {}

	static ISoundSource*	CreateSound(const char *szFilename);
	static void				DestroySound(ISoundSource *pSound);

//----------------------------------------------------

	virtual int             GetSamples(ubyte *pOutput, int nSamples, int nOffset, bool bLooping) = 0;
	virtual ubyte*			GetDataPtr(int& dataSize) const = 0;

	virtual soundFormat_t*	GetFormat() const = 0;
	virtual const char*		GetFilename() const = 0;
	virtual int				GetSampleCount() const = 0;

	virtual float           GetLoopPosition(float flPosition) const = 0;

	virtual bool			IsStreaming() const = 0;
private:
	virtual bool			Load(const char *szFilename) = 0;
	virtual void			Unload () = 0;
};

#endif // SND_SOURCE_H