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

#ifndef _LISTENER_H_
#define _LISTENER_H_

#include "sndwrk.h"

SW_BEGIN_HEADER()

/*
========================
Listener_SetPosition
    - Sets listener position in 3D world
========================
*/
void SW_Listener_SetPosition( TVec3 position );

/*
========================
Listener_GetPosition
    - Gets listener position in 3D world
========================
*/
TVec3 SW_Listener_GetPosition( void );

/*
========================
Listener_SetOrientation
    - This function sets orientation basis for lister, which allows to do spatialization
    - upVector is vector, that defines 'up' axis
    - lookVector is vector, that defines 'look' axis
    - By default these vectors are up = (0,1,0); look = (0,0,1)
========================
*/
void SW_Listener_SetOrientation( TVec3 upVector, TVec3 lookVector );

/*
========================
Listener_GetOrientaionUp
========================
*/
TVec3 SW_Listener_GetOrientaionUp( void );

/*
========================
Listener_GetOrientaionLook
========================
*/
TVec3 SW_Listener_GetOrientaionLook( void );

/*
========================
Listener_GetOrientaionRight
========================
*/
TVec3 SW_Listener_GetOrientaionRight( void );

/*
========================
Listener_SetGain
    - Sets 'gain' of the listener, meaning of this value is the same as of volume of a sound
========================
*/
void SW_Listener_SetGain( float gain );

/*
========================
Listener_GetGain
========================
*/
float SW_Listener_GetGain( void );


SW_END_HEADER()

#endif