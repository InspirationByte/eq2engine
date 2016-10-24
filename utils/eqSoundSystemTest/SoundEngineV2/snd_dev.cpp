//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: Eq sound engine channel DMA
//////////////////////////////////////////////////////////////////////////////////

#include "snd_dev.h"

#include "snd_dev_dsound.h"

#include "DebugInterface.h"

//---------------------------------------------------------------

ISoundDevice* ISoundDevice::Create(void* winhandle)
{
    ISoundDevice* pDevice;
    ESoundDeviceState  devState = SNDDEV_FAIL;

    MsgInfo( "\n------ Initializing Sound ------\n" );

#ifdef _WIN32
	pDevice = new CDirectSoundDevice((HWND)winhandle);
#else
	// do others on other platforms
#endif //

    //  try directsound
    if(pDevice)
    {
        if ( (devState = pDevice->GetState()) == SNDDEV_READY )
            return pDevice;

        pDevice->Destroy( );
        delete pDevice;
    }

    return NULL;
}

//---------------------------------------------------------------

void ISoundDevice::Destroy(ISoundDevice *pDevice)
{
    MsgInfo( "------ shutting down sound ------" );

    if ( pDevice )
    {
        pDevice->Destroy();
        delete pDevice;
    }

    return;
}
