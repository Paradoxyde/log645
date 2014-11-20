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
#include "oclUtils.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define true 1
#define false 0
#define bool int

void sequentialSolve();
void parallelSolve();
float** getInitialMatrix();
void printMatrix(int m, int n, float** matrix);
void checkForError(cl_int status);
void checkForError(cl_int status, char* taskDescription);

int rowCount, columnCount, timeSteps, procAllowedCount;
float td, h; // Discrete time and Section size

float parSolveTime = 0;

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
		rowCount = 8;
		columnCount = 12;
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

	return (EXIT_SUCCESS);
}

void sequentialSolve()
{
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

	if (k % 2 == 0)
		printMatrix(rowCount, columnCount, pM);
	else
		printMatrix(rowCount, columnCount, cM);
}

void parallelSolve()
{
	cl_platform_id platformId = nullptr;
	cl_device_id deviceId = nullptr;

	cl_context context = nullptr;
	cl_command_queue commandQueue = nullptr;
	cl_mem buffer = nullptr;
	cl_program program = nullptr;
	cl_kernel kernel = nullptr;

	cl_int status = 0;
	cl_int bufferSize = 16777216; // 16Mb
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

	buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, bufferSize, 0, &status);
	checkForError(status, "creating buffer");

	// Load the kernel file ([REMOVE] Remove oclUtils and read from file manually)
	size_t programLength = 0;
	char* programSource = oclLoadProgSource("TP4.cl", "", &programLength);

	program = clCreateProgramWithSource(context, 1, (const char **)&programSource, &programLength, &status);
	checkForError(status, "creating program from source");

	status = clBuildProgram(program, 0, nullptr, "", nullptr, nullptr);
	checkForError(status, "building program");


	// Try to actually do the thing and change a value
	cl_event taskComplete[4];

	size_t globalWorkSize[] = { bufferSize / sizeof(int) };
	int i;
	for (int n = 0; n < 4; n++)
	{
		i = 1000;
		kernel = clCreateKernel(program, "HeatTransfer", &status);
		checkForError(status, "creating kernel");

		status = clSetKernelArg(kernel, 0, sizeof(buffer), &buffer);
		int argValue = n;
		status = clSetKernelArg(kernel, 1, sizeof(argValue), &argValue);
		checkForError(status, "setting kernel arg");

		status = clEnqueueWriteBuffer(commandQueue, buffer, true, n * sizeof(int), sizeof(int), &i, 0, nullptr, nullptr);
		checkForError(status, "enqueueing write buffer");

		status = clEnqueueNDRangeKernel(commandQueue, kernel, 1, nullptr, globalWorkSize, nullptr, 0, nullptr, &taskComplete[n]);
		checkForError(status, "enqueueing range kernel");
	}

	clWaitForEvents(1, taskComplete);
	clReleaseEvent(*taskComplete);

	for (int n = 0; n < 4; n++)
	{
		clEnqueueReadBuffer(commandQueue, buffer, true, n * sizeof(int), sizeof(int), &i, 0, nullptr, nullptr);
		printf("Result : %d\n", i);
	}
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

void printMatrix(int m, int n, float** matrix)
{
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