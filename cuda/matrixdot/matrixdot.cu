#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <iostream>
#include <random>

using FLOAT = double;
constexpr int g_rowCount = 1000;
constexpr int g_colCount = 1000;
constexpr int g_blockSize = 16;

// 矩阵相乘的关键点就是找到结果矩阵对应元素与相乘矩阵元素之间的关系
// cuda中传入的dim3参数的含义是列行

void hostMatrixDot(FLOAT* hx, FLOAT* hy, FLOAT* hr, int size)
{
    for (int row = 0; row < g_rowCount; ++row)
    {
        for (int col = 0; col < g_colCount; ++col)
        {
            FLOAT sums = 0.;
            for (int step = 0; step < size; ++step)
            {
                sums += hx[row * size + step] * hy[size * step + col];
            }
            hr[row * size + col] = sums;
        }
    }
}

__global__ void matrixDot(FLOAT* dx, FLOAT* dy, FLOAT* dr, int size)
{
    int row = g_blockSize * blockIdx.y + threadIdx.y;
    int col = g_blockSize * blockIdx.x + threadIdx.x;
    FLOAT sums = 0.;
    if (row < size && col < size)
    {
        for (int step = 0; step < size; ++step)
        {
            sums += dx[row * size + step] * dy[size * step + col];
        }
        dr[row * size + col] = sums;
    }
}

int main()
{
    int dataSize = g_rowCount * g_colCount;
    int nbytes = dataSize * sizeof(FLOAT);

    // 定义并分配一维数组
    FLOAT* hx = new FLOAT[dataSize];
    FLOAT* hy = new FLOAT[dataSize];
    FLOAT* hr = new FLOAT[dataSize];
    FLOAT* hrr = new FLOAT[dataSize];
    FLOAT* dx = nullptr;
    FLOAT* dy = nullptr;
    FLOAT* dr = nullptr;

    // 初始化随机数引擎
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 100);

    // 初始化矩阵乘积的数组，以行主序方式填充一维数组
    for (int row = 0; row < g_rowCount; ++row)
    {
        for (int col = 0; col < g_colCount; ++col) 
        {
            hx[row * g_colCount + col] = dis(gen);
            hy[row * g_colCount + col] = dis(gen);
        }
    }

    // 在设备上分配一维数组
    cudaMalloc((void**)&dx, nbytes);
    cudaMalloc((void**)&dy, nbytes);
    cudaMalloc((void**)&dr, nbytes);
    if (!dx || !dy || !dr)
        return 0;

    // 将主机数据传输到设备
    cudaMemcpy(dx, hx, nbytes, cudaMemcpyHostToDevice);
    cudaMemcpy(dy, hy, nbytes, cudaMemcpyHostToDevice);

    // 阻塞，直至gpu返回
    cudaDeviceSynchronize();

    int gridRows = (g_rowCount + g_blockSize - 1) / g_blockSize;
    int gridCols = (g_colCount + g_blockSize - 1) / g_blockSize;

    dim3 grid(gridCols, gridRows);
    dim3 block(g_blockSize, g_blockSize);

    matrixDot << <grid, block>> > (dx, dy, dr, g_colCount);

    // 阻塞，直至gpu返回
    cudaDeviceSynchronize();

    // 返回结果
    cudaMemcpy(hrr, dr, nbytes, cudaMemcpyDeviceToHost);

    hostMatrixDot(hx, hy, hr, g_colCount);

    // 验证结果
    bool error = true;
    for (int row = 0; row < g_rowCount; ++row)
    {
        for (int col = 0; col < g_colCount; ++col)
        {
            if (fabs(hr[row * g_colCount + col] - hrr[row * g_colCount + col]) > (1.0e-10))
            {
                error = false;
                break;
            }  
        }

        if (!error)
            break;
    }

    cudaFree(dx);
    cudaFree(dy);
    cudaFree(dr);

    delete[] hx;
    delete[] hy;
    delete[] hr;
    delete[] hrr;

    if (error)
        std::cout << "error = true" << "\n";
    else
        std::cout << "error = false" << "\n";

	return 0;
}
