#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

typedef int bool;
#define true 1
#define false 0

void solveProblem1(int numberOfAlternations);
void solveProblem2(int numberOfAlternations);
void printMatrix();

int matrix[8][8];
    
int main(int argc, char** argv) {
    const bool USE_ARGUMENTS = false;
    
    int problemToSolve = 1;
    int initializationValue = 2;
    int numberOfAlterations = 1;
    
    if (USE_ARGUMENTS && argc >= 4)
    {
        problemToSolve = atoi (argv[1]);
        initializationValue = atoi (argv[2]);
        numberOfAlterations = atoi (argv[3]);
    }
    else
    {
        printf("No valid arguments, default values used.\n\n");
    }
    
    int i, j;
    struct timespec begin = {0,0}, end = {0,0};

    // Initialize the matrix with expected value
    for (i = 0; i < 8; i++) {
        for (j = 0; j < 8; j++) {
            matrix[i][j] = initializationValue;
        }
    }

    clock_gettime(CLOCK_REALTIME, &begin);
    
    if (problemToSolve == 1) {
        solveProblem1(numberOfAlterations);
        printMatrix();
    }
    else if (problemToSolve == 2) {
        solveProblem2(numberOfAlterations);
        printMatrix();
    }
    
    clock_gettime(CLOCK_REALTIME, &end);
    printf ("\nExecution time: %.0f ms\n", (((double)end.tv_sec + 1.0e-9*end.tv_nsec) - ((double)begin.tv_sec + 1.0e-9*begin.tv_nsec)) * 1000.0);
    
    return 0;
}

void solveProblem1(int numberOfAlterations)
{
    int i, j, k;
    for (k = 1; k <= numberOfAlterations; k++)
    {
        for (i = 0; i < 8; i++) {
            for (j = 0; j < 8; j++) {
                usleep(1000);
                matrix[i][j] += (i + j) * k;
            }
        }
    }
}

void solveProblem2(int numberOfAlterations)
{
    int i, j, k;
    for (k = 1; k <= numberOfAlterations; k++)
    {
        for (i = 0; i < 8; i++) {
            for (j = 0; j < 8; j++) {
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
}

void printMatrix(){
    int i, j;
    
    for (i = 0; i < 8; i++) {
        for (j = 0; j < 8; j++) {
            printf ("%5d ", matrix[i][j]);
        }
            printf ("\n");
    }
}
