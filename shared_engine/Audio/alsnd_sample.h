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
public:
					DkSoundSampleLocal();
					~DkSoundSampleLocal();

	void			Load(const char *name, bool streaming, bool looping, int nFlags = 0);
	void			LoadWav(const char *name, unsigned int buffer);
	void			LoadOgg(const char *name, unsigned int buffer);

	// flags
	int				GetFlags();

	int				m_nChannels;
	bool			m_bStreaming;
	bool			m_bLooping;
	EqString		m_szName;
	unsigned int	m_nALBuffer;
	int				m_nFlags;
	bool			m_bIsLoaded;
};


//----------------------------------------------------------------------------------------------------------

struct loadsample_t
{
	DkSoundSampleLocal*	sample;

	EqString			filename;
	bool				loop;
	bool				stream;
	int					flags;
};

class CEqSoundThreadedLoader : public CEqThread
{
public:
	virtual int Run()
	{
		for(int i = 0; i < m_pSamples.numElem(); i++)
		{
			m_pSamples[i]->sample->Load( m_pSamples[i]->filename.GetData(),
											m_pSamples[i]->stream,
											m_pSamples[i]->loop,
											m_pSamples[i]->flags);

			delete m_pSamples[i];
		}

		m_Mutex.Lock();
		m_pSamples.clear();
		m_Mutex.Unlock();

		return 0;
	}

	void AddSample(loadsample_t* pSample)
	{
		m_Mutex.Lock();
		m_pSamples.append( pSample );
		m_Mutex.Unlock();
	}

protected:

	DkList<loadsample_t*>	m_pSamples;
	CEqMutex				m_Mutex;
};

extern CEqSoundThreadedLoader* g_pThreadedSoundLoader;

#endif // ALSND_SAMPLE_H
