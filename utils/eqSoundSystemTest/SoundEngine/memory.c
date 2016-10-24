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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <malloc.h>
#include "memory.h"
#include <string.h>
#include "log.h"

typedef struct SMemoryNode {
    void * data;
    int size;
    struct SMemoryNode * next;
    struct SMemoryNode * prev;
} TMemoryNode;


TMemoryNode * g_rootAllocNode = NULL;
TMemoryNode * g_lastAllocNode = NULL;
int totalAllocatedMemory = 0;

void * SW_Memory_CommonAllocation( int size, bool clear ) {
    void * block = malloc( size );
    if( !block ) {
        Log_Write( "Unable to allocate %d bytes. Not enough memory! Allocation failed!", size );
    }
    if( clear ) {
        memset( block, 0, size );
    }
    totalAllocatedMemory += size;
    TMemoryNode * newAllocNode = malloc( sizeof( TMemoryNode ));
    newAllocNode->data = block;
    newAllocNode->next = NULL;
    newAllocNode->size = size;
    newAllocNode->prev = g_lastAllocNode;
    if( g_lastAllocNode ) {
        g_lastAllocNode->next = newAllocNode;
    }
    if( !g_rootAllocNode ) {
        g_rootAllocNode = newAllocNode;
    }
    g_lastAllocNode = newAllocNode;
    return block;
}

void * SW_Memory_Alloc( int size ) {
    return SW_Memory_CommonAllocation( size, false );
}

void * SW_Memory_CleanAlloc( int size ) {
    return SW_Memory_CommonAllocation( size, true );
}

int SW_Memory_GetAllocated( void ) {
    return totalAllocatedMemory;
}

void * SW_Memory_Realloc( void * data, int newSize ) {
    TMemoryNode * current = g_rootAllocNode;
    void * newData = 0;
    while( current ) {
        if( current->data == data ) {
            newData = realloc( data, newSize );
            if( !newData ) {
                Log_Write( "Memory reallocation failed!" );
            }
            if( newData != data ) {
                current->data = newData;
            }
        }
        current = current->next;
    }
    return newData;
}

void SW_Memory_Free( void * data ) {
    TMemoryNode * current = g_rootAllocNode;
    while( current ) {
        if( current->data == data ) {
            if( current->next ) {
                current->next->prev = current->prev;
            }
            if( current->prev ) {
                current->prev->next = current->next;
            }
            if( current == g_rootAllocNode ) {
                if( current->next ) {
                    g_rootAllocNode = current->next;
                } else {
                    g_rootAllocNode = NULL;
                }
            }
            if( current == g_lastAllocNode ) {
                if( current->prev ) {
                    g_lastAllocNode = current->prev;
                } else {
                    g_lastAllocNode = NULL;
                }
            }
            totalAllocatedMemory -= current->size;
            free( current );
            free( data );
            return;
        }
        current = current->next;
    }
    Log_Write( "Attempt to free memory %p allocated without Memory_Allocate[Clean]", data );
}


void SW_Memory_FreeAll( ) {
    TMemoryNode *next, *current;
    for( current = g_rootAllocNode; current; current = next ) {
        next = current->next;
        totalAllocatedMemory -= current->size;
        free( current->data );
        free( current );
        current = next;
    }
}