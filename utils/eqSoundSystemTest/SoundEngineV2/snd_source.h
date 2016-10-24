//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: Streamed WAVe source
//////////////////////////////////////////////////////////////////////////////////

#ifndef SND_FILES
#define SND_FILES

#include "soundinterface.h"

typedef struct soundFormat_s
{
    int     format;
    int     channels;
    int     bitwidth;
    int     frequency;
} soundFormat_t;

class ISoundSource
{
public:
    static ISoundSource*	CreateSound(const char *szFilename);
    static void				DestroySound(ISoundSource *pSound);

    virtual int             GetSamples(ubyte *pOutput, int nSamples, int nOffset, bool bLooping) = 0;
    virtual soundFormat_t*	GetFormat() = 0;
    virtual const char*		GetFilename() const = 0;

    virtual float           GetLoopPosition(float flPosition) = 0;

private:
    virtual bool			Load(const char *szFilename) = 0;
    virtual void			Unload () = 0;
};

#endif // SND_FILES