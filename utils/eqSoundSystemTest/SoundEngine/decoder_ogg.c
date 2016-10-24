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
#include "memory.h"

bool SW_OGGDecoder_OpenFile( TDecoder * decoder, const char * filename ) {
    decoder->file = fopen( filename, "rb" );
    if( !decoder->file ) {
        Log_Write( "Unable to load %s file!", filename );
        return false;
    }
    if( ov_open_callbacks( decoder->file, &decoder->vorbisFile, 0, -1, OV_CALLBACKS_DEFAULT ) != 0 ) {
        fclose( decoder->file );
        Log_Write( "Invalid ogg file %s !", filename );
        return false;
    }
    vorbis_info * vi = ov_info( &decoder->vorbisFile, -1 );
    decoder->channelCount = vi->channels;
    decoder->frequency = vi->rate;
    decoder->fullSize = ov_pcm_total( &decoder->vorbisFile, -1 ) * 4;
    decoder->data = NULL;
    decoder->size = 0;
    decoder->eof = false;
    decoder->format = vi->channels == 2 ? SW_FORMAT_STEREO16 : SW_FORMAT_MONO16;
    return true;
}

void SW_OGGDecoder_DecodeBlock( TDecoder * decoder, int blockSize ) {
    if( !decoder->data ) {
        decoder->data = SW_Memory_Alloc( blockSize );
    }
    uint32_t totalBytesDecoded = 0;
    int32_t curSec = 0;
    while( totalBytesDecoded < blockSize ) {
        int32_t bytesDecoded = ov_read( &decoder->vorbisFile, (char*)( decoder->data + totalBytesDecoded ), blockSize - totalBytesDecoded, 0, 2, 1, &curSec );
        if( bytesDecoded <= 0 ) {
            decoder->eof = true;
            decoder->Rewind( decoder );
            break;
        }
        totalBytesDecoded += bytesDecoded;
    }
    decoder->size = totalBytesDecoded;
}

void SW_OGGDecoder_Clean( TDecoder * decoder ) {
    fclose( decoder->file );
    ov_clear( &decoder->vorbisFile );
    if( decoder->data ) {
        SW_Memory_Free( decoder->data );
    }
}

void SW_OGGDecoder_Rewind( TDecoder * decoder ) {
    decoder->eof = false;
    ov_time_seek( &decoder->vorbisFile, 0.0f );
}