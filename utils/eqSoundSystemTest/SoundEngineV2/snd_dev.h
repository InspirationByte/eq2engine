//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: Eq Sound Engine Device Interface
//////////////////////////////////////////////////////////////////////////////////

#ifndef SND_DEVICE_H
#define SND_DEVICE_H

#include "soundinterface.h"

//--------------------------------------------------------

typedef struct buffer_info_s
{
    int     channels;
    int     bitwidth;
    int     frequency;

    int     read, write;
    int     size;
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
    static void					Destroy(ISoundDevice *pDevice);

    virtual void				Destroy() = 0;

    virtual ESoundDeviceState	GetState() = 0;
    virtual buffer_info_t		GetBufferInfo() = 0;

    virtual void				WriteToBuffer(ubyte *pAudioData, int nBytes) = 0;
};

#endif //SND_DEVICE_H
