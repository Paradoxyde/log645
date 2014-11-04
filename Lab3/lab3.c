#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <mpi.h>
#include <math.h>

#define true 1
#define false 0
#define bool int

void initializeMPI();
void finalizeMPI();
void sequentialSolve();
void parallelSolve();
void initializeMatrix(int m, int n, float matrix[m][n]);
void printMatrix(int m, int n, float matrix[m][n]);
void spinWait(int milliseconds);

int rowCount, columnCount, timeSteps, procAllowedCount;
float td, h; // Discrete time and Section size
    
// MPI variables
int mpiProcCount;
int procRank;
char procName[MPI_MAX_PROCESSOR_NAME];
int procNameLength;
float parSolveTime = 0;

int procAvailableCount;

int main(int argc, char** argv)
{
	initializeMPI();
	
    if (argc >= 6)
    {
        rowCount = atoi (argv[1]);
        columnCount = atoi (argv[2]);
        timeSteps = atoi (argv[3]);
        td = atof (argv[4]);
        h = atof (argv[5]);
        procAllowedCount = atoi (argv[6]);
    }
    else
    {
        // Initialize variables with default values
        rowCount = 150;
        columnCount = 20;
        timeSteps = 50;
        td = 0.0002f;
        h = 0.1f;
        procAllowedCount = 24;
		if (procRank == 0)
			printf("Arguments invalid, using default values.\n\n");
    }
	
	if (mpiProcCount < procAllowedCount)
		procAvailableCount = mpiProcCount;
	else
		procAvailableCount = procAllowedCount;

	if (procRank >= procAvailableCount)
	{
		finalizeMPI();
		return(EXIT_SUCCESS);
	}
	
    parallelSolve();
	
	double timeStart, timeEnd, Texec;
	struct timeval tp;
	if (procRank == 0)
	{
		gettimeofday (&tp, NULL);
		timeStart = (double) (tp.tv_sec) + (double) (tp.tv_usec) / 1e6;

		sequentialSolve();
		
		gettimeofday (&tp, NULL);
		timeEnd = (double) (tp.tv_sec) + (double) (tp.tv_usec) / 1e6;
		Texec = timeEnd - timeStart;
		
		printf("Sequential solve time : %.2f ms\n", Texec * 1000.0);
		
		printf("Solve time comparison : %.2f ms (par) vs %.2f ms (seq)\n", parSolveTime, Texec * 1000.0);
	}
    
	finalizeMPI();
	
    return (EXIT_SUCCESS);
}

void sequentialSolve()
{
    float pM[rowCount][columnCount]; // Previous Matrix (shortened for equation brevity)
    float cM[rowCount][columnCount]; // Current Matrix (shortened for equation brevity)
    initializeMatrix(rowCount, columnCount, pM);
    initializeMatrix(rowCount, columnCount, cM);
    
    int i, j, k;
    
    // Perform the calculations
    for (k = 1; k <= timeSteps; k++)
    {
        for (i = 1; i < rowCount - 1; i++)
        {
            for (j = 1; j < columnCount - 1; j++)
            {
				spinWait(5);
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

void parallelSolve()
{
	// Determine the size of a quarter of the matrix (the area we need to calculate)
	int qColCount = (columnCount - 1) / 2;
	int qRowCount = (rowCount - 1) / 2;
	int divisions = qColCount * qRowCount;
	
	int i, j, procCount = 0, aggregatorIndex = 0;
	int sectionRowCount = 0, sectionColCount = 0;
	
	// Determine the amount of processes we need for calculations
	if (procAvailableCount > divisions)
		procCount = divisions;
	else
	{
		for (i = procAvailableCount; i > 0 && procCount == 0; i--)
		{
			if (divisions % i == 0)
				procCount = i;
		}
	}
	
	// If we have spare processes, we can have a dedicated aggregator
	if (procAvailableCount > procCount)
		aggregatorIndex = procCount;
		
	double timeStart, timeEnd, Texec;
	struct timeval tp;
	if (procRank == aggregatorIndex)
	{
		gettimeofday (&tp, NULL);
		timeStart = (double) (tp.tv_sec) + (double) (tp.tv_usec) / 1e6;
	}
	
	// Determine the best section size
	/*for (i = sqrt(procCount); i > 0 && sectionRowCount == 0; i--)
	{
		if (qColCount % i == 0 && qRowCount % (procCount / (qColCount / i)) == 0)
		{
			sectionColCount = i;
			sectionRowCount = qRowCount / (procCount / (qColCount / i));
		}
	}*/
	for (i = qColCount; i > 0 && sectionRowCount == 0; i--)
	{
		if (procCount / (qColCount / i) <= 0)
		{
			printf("WARNING : THIS SHOULDN'T HAPPEN");
			continue;
		}
		if (qColCount % i == 0 && qRowCount % (procCount / (qColCount / i)) == 0)
		{
			sectionColCount = i;
			sectionRowCount = qRowCount / (procCount / (qColCount / i));
		}
	}
	
	if (sectionRowCount == 0 || sectionColCount == 0)
		printf("There was an error in determining section size.");
	
	// Perform actual calculations and communications
	if (procRank < procCount)
	{
		float baseMatrix[rowCount][columnCount];
		initializeMatrix(rowCount, columnCount, baseMatrix);
		
		// Define section as rectangle
		int secX, secY, secW, secH, secCountHor, secCountVer;
		secW = sectionColCount;
		secH = sectionRowCount;
		secCountHor = (qColCount / secW);
		secCountVer = (qRowCount / secH);
		secX = secW * (procRank % secCountHor);
		secY = secH * (procRank / secCountHor);
		
		// Each process needs to setup it's own pair of matrices
		float pM[secH + 2][secW + 2]; // Previous Matrix (shortened for equation brevity)
		float cM[secH + 2][secW + 2]; // Current Matrix (shortened for equation brevity)
		
		for (i = secY; i <= secY + secH + 1; i++)
		{
			for (j = secX; j <= secX + secW + 1; j++)
			{
				pM[i - secY][j - secX] = baseMatrix[i][j];
				cM[i - secY][j - secX] = baseMatrix[i][j];
			}
		}
		
		// Each process must determine who it's neighbours are with whom to share information
		int secTop, secRight, secBottom, secLeft;
		secTop = secRight = secBottom = secLeft = -1;
		if (secY != 0)
			secTop = procRank - secCountHor;
		if (secX != 0)
			secLeft = procRank - 1;
		if ((int)(procRank / secCountHor) != secCountVer - 1)
			secBottom = procRank + secCountHor;
		if (procRank % secCountHor != secCountHor - 1)
			secRight = procRank + 1;
		
		// Create our input and output buffers
		float outTop[secW];
		float outBottom[secW];
		float outLeft[secH];
		float outRight[secH];
		float inTop[secW];
		float inBottom[secW];
		float inLeft[secH];
		float inRight[secH];
		
		int k;
		for (k = 1; k <= timeSteps; k++)
		{
			// Each process calculates it's inner matrix
			for (i = 1; i <= secH; i++)
			{
				for (j = 1; j <= secW; j++)
				{
					spinWait(5);
					if (k % 2 == 0)
						cM[i][j] = (1 - 4 * td /(h * h)) * pM[i][j] + (td / (h * h)) * (pM[i-1][j] + pM[i+1][j] + pM[i][j-1] + pM[i][j+1]);
					else
						pM[i][j] = (1 - 4 * td /(h * h)) * cM[i][j] + (td / (h * h)) * (cM[i-1][j] + cM[i+1][j] + cM[i][j-1] + cM[i][j+1]);
				}
			}
			
			// Each process sends border information to other processes
			// Data is stored from left to right or top to bottom. Tag identifies the current layer.
			if (secTop >= 0)
			{
				for (i = 0; i < secW; i++)
				{
					if (k % 2 == 0)
						outTop[i] = cM[1][i + 1];
					else
						outTop[i] = pM[1][i + 1];
				}
				
				MPI_Send (outTop, secW, MPI_FLOAT, secTop, k, MPI_COMM_WORLD);
			}
			if (secBottom >= 0)
			{
				for (i = 0; i < secW; i++)
				{
					if (k % 2 == 0)
						outBottom[i] = cM[secH][i + 1];
					else
						outBottom[i] = pM[secH][i + 1];
				}
				
				MPI_Send (outBottom, secW, MPI_FLOAT, secBottom, k, MPI_COMM_WORLD);
			}
			if (secLeft >= 0)
			{
				for (i = 0; i < secH; i++)
				{
					if (k % 2 == 0)
						outLeft[i] = cM[i + 1][1];
					else
						outLeft[i] = pM[i + 1][1];
				}
				
				MPI_Send (outLeft, secH, MPI_FLOAT, secLeft, k, MPI_COMM_WORLD);
			}
			if (secRight >= 0)
			{
				for (i = 0; i < secH; i++)
				{
					if (k % 2 == 0)
						outRight[i] = cM[i + 1][secW];
					else
						outRight[i] = pM[i + 1][secW];
				}
				
				MPI_Send (outRight, secH, MPI_FLOAT, secRight, k, MPI_COMM_WORLD);
			}
			
			//if (procRank == 0) // [REMOVE]
			//{
			//	printMatrix(secH + 2, secW + 2, pM);
			//	printf("\n");
			//}
			// Each process waits for info about other borders and fills border information
			if (secTop >= 0)
			{
				MPI_Recv(inTop, secW, MPI_FLOAT, secTop, k, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				
				for (i = 0; i < secW; i++)
				{
					if (k % 2 == 0)
						cM[0][i + 1] = inTop[i];
					else
						pM[0][i + 1] = inTop[i];
				}
			}
			if (secBottom >= 0)
			{
				MPI_Recv(inBottom, secW, MPI_FLOAT, secBottom, k, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				
				for (i = 0; i < secW; i++)
				{
					if (k % 2 == 0)
						cM[secH + 1][i + 1] = inBottom[i];
					else
						pM[secH + 1][i + 1] = inBottom[i];
				}
			}
			else
			{
				for (i = 0; i < secW; i++)
				{
					int sourceRow = rowCount % 2 == 0 ? secH : secH -1;
					if (k % 2 == 0)
						cM[secH + 1][i + 1] = cM[sourceRow][i + 1];
					else
						pM[secH + 1][i + 1] = pM[sourceRow][i + 1];
				}
			}
			
			if (secLeft >= 0)
			{
				MPI_Recv(inLeft, secH, MPI_FLOAT, secLeft, k, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				
				for (i = 0; i < secH; i++)
				{
					if (k % 2 == 0)
						cM[i + 1][0] = inLeft[i];
					else
						pM[i + 1][0] = inLeft[i];
				}
			}
			if (secRight >= 0)
			{
				MPI_Recv(inRight, secH, MPI_FLOAT, secRight, k, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				
				for (i = 0; i < secH; i++)
				{
					if (k % 2 == 0)
						cM[i + 1][secW + 1] = inRight[i];
					else
						pM[i + 1][secW + 1] = inRight[i];
				}
			}
			else
			{
				for (i = 0; i < secH; i++)
				{
					int sourceCol = columnCount % 2 == 0 ? secW : secW - 1;
					
					if (k % 2 == 0)
						cM[i + 1][secW + 1] = cM[i + 1][sourceCol];
					else
						pM[i + 1][secW + 1] = pM[i + 1][sourceCol];
				}
			}
		}
		
		//if (procRank == 0) // [REMOVE]
		//{
		//	if (k % 2 == 0)
		//		printMatrix(secH + 2, secW + 2, pM);
		//	else
		//		printMatrix(secH + 2, secW + 2, cM);
		//}
		
		// When done, send info to aggregator
		
		float aggregationBuffer[secW * secH];
		for (i = 0; i < secH; i++)
		{
			for (j = 0; j < secW; j++)
			{
				if (k % 2 == 0)
					aggregationBuffer[i * secW + j] = pM[i + 1][j + 1];
				else
					aggregationBuffer[i * secW + j] = cM[i + 1][j + 1];
			}
		}
		
		MPI_Send(aggregationBuffer, secW * secH, MPI_FLOAT, aggregatorIndex, 9999, MPI_COMM_WORLD);
		
		// Debug info about process setup
		if (procRank == 0)
		{
			printf("Quarter matrix: %d x %d, Used proc: %d / %d, Aggregator process index: %d\n", qColCount, qRowCount, procCount, procAvailableCount, aggregatorIndex);
			printf("%d x %d sections of dimension: %d x %d\n", secCountHor, secCountVer, sectionColCount, sectionRowCount);
		}
	}
	
	// Aggregators will aggregate
	if (procRank == aggregatorIndex)
	{
		int entriesPerMessage = sectionRowCount * sectionColCount;
		float baseMatrix[rowCount][columnCount];
		float aggregationBuffer[entriesPerMessage];
		initializeMatrix(rowCount, columnCount, baseMatrix);
		
		int secX, secY;
				
		for (i = 0; i < procCount; i++)
		{
			secX = sectionColCount * (i % (qColCount / sectionColCount)) + 1;
			secY = sectionRowCount * (i / (qColCount / sectionColCount)) + 1;
		
			MPI_Recv(aggregationBuffer, entriesPerMessage, MPI_FLOAT, i, 9999, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			
			for (j = 0; j < entriesPerMessage; j++)
			{
				baseMatrix[j / sectionColCount + secY][j % sectionColCount + secX] = aggregationBuffer[j];
			}
		}
		
		// Fill out the other 3 quarters
		for (i = 1; i < rowCount / 2; i++)
		{
			for (j = 1; j < columnCount / 2; j++)
			{
				baseMatrix[rowCount - i - 1][j] = baseMatrix[i][j];
				baseMatrix[i][columnCount - j - 1] = baseMatrix[i][j];
				baseMatrix[rowCount - i - 1][columnCount - j - 1] = baseMatrix[i][j];
			}
		}
		
		printMatrix(rowCount, columnCount, baseMatrix);
		
		gettimeofday (&tp, NULL);
		timeEnd = (double) (tp.tv_sec) + (double) (tp.tv_usec) / 1e6;
		Texec = timeEnd - timeStart;
		
		printf("Parallel solve time : %.2f ms\n", Texec * 1000.0);
		parSolveTime = Texec * 1000.0;
		if (procRank != 0)
		{
			float confirm[1];
			confirm[0] = parSolveTime;
			MPI_Send(confirm, 1, MPI_FLOAT, 0, 9998, MPI_COMM_WORLD); 
		}
	}
	
	// Wait for the aggregator to finish before moving on to sequential solve
	if (procRank == 0 && aggregatorIndex != 0)
	{
		float confirm[1];
		MPI_Recv(confirm, 1, MPI_FLOAT, aggregatorIndex, 9998, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		parSolveTime = confirm[0];
	}
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
            printf ("%5.1f ", matrix[i][j]);
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
    } while ((endTime.tv_sec - startTime.tv_sec) * 1000000 + (endTime.tv_usec - startTime.tv_usec) < milliseconds);
    
    return;
}

// Exists only to improve main function readability
void initializeMPI()
{
	MPI_Init(NULL, NULL);

	MPI_Comm_size(MPI_COMM_WORLD, &mpiProcCount);

	MPI_Comm_rank(MPI_COMM_WORLD, &procRank);

	MPI_Get_processor_name(procName, &procNameLength);
}

void finalizeMPI()
{
	MPI_Finalize();
}