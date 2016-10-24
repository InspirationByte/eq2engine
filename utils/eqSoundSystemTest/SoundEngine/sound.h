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

#ifndef _SOUND_H_
#define _SOUND_H_

#include "buffer.h"
#include "effect.h"

SW_BEGIN_HEADER()

typedef struct SSound TSound;

/*
========================
SW_Sound_IsSound
    - Checks pointer for validity
    - Widely used in other functions SW_Sound_XXX, so there is no need to check pointer manually
========================
*/
bool SW_Sound_IsSound( TSound * snd );

/*
========================
SW_Sound_GetList
    - Gets first sound in list, next element can be accessed through Sound_Next
========================
*/
TSound * SW_Sound_GetList( void );

/*
========================
SW_Sound_Next
    - Note that this function do not check 'snd' for validity
========================
*/
TSound * SW_Sound_Next( TSound * snd );

/*
========================
SW_Sound_Create
    - Creates new sound from buffer
    - Buffer must be valid, otherwise NULL is returned
========================
*/
TSound * SW_Sound_Create( TBuffer * buf );

/*
========================
SW_Sound_Free
    - Frees sound, pointer becomes invalid
========================
*/
void SW_Sound_Free( TSound * snd );

/*
========================
SW_Sound_FreeAll
    - Frees all sounds, all user pointers become invalid
========================
*/
void SW_Sound_FreeAll();

/*
========================
SW_Sound_GetSamples
    - Reads samples from buffer, shifts read pointer
    - This function for internal use only (direct use can cause undefined behaviour)
    - No check for pointers validity!
    - 'left' and 'right' samples are 'wet' signals, which means that can be directly passed to the mixer 
       (they are already properly processed)
========================
*/
void SW_Sound_GetSamples( TSound * snd, float * left, float * right );

/*
========================
SW_Sound_IncreaseReadPtr
    - Inreases readptr and wrap it if needed 
    - Also performs streaming
========================
*/
void SW_Sound_IncreaseReadPtr( TSound * snd );

/*
========================
SW_Sound_SetLooping
========================
*/
void SW_Sound_SetLooping( TSound * snd, bool state );

/*
========================
SW_Sound_IsLooping
========================
*/
bool SW_Sound_IsLooping( TSound * snd );

/*
========================
SW_Sound_GetPan
========================
*/
float SW_Sound_GetPan( TSound * snd );

/*
========================
SW_Sound_SetPan
    - Pan must be in [-1.0; +1.0]
========================
*/
void SW_Sound_SetPan( TSound * snd, float pan );

/*
========================
SW_Sound_GetPitch
========================
*/
float SW_Sound_GetPitch( TSound * snd );

/*
========================
SW_Sound_SetPitch
    - Pitch must be > 0.0
    - Works only for 2D sounds
========================
*/
void SW_Sound_SetPitch( TSound * snd, float pitch );

/*
========================
SW_Sound_Play
    - No check for playing, immediate play
========================
*/
void SW_Sound_Play( TSound * snd );

/*
========================
SW_Sound_IsPlaying
========================
*/
bool SW_Sound_IsPlaying( TSound * snd );

/*
========================
SW_Sound_Unsafe_IsPlaying
========================
*/
bool SW_Sound_Unsafe_IsPlaying( TSound * snd );

/*
========================
SW_Sound_Unsafe_IsPaused
========================
*/
bool SW_Sound_Unsafe_IsPaused( TSound * snd );

/*
========================
SW_Sound_Pause
========================
*/
void SW_Sound_Pause( TSound * snd );

/*
========================
SW_Sound_IsPaused
========================
*/
bool SW_Sound_IsPaused( TSound * snd );

/*
========================
SW_Sound_Stop
========================
*/
void SW_Sound_Stop( TSound * snd );

/*
========================
SW_Sound_SetPosition
    - Sets position in 3D
========================
*/
void SW_Sound_SetPosition( TSound * snd, TVec3 position );

/*
========================
SW_Sound_GetPosition
    - Gets current sound position in 3D
========================
*/
TVec3 SW_Sound_GetPosition( TSound * snd );

/*
========================
SW_Sound_Set3D
    - If 'true' passed, sound becomes '3D sound' with proper spatial effects
    - In this case, you can't control panning of the sound
========================
*/
void SW_Sound_Set3D( TSound * snd, bool state );

/*
========================
Sound_Is3D
========================
*/
bool SW_Sound_Is3D( TSound * snd );

/*
========================
SW_Sound_SetVolume
========================
*/
void SW_Sound_SetVolume( TSound * snd, float vol );

/*
========================
SW_Sound_GetVolume
========================
*/
float SW_Sound_GetVolume( TSound * snd );

/*
========================
SW_Sound_SetRadius
    - Sets radius of imaginable sphere around the sound, which determines volume in which attenuation of the sound falling to zero after the border
    - Default law of attenuation is quadratic attenuation: 1 / ( 1 + ( d^2 / r^2 ))
    - Default radius is 10.0
========================
*/
void SW_Sound_SetRadius( TSound * snd, float radius );

/*
========================
SW_Sound_GetRadius
========================
*/
float SW_Sound_GetRadius( TSound * snd );

/*
========================
SW_Sound_SetEffectRadius
    - Controls decay of the sound intensity, which passed to effect
    - For more detailed description see SW_Sound_SetRadius
========================
*/
void SW_Sound_SetEffectRadius( TSound * snd, float radius );

/*
========================
SW_Sound_GetEffect
========================
*/
float SW_Sound_GetEffectRadius( TSound * snd );

/*
========================
SW_Sound_SetEffect
    - Sets effect for the sound, effect defines additional path of processing for the signal
    - If effect is set, result of SW_Sound_GetSamples contains mixed 'wet' signals from spatial processing and effect processing
      'wet' - is the processed signal
========================
*/
void SW_Sound_SetEffect( TSound * snd, TEffect * effect );

/*
========================
SW_Sound_GetEffect
========================
*/
TEffect * SW_Sound_GetEffect( TSound * snd );

/*
========================
SW_Sound_Update
    - For internal use only
========================
*/
void SW_Sound_Update( TSound * snd );

SW_END_HEADER()

#endif