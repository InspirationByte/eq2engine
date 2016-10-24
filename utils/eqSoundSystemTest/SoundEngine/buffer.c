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

#include "decoder_ogg.h"
#include "list.h"
#include "memory.h"
#include "buffer.h"

struct SBuffer {
    TDecoder decoder;
    float * samples;
    int sampleCount;
    int channelCount;
    int frequency;
    int refCount;
    int blockNum; 
    EBufferType type;
    struct SBuffer * next;
    struct SBuffer * prev;
};

TBuffer * bufferList;

#define SW_BUFFER_STREAM_HALF_SIZE (32768)

/*
========================
SW_Buffer_IsBuffer
========================
*/
bool SW_Buffer_IsBuffer( TBuffer * buf ) {
    TBuffer * b;
    
    for( b = bufferList; b; b = b->next ) {
        if( b == buf ) return true;
    }
    
    return false;
}

/*
========================
SW_Buffer_GetList
========================
*/
TBuffer * SW_Buffer_GetList() {
    return bufferList;    
}

/*
========================
SW_Buffer_ProcessDecodedData
    - Converts raw integer samples to IEEE floats
    - Must be called after SW_Buffer_DecodeNextBlock
========================
*/
void SW_Buffer_ProcessDecodedData( TBuffer * buf, int count, float * dest ) {
    int i, k, index;
    
    if( SW_Buffer_IsBuffer( buf )) {
        /* convert samples from 16-bit int to IEEE float */
        int decodedSampleCount = buf->decoder.size / sizeof( int16_t );
        for( i = 0; i < count; i += buf->decoder.channelCount ) {
            for( k = 0; k < buf->decoder.channelCount; k++ ) {
                index = i + k;
                if( index < decodedSampleCount ) {
                    dest[i+k] = ((int16_t*)buf->decoder.data)[i+k];                    
                } else {
                    dest[i+k] = 0;
                }
            }    
        }
        buf->frequency = buf->decoder.frequency;
        buf->channelCount = buf->decoder.channelCount;
    }
}

/*
========================
SW_Buffer_Create
========================
*/
TBuffer * SW_Buffer_Create( const char * fileName, EBufferType type ) {    
    TBuffer * buf = SW_Memory_New( TBuffer );

    LIST_ADD(TBuffer, bufferList, buf );
    
    buf->type = type;
    buf->refCount = 0;
    /* -1 indicates that at first time, buffer requiers two blocks when doing streaming */
    buf->blockNum = -1;
    
    /* select decoder */
    bool decoderPresent = false;
    if( strstr( fileName, ".ogg" )) {
        buf->decoder.Open = SW_OGGDecoder_OpenFile;
        buf->decoder.Clean = SW_OGGDecoder_Clean;
        buf->decoder.Rewind = SW_OGGDecoder_Rewind;
        buf->decoder.Decode = SW_OGGDecoder_DecodeBlock;
        
        buf->decoder.Open( &buf->decoder, fileName );
        
        decoderPresent = true;
    } else {
        Log_Write( "%s is not a valid file!", fileName );
        
        /* in case of error, remove buffer from list and free memory */
        LIST_ERASE( bufferList, buf );
        SW_Memory_Free( buf );
        buf = NULL;
    }   
    
    if( decoderPresent ) {
        /* begin decoding */
        if( buf->type == SW_BUFFER_FULL ) {
            /* decode entire file */
            buf->decoder.Decode( &buf->decoder, buf->decoder.fullSize );        
            /* allocate samples buffer */
            buf->sampleCount = buf->decoder.size / sizeof( int16_t );
            buf->samples = SW_Memory_Alloc( buf->sampleCount * sizeof( float ));
            /* convert and copy decoded data */
            SW_Buffer_ProcessDecodedData( buf, buf->sampleCount, buf->samples );        
            buf->decoder.Clean( &buf->decoder );   
        } else if ( buf->type == SW_BUFFER_STREAM ) {
            /* allocate samples buffer */
            buf->sampleCount = SW_BUFFER_STREAM_HALF_SIZE / sizeof( int16_t );
            buf->samples = SW_Memory_Alloc( 2 * buf->sampleCount * sizeof( float ));
            SW_Buffer_GetNextBlock( buf );
        }
    } 
    
    return buf;
}

/*
========================
SW_Buffer_GetNextBlock
========================
*/
float * SW_Buffer_GetNextBlock( TBuffer * buf ) {
    if( buf->blockNum == -1 ) {
        /* decode first block */
        buf->decoder.Decode( &buf->decoder, SW_BUFFER_STREAM_HALF_SIZE );        
        SW_Buffer_ProcessDecodedData( buf, buf->sampleCount, buf->samples );   
        /* decode second block */
        buf->decoder.Decode( &buf->decoder, SW_BUFFER_STREAM_HALF_SIZE );        
        SW_Buffer_ProcessDecodedData( buf, buf->sampleCount, buf->samples + buf->sampleCount ); 
        buf->blockNum = 0;
        return buf->samples;
    } 
    if( buf->blockNum == 0 ) {
        /* decode first block */
        buf->decoder.Decode( &buf->decoder, SW_BUFFER_STREAM_HALF_SIZE );        
        SW_Buffer_ProcessDecodedData( buf, buf->sampleCount, buf->samples ); 
        buf->blockNum = 1;
        return buf->samples + buf->sampleCount;        
    } 
    if( buf->blockNum == 1 ) {
        /* decode second block */
        buf->decoder.Decode( &buf->decoder, SW_BUFFER_STREAM_HALF_SIZE );        
        SW_Buffer_ProcessDecodedData( buf, buf->sampleCount, buf->samples + buf->sampleCount );   
        buf->blockNum = 0;
        return buf->samples;
    }
    /* should never happen */
    return NULL;
}

/*
========================
SW_Buffer_RawFree
========================
*/
void SW_Buffer_RawFree( TBuffer * buf ) {            
    SW_Memory_Free( buf->samples );
    SW_Memory_Free( buf );
}

/*
========================
SW_Buffer_Free
========================
*/
void SW_Buffer_Free( TBuffer * buf ) {
    if( SW_Buffer_IsBuffer( buf )) {
        --buf->refCount;
        if( buf->refCount <= 0 ) {
            LIST_ERASE( bufferList, buf );
            SW_Buffer_RawFree( buf );
        }
    }
}

/*
========================
SW_Buffer_FreeAll
========================
*/
void SW_Buffer_FreeAll() {
    if( bufferList ) {
        TBuffer * buf = bufferList;
        while( buf ) {
            TBuffer * next = buf->next;
            SW_Buffer_RawFree( buf );
            buf = next;
        }
        bufferList = NULL;
    }
}


/*
========================
SW_Buffer_GetSamples
========================
*/
float * SW_Buffer_GetSamples( TBuffer * buf ) {
    if( SW_Buffer_IsBuffer( buf )) { 
        ++buf->refCount;
        return buf->samples;
    } 
    return NULL;
}

/*
========================
SW_Buffer_GetSampleCount
========================
*/
uint32_t SW_Buffer_GetSampleCount( TBuffer * buf ) {
    if( SW_Buffer_IsBuffer( buf )) { 
        return buf->sampleCount;
    } 
    return 0;
}

/*
========================
SW_Buffer_GetChannelCount
========================
*/
uint32_t SW_Buffer_GetChannelCount( TBuffer * buf ) {
    if( SW_Buffer_IsBuffer( buf )) { 
        return buf->channelCount;
    }
    return 0;
}

/*
========================
SW_Buffer_GetFrequency
========================
*/
uint32_t SW_Buffer_GetFrequency( TBuffer * buf ) {
    if( SW_Buffer_IsBuffer( buf )) { 
        return buf->frequency;
    }
    return 0;
}

/*
========================
SW_Buffer_GetType
========================
*/
EBufferType SW_Buffer_GetType( TBuffer * buf ) {
    return buf->type;
}