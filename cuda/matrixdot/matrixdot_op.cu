#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <iostream>
#include <random>

using FLOAT = double;
constexpr int g_mRowCount = 1000; // m*n
constexpr int g_nCount = 1000;
constexpr int g_kColCount = 1000; // n*k
constexpr int g_blockSize = 16;

__managed__ FLOAT matrix01[g_mRowCount * g_nCount];
__managed__ FLOAT matrix02[g_nCount * g_kColCount];
__managed__ FLOAT result_cpu[g_mRowCount * g_kColCount];
__managed__ FLOAT result_gpu[g_mRowCount * g_kColCount];

// 原理是将dx，dy分成很多子块加载进入共享内存
__global__ void matrix_gpu(FLOAT* dx, FLOAT* dy)
{
	__shared__ FLOAT step_x[g_blockSize][g_blockSize];
	__shared__ FLOAT step_y[g_blockSize][g_blockSize];

	// 全局索引
	int row = blockIdx.y * blockDim.y + threadIdx.y;
	int col = blockIdx.x * blockDim.x + threadIdx.x;

	int index = 0;
	FLOAT sum = 0.;
	for (int step = 0; step <= g_nCount / g_blockSize; ++step)// 分成几个子块累加，一个线程处理结果矩阵中的一个数据
	{
		int step_row = row;// 计算子块的行索引
		int step_col = step * g_blockSize + threadIdx.x; // 计算子块的列索引
		index = step_row * g_nCount + step_col;// dx中的索引
		if (step_row >= g_mRowCount || step_col >= g_nCount)
			step_x[threadIdx.y][threadIdx.x] = 0;
		else
			step_x[threadIdx.y][threadIdx.x] = dx[index];

		step_row = step * g_blockSize + threadIdx.y;
		step_col = col;
		index = step_row * g_kColCount + step_col;// dy中的索引
		if (step_row >= g_nCount || step_col >= g_kColCount)
			step_y[threadIdx.y][threadIdx.x] = 0;
		else
			step_y[threadIdx.y][threadIdx.x] = dy[index];

		__syncthreads();// 同步一下，保证数据写入共享内存了

		for (int i = 0; i < g_blockSize; ++i)
		{
			sum += step_x[threadIdx.y][i] * step_y[i][threadIdx.x];
		}
		__syncthreads();
	}

	if (row < g_mRowCount && col < g_kColCount)
		result_gpu[row * g_kColCount + col] = sum;
}

void matrix_cpu(FLOAT* hx, FLOAT* hy)
{
	for (int row = 0; row < g_mRowCount; ++row)
	{
		for (int col = 0; col < g_kColCount; ++col)
		{
			FLOAT tmp = 0.;
			for (int step = 0; step < g_nCount; ++step)
			{
				tmp += hx[row * g_nCount + step] * hy[step * g_kColCount + col];
			}
			result_cpu[row * g_kColCount + col] = tmp;
		}
	}
}

int main()
{
	// 初始化随机数引擎
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(1, 100);

	for (int row = 0; row < g_mRowCount; ++row)
	{
		for (int col = 0; col < g_nCount; ++col)
		{
			matrix01[row * g_nCount + col] = dis(gen);
		}
	}

	for (int row = 0; row < g_nCount; ++row)
	{
		for (int col = 0; col < g_kColCount; ++col)
		{
			matrix02[row * g_kColCount + col] = dis(gen);
		}
	}

	int gridRows = (g_mRowCount + g_blockSize - 1) / g_blockSize;
	int gridCols = (g_kColCount + g_blockSize - 1) / g_blockSize;

	dim3 grid(gridCols, gridRows);
	dim3 block(g_blockSize, g_blockSize);

	matrix_cpu(matrix01, matrix02);

	matrix_gpu<<<grid, block>>>(matrix01, matrix02);
	cudaDeviceSynchronize();// 等待gpu完成

	// 验证结果
	bool error = true;
	for (int row = 0; row < g_mRowCount; ++row)
	{
		for (int col = 0; col < g_kColCount; ++col)
		{
			if (fabs(result_cpu[row * g_kColCount + col] - result_gpu[row * g_kColCount + col]) > (1.0e-10))
			{
				error = false;
				break;
			}
		}

		if (!error)
			break;
	}

	if (error)
		std::cout << "error = true" << "\n";
	else
		std::cout << "error = false" << "\n";

	return 0;
}