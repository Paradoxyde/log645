#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define true 1
#define false 0
#define bool int

void sequentialSolve();
void initializeMatrix(int m, int n, float matrix[m][n]);
void printMatrix(int m, int n, float matrix[m][n]);

int rowCount, columnCount, timeSteps;
float td, h; // Discrete time and Section size
    
int main(int argc, char** argv)
{
    
    if (argc >= 6)
    {
        rowCount = atoi (argv[1]);
        columnCount = atoi (argv[2]);
        timeSteps = atoi (argv[3]);
        td = atof (argv[4]);
        h = atof (argv[5]);
    }
    else
    {
        // Initialize variables with default values
        rowCount = 5;
        columnCount = 10;
        timeSteps = 100;
        td = 0.0002f;
        h = 0.1f;
        printf("Arguments invalid, using default values.\n\n");
    }
    
    sequentialSolve();
    
    return (EXIT_SUCCESS);
}

void sequentialSolve()
{
    float pM[rowCount][columnCount]; // Previous Matrix (shortened for calculation brevity)
    float cM[rowCount][columnCount]; // Current Matrix (shortened for calculation brevity)
    initializeMatrix(rowCount, columnCount, pM);
    initializeMatrix(rowCount, columnCount, cM);
    
    int i, j, k;
    
    // Perform the calculations
    for (k = 1; k <= 10; k++)
    {
        for (i = 1; i < rowCount - 1; i++)
        {
            for (j = 1; j < columnCount - 1; j++)
            {
                if (k % 2 == 0)
                    cM[i][j] = (1 - 4 * td /(h * h)) * pM[i][j] + (td / (h * h)) * (pM[i-1][j] + pM[i+1][j] + pM[i][j-1] + pM[i][j+1]);
                else
                    pM[i][j] = (1 - 4 * td /(h * h)) * cM[i][j] + (td / (h * h)) * (cM[i-1][j] + cM[i+1][j] + cM[i][j-1] + cM[i][j+1]);
            }
        }
    }
    
    if (k % 2 == 0)
        printMatrix(rowCount, columnCount, pM);
    else
        printMatrix(rowCount, columnCount, cM);
}

void initializeMatrix(int m, int n, float matrix[m][n])
{
    int i, j;
    for (i = 0; i < m; i++)
    {
        for (j = 0; j < n; j++)
        {
            matrix[i][j] = i * j * (m - i - 1) * (n - j - 1);
        }
    }
}

void printMatrix(int m, int n, float matrix[m][n])
{
    int i, j;
    
    for (i = 0; i < m; i++)
    {
        for (j = 0; j < n; j++)
        {
            printf ("%8.1f ", matrix[i][j]);
        }
		
        printf ("\n");
    }
}