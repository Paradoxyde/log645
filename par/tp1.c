#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <mpi.h>

#define true 1
#define false 0
#define bool int

void initializeMPI();
void finalizeMPI();
void solveProblem1(int numberOfAlternations, int initializationValue);
void solveProblem2(int numberOfAlternations, int initializationValue);
void initializeMatrix(long long matrix[8][8]);
bool matrixIsComplete(long long matrix[8][8]);
void printMatrix(long long matrix[8][8]);

int procCount;
int procRank;
char procName[MPI_MAX_PROCESSOR_NAME];
int procNameLength;
    
int main(int argc, char** argv)
{
    int problemToSolve = 2;
    int initializationValue = 1;
    int numberOfAlterations = 5;
    
	initializeMPI();
	
    if (argc >= 4)
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
        solveProblem2(numberOfAlterations, initializationValue);
    
    clock_gettime(CLOCK_REALTIME, &end);
	
	if (procRank == 0)
		printf ("\nExecution time: %.0f ms\n", (((double)end.tv_sec + 1.0e-9*end.tv_nsec) - ((double)begin.tv_sec + 1.0e-9*begin.tv_nsec)) * 1000.0);
    
	finalizeMPI();
	
    return 0;
}

// Exists only to improve main function readability
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
	long long message[3];

	if (procRank == 0)
	{
		long long matrix[8][8];
		initializeMatrix(matrix);
		
		while (!matrixIsComplete(matrix))
		{
			// Receive matrix coordinates along with value
			MPI_Recv(message, 3, MPI_LONG_LONG, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			matrix[message[0]][message[1]] = message[2];
		}
		
		printMatrix(matrix);
	}
	else
	{
		int i, j;
		
		for (i = procRank - 1; i <= 64; i += procCount - 1)
		{
			long long value = initializationValue;
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
			
			MPI_Send(message, 3, MPI_LONG_LONG, 0, 0, MPI_COMM_WORLD);
		}
	}
}

void solveProblem2(int numberOfAlterations, int initializationValue)
{
	long long message[9];
	if (procRank == 0)
	{
		long long matrix[8][8];
		initializeMatrix(matrix);
		
		while (!matrixIsComplete(matrix))
		{
			// Receive row number and all values for that row
			MPI_Recv(message, 9, MPI_LONG_LONG, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			
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
		long long row[8];
		long long previousRow[numberOfAlterations + 1];
		int rowNum = (procRank - 1) / 2;
		int i, j;
		
		for (i = 0; i < 8; i++)
			row[i] = initializationValue;
			
		for (i = 0; i <= numberOfAlterations; i++)
			previousRow[i] = initializationValue;
			
		if (procRank % 2 == 1)
		{
			for (j = 0; j < 8; j++)
			{
				for (i = 1; i <= numberOfAlterations / 2; i++)
				{
					usleep(1000);
					if (j == 0)
						row[0] += rowNum * i;
					else
						row[j] += previousRow[i] * i;
						
					previousRow[i] = row[j];
				}
				
				message[0] = row[j];
				MPI_Send(message, 1, MPI_LONG_LONG, procRank + 1, 0, MPI_COMM_WORLD);
			}
		}
		else
		{
			for (j = 0; j < 8; j++)
			{
				MPI_Recv(message, 1, MPI_LONG_LONG, procRank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				row[j] = message[0];
				for (i = numberOfAlterations / 2 + 1; i <= numberOfAlterations; i++)
				{
					usleep(1000);
					if (j == 0)
						row[0] += rowNum * i;
					else
						row[j] += previousRow[i] * i;
						
					previousRow[i] = row[j];
				}
			}
			
			message[0] = rowNum;
			for (i = 0; i < 8; i++)
				message[i+1] = row[i];
				
			MPI_Send(message, 9, MPI_LONG_LONG, 0, 0, MPI_COMM_WORLD);
		}
	}
}

// Not really necessary, but helps spot missing values
void initializeMatrix(long long matrix[8][8])
{
    int i, j;
    
    for (i = 0; i < 8; i++)
        for (j = 0; j < 8; j++)
            matrix[i][j] = -1;
}

// This proved necessary as counting messages yielded inconsistent results
bool matrixIsComplete(long long matrix[8][8])
{
    int i, j;
	bool completed = true;
    
    for (i = 0; i < 8 && completed; i++)
        for (j = 0; j < 8; j++)
            if (matrix[i][j] < 0)
				completed = false;
			
	return completed;
}

void printMatrix(long long matrix[8][8])
{
    int i, j;
    
    for (i = 0; i < 8; i++)
	{
        for (j = 0; j < 8; j++)
		{
            printf ("%8lld ", matrix[i][j]);
        }
            printf ("\n");
    }
}