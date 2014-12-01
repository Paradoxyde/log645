/******************************************************
Cours :             	LOG645
Session :           	Automne 2014
Groupe :            	1
Projet :            	Laboratoire #4
Étudiants :    			Vincent Couture
						Francis Lanthier
Équipe :				18
Professeur :        	Lévis Thériault
Chargé de laboatoire:	Kevin Lachance-Coulombe
Date création :     	2014-11-19
Date dern. modif. : 	2014-12-01

Description :
Ce programme simule la dispertion de chaleur dans une plaque. L'exécution se fait en parallèle à l'aide de OpenCL
*/

#include <CL/cl.h>
#include <chrono>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <Windows.h>

#define true 1
#define false 0
#define bool int

void sequentialSolve();
void parallelSolve();
float** getInitialMatrix();
float* getInitialMatrixAsArray();
void printMatrix(int m, int n, float** matrix);
void printMatrixAsArray(int m, int n, float* matrix);
void checkForError(cl_int status);
void checkForError(cl_int status, char* taskDescription);
char* oclLoadProgSource(const char* cFilename, const char* cPreamble, size_t*szFinalLength);

int rowCount, columnCount, timeSteps, procAllowedCount;
float td, h; // Discrete time and Section size

double parSolveTime = 0;
double seqSolveTime = 0;

int main(int argc, char** argv)
{
	if (argc >= 5)
	{
		rowCount = atoi(argv[1]);
		columnCount = atoi(argv[2]);
		timeSteps = atoi(argv[3]);
		td = (float)atof(argv[4]);
		h = (float)atof(argv[5]);
	}
	else
	{
		// Initialize variables with default values
		rowCount = 1000;
		columnCount = 1000;
		timeSteps = 100;
		td = 0.0002f;
		h = 0.1f;
		printf("Arguments invalid, using default values.\n\n");
	}

	float** originalMatrix = getInitialMatrix();
	printf("Original matrix:\n");
	printMatrix(rowCount, columnCount, originalMatrix);

	parallelSolve();

	printf("\nSequential solution:\n");

	sequentialSolve();

	printf("\nSeq/Par acceleration : %.3f\n\n", seqSolveTime / parSolveTime);

	return (EXIT_SUCCESS);
}

// Solve the problem with a sequential approach
void sequentialSolve()
{
	std::chrono::system_clock::time_point timeStart = std::chrono::high_resolution_clock::now();
	float** pM = getInitialMatrix(); // Previous Matrix (shortened for equation brevity
	float** cM = getInitialMatrix(); // Current Matrix (shortened for equation brevity)

	int i, j, k;

	// Perform the calculations
	for (k = 1; k <= timeSteps; k++)
	{
		for (i = 1; i < rowCount - 1; i++)
		{
			for (j = 1; j < columnCount - 1; j++)
			{
				if (k % 2 == 0)
					cM[i][j] = (1 - 4 * td / (h * h)) * pM[i][j] + (td / (h * h)) * (pM[i - 1][j] + pM[i + 1][j] + pM[i][j - 1] + pM[i][j + 1]);
				else
					pM[i][j] = (1 - 4 * td / (h * h)) * cM[i][j] + (td / (h * h)) * (cM[i - 1][j] + cM[i + 1][j] + cM[i][j - 1] + cM[i][j + 1]);
			}
		}
	}

	std::chrono::system_clock::time_point timerStop = std::chrono::high_resolution_clock::now();
	seqSolveTime = (double)std::chrono::duration_cast<std::chrono::nanoseconds>(timerStop - timeStart).count() / 1000000;

	if (k % 2 == 0)
		printMatrix(rowCount, columnCount, pM);
	else
		printMatrix(rowCount, columnCount, cM);
	printf("Sequential solve duration : %.2f ms\n", seqSolveTime);
}

// Solve the problem using a parallel approach with the help of OpenCL
void parallelSolve()
{
	std::chrono::system_clock::time_point timeStart = std::chrono::high_resolution_clock::now();
	float* solutionMatrix = getInitialMatrixAsArray();
	cl_platform_id platformId = nullptr;
	cl_device_id deviceId = nullptr;

	cl_context context = nullptr;
	cl_command_queue commandQueue = nullptr;
	cl_mem bufferPM = nullptr;
	cl_mem bufferCM = nullptr;
	cl_program program = nullptr;

	cl_int status = 0;
	cl_int bufferSize = rowCount * columnCount * sizeof(float);
	cl_uint numPlatforms;
	cl_uint numDevices;

	// Setup the context, command queue, buffer, and program.
	status = clGetPlatformIDs(1, &platformId, &numPlatforms);
	checkForError(status, "getting platforms");
	printf("Num platforms : %d\nPlatform ID : %d\n", numPlatforms, platformId);

	status = clGetDeviceIDs(platformId, CL_DEVICE_TYPE_GPU, 1, &deviceId, &numDevices);
	checkForError(status, "getting devices");
	printf("Num devices : %d\nDevice ID : %d\n", numDevices, deviceId);

	context = clCreateContext(0, 1, &deviceId, nullptr, nullptr, &status);
	checkForError(status, "creating context");

	commandQueue = clCreateCommandQueue(context, deviceId, 0, &status);
	checkForError(status, "creating commandQueue");

	bufferPM = clCreateBuffer(context, CL_MEM_READ_WRITE, bufferSize, 0, &status);
	checkForError(status, "creating bufferPM");

	bufferCM = clCreateBuffer(context, CL_MEM_READ_WRITE, bufferSize, 0, &status);
	checkForError(status, "creating bufferCM");

	size_t programLength = 0;
	char* programSource = oclLoadProgSource("TP4.cl", "", &programLength);

	program = clCreateProgramWithSource(context, 1, (const char **)&programSource, &programLength, &status);
	checkForError(status, "creating program from source");

	status = clBuildProgram(program, 0, nullptr, "", nullptr, nullptr);
	checkForError(status, "building program");

	float* pM = getInitialMatrixAsArray(); // Previous Matrix (shortened for equation brevity
	float* cM = getInitialMatrixAsArray(); // Current Matrix (shortened for equation brevity)

	status = clEnqueueWriteBuffer(commandQueue, bufferPM, true, 0, bufferSize, pM, 0, nullptr, nullptr);
	checkForError(status, "Copying the initial matrix (odd)");

	status = clEnqueueWriteBuffer(commandQueue, bufferCM, true, 0, bufferSize, cM, 0, nullptr, nullptr);
	checkForError(status, "Copying the initial matrix (even)");

	// The work size corresponds to the size of the matrix excluding borders
	size_t globalWorkSize[] = { (rowCount - 2) * (columnCount - 2)};

	// The two kernels use the same function but have opposite read/write matrices
	cl_kernel kernelOdd;
	cl_kernel kernelEven;

	kernelOdd = clCreateKernel(program, "HeatTransfer", &status);
	status = clSetKernelArg(kernelOdd, 0, sizeof(bufferCM), &bufferCM);
	status = clSetKernelArg(kernelOdd, 1, sizeof(bufferPM), &bufferPM);
	status = clSetKernelArg(kernelOdd, 2, sizeof(rowCount), &rowCount);
	status = clSetKernelArg(kernelOdd, 3, sizeof(columnCount), &columnCount);
	status = clSetKernelArg(kernelOdd, 4, sizeof(td), &td);
	status = clSetKernelArg(kernelOdd, 5, sizeof(h), &h);

	kernelEven = clCreateKernel(program, "HeatTransfer", &status);
	status = clSetKernelArg(kernelEven, 0, sizeof(bufferPM), &bufferPM);
	status = clSetKernelArg(kernelEven, 1, sizeof(bufferCM), &bufferCM);
	status = clSetKernelArg(kernelEven, 2, sizeof(rowCount), &rowCount);
	status = clSetKernelArg(kernelEven, 3, sizeof(columnCount), &columnCount);
	status = clSetKernelArg(kernelEven, 4, sizeof(td), &td);
	status = clSetKernelArg(kernelEven, 5, sizeof(h), &h);

	// Perform actual calculations
	int k;
	for (k = 0; k < timeSteps; k++)
	{
		cl_event taskComplete;
		if (k % 2 == 0)
			status = clEnqueueNDRangeKernel(commandQueue, kernelEven, 1, nullptr, globalWorkSize, nullptr, 0, nullptr, &taskComplete);
		else
			status = clEnqueueNDRangeKernel(commandQueue, kernelOdd, 1, nullptr, globalWorkSize, nullptr, 0, nullptr, &taskComplete);
		clWaitForEvents(1, &taskComplete);
		clReleaseEvent(taskComplete);
	}

	// Read appropriate buffer and display the result
	if (k % 2 == 0)
		status = clEnqueueReadBuffer(commandQueue, bufferCM, true, 0, bufferSize, solutionMatrix, 0, nullptr, nullptr);
	else
		status = clEnqueueReadBuffer(commandQueue, bufferPM, true, 0, bufferSize, solutionMatrix, 0, nullptr, nullptr);

	checkForError(status, "reading solution matrix");

	std::chrono::system_clock::time_point timerStop = std::chrono::high_resolution_clock::now();
	parSolveTime = (double)std::chrono::duration_cast<std::chrono::nanoseconds>(timerStop - timeStart).count() / 1000000;
	printMatrixAsArray(rowCount, columnCount, solutionMatrix);
	printf("Parallel solve duration : %.2f ms\n", parSolveTime);
}

// Overload with default message
void checkForError(cl_int status)
{
	checkForError(status, "");
}

// Displays error message if the operation wasn't a success
void checkForError(cl_int status, char* taskDescription)
{
	if (status != CL_SUCCESS)
	{
		printf("Error while %s, code : %d\n", taskDescription, status);
	}
}

// Populate and return the starting matrix
float** getInitialMatrix()
{
	int i, j;
	float** matrix = new float*[rowCount];
	for (i = 0; i < rowCount; i++)
		matrix[i] = new float[columnCount];

	for (i = 0; i < rowCount; i++)
	{
		for (j = 0; j < columnCount; j++)
		{
			matrix[i][j] = i * j * (rowCount - i - 1.0f) * (columnCount - j - 1.0f);
		}
	}

	return matrix;
}

// Populate and return the starting matrix as a 1D array. It's easier to deal with CUDA buffers this way.
float* getInitialMatrixAsArray()
{
	int i, j, k;
	float* matrix = new float[rowCount * columnCount];

	k = 0;
	for (i = 0; i < rowCount; i++)
	{
		for (j = 0; j < columnCount; j++)
		{
			matrix[k] = i * j * (rowCount - i - 1.0f) * (columnCount - j - 1.0f);
			k++;
		}
	}

	return matrix;
}

// Prints matrix to console
void printMatrix(int m, int n, float** matrix)
{
	if (m > 8 || n > 8)
	{
		if (m > 8) m = 8;
		if (n > 8) n = 8;
		printf("Printing only the top 8x8 corner.\n");
	}

	int i, j;

	for (i = 0; i < m; i++)
	{
		for (j = 0; j < n; j++)
		{
			printf("%5.1f ", matrix[i][j]);
		}

		printf("\n");
	}
}

// Print array to console in matrix form
void printMatrixAsArray(int m, int n, float* matrix)
{

	if (m > 8 || n > 8)
	{
		if (m > 8) m = 8;
		if (n > 8) n = 8;
		printf("Printing only the top 8x8 corner.\n");
	}

	int i, j, k;
	k = 0;

	for (i = 0; i < m; i++)
	{
		for (j = 0; j < n; j++)
		{
			printf("%5.1f ", matrix[i * columnCount + j]);
			k++;
		}

		printf("\n");
	}
}

//////////////////////////////////////////////////////////////////////////////
//! SOURCE : Tel que proposé sur le site du cours, avec la source orignale : //developer.nvidia.com/opencl
//! Loads a Program file and prepends the cPreamble to the code.
//!
//! @return the source string if succeeded, 0 otherwise
//! @param cFilename program filename
//! @param cPreamble code that is prepended to the loaded file, typically a set of#defines or a header
//! @param szFinalLength returned length of the code string
//////////////////////////////////////////////////////////////////////////////
char* oclLoadProgSource(const char* cFilename, const char* cPreamble, size_t*szFinalLength)
{
	// locals
	FILE* pFileStream = NULL;
	size_t szSourceLength;

	// open the OpenCL source code file
	if (fopen_s(&pFileStream, cFilename, "rb") != 0)
	{
		return NULL;
	}

	size_t szPreambleLength = strlen(cPreamble);

	// get the length of the source code
	fseek(pFileStream, 0, SEEK_END);
	szSourceLength = ftell(pFileStream);
	fseek(pFileStream, 0, SEEK_SET);

	// allocate a buffer for the source code string and read it in
	char* cSourceString = (char*)malloc(szSourceLength + szPreambleLength + 1);
	memcpy(cSourceString, cPreamble, szPreambleLength);
	if(fread((cSourceString)+szPreambleLength, szSourceLength, 1, pFileStream) != 1)
	{
		fclose(pFileStream);
		free(cSourceString);
		return 0;
	}

	// close the file and return the total length of the combined (preamble + source) string
	fclose(pFileStream);
	if(szFinalLength != 0)
	{
		*szFinalLength = szSourceLength + szPreambleLength;
	}
	cSourceString[szSourceLength + szPreambleLength] ='\0';

	return cSourceString;
}