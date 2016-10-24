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

#ifndef _BUFFER_H_
#define _BUFFER_H_

#include "sndwrk.h"

SW_BEGIN_HEADER()

typedef enum EBufferType {
    SW_BUFFER_FULL,     /* load entire file in memory */
    SW_BUFFER_STREAM    /* load small piece of data in beginning, another and another when playing */
} EBufferType;

typedef struct SBuffer TBuffer;

/*
========================
SW_Buffer_IsBuffer
    - Checks pointer for validity
    - Widely used in other functions SW_Buffer_XXX, so there is no need to check pointer manually
========================
*/
bool SW_Buffer_IsBuffer( TBuffer * buf );

/*
========================
SW_Buffer_GetList
    - Gets first buffer in list, next element can be accessed through SW_Buffer_GetNext
========================
*/
TBuffer * SW_Buffer_GetList( void );

/*
========================
SW_Buffer_GetNext
========================
*/
TBuffer * SW_Buffer_GetNext( TBuffer * buf );

/*
========================
SW_Buffer_Create
    - Creates new empty buffer, with zero reference counter
    - 'streamed' useful for large files
========================
*/
TBuffer * SW_Buffer_Create( const char * fileName, EBufferType type );

/*
========================
SW_Buffer_FreeAll
    - Frees buffers that are do not belong to any sound (refCount <= 0)
    - To be complete sure that all buffers are freed, just free all sounds before calling this function
========================
*/
void SW_Buffer_FreeAll();

/*
========================
SW_Buffer_Free
    - Buffer can only be freed, if no one owns it (refCount <= 0)
========================
*/
void SW_Buffer_Free( TBuffer * buf );

/*
========================
SW_Buffer_GetSamples
    - As a side effect, increases refCounter of the buffer
    - This function is rather slow to use it in loop, to speed up things, save returned pointer and use it
    - To be sure, that returned pointer is valid, use SW_Buffer_IsBuffer to check validity
========================
*/
float * SW_Buffer_GetSamples( TBuffer * buf );

/*
========================
SW_Buffer_GetSampleCount
    - This function is rather slow to use it in loop, to speed up things, save returned value and use it
========================
*/
uint32_t SW_Buffer_GetSampleCount( TBuffer * buf );

/*
========================
SW_Buffer_GetChannelCount
    - 1 - mono
    - 2 - stereo
    - more channels can be supported in future
========================
*/
uint32_t SW_Buffer_GetChannelCount( TBuffer * buf );

/*
========================
SW_Buffer_GetFrequency
    - Possible frequencies 11025 Hz, 22050 Hz, 44100 Hz, 48000 Hz, 96000 Hz
========================
*/
uint32_t SW_Buffer_GetFrequency( TBuffer * buf );

/*
========================
SW_Buffer_GetType
========================
*/
EBufferType SW_Buffer_GetType( TBuffer * buf );

/*
========================
SW_Buffer_GetNextBlock
    - Reads next block from source file
    - For internal use only
========================
*/
float * SW_Buffer_GetNextBlock( TBuffer * buf );

SW_END_HEADER()

#endif