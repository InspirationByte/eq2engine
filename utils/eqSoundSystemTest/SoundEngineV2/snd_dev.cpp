//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Eq sound engine channel DMA
//////////////////////////////////////////////////////////////////////////////////

#include "snd_dev.h"
#include "snd_dev_dsound.h"

#include "DebugInterface.h"

#pragma warning(disable:4310)

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

void ISoundDevice::MixStereo16(samplepair_t *pInput, stereo16_t *pOutput, int nSamples, int nVolume)
{
	int val;

	for(int i = 0; i < nSamples; i++)
	{
		val = (pInput[i].left * nVolume) >> 8;

		if ( val > 0x7fff )
			pOutput[i].left = 0x7fff;
		else if ( val < (short)0x8000 )
			pOutput[i].left = (short)0x8000;
		else
			pOutput[i].left = (short)val;

		val = (pInput[i].right * nVolume) >> 8;

		if ( val > 0x7fff )
			pOutput[i].right = 0x7fff;
		else if ( val < (short)0x8000 )
			pOutput[i].right = (short)0x8000;
		else
			pOutput[i].right = (short)val;
	}
}