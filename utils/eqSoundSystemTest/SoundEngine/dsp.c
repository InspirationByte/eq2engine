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
#include "dsp.h"
#include "memory.h"

/*
========================
SW_NearestPowOfTwo
========================
*/
uint32_t SW_NearestPowOfTwo( uint32_t num ) {
    uint32_t power = 1;
    while( power < num ) {
        power *= 2;
    }
    return power;
}

/*
========================
SW_Convert_TimeToSampleNum
========================
*/
int SW_Convert_TimeToSampleNum( float time ) {
    return time * SW_OUTPUT_DEVICE_SAMPLE_RATE;
}

/*
========================
SW_Onepole_Create
========================
*/
void SW_Onepole_Create( TOnepole * o, float pole ) {
    o->last = 0.0f;
    SW_Onepole_SetPole( o, pole );
}

/*
========================
SW_Onepole_Feed
========================
*/
float SW_Onepole_Feed( TOnepole * o, float input ) {
    o->last = o->b0 * input - o->a1 * o->last;
    return o->last;
}

/*
========================
SW_Onepole_SetPole
========================
*/
void SW_Onepole_SetPole( TOnepole * o, float pole ) {
    if( pole > 1.0f ) {
        pole = 1.0f;
    }
    if( pole < -1.0f ) {
        pole = -1.0f;
    }
    if( pole > 0.0f ) {
        o->b0 = 1.0f - pole;
    } else {
        o->b0 = 1.0f + pole;
    }
    o->a1 = -pole;
}

/*
========================
SW_DelayLine_Create
========================
*/
void SW_DelayLine_Create( TDelayLine * dl, float length_sec ) {
    dl->wrapMask = SW_NearestPowOfTwo( SW_Convert_TimeToSampleNum( length_sec ) + 1 ) - 1;
    dl->line = SW_Memory_CleanAlloc( (dl->wrapMask + 1) * sizeof( float ) );  
    dl->ptr = 0;
    dl->timeLength = length_sec;
    dl->last = 0.0f;
}

/*
========================
SW_DelayLine_Feed
========================
*/
float SW_DelayLine_Feed( TDelayLine * dl, float input ) {
    uint32_t p = dl->ptr & dl->wrapMask;
    dl->last = dl->line[p];
    dl->line[p] = input;
    ++dl->ptr;
    return dl->last;
}

/*
========================
SW_DelayLine_TapOut
========================
*/
float SW_DelayLine_TapOut( TDelayLine * dl, int offset ) {
    return dl->line[ ( dl->ptr + offset ) & dl->wrapMask ];
}

/*
========================
SW_DelayLine_Free
========================
*/
void SW_DelayLine_Free( TDelayLine * dl ) {
    SW_Memory_Free( dl->line );
}

/*
========================
SW_Allpass_Create
========================
*/
void SW_Allpass_Create( TAllpass * ap, float length_sec ) {
    SW_DelayLine_Create( &ap->dl, length_sec );
    ap->gain = 1.0f;
    ap->feed = 0.5f;
}

/*
========================
SW_Allpass_Free
========================
*/
void SW_Allpass_Free( TAllpass * ap ) {
    SW_DelayLine_Free( &ap->dl );
}

/*
========================
SW_Allpass_Feed
========================
*/
float SW_Allpass_Feed( TAllpass * ap, float input ) {
    /*
    return -input * ap->gain + DelayLine_Feed( &ap->dl, input + ap->dl.last * ap->gain ) * ( 1.0f - ap->gain * ap->gain );*/
    float out, feed;
    out = ap->dl.last;
    feed = ap->feed * input;
    SW_DelayLine_Feed( &ap->dl, ap->feed * (out - feed) + input );
    return ( ap->gain * out ) - feed;
}

/*
========================
SW_VariableDelayLine_Create
========================
*/
void SW_VariableDelayLine_Create( TVariableDelayLine * vdl, float fullLength ) {
    vdl->wrapMask = SW_NearestPowOfTwo( SW_Convert_TimeToSampleNum( fullLength ) + 1 ) - 1;
    vdl->line = SW_Memory_CleanAlloc( (vdl->wrapMask + 1) * sizeof( float ) );
    vdl->timeLength = fullLength;
    vdl->readPtr = 0;
    vdl->writePtr = 0;
}

/*
========================
SW_VariableDelayLine_SetDelay
========================
*/
void SW_VariableDelayLine_SetDelay( TVariableDelayLine * vdl, float timeSec ) {
    vdl->readPtr = (vdl->writePtr - SW_Convert_TimeToSampleNum( timeSec )) & vdl->wrapMask;
}

/*
========================
SW_VariableDelayLine_Feed
========================
*/
float SW_VariableDelayLine_Feed( TVariableDelayLine * vdl, float sample ) {
    vdl->line[ vdl->writePtr & vdl->wrapMask ] = sample;
    float out = vdl->line[ vdl->readPtr & vdl->wrapMask ];
    ++vdl->writePtr;
    ++vdl->readPtr;
    return out;
}

/*
========================
SW_VariableDelayLine_Free
========================
*/
void SW_VariableDelayLine_Free( TVariableDelayLine * vdl ) {
    SW_Memory_Free( vdl->line );
}