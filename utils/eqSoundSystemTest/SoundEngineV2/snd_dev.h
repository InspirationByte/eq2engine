//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Eq Sound Engine Device Interface
//////////////////////////////////////////////////////////////////////////////////

#ifndef SND_DEVICE_H
#define SND_DEVICE_H

#include "soundinterface.h"
#include "snd_defs.h"

//--------------------------------------------------------

typedef struct buffer_info_s
{
	int channels;
	int bitwidth;
	int frequency;

	int read, write;
	int size;
} buffer_info_t;

enum ESoundDeviceState
{
	SNDDEV_READY = 0,
	SNDDEV_FAIL,
	SNDDEV_ABORT
};

class ISoundDevice
{
public:
	static ISoundDevice*		Create(void* winhandle);
	static void					Destroy(ISoundDevice* device);

	virtual void				Destroy() = 0;

	virtual ESoundDeviceState	GetState() = 0;
	virtual void				GetBufferInfo( buffer_info_t& outInfo ) = 0;

	virtual void				WriteToBuffer( ubyte* sampleData, int sizeInBytes ) = 0;

protected:
	void						MixStereo16(samplepair_t *pInput, stereo16_t *pOutput, int nSamples, int nVolume);
};

#endif //SND_DEVICE_H
