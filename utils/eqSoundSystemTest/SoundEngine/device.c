/* The MIT License (MIT)

Copyright (c) 2015-2016 Stepanov Dmitriy aka mrDIMAS

Permission is hereby granted, free of charge, to any person obtaining a copy of this 
software and associated documentation files (the "Software"), to deal in the Software 
without restriction, including without limitation the rights to use, copy, modify, merge, 
publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
 to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or 
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "sound.h"
#include "buffer.h"
#include "effect.h"
#include "memory.h"

#ifdef _WIN32
#   include <initguid.h>
#   include <windows.h>
#   include "dsound.h"
#else
#   include <alsa/asoundlib.h>
#endif

#define SAMPLE_16BIT_SHIFT 1

struct SOutputDevice {
    uint32_t bufferHalfSize;
#ifdef _WIN32
	MMTIME			mmstarttime;
    IDirectSound8 * dsound;
    IDirectSoundBuffer8 * buffer;
    IDirectSoundNotify * notify;
    HANDLE points[ 2 ];
#else
    snd_pcm_t * playbackDevice;
    int frameCount;
#endif

};

#include "device.h"

TOutputDevice * SW_OutputDevice_Create( uint32_t bufferHalfSize ) {
    TOutputDevice * dev = SW_Memory_New( TOutputDevice );
    dev->bufferHalfSize = bufferHalfSize;   

    /* platform specific code */
#ifdef _WIN32
    IDirectSoundBuffer * pDSB;
    
    if( FAILED( DirectSoundCreate8( 0, &dev->dsound, 0 )) ) {
        Log_Write( "Failed to create DirectSound8 device" );
        return false;
    } 
    
    if( FAILED( IDirectSound8_SetCooperativeLevel( dev->dsound, GetForegroundWindow(), DSSCL_PRIORITY ))) {
        Log_Write( "Failed to set DirectSound8 coop level" );       
        goto cleanup;
    }         
      
    WAVEFORMATEX bufferFormat = { 0 };
    bufferFormat.cbSize = sizeof( WAVEFORMATEX );
    bufferFormat.wFormatTag = WAVE_FORMAT_PCM;
    bufferFormat.nChannels = 2;
    bufferFormat.nSamplesPerSec = SW_OUTPUT_DEVICE_SAMPLE_RATE;
    bufferFormat.wBitsPerSample = 16;
    bufferFormat.nBlockAlign = ( bufferFormat.wBitsPerSample / 8 ) * bufferFormat.nChannels;
    bufferFormat.nAvgBytesPerSec = bufferFormat.nSamplesPerSec * bufferFormat.nBlockAlign;

    DSBUFFERDESC bufferDesc = { 0 };
    bufferDesc.dwSize = sizeof( DSBUFFERDESC );
    bufferDesc.dwFlags = DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GLOBALFOCUS;
    bufferDesc.dwBufferBytes = bufferHalfSize * 2;
    bufferDesc.lpwfxFormat = &bufferFormat;

    if( FAILED( IDirectSound8_CreateSoundBuffer( dev->dsound, &bufferDesc, &pDSB, NULL ))) {        
        Log_Write( "Failed to create DirectSoundBuffer8!" );
        goto cleanup;
    } 
    
    IDirectSoundBuffer_QueryInterface( pDSB, &IID_IDirectSoundBuffer8, (void**)&dev->buffer );
    IDirectSoundBuffer_Release( pDSB );
    
    IDirectSoundBuffer8_QueryInterface( dev->buffer, &IID_IDirectSoundNotify8, (void**)&dev->notify );

    dev->points[0] = CreateEvent( 0, 0, TRUE, 0 );
    dev->points[1] = CreateEvent( 0, 0, TRUE, 0 );

    DSBPOSITIONNOTIFY pPosNotify[ 2 ];
    pPosNotify[0].dwOffset = 0;
    pPosNotify[0].hEventNotify = dev->points[0];

    pPosNotify[1].dwOffset = bufferHalfSize;
    pPosNotify[1].hEventNotify = dev->points[1];

    IDirectSoundNotify_SetNotificationPositions( dev->notify, 2, pPosNotify );
 
    IDirectSoundBuffer8_Play( dev->buffer, 0, 0, DSBPLAY_LOOPING );

	IDirectSoundBuffer8_Stop( dev->buffer );
	DWORD	dwWrite;

	IDirectSoundBuffer8_GetCurrentPosition( dev->buffer, &dev->mmstarttime.u.sample, &dwWrite);
	IDirectSoundBuffer8_Play( dev->buffer, 0, 0, DSBPLAY_LOOPING);
    
    Log_Write( "DirectSound8 device successfully initialized!" );
    
    return dev;
cleanup:
    IDirectSound8_Release( dev->dsound );
    return NULL;
    
#else /* !_WIN32 */ 

    /* alsa code */
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_sw_params_t *sw_params;
    int err;

    dev->frameCount = bufferHalfSize / 4; /* 16-bit stereo is 4 bytes, so frame count is bufferHalfSize / 4 */
    
    if ((err = snd_pcm_open (&dev->playbackDevice, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        Log_Write( "ALSA Error unable to snd_pcm_open: %s",  snd_strerror( err ));
    }
       
    if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
        Log_Write( "ALSA Error unable to snd_pcm_hw_params_malloc: %s",  snd_strerror( err ));
    }
             
    if ((err = snd_pcm_hw_params_any (dev->playbackDevice, hw_params)) < 0) {
        Log_Write( "ALSA Error unable to snd_pcm_hw_params_any: %s",  snd_strerror( err ));
    }
    
    snd_pcm_access_t access = SND_PCM_ACCESS_RW_INTERLEAVED;   
    if ((err = snd_pcm_hw_params_set_access( dev->playbackDevice, hw_params, access)) < 0) {
        Log_Write( "ALSA Error unable to snd_pcm_hw_params_set_access: %s",  snd_strerror( err ));
    }

    if ((err = snd_pcm_hw_params_set_format (dev->playbackDevice, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
        Log_Write( "ALSA Error unable to snd_pcm_hw_params_set_format: %s",  snd_strerror( err ));
    }
    
    unsigned int exactRate = SW_OUTPUT_DEVICE_SAMPLE_RATE;
    if ((err = snd_pcm_hw_params_set_rate_near (dev->playbackDevice, hw_params, &exactRate, 0)) < 0) {
        Log_Write( "ALSA Error unable to snd_pcm_hw_params_set_rate_near: %s",  snd_strerror( err ));
    }
    if( exactRate != SW_OUTPUT_DEVICE_SAMPLE_RATE ) {
        Log_Write( "Warning! %d playback rate is not supported, using %d instead.", SW_OUTPUT_DEVICE_SAMPLE_RATE, exactRate );
    }
    
    if ((err = snd_pcm_hw_params_set_channels (dev->playbackDevice, hw_params, 2)) < 0) {
        Log_Write( "ALSA Error unable to snd_pcm_hw_params_set_channels: %s",  snd_strerror( err ));
    }
    
    /* dev->frameCount * 2 because user defines size of buffer's half, but for removing 'clicks' we need to use full sized buffer */
    long unsigned int exactSize = dev->frameCount * 2; 
    if (snd_pcm_hw_params_set_buffer_size_near( dev->playbackDevice, hw_params, &exactSize ) < 0) {
        Log_Write( "ALSA Error unable to snd_pcm_hw_params_set_buffer_size_near: %s",  snd_strerror( err ));
    }
    if( exactSize != dev->frameCount * 2 ) {
        Log_Write( "Warning! %d buffer size is not supported, using %d instead.", dev->frameCount, exactSize );
        dev->frameCount = exactSize;
    }
    
    if ((err = snd_pcm_hw_params (dev->playbackDevice, hw_params)) < 0) {
        Log_Write( "ALSA Error unable to snd_pcm_hw_params: %s",  snd_strerror( err ));
    }

    snd_pcm_hw_params_free (hw_params);

    if ((err = snd_pcm_sw_params_malloc (&sw_params)) < 0) {
        Log_Write( "ALSA Error unable to snd_pcm_sw_params_malloc: %s",  snd_strerror( err ));
    }
    if ((err = snd_pcm_sw_params_current (dev->playbackDevice, sw_params)) < 0) {
        Log_Write( "ALSA Error unable to snd_pcm_sw_params_current: %s",  snd_strerror( err ));
    }
    if ((err = snd_pcm_sw_params_set_avail_min (dev->playbackDevice, sw_params, dev->frameCount )) < 0) {
        Log_Write( "ALSA Error unable to snd_pcm_sw_params_set_avail_min: %s",  snd_strerror( err ));
    }
    if ((err = snd_pcm_sw_params_set_start_threshold (dev->playbackDevice, sw_params, 0U)) < 0) {
        Log_Write( "ALSA Error unable to snd_pcm_sw_params_set_start_threshold: %s",  snd_strerror( err ));
    }
    if ((err = snd_pcm_sw_params (dev->playbackDevice, sw_params)) < 0) {
        Log_Write( "ALSA Error unable to snd_pcm_sw_params: %s",  snd_strerror( err ));
    }

    if ((err = snd_pcm_prepare (dev->playbackDevice)) < 0) {        
        Log_Write( "ALSA Error unable to snd_pcm_prepare: %s",  snd_strerror( err ));
    }
    
    Log_Write( "ALSA device successfully initialized!" );
    
#endif
    return dev;  
}

void SW_OutputDevice_Destroy( TOutputDevice * dev ) {
#ifdef _WIN32
    IDirectSoundNotify_Release( dev->notify );
    IDirectSoundBuffer8_Release( dev->buffer );
    IDirectSound8_Release( dev->dsound );
#else
    snd_pcm_close( dev->playbackDevice );
#endif
    /* also release all resources */
    SW_Sound_FreeAll();
    SW_Buffer_FreeAll();
    SW_Effect_FreeAll();
    SW_Memory_Free( dev );
}
    
void SW_OutputDevice_SendData( TOutputDevice * dev, short * inputData ) {
#ifdef _WIN32
    void * outputData;
    int size;

    int result = WaitForMultipleObjects( 2, dev->points, 0, 0 ); 

    if( result == WAIT_OBJECT_0 ) {        
        IDirectSoundBuffer8_Lock( dev->buffer, dev->bufferHalfSize, dev->bufferHalfSize, &outputData, (LPDWORD)&size, 0, 0, 0 );
        memcpy( outputData, inputData, size );
        IDirectSoundBuffer8_Unlock( dev->buffer, outputData, size, 0, 0 );
    }
    if( result == ( WAIT_OBJECT_0 + 1 ) ) {
        IDirectSoundBuffer8_Lock( dev->buffer, 0, dev->bufferHalfSize, &outputData, (LPDWORD)&size, 0, 0, 0 );
        memcpy( outputData, inputData, size );
        IDirectSoundBuffer8_Unlock( dev->buffer, outputData, size, 0, 0 );
    } 

#else
    int err;
	sent = false;

    if( ( err = snd_pcm_writei( dev->playbackDevice, inputData, dev->frameCount )) < 0 ) {
        if( err == -EPIPE ) {
            Log_Write( "ALSA Error: Buffer underrun!" );
        }
        if( ( err = snd_pcm_prepare( dev->playbackDevice )) < 0 ) {
            Log_Write( "ALSA Error unable to snd_pcm_prepare: %s",  snd_strerror( err ));
        } else {
            if( ( err = snd_pcm_writei( dev->playbackDevice, inputData, dev->frameCount )) < 0 ) {
                Log_Write( "ALSA Error unable to snd_pcm_writei: %s",  snd_strerror( err ));
            }
        }
    }  
#endif
};


uint32_t SW_OutputDevice_GetHalfBufferSize( TOutputDevice * dev ) {
    return dev->bufferHalfSize;
}

int SW_OutputDevice_GetOutputPosition( TOutputDevice * dev )
{
	MMTIME	mmtime;
	int		s;
	DWORD	dwWrite;

	mmtime.wType = TIME_SAMPLES;
	IDirectSoundBuffer8_GetCurrentPosition(dev->buffer, &mmtime.u.sample, &dwWrite);
	s = mmtime.u.sample - dev->mmstarttime.u.sample;

	s >>= SAMPLE_16BIT_SHIFT;

	int devSampleCount = dev->bufferHalfSize*2 / 16;

	s &= (devSampleCount-1);

	return s;
}