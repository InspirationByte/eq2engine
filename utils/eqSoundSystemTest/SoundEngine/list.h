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

#ifndef _LIST_H_
#define _LIST_H_

#include "sndwrk.h"

SW_BEGIN_HEADER()

#define LIST_ADD( LISTTYPE, list, element )                     \
    if( !list ) {                                               \
        list = element;                                         \
    } else {                                                    \
        LISTTYPE* last = list;                              \
        while( last->next ) {                                   \
            last = last->next;                                  \
        }                                                       \
        if( last ) {                                            \
            last->next = element;                               \
            element->prev = last;                               \
        }                                                       \
    }                                                           \

#define LIST_ERASE( list, element )                             \
    if( element->prev ) element->prev->next = element->next;    \
    if( element->next ) element->next->prev = element->prev;    \
    if( element == list ) list = NULL;                          \
    
SW_END_HEADER()
    
#endif