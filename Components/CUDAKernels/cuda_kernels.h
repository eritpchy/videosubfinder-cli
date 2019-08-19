
#include <stdio.h>
#include "kmeans/kmeans.h"

int GetCUDADeviceCount();
int NV12_to_BGRA(unsigned char *src_y, unsigned char *src_uv, int src_linesize, unsigned char *dst_data, int w, int h, int W, int H);
void init_cuda_memory(int w, int h, int W, int H);
void release_cuda_memory();
