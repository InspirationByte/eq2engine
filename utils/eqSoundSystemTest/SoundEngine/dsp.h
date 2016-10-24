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

#ifndef _DSP_H_
#define _DSP_H_

#include "sndwrk.h"

SW_BEGIN_HEADER()

typedef struct TOnepole {
    float a1, b0;
    float last;
} TOnepole;

typedef struct TDelayLine {
    float * line;
    float last;
    float timeLength;
    int32_t ptr;
    uint32_t wrapMask;
} TDelayLine;

typedef struct TVariableDelayLine {
    float * line;
    uint32_t readPtr;
    uint32_t writePtr;
    uint32_t wrapMask;
    float timeLength;
} TVariableDelayLine;

typedef struct TAllpass {
    TDelayLine dl;
    float gain;
    float feed;
} TAllpass;

int SW_Convert_TimeToSampleNum( float time );
void SW_Onepole_Create( TOnepole * o, float pole );
float SW_Onepole_Feed( TOnepole * o, float input );
void SW_Onepole_SetPole( TOnepole * o, float pole );

void SW_DelayLine_Create( TDelayLine * dl, float length_sec );
float SW_DelayLine_Feed( TDelayLine * dl, float input );
float SW_DelayLine_TapOut( TDelayLine * dl, int offset );
void SW_DelayLine_Free( TDelayLine * dl );

void SW_VariableDelayLine_Create( TVariableDelayLine * vdl, float fullLength );
void SW_VariableDelayLine_SetDelay( TVariableDelayLine * vdl, float timeSec );
float SW_VariableDelayLine_Feed( TVariableDelayLine * vdl, float sample );
void SW_VariableDelayLine_Free( TVariableDelayLine * vdl );

void SW_Allpass_Create( TAllpass * ap, float length_sec );
void SW_Allpass_Free( TAllpass * ap );
float SW_Allpass_Feed( TAllpass * ap, float input );

SW_END_HEADER()

#endif