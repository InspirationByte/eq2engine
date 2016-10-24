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

#ifndef _MEMORY_
#define _MEMORY_


void * SW_Memory_Alloc( int size );
void * SW_Memory_CleanAlloc( int size );
void * SW_Memory_Realloc( void * data, int newSize );
void SW_Memory_Free( void * data );
void SW_Memory_FreeAll( void );
int SW_Memory_GetAllocated( void );

#define SW_Memory_New( typeName ) ((typeName*)SW_Memory_CleanAlloc( sizeof(typeName )))
#define SW_Memory_NewCount( count, typeName ) ((typeName*)SW_Memory_CleanAlloc( count * sizeof(typeName )))

#endif