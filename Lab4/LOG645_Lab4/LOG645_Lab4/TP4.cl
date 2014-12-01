__kernel void HeatTransfer(__global float* previousMatrix, __global float* nextMatrix, int rowCount, int columnCount, float td, float h)
{
	int matrixSize = rowCount * columnCount;
	int id = get_global_id(0);

	int i = id / (columnCount - 2) + 1;
	int j = id % (columnCount - 2) + 1;

	previousMatrix[i * columnCount + j] = (1 - 4 * td / (h * h)) * nextMatrix[i * columnCount + j] + (td / (h * h)) * (nextMatrix[(i - 1) * columnCount + j] + nextMatrix[(i + 1) * columnCount + j] + nextMatrix[i * columnCount + (j - 1)] + nextMatrix[i * columnCount + (j + 1)]);
}
