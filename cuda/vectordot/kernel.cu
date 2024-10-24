#include <iostream>
#include <random>

#include "cuda_runtime.h"
#include "device_launch_parameters.h"

// 适用于一维块和二维网格
#define get_tid() (blockDim.x * (blockIdx.x + blockIdx.y * gridDim.x) + threadIdx.x) // 线程全局索引
#define get_bid() (blockIdx.x + blockIdx.y * gridDim.x) // 块全局索引

using FLOAT = double;

// 每个线程块中线程的数量
constexpr int threadPerBlockNums = 256;

__device__ void  warpReduce(volatile FLOAT* sdata, int tid)
{
    // 同一个线程束一次性计算完，该线程块最终的计算结果为sdata[0]
    sdata[tid] += sdata[tid + 32];
    sdata[tid] += sdata[tid + 16];
    sdata[tid] += sdata[tid + 8];
    sdata[tid] += sdata[tid + 4];
    sdata[tid] += sdata[tid + 2];
    sdata[tid] += sdata[tid + 1];
}

// 每个块处理256个数据，结果存在对应的块索引中
__global__ void dotStage1(FLOAT* dx, FLOAT* dy, FLOAT* dz, int size)
{
    // 每个块处理256个数据
    __shared__ FLOAT sdatas[threadPerBlockNums];

    int globleThreadid = get_tid();
    int blockThreadid = threadIdx.x;
    int blockid = get_bid();

    if (globleThreadid < size)
        sdatas[blockThreadid] = dx[globleThreadid] * dy[globleThreadid];
    else
        sdatas[blockThreadid] = 0.f;

    // 块内同步，保证sdatas已经被当前线程块设置好
    __syncthreads();

    if (blockThreadid < 128)
        sdatas[blockThreadid] += sdatas[blockThreadid + 128];

    __syncthreads();

    if (blockThreadid < 64)
        sdatas[blockThreadid] += sdatas[blockThreadid + 64];

    __syncthreads();

    if (blockThreadid < 32)
        warpReduce(sdatas, blockThreadid);

    __syncthreads();

    if (blockThreadid == 0)
        dz[blockid] = sdatas[0];
}

__global__ void dotStage2(FLOAT* dx, FLOAT* d, int size)
{
    // 每个块处理256个数据
    __shared__ FLOAT sdatas[threadPerBlockNums];

    int globleThreadid = get_tid();
    int blockThreadid = threadIdx.x;
    int blockid = get_bid();

    if (globleThreadid < size)
        sdatas[blockThreadid] = dx[globleThreadid];
    else
        sdatas[blockThreadid] = 0.f;

    // 块内同步，保证sdatas已经被当前线程块设置好
    __syncthreads();

    if (blockThreadid < 128)
        sdatas[blockThreadid] += sdatas[blockThreadid + 128];

    __syncthreads();

    if (blockThreadid < 64)
        sdatas[blockThreadid] += sdatas[blockThreadid + 64];

    __syncthreads();

    if (blockThreadid < 32)
        warpReduce(sdatas, blockThreadid);

    __syncthreads();

    if (blockThreadid == 0)
        d[blockid] = sdatas[0];
}

__global__ void dotStage3(FLOAT* d, int size)
{
    // 每个块处理256个数据
    __shared__ FLOAT sdatas[threadPerBlockNums];
    int blockThreadid = threadIdx.x;

    if (blockThreadid < size)
        sdatas[blockThreadid] = d[blockThreadid];
    else
        sdatas[blockThreadid] = 0.f;

    __syncthreads();

    if (blockThreadid < 128)
        sdatas[blockThreadid] += sdatas[blockThreadid + 128];

    __syncthreads();

    if (blockThreadid < 64)
        sdatas[blockThreadid] += sdatas[blockThreadid + 64];

    __syncthreads();

    if (blockThreadid < 32)
        warpReduce(sdatas, blockThreadid);

    __syncthreads();

    if (blockThreadid == 0)
        d[0] = sdatas[0];
}

void dotProduct(FLOAT* dx, FLOAT* dy, FLOAT* dz, FLOAT* d, int size)
{
    /*第一阶段，完成乘法的计算，转为blockNums大小的数组求和的计算*/
    // 计算二维grid的大小
     // 线程块的数量
    int blockNums = (size + threadPerBlockNums - 1) / threadPerBlockNums;
    int nums = (int)ceil(sqrt(blockNums));
    dim3 grid = dim3(nums, nums);

    dotStage1<<<grid, threadPerBlockNums>>>(dx, dy, dz, size);

    /*第二阶段(可以循环计算的，直到数据量级为一个块的大小)，将blockNums大小的数组长度再减少threadPerBlockNums倍*/
     // 线程块的数量
    int blockNums2 = (blockNums + threadPerBlockNums - 1) / threadPerBlockNums;
    nums = (int)ceil(sqrt(blockNums2));
    grid = dim3(nums, nums);

    dotStage2<<<grid, threadPerBlockNums>>>(dz, d, blockNums);

    /*第三阶段，将blockNums2大小的数组长度再减少threadPerBlockNums倍*/
    dotStage3 <<<1, threadPerBlockNums >>>(d, blockNums2);
}

int main()
{
    int size = 10000070;
    int nbytes = size * sizeof(FLOAT);

    FLOAT* hx = nullptr, * hy = nullptr;
    FLOAT* dx = nullptr, * dy = nullptr, * dz = nullptr, * d = nullptr;

    // 分配cpu的内存
    hx = new FLOAT[size];
    hy = new FLOAT[size];

    // 初始化随机数引擎（1-100）
    std::random_device rd; // 用于生成随机种子
    std::mt19937 gen(rd()); // 随机数引擎
    std::uniform_int_distribution<> dis(1, 100); // 生成 1 到 100 的随机整数

    // 初始化内积的数组
    for (size_t index = 0; index < size; ++index)
    {
        hx[index] = (FLOAT)dis(gen);
        hy[index] = (FLOAT)dis(gen);
    }

    // 分配GPU的内存，加上255是为了不漏掉任何数据
    cudaMalloc((void**)&dx, nbytes);
    cudaMalloc((void**)&dy, nbytes);
    cudaMalloc((void**)&dz, sizeof(FLOAT) * ((size + 255) / 256));
    cudaMalloc((void**)&d, sizeof(FLOAT) * ((size + 255) / 256));
    if (!dx || !dy || !dz || !d)
        return 0;

    // 传输数据
    cudaMemcpy(dx, hx, nbytes, cudaMemcpyHostToDevice);
    cudaMemcpy(dy, hy, nbytes, cudaMemcpyHostToDevice);

    // 阻塞，直至gpu返回
    cudaDeviceSynchronize();

    // 调用核函数
    dotProduct(dx, dy, dz, d, size);

    // 阻塞，直至gpu返回
    cudaDeviceSynchronize();

    FLOAT result = 0.f;
    cudaMemcpy(&result, d, sizeof(FLOAT), cudaMemcpyDeviceToHost);

    std::cout << result << "\n";

    // 使用cpu计算一下，验证结果是否正确
    result = 0.f;
    for (int i = 0; i < size; ++i)
    {
        result += hx[i] * hy[i];
    }

    std::cout << result << "\n";
}