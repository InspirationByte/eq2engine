//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech Engine Sound system
//
//				Sound sample loader
//////////////////////////////////////////////////////////////////////////////////

#ifndef ALSND_SAMPLE_H
#define ALSND_SAMPLE_H

#include "utils/eqthread.h"
#include "utils/DkList.h"

using namespace Threading;

//
// Threaded sound loader
//

class DkSoundSampleLocal : public ISoundSample
{
	friend class DkSoundAmbient;
	friend class DkSoundEmitterLocal;
public:
					DkSoundSampleLocal();
					~DkSoundSampleLocal();

	static void		SampleLoaderJob( void* loadSampleData );

	void			Init(const char *name, bool streaming, bool looping, int nFlags = 0);

	bool			Load();
	bool			LoadWav(const char *name, unsigned int buffer);
	bool			LoadOgg(const char *name, unsigned int buffer);

	// flags
	int				GetFlags();

	const char*		GetName() const {return m_szName.c_str();}

	void			WaitForLoad();

private:
	int				m_nChannels;
	bool			m_bStreaming;
	bool			m_bLooping;
	EqString		m_szName;
	unsigned int	m_nALBuffer;
	int				m_nFlags;

	volatile int	m_loadState;
};

//----------------------------------------------------------------------------------------------------------

#endif // ALSND_SAMPLE_H
