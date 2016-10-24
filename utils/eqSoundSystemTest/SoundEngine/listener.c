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

#include "listener.h"
#include <time.h>
#include "sound.h"

typedef struct TListener {
    float gain;
    TVec3 pos;
    TVec3 up;
    TVec3 look;
    TVec3 right;
} TListener;

TListener listener = { 
    1.0f,
    { 0, 0, 0 },
    { 0, 1, 0 },
    { 0, 0, 1 },
    { 1, 0, 0 },
};

void SW_Listener_SetPosition( TVec3 position ) {
    listener.pos = position;
}

TVec3 SW_Listener_GetPosition( void ) {
    return listener.pos;
}

void SW_Listener_SetOrientation( TVec3 upVector, TVec3 lookVector ) {
    listener.up = Vec3_Normalize( upVector );
    listener.look = Vec3_Normalize( lookVector );
    listener.right = Vec3_Normalize( Vec3_Cross( upVector, lookVector ));
}

TVec3 SW_Listener_GetOrientaionUp( void ) {
    return listener.up;
}

TVec3 SW_Listener_GetOrientaionLook( void ) {
    return listener.look;
}

TVec3 SW_Listener_GetOrientaionRight( void ) {
    return listener.right;
}

void SW_Listener_SetGain( float gain ) {
    listener.gain = gain;
}

float SW_Listener_GetGain( void ) { 
    return listener.gain;
}