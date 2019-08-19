
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

Npp8u * device_nv12[2] = { NULL, NULL };
Npp8u * device_yuv420[3] = { NULL, NULL, NULL };
Npp8u * device_data = NULL;
Npp8u * device_data_resized = NULL;

void init_cuda_memory(int w, int h, int W, int H)
{
	checkCuda(cudaMalloc(&device_nv12[0], W * H * sizeof(Npp8u)));
	checkCuda(cudaMalloc(&device_nv12[1], ((W * H) / 2) * sizeof(Npp8u)));

	checkCuda(cudaMalloc(&device_yuv420[0], (W * H) * sizeof(Npp8u)));
	checkCuda(cudaMalloc(&device_yuv420[1], ((W * H)/4) * sizeof(Npp8u)));
	checkCuda(cudaMalloc(&device_yuv420[2], ((W * H)/4) * sizeof(Npp8u)));

	checkCuda(cudaMalloc(&device_data, ((W * H) * 4) * sizeof(Npp8u)));

	if (w != W)
	{
		checkCuda(cudaMalloc(&device_data_resized, ((w * h) * 4) * sizeof(Npp8u)));
	}
	else
	{
		device_data_resized = NULL;
	}
}

void release_cuda_memory()
{
	checkCuda(cudaFree(device_nv12[0]));
	checkCuda(cudaFree(device_nv12[1]));
	
	checkCuda(cudaFree(device_yuv420[0]));
	checkCuda(cudaFree(device_yuv420[1]));
	checkCuda(cudaFree(device_yuv420[2]));

	checkCuda(cudaFree(device_data));

	if (device_data_resized != NULL)
	{
		checkCuda(cudaFree(device_data_resized));
	}
}

int NV12_to_BGRA(unsigned char *src_y, unsigned char *src_uv, int src_linesize, unsigned char *dst_data, int w, int h, int W, int H)
{
	NppStatus err;	
	int nSrcPitchCUDA, res = 0;

	checkCuda(cudaMemcpy(device_nv12[0], src_y,
		W * H * sizeof(Npp8u), cudaMemcpyHostToDevice));
	checkCuda(cudaMemcpy(device_nv12[1], src_uv,
		((W * H) / 2) * sizeof(Npp8u), cudaMemcpyHostToDevice));

	int aDstStep[3] = { W, W / 2, W / 2 };
	err = nppiNV12ToYUV420_8u_P2P3R(device_nv12, W, device_yuv420, aDstStep, NppiSize{ W, H });

	if (err == NPP_SUCCESS) {
		err = nppiYUV420ToBGR_8u_P3C4R(device_yuv420, aDstStep, device_data, (W * 4), NppiSize{ W, H });
	}

	if (err == NPP_SUCCESS) {
		if (w != W) {
			err = nppiResize_8u_C4R(device_data, (W * 4), NppiSize{ W, H }, NppiRect{ 0, 0,  W, H }, device_data_resized, (w * 4), NppiSize{ w, h }, NppiRect{ 0, 0,  w, h }, NPPI_INTER_LINEAR);
		}
	}

	if (err == NPP_SUCCESS){
		if (w != W) {
			checkCuda(cudaMemcpy(dst_data, device_data_resized,
				((w * h) * 4) * sizeof(Npp8u), cudaMemcpyDeviceToHost));
		}
		else
		{
			checkCuda(cudaMemcpy(dst_data, device_data,
				((w * h) * 4) * sizeof(Npp8u), cudaMemcpyDeviceToHost));
		}

		res = 1;
	}	
	
	return res;
}
