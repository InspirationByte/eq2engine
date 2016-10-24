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

#ifndef _MIXER_H_
#define _MIXER_H_

#include "device.h"

SW_BEGIN_HEADER()

typedef struct SMixer TMixer;

TMixer * SW_Mixer_Create( TOutputDevice * dev );
void SW_Mixer_Start( TMixer * mix );
void SW_Mixer_Stop( TMixer * mix );
void SW_Mixer_Suspend( TMixer * mix );
void SW_Mixer_Process( TMixer * mix );
void SW_Mixer_Free( TMixer * mix );
bool SW_Mixer_IsActive( TMixer * mix );
void SW_Mixer_Update( TMixer* mix );

SW_END_HEADER()

#endif