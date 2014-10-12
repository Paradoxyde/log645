#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#define true 1
#define false 0
#define bool int

void solveProblem1Seq(int numberOfAlternations, int initializationValue);
void solveProblem2Seq(int numberOfAlternations, int initializationValue);
void solveProblem1Par(int numberOfAlternations, int initializationValue);
void solveProblem2Par(int numberOfAlternations, int initializationValue);
void printMatrix(long long matrix[10][10]);
void spinWait(int milliseconds);

int main(int argc, char** argv)
{
    int problemToSolve = 2;
    int initializationValue = 2;
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
	
	double timerStart, timerEnd, Texec;
	struct timeval time;
	gettimeofday (&time, NULL);
	timerStart = (double) (time.tv_sec) + (double) (time.tv_usec) / 1e6;

    if (problemToSolve == 1)
        solveProblem1Seq(numberOfAlterations, initializationValue);
    else if (problemToSolve == 2)
        solveProblem2Seq(numberOfAlterations, initializationValue);
    
	gettimeofday (&time, NULL);
	timerEnd = (double) (time.tv_sec) + (double) (time.tv_usec) / 1e6;
	Texec = timerEnd - timerStart;
	
    printf ("\nSequential execution time: %.3f ms\n", Texec);
	
	gettimeofday (&time, NULL);
	timerStart = (double) (time.tv_sec) + (double) (time.tv_usec) / 1e6;

    if (problemToSolve == 1)
        solveProblem1Par(numberOfAlterations, initializationValue);
    else if (problemToSolve == 2)
        solveProblem2Par(numberOfAlterations, initializationValue);
    
	gettimeofday (&time, NULL);
	timerEnd = (double) (time.tv_sec) + (double) (time.tv_usec) / 1e6;
	Texec = timerEnd - timerStart;
	
    printf ("\nParallel execution time: %.3f ms\n", Texec);
    
    return 0;
}

void solveProblem1Seq(int numberOfAlterations, int initializationValue)
{
    long long matrix[10][10];

    // Initialize the matrix with expected value
    int i, j, k;
    for (i = 0; i < 10; i++)
    {
        for (j = 0; j < 10; j++)
        {
            matrix[i][j] = initializationValue;
        }
    }

    // Perform the calculations
    for (k = 1; k <= numberOfAlterations; k++)
    {
        for (i = 0; i < 10; i++)
        {
            for (j = 0; j < 10; j++)
            {
                spinWait(50);
                matrix[i][j] += i + j;
            }
        }
    }
	
	printMatrix(matrix);
}

void solveProblem1Par(int numberOfAlterations, int initializationValue)
{
    long long matrix[10][10];

    // Initialize the matrix with expected value
    int i, j, k;
    for (i = 0; i < 10; i++)
    {
        for (j = 0; j < 10; j++)
        {
            matrix[i][j] = initializationValue;
        }
    }

    // Perform the calculations
    for (k = 1; k <= numberOfAlterations; k++)
    {
		#pragma omp parallel for
        for (i = 0; i < 100; i++)
        {
			int pi = i / 10;
			int pj = i % 10;
			spinWait(50);
			matrix[pi][pj] += pi + pj;
        }
    }
	
	printMatrix(matrix);
}

void solveProblem2Seq(int numberOfAlterations, int initializationValue)
{
	long long matrix[10][10];

    // Initialize the matrix with expected value
    int i, j, k;
    for (i = 0; i < 10; i++)
	{
        for (j = 0; j < 10; j++)
		{
            matrix[i][j] = initializationValue;
        }
    }

    // Perform the calculations
    for (k = 1; k <= numberOfAlterations; k++)
    {
        for (i = 0; i < 10; i++) 
		{
            for (j = 9; j >= 0; j--)
            {
                spinWait(50);
				
                if (j == 9)
                    matrix[i][j] += i;
                else
                    matrix[i][j] += matrix[i][j + 1];
            }
        }
    }
	
    printMatrix(matrix);
}

void solveProblem2Par(int numberOfAlterations, int initializationValue)
{
	long long matrix[10][10];

    // Initialize the matrix with expected value
    int i, j, k;
    for (i = 0; i < 10; i++)
	{
        for (j = 0; j < 10; j++)
		{
            matrix[i][j] = initializationValue;
        }
    }

    // Perform the calculations
    for (k = 1; k <= numberOfAlterations; k++)
    {
		#pragma omp parallel for ordered schedule(dynamic)
		for (i = 0; i < 100; i++)
		{
			int pi = i / 10;
			int pj = 9 - (i % 10); // Value loops from 9 to 0
			
			spinWait(50);
			
			#pragma omp ordered
			{
				if (pj == 9)
					matrix[pi][pj] += pi;
				else
					matrix[pi][pj] += matrix[pi][pj + 1];
			}
        }
    }
	
    printMatrix(matrix);
}

void printMatrix(long long matrix[10][10])
{
    int i, j;
    
    for (i = 0; i < 10; i++)
    {
        for (j = 0; j < 10; j++)
        {
            printf ("%8lld ", matrix[i][j]);
        }
		
        printf ("\n");
    }
}

void spinWait(int milliseconds)
{
    struct timeval startTime;
    struct timeval endTime;
    
    gettimeofday(&startTime, NULL);
    do
    {
        gettimeofday(&endTime, NULL);
    } while ((endTime.tv_sec - startTime.tv_sec) * 1000000 + (endTime.tv_usec - startTime.tv_usec) < milliseconds * 1000);
    
    return;
}