#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define true 1
#define false 0
#define bool int

void solveProblem1(int numberOfAlternations, int initializationValue);
void solveProblem2(int numberOfAlternations, int initializationValue);
void printMatrix(long long matrix[8][8]);

int main(int argc, char** argv)
{
    int problemToSolve = 2;
    int initializationValue = 1;
    int numberOfAlterations = 5;
    
    if (argc >= 4)
    {
        problemToSolve = atoi (argv[1]);
        initializationValue = atoi (argv[2]);
        numberOfAlterations = atoi (argv[3]);
    }
    else
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
	
	printf ("\nExecution time: %.0f ms\n", (((double)end.tv_sec + 1.0e-9*end.tv_nsec) - ((double)begin.tv_sec + 1.0e-9*begin.tv_nsec)) * 1000.0);
    
    return 0;
}

void solveProblem1(int numberOfAlterations, int initializationValue)
{
	long long matrix[8][8];

    // Initialize the matrix with expected value
    int i, j, k;
    for (i = 0; i < 8; i++)
	{
        for (j = 0; j < 8; j++)
		{
            matrix[i][j] = initializationValue;
        }
    }

	// Perform the calculations
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
	
	printMatrix(matrix);
}

void solveProblem2(int numberOfAlterations, int initializationValue)
{
	long long matrix[8][8];

    // Initialize the matrix with expected value
    int i, j, k;
    for (i = 0; i < 8; i++)
	{
        for (j = 0; j < 8; j++)
		{
            matrix[i][j] = initializationValue;
        }
    }

	// Perform the calculations
    for (k = 1; k <= numberOfAlterations; k++)
    {
        for (i = 0; i < 8; i++) 
		{
            for (j = 0; j < 8; j++)
			{
				usleep(1000);
				
                if (j == 0)
                    matrix[i][j] += i * k;
                else
                    matrix[i][j] += matrix[i][j - 1] * k;
            }
        }
    }
	
	printMatrix(matrix);
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