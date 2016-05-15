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

	void			Init(const char *name, int flags);

	bool			Load();
	bool			LoadWav(const char *name, unsigned int buffer);
	bool			LoadOgg(const char *name, unsigned int buffer);

	const char*		GetName() const {return m_szName.c_str();}

	void			WaitForLoad();

	int				GetFlags() const {return m_flags;}

private:
	EqString		m_szName;
	unsigned int	m_alBuffer;
	
	volatile int	m_loadState;

	uint8			m_nChannels;
	uint8			m_flags;

	int				m_loopStart;
	int				m_loopEnd;
};

//----------------------------------------------------------------------------------------------------------

#endif // ALSND_SAMPLE_H
