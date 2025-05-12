#pragma once

#include <webgpu/webgpu.h>

#ifdef WGPU_BREAKING_CHANGE_STRING_VIEW_LABELS
#define _WSTR(x) {(x), WGPU_STRLEN}
#else
#define _WSTR(x) (x)
#endif