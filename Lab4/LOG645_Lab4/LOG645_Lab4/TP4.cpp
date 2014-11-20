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
Date dern. modif. : 	2014-1

Description :
Ce programme simule la dispertion de chaleur dans une plaque. L'exécution se fait en parallèle à l'aide de OpenCL
*/

#include <CL/cl.h>
#include <chrono>
#include "oclUtils.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <Windows.h>

#define true 1
#define false 0
#define bool int

void sequentialSolve();
void parallelSolve(int kernelCount);
float** getInitialMatrix();
float* getInitialMatrixAsArray();
void printMatrix(int m, int n, float** matrix);
void printMatrixAsArray(int m, int n, float* matrix);
void checkForError(cl_int status);
void checkForError(cl_int status, char* taskDescription);

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
		rowCount = 50;
		columnCount = 50;
		timeSteps = 1000;
		td = 0.0002f;
		h = 0.1f;
		printf("Arguments invalid, using default values.\n\n");
	}

	float** originalMatrix = getInitialMatrix();
	printf("Original matrix:\n");
	printMatrix(rowCount, columnCount, originalMatrix);

	parallelSolve(8);

	printf("\nSequential solution:\n");


	sequentialSolve();




	return (EXIT_SUCCESS);
}

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

void parallelSolve(int kernelCount)
{
	std::chrono::system_clock::time_point timeStart = std::chrono::high_resolution_clock::now();
	float* solutionMatrix = getInitialMatrixAsArray();
	// Setup the context, command queue, buffer, and program.
	cl_platform_id platformId = nullptr;
	cl_device_id deviceId = nullptr;

	cl_context context = nullptr;
	cl_command_queue commandQueue = nullptr;
	cl_mem bufferPM = nullptr;
	cl_mem bufferCM = nullptr;
	cl_program program = nullptr;
	cl_kernel kernel = nullptr;

	cl_int status = 0;
	cl_int bufferSize = rowCount * columnCount * sizeof(float);
	cl_uint numPlatforms;
	cl_uint numDevices;

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

	// Load the kernel file (TODO : [REMOVE] Remove oclUtils and read from file manually)
	size_t programLength = 0;
	char* programSource = oclLoadProgSource("TP4.cl", "", &programLength);

	program = clCreateProgramWithSource(context, 1, (const char **)&programSource, &programLength, &status);
	checkForError(status, "creating program from source");

	status = clBuildProgram(program, 0, nullptr, "", nullptr, nullptr);
	checkForError(status, "building program");

	// Launch kernels and perform calculations
	float* pM = getInitialMatrixAsArray(); // Previous Matrix (shortened for equation brevity
	float* cM = getInitialMatrixAsArray(); // Current Matrix (shortened for equation brevity)

	status = clEnqueueWriteBuffer(commandQueue, bufferPM, true, 0, bufferSize, pM, 0, nullptr, nullptr);
	checkForError(status, "Copying the initial matrix (odd)");

	status = clEnqueueWriteBuffer(commandQueue, bufferCM, true, 0, bufferSize, cM, 0, nullptr, nullptr);
	checkForError(status, "Copying the initial matrix (even)");

	size_t globalWorkSize[] = { bufferSize / sizeof(float) };

	int k;
	for (k = 0; k < timeSteps; k++)
	{
		cl_event* taskComplete = new cl_event[kernelCount];
		for (int n = 0; n < kernelCount; n++) // Where n is the kernel Id
		{
			kernel = clCreateKernel(program, "HeatTransfer", &status);

			if (k % 2 == 0)
			{
				status = clSetKernelArg(kernel, 0, sizeof(bufferPM), &bufferPM);
				status = clSetKernelArg(kernel, 1, sizeof(bufferCM), &bufferCM);
			}
			else
			{
				status = clSetKernelArg(kernel, 0, sizeof(bufferCM), &bufferCM);
				status = clSetKernelArg(kernel, 1, sizeof(bufferPM), &bufferPM);
			}
			status = clSetKernelArg(kernel, 2, sizeof(rowCount), &rowCount);
			status = clSetKernelArg(kernel, 3, sizeof(columnCount), &columnCount);
			status = clSetKernelArg(kernel, 4, sizeof(kernelCount), &kernelCount);
			int kernelId = n;
			status = clSetKernelArg(kernel, 5, sizeof(kernelId), &kernelId);
			status = clSetKernelArg(kernel, 6, sizeof(td), &td);
			status = clSetKernelArg(kernel, 7, sizeof(h), &h);

			status = clEnqueueNDRangeKernel(commandQueue, kernel, 1, nullptr, globalWorkSize, nullptr, 0, nullptr, &taskComplete[n]);
		}
		clWaitForEvents(kernelCount, taskComplete);
		clReleaseEvent(*taskComplete);
	}

	if (k % 2 == 0)
		status = clEnqueueReadBuffer(commandQueue, bufferPM, true, 0, bufferSize, solutionMatrix, 0, nullptr, nullptr);
	else
		status = clEnqueueReadBuffer(commandQueue, bufferCM, true, 0, bufferSize, solutionMatrix, 0, nullptr, nullptr);

	checkForError(status, "reading solution matrix");

	std::chrono::system_clock::time_point timerStop = std::chrono::high_resolution_clock::now();
	parSolveTime = (double)std::chrono::duration_cast<std::chrono::nanoseconds>(timerStop - timeStart).count() / 1000000;
	printMatrixAsArray(rowCount, columnCount, solutionMatrix);
	printf("Parallel solve duration : %.2f ms\n", parSolveTime);
}

void checkForError(cl_int status)
{
	checkForError(status, "");
}

void checkForError(cl_int status, char* taskDescription)
{
	if (status != CL_SUCCESS)
	{
		printf("Error while %s, code : %d\n", taskDescription, status);
	}
}

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

void printMatrix(int m, int n, float** matrix)
{
	if (m * n > 1000)
	{
		printf("Did not print matrix because it had over 1000 entries.\n");
		return;
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

void printMatrixAsArray(int m, int n, float* matrix)
{
	if (m * n > 1000)
	{
		printf("Did not print matrix because it had over 1000 entries.\n");
		return;
	}

	int i, j, k;
	k = 0;

	for (i = 0; i < m; i++)
	{
		for (j = 0; j < n; j++)
		{
			printf("%5.1f ", matrix[k]);
			k++;
		}

		printf("\n");
	}
}