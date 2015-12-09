//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Lightmap UV packer on OpenCL
//////////////////////////////////////////////////////////////////////////////////

#ifndef LIGHTMAP_UV_CL_H
#define LIGHTMAP_UV_CL_H

#include <CL/opencl.h>

extern bool g_cl_initialized;

void OpenCLInit();
void OpenCLRelease();

cl_mem		OpenCLCreateBuffer(int flags, int sizeInBytes, void* pSrc = NULL);
void		OpenCLDeleteBuffer(cl_mem buf);

void		OpenCLReadBuffer(cl_mem buf, void* pDst, int nReadSizeBytes);
void		OpenCLWriteBuffer(cl_mem buf, void* pSrc, int nWriteSizeBytes);

cl_kernel	OpenCLCreateKernelSrcFile(char* kernel_name, char* pszFileName);
void		OpenCLReleaseKernel(cl_kernel kern);

#endif // LIGHTMAP_UV_CL_H