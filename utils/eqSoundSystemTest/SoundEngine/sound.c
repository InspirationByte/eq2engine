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

#include "buffer.h"
#include "list.h"
#include "listener.h"
#include "dsp.h"
#include "effect.h"
#include "memory.h"
#include "mixer.h"

struct SSound {
    struct SSound * next;
    struct SSound * prev;
    
    TBuffer * buffer;
    float * bufferSamples;
    uint32_t bufferSampleCount;
    
    bool playing;
    bool looping;
    bool is3D;
    bool paused;
    float gain;
    float pitch;
    float readPtr;
    float pan;
    float leftGain;
    float rightGain;
    float sampleRateMultiplier;
    float currentSampleRate;  

    /* 3d properties */
    TVec3 pos;
    float radius;
    float distanceGain;
    float effectDistanceGain;
    float effectRadius;
    TVariableDelayLine vdlLeft;
    TVariableDelayLine vdlRight;
    TOnepole opLeft;
    TOnepole opRight;
    
    TEffect * effect;
    
    /* proper function selected by number of channels in buffer */
    void (*GetSamples)( struct SSound * snd, float * left, float * right );
};

#include "sound.h"

TSound * soundList;

float hfDampingTableLeft[360] = {
    0.35f,  0.34f,  0.33f,   0.32f,   0.31f,   0.3f,    0.29f,   0.28f,   0.27f,   0.26f,    /* 10  deg */
    0.22f,  0.18f,  0.14f,   0.11f,   0.07f,   0.06f,   0.04f,   0.03f,   0.02f,   0.0f,    /* 20  deg */
    0.0f,   0.0f,   0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    /* 30  deg */
    0.0f,   0.0f,   0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    /* 40  deg */
    0.0f,   0.0f,   0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    /* 50  deg */
    0.1f,   0.11,   0.12f,   0.13f,   0.14f,   0.15f,   0.16f,   0.17f,   0.18f,   0.19f,   /* 60  deg */
    0.2f,   0.21,   0.22f,   0.23f,   0.24f,   0.25f,   0.26f,   0.27f,   0.28f,   0.29f,   /* 70  deg */
    0.3f,   0.31,   0.32f,   0.33f,   0.34f,   0.35f,   0.36f,   0.37f,   0.38f,   0.39f,   /* 80  deg */
    0.4f,   0.41,   0.42f,   0.43f,   0.44f,   0.45f,   0.46f,   0.47f,   0.48f,   0.49f,   /* 90  deg */
    0.5f,   0.51,   0.52f,   0.53f,   0.54f,   0.55f,   0.56f,   0.57f,   0.58f,   0.59f,   /* 100 deg */
    0.6f,   0.61,   0.62f,   0.63f,   0.64f,   0.65f,   0.66f,   0.67f,   0.68f,   0.69f,   /* 110 deg */
    0.7f,   0.71,   0.711f,  0.712f,  0.713f,  0.714f,  0.715f,  0.716f,  0.717f,  0.718f,  /* 120 deg */
    0.718f, 0.718f, 0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  /* 130 deg */
    0.718f, 0.718f, 0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  /* 140 deg */
    0.718f, 0.718f, 0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  /* 150 deg */
    0.718f, 0.718f, 0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  /* 160 deg */
    0.718f, 0.718f, 0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  /* 170 deg */
    0.718f, 0.718f, 0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  /* 180 deg */
    0.718f, 0.718f, 0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  /* 190 deg */
    0.718f, 0.718f, 0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  /* 200 deg */
    0.718f, 0.718f, 0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  /* 210 deg */
    0.718f, 0.718f, 0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  /* 220 deg */
    0.718f, 0.718f, 0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  /* 230 deg */
    0.718f, 0.718f, 0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  /* 240 deg */
    0.718f, 0.718f, 0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  /* 250 deg */
    0.718f, 0.718f, 0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  /* 260 deg */  
    0.718f, 0.718f, 0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  /* 270 deg */
    0.718f, 0.718f, 0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  /* 280 deg */
    0.718f, 0.718f, 0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  /* 290 deg */
    0.718f, 0.718f, 0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  /* 300 deg */    
    0.718f, 0.717f, 0.716f,  0.715f,  0.714f,  0.713f,  0.712f,  0.711f,  0.71f,   0.71f,   /* 310 deg */        
    0.7f,   0.69f,  0.68f,   0.67f,   0.66f,   0.65f,   0.64f,   0.63f,   0.62f,   0.61f,   /* 320 deg */ 
    0.5f,   0.49f,  0.46f,   0.42f,   0.40f,   0.39f,   0.38f,   0.37f,   0.36f,   0.35f,   /* 330 deg */
    0.35f,  0.35f,  0.35f,   0.35f,   0.35f,   0.35f,   0.35f,   0.35f,   0.35f,   0.35f,   /* 340 deg */
    0.35f,  0.35f,  0.35f,   0.35f,   0.35f,   0.35f,   0.35f,   0.35f,   0.35f,   0.35f,   /* 350 deg */
    0.35f,  0.35f,  0.35f,   0.35f,   0.35f,   0.35f,   0.35f,   0.35f,   0.35f,   0.35f,   /* 360 deg */
};

float hfDampingTableRight[360] = {
    0.35f,  0.35f,  0.35f,   0.35f,   0.35f,   0.35f,   0.35f,   0.35f,   0.35f,   0.35f,   /* 10 deg */
    0.35f,  0.35f,  0.35f,   0.35f,   0.35f,   0.35f,   0.35f,   0.35f,   0.35f,   0.35f,   /* 20 deg */
    0.42f,  0.44f,  0.46f,   0.48f,   0.50f,   0.51f,   0.52f,   0.53f,   0.54f,   0.56f,   /* 30 deg */
    0.58f,  0.59f,  0.6f,    0.61f,   0.62f,   0.63f,   0.64f,   0.65f,   0.67f,   0.68f,   /* 40 deg */
    0.69f,  0.7f,   0.71f,   0.711f,  0.712f,  0.713f,  0.714f,  0.715f,  0.716f,  0.717f,  /* 50 deg */
    0.718f, 0.718f, 0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  /* 60 deg */
    0.718f, 0.718f, 0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  /* 70 deg */
    0.718f, 0.718f, 0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  /* 80 deg */
    0.718f, 0.718f, 0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  /* 90 deg */
    0.718f, 0.718f, 0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  /* 100 deg */
    0.718f, 0.718f, 0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  /* 110 deg */
    0.718f, 0.718f, 0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  /* 120 deg */
    0.718f, 0.718f, 0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  /* 130 deg */
    0.718f, 0.718f, 0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  /* 140 deg */
    0.718f, 0.718f, 0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  /* 150 deg */
    0.718f, 0.718f, 0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  /* 160 deg */
    0.718f, 0.718f, 0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  /* 170 deg */
    0.718f, 0.718f, 0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  /* 180 deg */
    0.718f, 0.718f, 0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  /* 190 deg */
    0.718f, 0.718f, 0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  /* 200 deg */
    0.718f, 0.718f, 0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  /* 210 deg */
    0.718f, 0.718f, 0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  /* 220 deg */
    0.718f, 0.718f, 0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  0.718f,  /* 230 deg */
    0.718f, 0.717f, 0.716f,  0.715f,  0.714f,  0.713f,  0.712f,  0.711f,  0.71f,   0.7f,    /* 240  deg */
    0.69f,  0.68f,  0.67f,   0.66f,   0.65f,   0.64f,   0.63f,   0.62f,   0.61f,   0.6f,    /* 250  deg */
    0.59f,  0.58f,  0.57f,   0.56f,   0.55f,   0.54f,   0.53f,   0.52f,   0.51f,   0.5f,    /* 260  deg */
    0.49f,  0.48f,  0.47f,   0.46f,   0.45f,   0.44f,   0.43f,   0.42f,   0.41f,   0.4f,    /* 270  deg */
    0.39f,  0.38f,  0.37f,   0.36f,   0.35f,   0.34f,   0.33f,   0.32f,   0.31f,   0.3f,    /* 280  deg */
    0.29f,  0.28f,  0.27f,   0.26f,   0.25f,   0.24f,   0.23f,   0.22f,   0.21f,   0.2f,    /* 290  deg */
    0.19f,  0.18f,  0.17f,   0.16f,   0.15f,   0.14f,   0.13f,   0.12f,   0.11f,   0.1f,    /* 300  deg */
    0.0f,   0.0f,   0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    /* 310  deg */
    0.0f,   0.0f,   0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    /* 320  deg */
    0.0f,   0.0f,   0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    /* 330  deg */
    0.0f,   0.0f,   0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    /* 340  deg */
    0.0f,   0.01f,  0.02f,   0.04f,   0.06f,   0.08f,   0.11f,   0.14f,   0.18f,   0.22f,   /* 350  deg */
    0.26f,  0.27f,  0.28f,   0.29f,   0.3f,    0.31f,   0.32f,   0.33f,   0.34f,   0.35f,   /* 360  deg */
};

void SW_Sound_GetSamplesMono( TSound * snd, float * left, float * right );
void SW_Sound_GetSamplesStereo( TSound * snd, float * left, float * right );  
void SW_Sound_GetSamplesMonoConvert( TSound * snd, float * left, float * right );

/*
========================
SW_Sound_Update
========================
*/
void SW_Sound_Update( TSound * snd ) {
    snd->distanceGain = 1.0f;
    snd->effectDistanceGain = 1.0f;
    
    if( snd->is3D ) {
        /* compute gain for the sound according to distance to the listener */
        TVec3 d = Vec3_Sub( SW_Listener_GetPosition(), snd->pos );
        float sqrD = Vec3_SqrLength( d );
        snd->distanceGain = 1.0f / ( 1.0f + ( sqrD / (snd->radius * snd->radius)));
        snd->effectDistanceGain = 1.0f / ( 1.0f + ( sqrD / (snd->effectRadius * snd->effectRadius )));
        
        /* compute panning for 3d sound */
        if( sqrD < 0.0001f ) {
            snd->pan = 0.0f;
        } else {
            d.y = 0.0f;
            d = Vec3_Normalize( d );
            snd->pan = Vec3_Dot( SW_Listener_GetOrientaionRight(), d );
            if( snd->pan < -1.0f ) {
                snd->pan = -1.0f;
            }
            if( snd->pan > 1.0f ) {
                snd->pan = 1.0f;
            }
        }
        
        TVec3 look = SW_Listener_GetOrientaionLook();        
        float angle = 180 - (atan2( look.z, look.x ) - atan2( d.z, d.x )) * 180.0f / 3.14159f;
        if( angle < 0.0f ) {
            angle += 360.0f;
        }
        
        float leftDamping = hfDampingTableLeft[ (int)angle ];
        float rightDamping = hfDampingTableRight[ (int)angle ];
        
        SW_Onepole_SetPole( &snd->opLeft, leftDamping );
        SW_Onepole_SetPole( &snd->opRight, rightDamping );

        SW_VariableDelayLine_SetDelay( &snd->vdlLeft, snd->vdlLeft.timeLength * ( 1.0f - snd->pan ) * 0.5f );
        SW_VariableDelayLine_SetDelay( &snd->vdlRight, snd->vdlLeft.timeLength * ( 1.0f + snd->pan ) * 0.5f );
    }
       
    float panChannels[2] = { (1.0f + snd->pan), (1.0f - snd->pan) };
    float k = 1.0f / ( 1.41f * sqrt( 1 + snd->pan * snd->pan ));
    panChannels[0] *= k;
    panChannels[1] *= k;
    
    const float minimal = 0.5f;
    
    if( panChannels[0] < minimal ) {
        panChannels[0] = minimal;
    }   
    if( panChannels[1] < minimal ) {
        panChannels[1] = minimal;
    }

    float commonGain = snd->gain * snd->distanceGain;
    
    /* calculate gain for each channel separately */
    snd->leftGain = commonGain * panChannels[0];
    snd->rightGain = commonGain * panChannels[1];    
}

/*
========================
SW_Sound_SelectSamplesFunc
========================
*/
void SW_Sound_SelectSamplesFunc( TSound * snd ) {
    if( SW_Buffer_GetChannelCount( snd->buffer ) == 2 ) {
        if( snd->is3D ) {
            snd->GetSamples = SW_Sound_GetSamplesMonoConvert;
        } else {
            snd->GetSamples = SW_Sound_GetSamplesStereo;
        }
    } else {
        snd->GetSamples = SW_Sound_GetSamplesMono;
    }
}

/*
========================
SW_Sound_IsSound
========================
*/
bool SW_Sound_IsSound( TSound * snd ) {
    TSound * s;
    
    for( s = soundList; s; s = s->next ) {
        if( s == snd ) return true;
    }
    
    return false;
}

/*
========================
SW_Sound_GetList
========================
*/
TSound * SW_Sound_GetList( ) {
    return soundList;
}

/*
========================
SW_Sound_Next
========================
*/
TSound * SW_Sound_Next( TSound * snd ) {
    return snd->next;
}

/*
========================
SW_Sound_SetPan
========================
*/
void SW_Sound_SetPan( TSound * snd, float pan ) {
    if( SW_Sound_IsSound( snd )) {
        if( pan < -1.0f ) {
            pan = -1.0f;
        }
        if( pan > 1.0f ) { 
            pan = 1.0f;
        }
        snd->pan = pan;        
        SW_Sound_Update( snd );
    }
}

/*
========================
SW_Sound_GetPan
========================
*/
float SW_Sound_GetPan( TSound * snd ) {
    if( SW_Sound_IsSound( snd )) {
        return snd->pan;
    } else {
        return 0.0f;
    }
}

/*
========================
SW_Sound_IncreaseReadPtr
    - Inreases readptr and wrap it if needed 
    - Also performs streaming
========================
*/
void SW_Sound_IncreaseReadPtr( TSound * snd ) {
    snd->readPtr += snd->currentSampleRate;     
    if( snd->readPtr >= snd->bufferSampleCount ) {
        snd->readPtr = 0;        
        if( SW_Buffer_GetType( snd->buffer ) == SW_BUFFER_STREAM ) {
            snd->bufferSamples = SW_Buffer_GetNextBlock( snd->buffer );
        } else {
            snd->playing = snd->looping;
        }
    }
}

/*
========================
SW_Sound_AddSpatialEffects
    - There is only one input sample, because spatial effects can only be applied to the 3d sounds 
========================
*/
void SW_Sound_AddSpatialEffects( TSound * snd, float sample, float * left, float * right ) {
    *left = SW_Onepole_Feed( &snd->opLeft, SW_VariableDelayLine_Feed( &snd->vdlLeft, sample * snd->leftGain ));
    *right = SW_Onepole_Feed( &snd->opRight, SW_VariableDelayLine_Feed( &snd->vdlRight, sample * snd->rightGain ));
    /* feed effect if present */
    if( snd->effect ) {
        SW_Effect_AddDrySample( snd->effect, sample * snd->effectDistanceGain );
    }
}

/*
========================
SW_Sound_GetSamplesMono
========================
*/
void SW_Sound_GetSamplesMono( TSound * snd, float * left, float * right ) {
    float sample = snd->bufferSamples[ (int)snd->readPtr ];
    
    if( snd->is3D ) {
        SW_Sound_AddSpatialEffects( snd, sample, left, right );
    } else {
        *left = sample * snd->leftGain;
        *right = sample * snd->rightGain;       
    }
}

/*
========================
SW_Sound_GetSamplesStereo
========================
*/
void SW_Sound_GetSamplesStereo( TSound * snd, float * left, float * right ) {   
    /* read_ptr must be always even when stereo buffer */
    uint32_t read_ptr = ((int)snd->readPtr ) & ~1;

    *left = snd->bufferSamples[ read_ptr++ ] * snd->leftGain;
    *right = snd->bufferSamples[ read_ptr ] * snd->rightGain; 
}

/*
========================
SW_Sound_GetSamplesMonoConvert
    - Used with stereo buffers for 3d sounds (performs conversion from stereo to mono on the fly)
========================
*/
void SW_Sound_GetSamplesMonoConvert( TSound * snd, float * left, float * right ) {
    /* read_ptr must be always even when stereo buffer */
    uint32_t read_ptr = ((int)snd->readPtr) & ~1;

    float sample = (snd->bufferSamples[ read_ptr ] + snd->bufferSamples[ read_ptr + 1 ]) * 0.5f;
    
    SW_Sound_AddSpatialEffects( snd, sample, left, right );
}

/*
========================
SW_Sound_SetPitch
========================
*/
void SW_Sound_SetPitch( TSound * snd, float pitch ) {
    if( SW_Sound_IsSound( snd )) {
        if( pitch < 0.0f ) pitch = 0.0f;
        snd->pitch = pitch;
        /* recompute sample rate */
        snd->currentSampleRate = snd->pitch * snd->sampleRateMultiplier * SW_Buffer_GetChannelCount( snd->buffer );
    }
}

/*
========================
SW_Sound_GetPitch
========================
*/
float SW_Sound_GetPitch( TSound * snd ) {
    if( SW_Sound_IsSound( snd )) {
        return snd->pitch;
    } else {
        return 0.0f;
    }
}

/*
========================
SW_Sound_Create
========================
*/
TSound * SW_Sound_Create( TBuffer * buf ) {
    TSound * snd = NULL;
    if( SW_Buffer_IsBuffer( buf )) {
        snd = SW_Memory_New( TSound ); 
        LIST_ADD(TSound, soundList, snd );        
        snd->buffer = buf;
        snd->bufferSamples = SW_Buffer_GetSamples( buf );
        snd->bufferSampleCount = SW_Buffer_GetSampleCount( buf );
        snd->gain = 1.0f;
        snd->is3D = false;
        snd->playing = false;
        snd->looping = false;
        snd->pan = 0;
        snd->paused = false;
        snd->distanceGain = 1.0f;
        snd->effectDistanceGain = 1.0f;
        snd->radius = 10.0f;
        snd->effectRadius = 10.0f;
        snd->leftGain = 1.0f;
        snd->rightGain = 1.0f; 
        snd->sampleRateMultiplier = (float)SW_Buffer_GetFrequency( buf ) / (float)SW_OUTPUT_DEVICE_SAMPLE_RATE;
        snd->currentSampleRate = 1.0f;
        SW_Sound_SelectSamplesFunc( snd );
        SW_Sound_SetPitch( snd, 1.0f );      
        SW_Sound_Update( snd );
        SW_VariableDelayLine_Create( &snd->vdlLeft,  0.0006666f );
        SW_VariableDelayLine_Create( &snd->vdlRight, 0.0006666f );
        SW_Onepole_Create( &snd->opLeft, 0.09f );
        SW_Onepole_Create( &snd->opRight, 0.09f );
    } 
    return snd;    
}

/*
========================
SW_Sound_Free
========================
*/
void SW_Sound_Free( TSound * snd ) {
    if( SW_Sound_IsSound( snd )) {
        SW_VariableDelayLine_Free( &snd->vdlLeft );
        SW_VariableDelayLine_Free( &snd->vdlRight );
        SW_Buffer_Free( snd->buffer );
        LIST_ERASE( soundList, snd );
        SW_Memory_Free( snd );
    }
}

/*
========================
SW_Sound_FreeAll
========================
*/
void SW_Sound_FreeAll() {
    if( soundList ) {
        TSound * snd = soundList;
        while( snd ) {
            TSound * next = snd->next;
            SW_Sound_Free( snd );            
            snd = next;
        }
    }
}

/*
========================
SW_Sound_SetLooping
========================
*/
void SW_Sound_SetLooping( TSound * snd, bool state ) {
    if( SW_Sound_IsSound( snd )) {
        snd->looping = state;
    }
}

/*
========================
SW_Sound_IsLooping
========================
*/
bool SW_Sound_IsLooping( TSound * snd ) {
    if( SW_Sound_IsSound( snd )) {
        return snd->looping;
    } else {
        return false;
    }
}

/*
========================
SW_Sound_Pause
========================
*/
void SW_Sound_Pause( TSound * snd ) {
    if( SW_Sound_IsSound( snd )) {
        snd->paused = true;
    }
}

/*
========================
SW_Sound_IsPaused
========================
*/
bool SW_Sound_IsPaused( TSound * snd ) {
    if( SW_Sound_IsSound( snd )) {
        return snd->paused;
    }
    return false;
}

/*
========================
SW_Sound_Unsafe_IsPlaying
========================
*/
bool SW_Sound_Unsafe_IsPlaying( TSound * snd ) {
    return snd->playing;
}

/*
========================
SW_Sound_Unsafe_IsPaused
========================
*/
bool SW_Sound_Unsafe_IsPaused( TSound * snd ) {
    return snd->paused;
}

/*
========================
SW_Sound_Stop
========================
*/
void SW_Sound_Stop( TSound * snd ) {
    if( SW_Sound_IsSound( snd )) {
        snd->readPtr = 0;        
        snd->playing = false;
    }
}

/*
========================
SW_Sound_Play
========================
*/
void SW_Sound_Play( TSound * snd ) {
    if( SW_Sound_IsSound( snd )) {
        snd->paused = false;    
        snd->playing = true;
    }
}

/*
========================
SW_Sound_IsPlaying
========================
*/
bool SW_Sound_IsPlaying( TSound * snd ) {
    if( SW_Sound_IsSound( snd )) {
        return snd->playing;
    } else {
        return false;
    }
}

/*
========================
SW_Sound_GetSamples
========================
*/
void SW_Sound_GetSamples( TSound * snd, float * left, float * right ) {
    snd->GetSamples( snd, left, right );
}

/*
========================
SW_Sound_SetPosition
========================
*/
void SW_Sound_SetPosition( TSound * snd, TVec3 position ) {
    if( SW_Sound_IsSound( snd )) {
        snd->pos = position;
        SW_Sound_Update( snd );
    }
}

/*
========================
SW_Sound_GetPosition
========================
*/
TVec3 SW_Sound_GetPosition( TSound * snd ) {
    if( SW_Sound_IsSound( snd )) {
        return snd->pos;
    } else {
        return (TVec3) { 0, 0, 0 };
    }
}

/*
========================
SW_Sound_Set3D
========================
*/
void SW_Sound_Set3D( TSound * snd, bool state ) {
    if( SW_Sound_IsSound( snd )) {
        snd->is3D = state;
        SW_Sound_SelectSamplesFunc( snd );
    }
}

/*
========================
SW_Sound_Is3D
========================
*/
bool SW_Sound_Is3D( TSound * snd ) {
    if( SW_Sound_IsSound( snd )) {
        return snd->is3D;
    }
    return false;
}

/*
========================
SW_Sound_SetVolume
========================
*/
void SW_Sound_SetVolume( TSound * snd, float vol ) {
    if( SW_Sound_IsSound( snd )) {
        if( vol < 0.0f ) {
            vol = 0.0f;
        }
        snd->gain = vol;
        SW_Sound_Update( snd );
    }
}

/*
========================
SW_Sound_GetVolume
========================
*/
float SW_Sound_GetVolume( TSound * snd ) {
    if( SW_Sound_IsSound( snd )) {
        return snd->gain;
    }
    return 0.0f;
}

/*
========================
SW_Sound_SetRadius
========================
*/
void SW_Sound_SetRadius( TSound * snd, float radius ) {
    if( SW_Sound_IsSound( snd )) {
        snd->radius = radius;
        SW_Sound_Update( snd );
    }
}

/*
========================
SW_Sound_GetRadius
========================
*/
float SW_Sound_GetRadius( TSound * snd ) {
    if( SW_Sound_IsSound( snd )) {
        return snd->radius;
    }
    return 0.0f;
}

/*
========================
SW_Sound_SetEffectRadius
========================
*/
void SW_Sound_SetEffectRadius( TSound * snd, float radius ) {
    if( SW_Sound_IsSound( snd )) {
        snd->effectRadius = radius;
    }
}

/*
========================
SW_Sound_GetEffectRadius
========================
*/
float SW_Sound_GetEffectRadius( TSound * snd ) {
    if( SW_Sound_IsSound( snd )) {
        return snd->effectRadius;
    }
    return 0.0f;
}

/*
========================
SW_Sound_SetEffect
========================
*/
void SW_Sound_SetEffect( TSound * snd, TEffect * effect ) {
    if( SW_Sound_IsSound( snd )) {
        snd->effect = effect;
    }
}

/*
========================
SW_Sound_GetEffect
========================
*/
TEffect * SW_Sound_GetEffect( TSound * snd ) {
    if( SW_Sound_IsSound( snd )) {
        return snd->effect;
    }
    return NULL;
}
