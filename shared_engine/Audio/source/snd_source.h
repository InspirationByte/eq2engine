//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: WAVe source
//////////////////////////////////////////////////////////////////////////////////

#ifndef SND_SOURCE_H
#define SND_SOURCE_H

#include "dktypes.h"

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
	static ISoundSource*	CreateSound(const char *szFilename);
	static void				DestroySound(ISoundSource *pSound);

//----------------------------------------------------

	virtual int             GetSamples(ubyte *pOutput, int nSamples, int nOffset, bool bLooping) = 0;
	virtual soundFormat_t*	GetFormat() = 0;
	virtual const char*		GetFilename() const = 0;

	virtual float           GetLoopPosition(float flPosition) = 0;

	virtual bool			IsStreaming() = 0;

private:
	virtual bool			Load(const char *szFilename) = 0;
	virtual void			Unload () = 0;
};

#endif // SND_SOURCE_H