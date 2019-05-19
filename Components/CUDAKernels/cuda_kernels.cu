
#include "cuda_kernels.h"

int GetCUDADeviceCount()
{
	int count;
	cudaError_t error = cudaGetDeviceCount(&count);

	if (error == cudaSuccess)
	{
		return count;
	}
	else
	{
		return 0;
	}
}
