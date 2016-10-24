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

#include "vector3.h"

TVec3 Vec3_Clamp( TVec3 vec, float min, float max ) {  
    return (TVec3) { .x = ((vec.x < min) ? min : ((vec.x > max) ? max : vec.x )),
                     .y = ((vec.y < min) ? min : ((vec.y > max) ? max : vec.y )),
                     .z = ((vec.z < min) ? min : ((vec.z > max) ? max : vec.z ))};
}

TVec3 Vec3_Set( float x, float y, float z ) {
    return (TVec3) { .x = x, .y = y, .z = z };
}

TVec3 Vec3_Zero( void ) {
    return (TVec3) { .x = 0.0f, .y = 0.0f, .z = 0.0f };
}

TVec3 Vec3_Add( TVec3 a, TVec3 b ) {
    return (TVec3) { .x = a.x + b.x, .y = a.y + b.y, .z = a.z + b.z };
}

TVec3 Vec3_Sub( TVec3 a, TVec3 b ) {
    return (TVec3) { .x = a.x - b.x, .y = a.y - b.y, .z = a.z - b.z };
}

TVec3 Vec3_Mul( TVec3 a, TVec3 b ) {
    return (TVec3) { .x = a.x * b.x, .y = a.y * b.y, .z = a.z * b.z };
}

TVec3 Vec3_Div( TVec3 a, TVec3 b ) {
    return (TVec3) { .x = a.x / b.x, .y = a.y / b.y, .z = a.z / b.z };
}

TVec3 Vec3_Negate( TVec3 a ) {
    return (TVec3) { .x = -a.x, .y = -a.y, .z = -a.z };
}

TVec3 Vec3_Middle( TVec3 a, TVec3 b ) {
    return (TVec3) { .x = (a.x + b.x) * 0.5f, .y = (a.y + b.y) * 0.5f, .z = (a.z + b.z) * 0.5f };
}

TVec3 Vec3_Scale( TVec3 a, float scale ) {
    return (TVec3) { .x = a.x * scale, .y = a.y * scale, .z = a.z * scale };
}

TVec3 Vec3_Cross( TVec3 a, TVec3 b ) {
    return (TVec3) { .x = a.y * b.z - a.z * b.y, .y = a.z * b.x - a.x * b.z, .z = a.x * b.y - a.y * b.x };
}

TVec3 Vec3_Lerp( TVec3 a, TVec3 b, float t ) {
    return (TVec3) { .x = a.x + ( b.x - a.x ) * t, .y = a.y + ( b.y - a.y ) * t, .z = a.z + ( b.z - a.z ) * t };
}

TVec3 Vec3_Min( TVec3 a, TVec3 b ) {
    return ((a.x < b.x) || (a.y < b.y) || (a.z < b.z)) ? a : b;
}

TVec3 Vec3_Max( TVec3 a, TVec3 b ) {
    return ((a.x > b.x) || (a.y > b.y) || (a.z > b.z)) ? a : b;
}

float Vec3_Length( TVec3 a ) {
    return sqrt( a.x * a.x + a.y * a.y + a.z * a.z );
}

float Vec3_SqrLength( TVec3 a ) {
    return a.x * a.x + a.y * a.y + a.z * a.z;
}

float Vec3_SqrDistance( TVec3 a, TVec3 b ) {
    return Vec3_SqrLength( Vec3_Sub( a, b ) );
}

float Vec3_Distance( TVec3 a, TVec3 b ) {
    return Vec3_Length( Vec3_Sub( a, b ));
}

float Vec3_Dot( TVec3 a, TVec3 b ) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

float Vec3_Angle( TVec3 a, TVec3 b ) {
    return acos( Vec3_Dot( a, b ) / ( Vec3_Length( a ) * Vec3_Length( b )));
}

TVec3 Vec3_Normalize( TVec3 a ) {
    float length = Vec3_Length( a );
    return (length > 0.000001f) ? (TVec3) { .x = a.x / length, .y = a.y / length, .z = a.z / length } 
                                : (TVec3) { .x = a.x, .y = a.y, .z = a.z };
}

TVec3 Vec3_NormalizeEx( TVec3 a, float * len ) {
    float length = Vec3_Length( a );
    if( len ) *len = length;
    return (length > 0.000001f) ? (TVec3) { .x = a.x / length, .y = a.y / length, .z = a.z / length } 
                                : (TVec3) { .x = a.x, .y = a.y, .z = a.z }; 
}