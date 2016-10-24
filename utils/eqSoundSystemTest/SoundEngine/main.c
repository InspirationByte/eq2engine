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

#include "sndwrk.h"
#include "decoder_ogg.h"
#include "mixer.h"
#include "buffer.h"
#include "sound.h"
#include <ctype.h>

#ifdef _WIN32
    #include <conio.h>
    
    int GetKeyPressed() {
        return getch();
    }
#else 
    #include <stdio.h>
    #include <termios.h>
    #include <unistd.h>
    
    int GetKeyPressed() {
        struct termios oldt,newt;
        int ch;
        tcgetattr( STDIN_FILENO, &oldt );
        newt = oldt;
        newt.c_lflag &= ~( ICANON | ECHO );
        tcsetattr( STDIN_FILENO, TCSANOW, &newt );
        ch = getchar();
        tcsetattr( STDIN_FILENO, TCSANOW, &oldt );
        return ch;
    }
#endif

#define soundCount 4

#include "dsp.h"

int main(int argc, char **argv) {   
    unsigned int k = -1200;
    
    k &= 32767;
    
    short t = (short)k;
    
    t++;
 
    int i;
    
    Log_Open( "sndwrk.log" );
        
    printf( "SoundWorks Test Suite. (C) mrDIMAS 2015-2016\nH - for help\n" );
    
    TOutputDevice * dev = SW_OutputDevice_Create( 16000 );
        
    TMixer * mixer = SW_Mixer_Create( dev );
    SW_Mixer_Start( mixer );
    
    TEffect * reverb = SW_Effect_Create( SW_EFFECT_REVERB );
        
        
    const char * files[soundCount] = { 
        "FootStep_shoe_metal_step1.ogg", "FootStep_shoe_metal_step2.ogg", 
        "FootStep_shoe_metal_step3.ogg", "FootStep_shoe_metal_step4.ogg"
    };
  
  /*
    const char * files[soundCount] = { 
        "FootStep_shoe_grass_step1.ogg", "FootStep_shoe_grass_step2.ogg", 
        "FootStep_shoe_grass_step3.ogg", "FootStep_shoe_grass_step4.ogg"
    };
  */
    TSound * snd[soundCount];
    for( i = 0; i < soundCount; i++ ) {
        TBuffer * buffer = SW_Buffer_Create( files[i], SW_BUFFER_FULL );    
        snd[i] = SW_Sound_Create( buffer );
        SW_Sound_SetLooping( snd[i], false );
        SW_Sound_SetEffect( snd[i], reverb );
    }
    
    TBuffer * buffer = SW_Buffer_Create( "music3.ogg", SW_BUFFER_STREAM );
    TSound * music = SW_Sound_Create( buffer );
    SW_Sound_SetLooping( music, true );
    SW_Sound_Play( music );
    SW_Sound_Set3D( music, true );
    SW_Sound_SetEffect( music, reverb );
    SW_Sound_SetPosition( music, Vec3_Set( 0, 0, 2 ));

    const float pitchDelta = 0.15f;
    const float volumeDelta = 0.1f;
    const float panDelta = 0.1f;
    const float speed = 0.1f;
    const float decayTimeDelta = 0.05f;
    const float diffusionDelta = 0.01f;
    
    float pathLen = 0.0f;
    while( SW_Mixer_IsActive( mixer ) ) {
        int cmd = toupper(GetKeyPressed());
        switch( cmd ) {
            default:
                printf( "Unknown command!\n" );
            case 'H':
                printf( "Press [W][S][A][D] to walk\n");
                printf( "Esc - exit\n" );
                printf( "H   - print help\n" );
                printf( "W   - move forward\n" );
                printf( "S   - move backward\n" );
                printf( "A   - move left\n" );
                printf( "D   - move right\n" );           
                printf( "G   - pause sound\n" );
                printf( "Q   - increase pitch\n" );
                printf( "F   - decrease pitch\n" );
                printf( "E   - increase pan\n" );
                printf( "R   - decrease pan\n" );
                printf( "T   - increase gain\n" );
                printf( "Y   - decrease gain\n" );
                printf( "U   - make sound 3D/2D\n" );
                printf( "J   - enable/disable reverb\n" );
                printf( "X   - decrease reverb time\n" );
                printf( "Z   - increase reverb time\n" );
                printf( "C   - decrease late diffusion\n" );
                printf( "V   - increase late diffusion\n" );
                break;
            case 27:
                SW_Mixer_Stop( mixer );
                break;
            /* pitch */
            case 'Q':
                for( i = 0; i < soundCount; i++ ) {
                    SW_Sound_SetPitch( snd[i], SW_Sound_GetPitch( snd[i] ) + pitchDelta );
                }
                printf( "Sound pitch: %f\n", SW_Sound_GetPitch( snd[0] ) );
                break;
            case 'F':
                for( i = 0; i < soundCount; i++ ) {
                    SW_Sound_SetPitch( snd[i], SW_Sound_GetPitch( snd[i] ) - pitchDelta );
                }
                printf( "Sound pitch: %f\n", SW_Sound_GetPitch( snd[0] ) );
                break;
            /* volume */
            case 'E':
                for( i = 0; i < soundCount; i++ ) {
                    SW_Sound_SetVolume( snd[i], SW_Sound_GetVolume( snd[i] ) + volumeDelta );
                }
                printf( "Sound volume: %f\n", SW_Sound_GetVolume( snd[0] ) );
                break;
            case 'R':
                for( i = 0; i < soundCount; i++ ) {
                    SW_Sound_SetVolume( snd[i], SW_Sound_GetVolume( snd[i] ) - volumeDelta );
                }
                printf( "Sound volume: %f\n", SW_Sound_GetVolume( snd[0] ) );
                break;
            /* panning */
            case 'T':
                for( i = 0; i < soundCount; i++ ) {
                    SW_Sound_SetPan( snd[i], SW_Sound_GetPan( snd[i] ) + panDelta );
                }
                printf( "Sound pan: %f\n", SW_Sound_GetPan( snd[0] ) );
                break;
            case 'Y':
                for( i = 0; i < soundCount; i++ ) {
                    SW_Sound_SetPan( snd[i], SW_Sound_GetPan( snd[i] ) - panDelta );
                }
                printf( "Sound pan: %f\n", SW_Sound_GetPan( snd[0] ) );
                break;
            /* 3D */
            case 'U':
                for( i = 0; i < soundCount; i++ ) {
                    SW_Sound_Set3D( snd[i], !SW_Sound_Is3D( snd[i] ) );
                }
                printf( "Sound now: %s\n", SW_Sound_Is3D( snd[0] ) ? "3D" : "2D" );
                break;
            /* position */
            case 'A':
                for( i = 0; i < soundCount; i++ ) {
                    SW_Sound_SetPosition( snd[i], Vec3_Add( SW_Sound_GetPosition( snd[i] ), Vec3_Set( -speed, 0, 0 )));
                }
                /* SW_Sound_SetPosition( music, Vec3_Add( SW_Sound_GetPosition( music ), Vec3_Set( -speed, 0, 0 ))); */
                pathLen += speed;
                printf( "Sound position: (%f,%f,%f)\n", SW_Sound_GetPosition( snd[0] ).x, SW_Sound_GetPosition( snd[0] ).y, SW_Sound_GetPosition( snd[0] ).z );
                break;
            case 'D':
                for( i = 0; i < soundCount; i++ ) {
                    SW_Sound_SetPosition( snd[i], Vec3_Add( SW_Sound_GetPosition( snd[i] ), Vec3_Set( speed, 0, 0 )));
                }
                /*   SW_Sound_SetPosition( music, Vec3_Add( SW_Sound_GetPosition( music ), Vec3_Set( speed, 0, 0 ))); */
                pathLen += speed;
                printf( "Sound position: (%f,%f,%f)\n", SW_Sound_GetPosition( snd[0] ).x, SW_Sound_GetPosition( snd[0] ).y, SW_Sound_GetPosition( snd[0] ).z );
                break;
            case 'S':
                for( i = 0; i < soundCount; i++ ) {
                    SW_Sound_SetPosition( snd[i], Vec3_Add( SW_Sound_GetPosition( snd[i] ), Vec3_Set( 0, 0, -speed )));
                }
                /*  SW_Sound_SetPosition( music, Vec3_Add( SW_Sound_GetPosition( music ), Vec3_Set( 0, 0, -speed ))); */
                pathLen += speed;
                printf( "Sound position: (%f,%f,%f)\n", SW_Sound_GetPosition( snd[0] ).x, SW_Sound_GetPosition( snd[0] ).y, SW_Sound_GetPosition( snd[0] ).z );     
                break;
            case 'W':
                for( i = 0; i < soundCount; i++ ) {
                    SW_Sound_SetPosition( snd[i], Vec3_Add( SW_Sound_GetPosition( snd[i] ), Vec3_Set( 0, 0, speed )));
                }
                /* SW_Sound_SetPosition( music, Vec3_Add( SW_Sound_GetPosition( music ), Vec3_Set( 0, 0, speed ))); */
                pathLen += speed;
                printf( "Sound position: (%f,%f,%f)\n", SW_Sound_GetPosition( snd[0] ).x, SW_Sound_GetPosition( snd[0] ).y, SW_Sound_GetPosition( snd[0] ).z );
                break;
            case 'G':
                printf( "Test sound paused\n" );
                SW_Sound_Pause( snd[0] );
                break;
            case 'J':
                SW_Effect_SetEnabled( reverb, !SW_Effect_IsEnabled( reverb ) );
                printf( "Reverb %s\n", SW_Effect_IsEnabled( reverb ) ? "enabled" : "disabled" );
                break; 
            case 'Z':
                SW_Reverb_SetDecayTime( reverb, SW_Reverb_GetDecayTime( reverb ) - decayTimeDelta );
                printf( "Reverb decay time: %f\n", SW_Reverb_GetDecayTime( reverb ));
                break;
            case 'X':
                SW_Reverb_SetDecayTime( reverb, SW_Reverb_GetDecayTime( reverb ) + decayTimeDelta );
                printf( "Reverb decay time: %f\n", SW_Reverb_GetDecayTime( reverb ));
                break;
            case 'C':
                SW_Reverb_SetLateDiffusion( reverb, SW_Reverb_GetLateDiffusion( reverb ) - diffusionDelta );
                printf( "Late diffusion: %f\n", SW_Reverb_GetLateDiffusion( reverb ));
                break;
            case 'V':
                SW_Reverb_SetLateDiffusion( reverb, SW_Reverb_GetLateDiffusion( reverb ) + diffusionDelta );
                printf( "Late diffusion: %f\n", SW_Reverb_GetLateDiffusion( reverb ));
                break;
        }
        if( pathLen > 1.1f ) {
            int soundNum = rand() % soundCount;
            while( SW_Sound_IsPlaying( snd[soundNum] )) {
                soundNum = rand() % soundCount;
            }
            SW_Sound_Play( snd[ soundNum ] );
            pathLen = 0.0f;
        }
    }

    SW_Mixer_Stop( mixer );
    SW_Mixer_Free( mixer );
    SW_OutputDevice_Destroy( dev );
    printf( "DEBUG. Still allocated memory: %d bytes\n", SW_Memory_GetAllocated());
    
	return 0;
}

