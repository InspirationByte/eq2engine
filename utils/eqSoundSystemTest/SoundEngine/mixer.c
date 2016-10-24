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

#include "device.h"
#include "sound.h"
#include <limits.h>
#include "memory.h"

struct SMixer {
    int16_t * paintbuffer;
    TOutputDevice * dev;
    bool active;
    bool suspended;
    bool threadActive;
    bool locked;
	int bufferavail;
};

#include "mixer.h"

TMixer * gMixer;

TMixer * SW_Mixer_GetCurrent() {
    return gMixer;
}

/*
static float sampleMinLimit = SHRT_MIN;
static float sampleMaxLimit = SHRT_MAX;
*/

void SW_Mixer_Update( TMixer* mixer )
{
    int n;
    float left, right;
    float leftMixed, rightMixed;

	int sampleCount = SW_OutputDevice_GetHalfBufferSize( mixer->dev ) / sizeof( int16_t );

	for( n = 0; n < sampleCount;  )
	{
		leftMixed = 0.0f;
		rightMixed = 0.0f;
                
		// mix all playing sounds 
		TSound * snd = SW_Sound_GetList();
		while( snd ) {                
			if( SW_Sound_Unsafe_IsPlaying( snd ) && !SW_Sound_Unsafe_IsPaused( snd ) ) {

				SW_Sound_GetSamples( snd, &left, &right );

				SW_Sound_IncreaseReadPtr( snd );
				
				leftMixed += left;
				rightMixed += right;
			}
			snd = SW_Sound_Next( snd );
		}

		// process effects 
		TEffect * effect = SW_Effect_GetList();
		while( effect ) {
			if( SW_Effect_IsEnabled( effect )) {
				SW_Effect_Process( effect, &left, &right );
				leftMixed += left;
				rightMixed += right;
			}
			effect = SW_Effect_Next( effect );
		}

		/* clamp to [-32768;32767] */
		//if( leftMixed > sampleMaxLimit ) leftMixed = sampleMaxLimit;
		//if( leftMixed < sampleMinLimit ) leftMixed = sampleMinLimit;
		//if( rightMixed > sampleMaxLimit ) rightMixed = sampleMaxLimit;
		//if( rightMixed < sampleMinLimit ) rightMixed = sampleMinLimit;
                
		// write to buffer
		uint32_t l = lrintf( leftMixed );
		uint32_t r = lrintf( rightMixed );

		l &= 65535;
		r &= 65535;
                                
		mixer->paintbuffer[ n++ ] = l;
		mixer->paintbuffer[ n++ ] = r;
	}

    /* send mixed data to the output device */
	SW_OutputDevice_SendData( mixer->dev, mixer->paintbuffer );
}

void SW_Mixer_MainFunc( void * param ) {
    Log_Write( "Mixer thread started!" );
    TMixer * mixer = param;    
    mixer->threadActive = true;
    
    while( mixer->active ) {   
		if( !mixer->suspended )
			SW_Mixer_Update( mixer );
    }
    Log_Write( "Mixer thread exited successfully!" );
    mixer->threadActive = false;
}

#ifdef _WIN32 
#include <windows.h>
/* main win32 mixing thread */
DWORD WINAPI SW_Mixer_Mix( LPVOID param ) {
    SW_Mixer_MainFunc(param);
    return 0;
}
#else
/* main linux mixing thread */
#include <pthread.h>
void SW_Mixer_Mix( void * param ) {
    SW_Mixer_MainFunc( param );
}
#endif

TMixer * SW_Mixer_Create( TOutputDevice * dev ) {
    TMixer * mix = SW_Memory_New( TMixer );
    
    mix->dev = dev;
    mix->paintbuffer = SW_Memory_Alloc( SW_OutputDevice_GetHalfBufferSize( dev ));
    mix->suspended = false;
    mix->locked = false;
	mix->bufferavail = true;
     
    gMixer = mix;
    
    return mix;
}

void SW_Mixer_Start( TMixer * mix ) {
    mix->active = true;    
   
#ifdef _WIN32
    CreateThread( 0, 0, SW_Mixer_Mix, mix, 0, 0 );
#else
    pthread_t mThread;
    pthread_create( &mThread, NULL, (void *(*)( void*))SW_Mixer_Mix, mix );
#endif
}

void SW_Mixer_Stop( TMixer * mix ) {
    mix->active = false;  
    while( mix->threadActive ) { 
        printf( "Avaiting mixer thread to stop...\r" );
    };
}

void SW_Mixer_Suspend( TMixer * mix ) {
    mix->suspended = true;
}

void SW_Mixer_Process( TMixer * mix ) {
    mix->suspended = false;
}

void SW_Mixer_Free( TMixer * mix ) {
    SW_Memory_Free( mix->paintbuffer );
    SW_Memory_Free( mix );
}

bool SW_Mixer_IsActive( TMixer * mix ) {
    return mix->active;
}
