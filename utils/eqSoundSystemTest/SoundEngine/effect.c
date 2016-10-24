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

#include "list.h"
#include "dsp.h"
#include "memory.h"

typedef struct TReverb {
    float decayTime;
    float diffusion;
    
    struct {
        TDelayLine delay;
        int tap[4];
    } decorellator;
    
    struct {
        TOnepole inputLowpass;
        TDelayLine inputDelay;
        TDelayLine delay[4];
        float decay[4];
        float gain;
    } early;
    
    struct {
        TOnepole inputLowpass;
        TDelayLine inputDelay;
        TDelayLine delay[4];
        float decay[4];
        TOnepole lowpass[4];
        TAllpass allpass[4];
        float gain;
        float mix;
        float diffusion;
    } late;   
} TReverb;

struct SEffect {
    int type;
    struct SEffect * prev;
    struct SEffect * next;
    
    float gain;
    float accumulationSample;
    float inputSampleCount;
    
    TReverb reverb;
    
    bool enabled;
    
    void (*Process)( struct SEffect * effect, float * outLeft, float * outRight );
};

#include "effect.h"

TEffect * effectList;


const float SW_EARLY_LINE_LENGTH[4] = { 0.0015f, 0.0045f, 0.0135f, 0.0405f };
const float SW_ALLPASS_LINE_LENGTH[4] = { 0.0151f, 0.0167f, 0.0183f, 0.0200f };
const float SW_LATE_LINE_LENGTH[4] = { 0.0211f, 0.0311f, 0.0461f, 0.0680f };

/*
========================
SW_Effect_GetList
========================
*/
TEffect * SW_Effect_GetList() {
    return effectList;
}

/*
========================
SW_Effect_Next
========================
*/
TEffect * SW_Effect_Next( TEffect * effect ) {
    return effect->next;
}

/*
========================
SW_Effect_IsEffect
========================
*/
bool SW_Effect_IsEffect( TEffect * effect ) {
    TEffect * e;
    
    for( e = effectList; e; e = e->next ) {
        if( e == effect ) return true;
    }
    
    return false;
}

/*
========================
SW_Effect_CalculateDecay
========================
*/
float SW_Effect_CalculateDecay( float timeLength, float decayTime ) {
    return pow( 0.001, timeLength / decayTime );
}

/*
========================
SW_Reverb_SetDecayTime
========================
*/
void SW_Reverb_SetDecayTime( TEffect * effect, float time ) {
    int i;
    
    if( time < 0.01f ) {
        time = 0.01f;
    }
    
    effect->reverb.decayTime = time;
    
    for( i = 0; i < 4; i++ ) {
        effect->reverb.late.decay[i] = SW_Effect_CalculateDecay( SW_LATE_LINE_LENGTH[i], time );
        effect->reverb.early.decay[i] = SW_Effect_CalculateDecay( SW_EARLY_LINE_LENGTH[i], 0.03 );
        effect->reverb.late.allpass[i].gain = SW_Effect_CalculateDecay( SW_ALLPASS_LINE_LENGTH[i], time );
    }
    
}

/*
========================
SW_Reverb_GetDecayTime
========================
*/
float SW_Reverb_GetDecayTime( TEffect * effect ) {
    return effect->reverb.decayTime;
}

/*
========================
SW_Effect_CalcLateMixCoeff
========================
*/
float SW_Effect_CalcLateMixCoeff( float diffusion ) {
    float n = 1.73205; /* sqrt( 3 ) */
    float t = diffusion * 1.04719f; /* diffusion * atan( sqrt( 3 ) ) */

    return ( sin(t) / n ) / cos(t);
}

/*
========================
SW_Reverb_Setup
========================
*/
void SW_Reverb_Setup( TEffect * effect ) {  
    memset( &effect->reverb, 0, sizeof( effect->reverb ));
    
    /* decorellator */
    SW_DelayLine_Create( &effect->reverb.decorellator.delay, 0.1f );
    effect->reverb.decorellator.tap[0] = -SW_Convert_TimeToSampleNum( 0.02f );
    effect->reverb.decorellator.tap[1] = SW_Convert_TimeToSampleNum( 0.07 );
    effect->reverb.decorellator.tap[2] = SW_Convert_TimeToSampleNum( 0.04 );
    effect->reverb.decorellator.tap[3] = -SW_Convert_TimeToSampleNum( 0.1 );
    
    /* early */
    effect->reverb.early.gain = 0.15f;
    
    SW_DelayLine_Create( &effect->reverb.early.inputDelay, 0.05f );    
    SW_Onepole_Create( &effect->reverb.early.inputLowpass, 0.98f );
    SW_DelayLine_Create( &effect->reverb.early.delay[0], SW_EARLY_LINE_LENGTH[0] );
    SW_DelayLine_Create( &effect->reverb.early.delay[1], SW_EARLY_LINE_LENGTH[1] );
    SW_DelayLine_Create( &effect->reverb.early.delay[2], SW_EARLY_LINE_LENGTH[2] );
    SW_DelayLine_Create( &effect->reverb.early.delay[3], SW_EARLY_LINE_LENGTH[3] );
    
    /* late */
    effect->reverb.late.mix = SW_Effect_CalcLateMixCoeff( 0.2 );
    effect->reverb.late.gain = 0.5f;
    
    SW_DelayLine_Create( &effect->reverb.late.inputDelay, 0.075f );   
    SW_Onepole_Create( &effect->reverb.late.inputLowpass, 0.9f );    
    
    SW_Allpass_Create( &effect->reverb.late.allpass[0], SW_ALLPASS_LINE_LENGTH[0] );
    SW_Allpass_Create( &effect->reverb.late.allpass[1], SW_ALLPASS_LINE_LENGTH[1] );
    SW_Allpass_Create( &effect->reverb.late.allpass[2], SW_ALLPASS_LINE_LENGTH[2] );
    SW_Allpass_Create( &effect->reverb.late.allpass[3], SW_ALLPASS_LINE_LENGTH[3] );
    
    SW_DelayLine_Create( &effect->reverb.late.delay[0], SW_LATE_LINE_LENGTH[0] );
    SW_DelayLine_Create( &effect->reverb.late.delay[1], SW_LATE_LINE_LENGTH[1] );
    SW_DelayLine_Create( &effect->reverb.late.delay[2], SW_LATE_LINE_LENGTH[2] );
    SW_DelayLine_Create( &effect->reverb.late.delay[3], SW_LATE_LINE_LENGTH[3] );
    
    SW_Onepole_Create( &effect->reverb.late.lowpass[0], 0.9f );
    SW_Onepole_Create( &effect->reverb.late.lowpass[1], 0.9f );
    SW_Onepole_Create( &effect->reverb.late.lowpass[2], 0.9f );
    SW_Onepole_Create( &effect->reverb.late.lowpass[3], 0.9f );
    
    SW_Reverb_SetDecayTime( effect, 0.8f ); 
}

/*
========================
SW_Reverb_Free
========================
*/
void SW_Reverb_Free( TEffect * effect ) {    
    /* decorellator */
    SW_DelayLine_Free( &effect->reverb.decorellator.delay );
    
    /* early */
    SW_DelayLine_Free( &effect->reverb.early.inputDelay );
    SW_DelayLine_Free( &effect->reverb.early.delay[0] );
    SW_DelayLine_Free( &effect->reverb.early.delay[1] );
    SW_DelayLine_Free( &effect->reverb.early.delay[2] );
    SW_DelayLine_Free( &effect->reverb.early.delay[3] );
    
    /* late */
    SW_DelayLine_Free( &effect->reverb.late.inputDelay );    
    SW_DelayLine_Free( &effect->reverb.late.delay[0] );
    SW_DelayLine_Free( &effect->reverb.late.delay[1] );
    SW_DelayLine_Free( &effect->reverb.late.delay[2] );
    SW_DelayLine_Free( &effect->reverb.late.delay[3] );
    
    SW_Allpass_Free( &effect->reverb.late.allpass[0] );
    SW_Allpass_Free( &effect->reverb.late.allpass[1] );
    SW_Allpass_Free( &effect->reverb.late.allpass[2] );
    SW_Allpass_Free( &effect->reverb.late.allpass[3] );    
}

/*
========================
SW_Effect_ExtractAverageSample
    - Extracts average sample, discards accumulation sample
========================
*/
float SW_Effect_ExtractAverageSample( TEffect * effect ) {
    float average = 0.0f;
    if( effect->inputSampleCount > 0 ) {
        average = effect->accumulationSample / effect->inputSampleCount;
    }
    effect->accumulationSample = 0.0f;
    effect->inputSampleCount = 0.0f;
    return average;
}

/*
========================
SW_Effect_AddDrySample
========================
*/
void SW_Effect_AddDrySample( TEffect * effect, float sample ) {
    effect->accumulationSample += sample;
    effect->inputSampleCount += 1.0f;
}

/*
========================
SW_Reverb_SetLateDiffusion
========================
*/
void SW_Reverb_SetLateDiffusion( TEffect * effect, float diffusion )  {
    if( diffusion > 0.45 ) {
        diffusion = 0.45f;
    } 
    if( diffusion < 0.005f ) {
        diffusion = 0.005f;
    }
    float feedCoeff = 0.5f * diffusion * diffusion;
    effect->reverb.late.allpass[0].feed = feedCoeff;
    effect->reverb.late.allpass[1].feed = feedCoeff;
    effect->reverb.late.allpass[2].feed = feedCoeff;
    effect->reverb.late.allpass[3].feed = feedCoeff;
    
    effect->reverb.diffusion = diffusion;
    effect->reverb.late.mix = SW_Effect_CalcLateMixCoeff( diffusion );
}

/*
========================
SW_Reverb_GetLateDiffusion
========================
*/
float SW_Reverb_GetLateDiffusion( TEffect * effect ) {
    return effect->reverb.diffusion;
}

/*
========================
SW_Reverb_Process
========================
*/
void SW_Reverb_Process( TEffect * effect, float * outLeft, float * outRight ) {
    float sample = SW_Effect_ExtractAverageSample( effect );
    
    float earlyFiltered = SW_Onepole_Feed( &effect->reverb.early.inputLowpass, sample );
    float lateFiltered = SW_Onepole_Feed( &effect->reverb.late.inputLowpass, sample );
    
    float earlyInput = SW_DelayLine_Feed( &effect->reverb.early.inputDelay,earlyFiltered );
    float lateInput = SW_DelayLine_Feed( &effect->reverb.late.inputDelay, lateFiltered );
    
    /* early */
    float edOut[4];
    edOut[0] = effect->reverb.early.delay[0].last * effect->reverb.early.decay[0];
    edOut[1] = effect->reverb.early.delay[1].last * effect->reverb.early.decay[1];
    edOut[2] = effect->reverb.early.delay[2].last * effect->reverb.early.decay[2];
    edOut[3] = effect->reverb.early.delay[3].last * effect->reverb.early.decay[3];
    
    float vol = ( edOut[0] + edOut[1] + edOut[2] + edOut[3] ) * 0.5f + earlyInput;
    
    float feed[4];
    feed[0] = vol - edOut[0]; 
    feed[1] = vol - edOut[1];
    feed[2] = vol - edOut[2];
    feed[3] = vol - edOut[3];
    
    SW_DelayLine_Feed( &effect->reverb.early.delay[0], feed[0] );
    SW_DelayLine_Feed( &effect->reverb.early.delay[1], feed[1] );
    SW_DelayLine_Feed( &effect->reverb.early.delay[2], feed[2] );
    SW_DelayLine_Feed( &effect->reverb.early.delay[3], feed[3] );
    
    float earlyOut[4] = { 0 };

    earlyOut[0] = effect->reverb.early.gain * feed[0];
    earlyOut[1] = effect->reverb.early.gain * feed[1];
    earlyOut[2] = effect->reverb.early.gain * feed[2];
    earlyOut[3] = effect->reverb.early.gain * feed[3];
    
    /* late */
    float lateOut[4], f[4], d[4];
    SW_DelayLine_Feed( &effect->reverb.decorellator.delay, lateInput );
    
    f[0] = SW_DelayLine_TapOut( &effect->reverb.decorellator.delay, effect->reverb.decorellator.tap[0] );
    f[1] = SW_DelayLine_TapOut( &effect->reverb.decorellator.delay, effect->reverb.decorellator.tap[1] );
    f[2] = SW_DelayLine_TapOut( &effect->reverb.decorellator.delay, effect->reverb.decorellator.tap[2] );
    f[3] = SW_DelayLine_TapOut( &effect->reverb.decorellator.delay, effect->reverb.decorellator.tap[3] );
    
    f[0] += effect->reverb.late.delay[0].last * effect->reverb.late.decay[0];
    f[1] += effect->reverb.late.delay[1].last * effect->reverb.late.decay[1];
    f[2] += effect->reverb.late.delay[2].last * effect->reverb.late.decay[2];
    f[3] += effect->reverb.late.delay[3].last * effect->reverb.late.decay[3];

    d[0] = SW_Onepole_Feed( &effect->reverb.late.lowpass[2], f[2] );
    d[1] = SW_Onepole_Feed( &effect->reverb.late.lowpass[0], f[0] );
    d[2] = SW_Onepole_Feed( &effect->reverb.late.lowpass[3], f[3] );
    d[3] = SW_Onepole_Feed( &effect->reverb.late.lowpass[1], f[1] );

    d[0] = SW_Allpass_Feed( &effect->reverb.late.allpass[0], d[0] );
    d[1] = SW_Allpass_Feed( &effect->reverb.late.allpass[1], d[1] );
    d[2] = SW_Allpass_Feed( &effect->reverb.late.allpass[2], d[2] );
    d[3] = SW_Allpass_Feed( &effect->reverb.late.allpass[3], d[3] );
    
    f[0] = d[0] + effect->reverb.late.mix * ( d[1] - d[2] + d[3] );
    f[1] = d[1] + effect->reverb.late.mix * ( -d[0] + d[2] + d[3] );
    f[2] = d[2] + effect->reverb.late.mix * ( d[0] - d[1] + d[3] );
    f[3] = d[3] + effect->reverb.late.mix * ( -d[0] - d[1] - d[2] );

    lateOut[0] = effect->reverb.late.gain * f[0];
    lateOut[1] = effect->reverb.late.gain * f[1];
    lateOut[2] = effect->reverb.late.gain * f[2];
    lateOut[3] = effect->reverb.late.gain * f[3];
    
    SW_DelayLine_Feed( &effect->reverb.late.delay[0], f[0] );
    SW_DelayLine_Feed( &effect->reverb.late.delay[1], f[1] );
    SW_DelayLine_Feed( &effect->reverb.late.delay[2], f[2] );
    SW_DelayLine_Feed( &effect->reverb.late.delay[3], f[3] );
    
    /* mix */
    float h = lateOut[0] + lateOut[1] + lateOut[2] + lateOut[3] + earlyOut[0] + earlyOut[1] + earlyOut[2] + earlyOut[3];
    
    float left = h;
    float right = -h;
    
    *outLeft = left * effect->gain;
    *outRight = right * effect->gain;
}

/*
========================
SW_Effect_ProcessDummy
========================
*/
void SW_Effect_ProcessDummy( TEffect * effect, float * outLeft, float * outRight ) {
    float sample = SW_Effect_ExtractAverageSample( effect );
    *outLeft = sample * effect->gain;
    *outRight = sample * effect->gain;    
}

/*
========================
SW_Effect_Create
========================
*/
TEffect * SW_Effect_Create( EEffectType type ) { 
    TEffect * effect = SW_Memory_New( TEffect );
    LIST_ADD(TEffect, effectList, effect );
    
    effect->type = type;
    effect->gain = 0.3162f;
    effect->enabled = true;
    
    if( type == SW_EFFECT_REVERB ) {
        effect->Process = SW_Reverb_Process;
        SW_Reverb_Setup( effect );
    } else {
        effect->Process = SW_Effect_ProcessDummy;
        Log_Write( "WARNING: Effect created with undefined type! Using dummy processing function!" );
    } 
        
    return effect;
}

/*
========================
SW_Effect_SetGain
========================
*/
void SW_Effect_SetGain( TEffect * effect, float gain ) {
    if( SW_Effect_IsEffect( effect )) {
        effect->gain = gain;
    }
}

/*
========================
SW_Effect_GetGain
========================
*/
float SW_Effect_GetGain( TEffect * effect ) {
    if( SW_Effect_IsEffect( effect )) {
        return effect->gain;
    }
    return 0.0f;
}

/*
========================
SW_Effect_Process
========================
*/
void SW_Effect_Process( TEffect * effect, float * outLeft, float * outRight ) {
    effect->Process( effect, outLeft, outRight );
}

/*
========================
SW_Effect_Free
========================
*/
void SW_Effect_Free( TEffect * effect ) {
    if( effect->type == SW_EFFECT_REVERB ) {
        SW_Reverb_Free( effect );
    }
    LIST_ERASE( effectList, effect );
    SW_Memory_Free( effect );
}

/*
========================
SW_Effect_FreeAll
========================
*/
void SW_Effect_FreeAll( ) {
    if( effectList ) {
        TEffect * effect = effectList;
        while( effect ) {
            TEffect * next = effect->next;
            SW_Effect_Free( effect );            
            effect = next;
        }
    }
}

/*
========================
SW_Effect_SetEnabled
========================
*/
void SW_Effect_SetEnabled( TEffect * effect, bool state ) {
    effect->enabled = state;
}

/*
========================
SW_Effect_IsEnabled
========================
*/
bool SW_Effect_IsEnabled( TEffect * effect ) {
    return effect->enabled;
}
