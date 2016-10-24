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

#ifndef _VECTOR3_
#define _VECTOR3_

#include "sndwrk.h"

SW_BEGIN_HEADER()

typedef struct TVec3 {
    float x, y, z;
} TVec3;

TVec3 Vec3_Clamp( TVec3 vec, float min, float max ) ;
TVec3 Vec3_Set( float x, float y, float z );
TVec3 Vec3_Zero( void );
TVec3 Vec3_Add( TVec3 a, TVec3 b );
TVec3 Vec3_Sub( TVec3 a, TVec3 b );
TVec3 Vec3_Mul( TVec3 a, TVec3 b );
TVec3 Vec3_Div( TVec3 a, TVec3 b );
TVec3 Vec3_Negate( TVec3 a );
TVec3 Vec3_Middle( TVec3 a, TVec3 b );
TVec3 Vec3_Scale( TVec3 a, float scale );
TVec3 Vec3_Cross( TVec3 a, TVec3 b );
TVec3 Vec3_Lerp( TVec3 a, TVec3 b, float t );
TVec3 Vec3_Min( TVec3 a, TVec3 b );
TVec3 Vec3_Max( TVec3 a, TVec3 b );
float Vec3_Length( TVec3 a );
float Vec3_SqrLength( TVec3 a );
float Vec3_SqrDistance( TVec3 a, TVec3 b );
float Vec3_Distance( TVec3 a, TVec3 b );
float Vec3_Dot( TVec3 a, TVec3 b );
float Vec3_Angle( TVec3 a, TVec3 b );
TVec3 Vec3_Normalize( TVec3 a );
TVec3 Vec3_NormalizeEx( TVec3 a, float * len );

SW_END_HEADER()

#endif