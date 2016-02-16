//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Lightmap UV packer on OpenCL
//////////////////////////////////////////////////////////////////////////////////

#include "lightmap_uv_cl.h"
#include "DebugInterface.h"

cl_int error = 0;   // Used to handle error codes
cl_platform_id platform = NULL;
cl_context context;
cl_command_queue queue;
cl_device_id device;
bool g_cl_initialized = false;

void OpenCLInit()
{
	// Platform
	cl_uint numPlatforms = 0;
	error = clGetPlatformIDs(0, NULL, &numPlatforms);

	if (error != CL_SUCCESS)
	{
		Msg("clGetPlatformIDs failed (%d)\n", error);
		return;
	}

    if(numPlatforms) 
    {
        cl_platform_id* platforms = new cl_platform_id[numPlatforms];
        error = clGetPlatformIDs(numPlatforms, platforms, NULL);
		if (error != CL_SUCCESS)
		{
			Msg("clGetPlatformIDs second failed (%d)\n", error);
			return;
		}

        char platformName[100];
        for (unsigned i = 0; i < numPlatforms; ++i) 
        {
            error = clGetPlatformInfo(platforms[i],
                                       CL_PLATFORM_VENDOR,
                                       sizeof(platformName),
                                       platformName,
                                       NULL);

			if (error != CL_SUCCESS)
			{
				Msg("clGetPlatformInfo failed (%d)\n", error);
				return;
			}

            platform = platforms[i];
			Msg("ClPlatform: %s\n", platformName);

			if (!strcmp(platformName, "Advanced Micro Devices, Inc.")) 
                break;
        }

        delete[] platforms;
    }

	// Device
	error = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);

	if (error != CL_SUCCESS)
	{
		Msg("clGetDeviceIDs failed (%d)\n", error);
		return;
	}

	// Context
	context = clCreateContext(0, 1, &device, NULL, NULL, &error);
	if (error != CL_SUCCESS)
	{
		Msg("clCreateContext failed (%d)\n", error);
		return;
	}

	// Command-queue
	queue = clCreateCommandQueue(context, device, 0, &error);
	if (error != CL_SUCCESS)
	{
		Msg("clCreateCommandQueue failed (%d)\n", error);
		return;
	}

	g_cl_initialized = true;
}

void OpenCLRelease()
{
	clReleaseCommandQueue(queue);
	clReleaseContext(context);
}

cl_mem OpenCLCreateBuffer(int flags, int sizeInBytes, void* pSrc)
{
	cl_mem memPtrCL = clCreateBuffer(context, flags | (pSrc ? CL_MEM_COPY_HOST_PTR : 0), sizeInBytes, pSrc, &error);

	return memPtrCL;
}

void OpenCLReadBuffer(cl_mem buf, void* pDst, int nReadSizeBytes)
{
	clEnqueueReadBuffer(queue, buf, CL_TRUE, 0, nReadSizeBytes, pDst, 0, NULL, NULL);
}

void OpenCLWriteBuffer(cl_mem buf, void* pSrc, int nWriteSizeBytes)
{
	clEnqueueWriteBuffer(queue, buf, CL_TRUE, 0, nWriteSizeBytes, pSrc, 0, NULL, NULL);
}

void OpenCLDeleteBuffer(cl_mem buf)
{
	clReleaseMemObject(buf);
}

cl_kernel OpenCLCreateKernelSrcFile(char* kernel_name, char* pszFileName)
{
	size_t src_size = 0;
	long size = 0;
	const char* source = (char*)g_fileSystem->GetFileBuffer(pszFileName, &size);

	src_size = size;

	cl_program program = clCreateProgramWithSource(context, 1, &source, &src_size, &error);
	ASSERT(error == CL_SUCCESS);

	// Builds the program
	error = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
	ASSERT(error == CL_SUCCESS);

	// Shows the log
	char* build_log;
	size_t log_size;
	// First call to know the proper size
	clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);

	build_log = new char[log_size+1];
	// Second call to get the log
	clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, build_log, NULL);
	build_log[log_size] = '\0';

	Msg("%s\n", build_log);

	delete[] build_log;

	// Extracting the kernel
	cl_kernel kernel = clCreateKernel(program, kernel_name, &error);

	ASSERT(error == CL_SUCCESS);

	return kernel;
}

void OpenCLReleaseKernel(cl_kernel kern)
{
	clReleaseKernel(kern);
}