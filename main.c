#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <mpi.h>

typedef int bool;
#define true 1
#define false 0
#define bool int

void initializeMPI();
void finalizeMPI();
void solveProblem1(int numberOfAlternations, int initializationValue);
void solveProblem1Old(int numberOfAlternations, int initializationValue);
void solveProblem2(int numberOfAlternations, int initializationValue);
void solveProblem2Old(int numberOfAlternations, int initializationValue);
void solveProblem2v2(int numberOfAlternations, int initializationValue);
void initializeMatrix(int matrix[8][8]);
bool matrixIsComplete(int matrix[8][8]);
void printMatrix(int matrix[8][8]);

int procCount;
int procRank;
char procName[MPI_MAX_PROCESSOR_NAME];
int procNameLength;
    
int main(int argc, char** argv)
{
    const bool USE_ARGUMENTS = true;
    
    int problemToSolve = 2;
    int initializationValue = 1;
    int numberOfAlterations = 2;
    
	initializeMPI();
	
    if (USE_ARGUMENTS && argc >= 4)
    {
        problemToSolve = atoi (argv[1]);
        initializationValue = atoi (argv[2]);
        numberOfAlterations = atoi (argv[3]);
    }
    else if (procRank == 0)
    {
        printf("No valid arguments, default values used.\n\n");
    }
	
    struct timespec begin = {0,0}, end = {0,0};

    clock_gettime(CLOCK_REALTIME, &begin);
    
    if (problemToSolve == 1)
        solveProblem1(numberOfAlterations, initializationValue);
    else if (problemToSolve == 2)
        solveProblem2v2(numberOfAlterations, initializationValue);
    
    clock_gettime(CLOCK_REALTIME, &end);
	
	if (procRank == 0)
		printf ("\nExecution time: %.0f ms\n", (((double)end.tv_sec + 1.0e-9*end.tv_nsec) - ((double)begin.tv_sec + 1.0e-9*begin.tv_nsec)) * 1000.0);
    
	finalizeMPI();
	
    return 0;
}

void initializeMPI()
{
	MPI_Init(NULL, NULL);

	MPI_Comm_size(MPI_COMM_WORLD, &procCount);

	MPI_Comm_rank(MPI_COMM_WORLD, &procRank);

	MPI_Get_processor_name(procName, &procNameLength);
}

void finalizeMPI()
{
	MPI_Finalize();
}

void solveProblem1(int numberOfAlterations, int initializationValue)
{
	int message[3];

	if (procRank == 0)
	{
		int matrix[8][8];
		initializeMatrix(matrix);
		
		while (!matrixIsComplete(matrix))
		{
			MPI_Recv(message, 3, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			matrix[message[0]][message[1]] = message[2];
		}
		
		printMatrix(matrix);
	}
	else
	{
		int i, j;
		
		for (i = procRank - 1; i <= 64; i += procCount - 1)
		{
			int value = initializationValue;
			int x = i / 8;
			int y = i % 8;
			
			for (j = 1; j <= numberOfAlterations; j++)
			{
				usleep(1000);
				value += (x + y) * j;
			}
			
			message[0] = x;
			message[1] = y;
			message[2] = value;
			
			MPI_Send(message, 3, MPI_INT, 0, 0, MPI_COMM_WORLD);
		}
	}
}

void solveProblem1Old(int numberOfAlterations, int initializationValue)
{
	int matrix[8][8];

    // Initialize the matrix with expected value
    int i, j, k;
    for (i = 0; i < 8; i++)
	{
        for (j = 0; j < 8; j++)
		{
            matrix[i][j] = initializationValue;
        }
    }

    for (k = 1; k <= numberOfAlterations; k++)
    {
        for (i = 0; i < 8; i++)
		{
            for (j = 0; j < 8; j++)
			{
                usleep(1000);
                matrix[i][j] += (i + j) * k;
            }
        }
    }
}

void solveProblem2(int numberOfAlterations, int initializationValue)
{
	int message[9];

	if (procRank == 0)
	{
		int matrix[8][8];
		initializeMatrix(matrix);
		
		while (!matrixIsComplete(matrix))
		{
			MPI_Recv(message, 9, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			int i;
			for (i = 0; i < 8; i++)
			{
				matrix[message[0]][i] = message[i+1];
			}
		}
		
		printMatrix(matrix);
	}
	else
	{
		if (procRank <= 8)
		{
			int row[8];
			int rowNum = procRank - 1;
			int i, j;
			
			for (i = 0; i < 8; i++)
				row[i] = initializationValue;
				
			for (i = 1; i <= numberOfAlterations; i++)
			{
				usleep(1000);
				row[0] += rowNum * i;
				for (j = 1; j < 8; j++)
				{
					usleep(1000);
                    row[j] += row[j - 1] * i;
				}
			}
			
			message[0] = rowNum;
			
			for (i = 0; i < 8; i++)
				message[i+1] = row[i];
			
			MPI_Send(message, 9, MPI_INT, 0, 0, MPI_COMM_WORLD);
		}
	}
}

void solveProblem2v2(int numberOfAlterations, int initializationValue)
{
	int message[9];
	if (procRank == 0)
	{
		int matrix[8][8];
		initializeMatrix(matrix);
		
		while (!matrixIsComplete(matrix))
		{
			MPI_Recv(message, 9, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			
			int i;
			for (i = 0; i < 8; i++)
			{
				matrix[message[0]][i] = message[i+1];
			}
		}
		
		printMatrix(matrix);
	}
	else if (procRank <= 16)
	{
		int row[8];
		int rowNum = (procRank - 1) / 2;
		int i, j;
		
		for (i = 0; i < 8; i++)
			row[i] = initializationValue;
			
		if (procRank % 2 == 1)
		{
			for (j = 0; j < 8; j++)
			{
				for (i = 1; i <= numberOfAlterations / 2; i++)
				{
					usleep(1000);
					if (j == 0)
					{
						row[0] += rowNum * i;
					}
					else
					{
						row[j] += row[j - 1] * i;
					}
				}
				
				message[0] = row[j];
				MPI_Send(message, 1, MPI_INT, procRank + 1, 0, MPI_COMM_WORLD);
			}
		}
		else
		{
			for (j = 0; j < 8; j++)
			{
				MPI_Recv(message, 1, MPI_INT, procRank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				row[j] = message[0];
				for (i = numberOfAlterations / 2 + 1; i <= numberOfAlterations; i++)
				{
					usleep(1000);
					
					if (j == 0)
						row[0] += rowNum * i;
					else
						row[j] += row[j - 1] * i;
				}
			}
			
			message[0] = rowNum;
			for (i = 0; i < 8; i++)
				message[i+1] = row[i];
				
			MPI_Send(message, 9, MPI_INT, 0, 0, MPI_COMM_WORLD);
		}
	}
}

void solveProblem2Old(int numberOfAlterations, int initializationValue)
{
	int matrix[8][8];

    // Initialize the matrix with expected value
    int i, j, k;
    for (i = 0; i < 8; i++)
	{
        for (j = 0; j < 8; j++)
		{
            matrix[i][j] = initializationValue;
        }
    }

    for (k = 1; k <= numberOfAlterations; k++)
    {
        for (i = 0; i < 8; i++) 
		{
            for (j = 0; j < 8; j++)
			{
                if (j == 0)
                {
                    usleep(1000);
                    matrix[i][j] += i * k;
                }
                else
                {
                    usleep(1000);
                    matrix[i][j] += matrix[i][j - 1] * k;
                }
            }
        }
    }
	
	printMatrix(matrix);
}

void initializeMatrix(int matrix[8][8])
{
    int i, j;
    
    for (i = 0; i < 8; i++)
        for (j = 0; j < 8; j++)
            matrix[i][j] = -1;
}

bool matrixIsComplete(int matrix[8][8])
{
    int i, j;
	bool completed = true;
    
    for (i = 0; i < 8 && completed; i++)
        for (j = 0; j < 8; j++)
            if (matrix[i][j] < 0)
				completed = false;
			
	return completed;
}

void printMatrix(int matrix[8][8])
{
    int i, j;
    
    for (i = 0; i < 8; i++)
	{
        for (j = 0; j < 8; j++)
		{
            printf ("%5d ", matrix[i][j]);
        }
            printf ("\n");
    }
}
