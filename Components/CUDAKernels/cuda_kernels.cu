
#include <nppi.h>
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

int NV21_to_BGRA(unsigned char *src_y, unsigned char *src_uv, int src_linesize,
	unsigned char *dst_data, int w, int h)
{
	NppStatus err;
	Npp8u * pSrc[2];
	int nSrcPitchCUDA;
	pSrc[0] = src_y; // nppiMalloc_8u_C1(w, h, &nSrcPitchCUDA);
	pSrc[1] = src_uv;
	/*
	Npp8u *pSrcImageCUDA 
	NPP_ASSERT_NOT_NULL(pSrcImageCUDA);
	// copy image loaded via FreeImage to into CUDA device memory, i.e.
	// transfer the image-data up to the GPU's video-memory
	NPP_CHECK_CUDA(cudaMemcpy2D(pSrcImageCUDA, nSrcPitchCUDA, pSrcData, nSrcPitch,
		nImageWidth, nImageHeight, cudaMemcpyHostToDevice));
		*/
	err = nppiNV21ToBGR_8u_P2C4R(pSrc, src_linesize,
								dst_data, w*4,
								NppiSize{w, h});
	if (err != NPP_SUCCESS){		    
		return -1;
	}
	
	return 0;
}
