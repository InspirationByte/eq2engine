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

#ifndef _EFFECT_H_
#define _EFFECT_H_

#include "sndwrk.h"

SW_BEGIN_HEADER()

typedef struct SEffect TEffect;

typedef enum EEffectType {
    /* Reverb is the basic effect and mostly effective effect */
    SW_EFFECT_REVERB = 0,
} EEffectType;

/*
========================
SW_Effect_GetList
========================
*/
TEffect * SW_Effect_GetList();

/*
========================
Effect_GetFirst
========================
*/
TEffect * SW_Effect_Next( TEffect * effect );

/*
========================
SW_Effect_IsEffect
========================
*/
bool SW_Effect_IsEffect( TEffect * effect );

/*
========================
Effect_Create
========================
*/
TEffect * SW_Effect_Create( EEffectType type );

/*
========================
Effect_Feed
    - Performs processing of the accumulation sample
    - Returns stereo pair for the mixer
========================
*/
void SW_Effect_Process( TEffect * effect, float * outLeft, float * outRight );

/*
========================
SW_Effect_AddDrySample
    - Adds dry sample to accumulation sample
    - When mixer calls SW_Effect_Process this accumulation sample is divided by count of input sources, thus it becomes average sample of all of the inputs
========================
*/
void SW_Effect_AddDrySample( TEffect * effect, float sample );

/*
========================
SW_Effect_SetGain
========================
*/
void SW_Effect_SetGain( TEffect * effect, float gain );

/*
========================
SW_Effect_GetGain
========================
*/
float SW_Effect_GetGain( TEffect * effect );

/*
========================
SW_Effect_SetEnabled
========================
*/
void SW_Effect_SetEnabled( TEffect * effect, bool state );

/*
========================
SW_Effect_IsEnabled
========================
*/
bool SW_Effect_IsEnabled( TEffect * effect );

/*
========================
SW_Effect_Free
========================
*/
void SW_Effect_Free( TEffect * effect );

/*
========================
SW_Effect_FreeAll
========================
*/
void SW_Effect_FreeAll( void );

/*
========================
SW_Reverb_SetDecayTime
========================
*/
void SW_Reverb_SetDecayTime( TEffect * effect, float time );

/*
========================
SW_Reverb_GetDecayTime
========================
*/
float SW_Reverb_GetDecayTime( TEffect * effect );


/*
========================
SW_Reverb_SetLateDiffusion
========================
*/
void SW_Reverb_SetLateDiffusion( TEffect * effect, float diffusion );

/*
========================
SW_Reverb_GetLateDiffusion
========================
*/
float SW_Reverb_GetLateDiffusion( TEffect * effect );

SW_END_HEADER()

#endif