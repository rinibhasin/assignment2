

#include <stdio.h>
#include <float.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <float.h>
#include <stdlib.h>
#include <time.h>
#include<math.h>
#include<stdbool.h>
#include <omp.h>
#include <stdlib.h>

int readNumOfCoords(char *fileName);
double **readCoords(char *filename, int numOfCoords);
void *writeTourToFile(int *tour, int tourLength, char *filename);


double calculateDistance(double x1, double y1, double x2, double y2) {
    // Calculate the Euclidean distance between two points.
    return sqrt(((x1 - x2) * (x1 - x2)) + ((y1 - y2) * (y1 - y2)));
}

double **calculateDistanceMatrix(double **coordinates, int numOfCoords, double **distanceMatrix) {

    int i =0; int j =0;
    double x1 =0;
    double x2 =0;
    double y1 =0;
    double y2 =0;
    double distance = 0;

#pragma omp parallel for collapse(2) private(i, j, x1, y1, x2, y2, distance) shared(numOfCoords)
    for (i = 0; i < numOfCoords; i++) {
        for (j = 0; j < numOfCoords; j++) {

            x1 = coordinates[i][0];
            y1 = coordinates[i][1];
            x2 = coordinates[j][0];
            y2 = coordinates[j][1];

            distance = calculateDistance(x1, y1, x2, y2);

            distanceMatrix[i][j] = distance;

        }
    }

    return distanceMatrix;
}

void cheapestInsertion(double **distanceMatrix, int numOfCoords, char *outputFileName)
{
    double shortestTour = DBL_MAX;
    int *shortestTourArray =  (int *)malloc((numOfCoords+1) * sizeof(int *));

    for(int top = 0; top<numOfCoords; top++)
    {
        int startingNode = top;

        int visitedCount = 0;

        int *tour = (int*)malloc((numOfCoords+1)*sizeof(int));
        bool *visited = (bool*)malloc(numOfCoords*sizeof(bool));
        int m=0;
        for(m=0; m<numOfCoords; m++)
        {
            tour[m] = 0;
            visited[m] =0;
        }

        tour[numOfCoords] = 0;

        // Initialise with the first vertex
        tour[0] = startingNode;
        visited[startingNode] = true;
        visitedCount++;

        // Find the nearest vertex
        double minimumDistance = DBL_MAX;

        int nearestVertex;
        int i = 0;

        int noOfThreads = omp_get_max_threads();

        for(i = 0; i <numOfCoords; i++)
        {
            if(visited[i] == false && distanceMatrix[0][i]< minimumDistance)
            {
                minimumDistance = distanceMatrix[0][i];
                nearestVertex = i;
            }
        }

        // Add the nearest vertex in the tour
        tour[1]= nearestVertex;
        visited[nearestVertex] = true;
        visitedCount++; // 2
        tour[2] = startingNode;

        double *minimumAdditionalCosts = (double*)malloc(noOfThreads*sizeof(double));
        int *positions = (int*)malloc(noOfThreads*sizeof(int));
        int *nearestVertexes = (int*)malloc(noOfThreads*sizeof(int));

        int y =0;
        while(visitedCount < numOfCoords)
        {
            for(y =0;y< noOfThreads; y++)
            {
                minimumAdditionalCosts[y] = DBL_MAX;
                positions[y] =0;
                nearestVertexes[y]=0;
            }

            double minimumAdditionalCost = DBL_MAX;

            int minN;
            int minUnvisited;
            double additionalCost;
            int j;int threadID;
            // tour = {0,1}

            #pragma omp parallel for collapse(2) private(i,j, additionalCost, threadID) shared(visited, distanceMatrix, minimumAdditionalCosts, positions,nearestVertexes)
            for (i = 0; i < visitedCount; i++) {
                // unvisited nodes
                for (j = 0; j < numOfCoords; j++) {
                    threadID  = omp_get_thread_num();
                    // check for unvisited nodes
                    if (!visited[j]) {
                        // j =2
                        additionalCost = distanceMatrix[j][tour[i]] + distanceMatrix[j][tour[i + 1]] -
                                         distanceMatrix[tour[i]][tour[i + 1]];
                        if (additionalCost < minimumAdditionalCosts[threadID]) {

                            minimumAdditionalCosts[threadID] = additionalCost;
                            positions[threadID] = i;
                            nearestVertexes[threadID] = j;
                        }
                    }
                }
            }

            int x=0;
            for(x =0; x< noOfThreads; x++)
            {
                if(minimumAdditionalCosts[x]< minimumAdditionalCost)
                {
                    minimumAdditionalCost = minimumAdditionalCosts[x];
                    minN = positions[x];
                    minUnvisited = nearestVertexes[x];
                }
            }

            // Make space to add unvisited node to computed index
            for(i = visitedCount; i > minN; i--)
            {
                tour[i+1] = tour[i];
            }

            // add the node to tour
            visited[minUnvisited] = true;
            tour[minN+1] = minUnvisited;


            visitedCount++;

        }

        double totalLength = 0;
        for (i = 0; i <=numOfCoords; i++) {
            printf("%d ", tour[i]);
            if(i>0) {
                totalLength += distanceMatrix[tour[i]][tour[i - 1]];
            }
        }

        if(totalLength < shortestTour)
        {

            printf("Found tour shorter than current tour");
            printf("shortest tour now starting with %d", tour[0]);

            shortestTour = totalLength;
            double tourLength = visitedCount+1;

            //make copy
            int copy =0;
            for(copy =0; copy <numOfCoords+1; copy++)
            {
                shortestTourArray[copy] = tour[copy];
            }
        }

        else
        {
            printf("Current tour size:");
            printf("%f", totalLength);
            printf("\n");

            printf("Shortes tour size:");
            printf("%f", shortestTour);
            printf("\n");
            printf("Found tour longer than current tour");
            printf("");
        }
    }


    writeTourToFile(shortestTourArray, numOfCoords+1, outputFileName);



}


int main(int argc, char *argv[]) {

    // Taking default file names if user didn't provide input
    char *fileName = "16_coords.coord";
    char *outputfile = "output.txt";


    if (argc > 1) {
        fileName = argv[1];
        outputfile = argv[2];
    }

    double start, end;
    double time_taken;

    start = omp_get_wtime();;


    int numOfCoords = readNumOfCoords(fileName);
    double **coordinates = readCoords(fileName, numOfCoords);

    double **distanceMatrix = (double **)malloc(numOfCoords * sizeof(double *));

    int i = 0;
#pragma omp parallel for private(i)
    for (i = 0; i < numOfCoords; i++) {
        distanceMatrix[i] = (double *)malloc(numOfCoords * sizeof(double));
    }

    distanceMatrix = calculateDistanceMatrix(coordinates, numOfCoords, distanceMatrix);

    cheapestInsertion(distanceMatrix, numOfCoords, outputfile);

    end = omp_get_wtime();
    time_taken = (end - start);

    printf("Cheapest insertion for %d nodes completed\n", numOfCoords);
    printf("The time taken is %fs .\n", time_taken);

    // Free memory
    for (i = 0; i < numOfCoords; i++) {
        free(coordinates[i]);
    }
    free(coordinates);

    for (i = 0; i < numOfCoords; i++) {
        free(distanceMatrix[i]);
    }
    free(distanceMatrix);

    return 0;
}

