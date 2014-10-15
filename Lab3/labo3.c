#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <mpi.h>

int nbLigne;
int nbCol;
int nbStep;

int main(int argc, char** argv)
{
	nbLigne = 0;
	nbCol = 0;
	nbStep = 0;
	double tempsDiscretise = 0.0;
	double tailleCoteSubdivision = 0.0;
	int nbProc; = 0
	
	if (argc >= 7)
    {
		nbLigne = atoi (argv[1]);
		nbCol = atoi (argv[2]);
		nbStep = atoi (argv[3]);
		nbProc = atoi (argv[6]);
		
		tempsDiscretise = atof(argv[4]);
		tailleCoteSubdivision = atof(argv[5]);
	}
	else
	{
		printf("Pas assez d'argument donnée en paramètre!");
	}
	double timeStart, timeEnd, Texec;
	struct timeval tp;
	gettimeofday (&tp, NULL); // Début du chronomètre
	timeStart = (double) (tp.tv_sec) + (double) (tp.tv_usec) / 1e6;
	
	//Insérer appel vers logique ici
	
	gettimeofday (&tp, NULL); // Fin du chronomètre
	timeEnd = (double) (tp.tv_sec) + (double) (tp.tv_usec) / 1e6;
	Texec = timeEnd – timeStart; //Temps d'exécution en secondes

	printf("Temps d'exécution total : %d s",Texec);
}

void finalizeMPI()
{
	MPI_Finalize();
}

void initializeMatrix(long long matrix[nbLigne][nbCol])
{
    int i, j;
    
    for (i = 0; i < nbCol; i++)
        for (j = 0; j < nbLigne; j++)
            matrix[i][j] = (i*(nbCol-i-1))*(j*(nbLigne-j-1))
}

void printMatrix(long long matrix[nbLigne][nbCol])
{
    int i, j;
    
    for (i = 0; i < nbLigne; i++)
	{
        for (j = 0; j < nbCol; j++)
		{
            printf ("%8lld ", matrix[i][j]);
        }
            printf ("\n");
    }
}
			