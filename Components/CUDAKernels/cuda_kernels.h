
#include <stdio.h>
#include "kmeans/kmeans.h"

int GetCUDADeviceCount();
int NV12_to_BGR(unsigned char *src_y, unsigned char *src_uv, int src_linesize, unsigned char *dst_data, int w, int h, int W, int H);
