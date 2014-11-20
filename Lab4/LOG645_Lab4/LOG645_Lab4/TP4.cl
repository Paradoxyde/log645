__kernel void HeatTransfer(__global float* previousMatrix, __global float* nextMatrix, int rowCount, int columnCount, int kernelCount, int kernelId, float td, float h)
{
	int matrixSize = rowCount * columnCount;

	int i = 1;
	int j = 1 + kernelId;
	for (; i < rowCount - 1;)
	{
		for (; j < columnCount - 1; j += kernelCount)
		{
			nextMatrix[i * columnCount + j] = (1 - 4 * td / (h * h)) * previousMatrix[i * columnCount + j] + (td / (h * h)) * (previousMatrix[(i - 1) * columnCount + j] + previousMatrix[(i + 1) * columnCount + j] + previousMatrix[i * columnCount + (j - 1)] + previousMatrix[i * columnCount + (j + 1)]);
		}
		while (j >= columnCount - 1)
		{
			j -= columnCount - 2;
			i++;
		}
	}
}
