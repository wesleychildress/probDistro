/****************************************************************************
* probDist.c
* DESCRIPTION:
*   -  Probability Distribution
*   -  Random vertices
*   -  Calculates Area
*   -
*   -
*   @@@@@@@@@  sizeOfWorld must be even and divisible  @@@@@@@@@
* AUTHORS: Alieu, Andre, Wesley
* LAST REVISED: 11/26/18
****************************************************************************/
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <math.h>
#define  ARRAYSIZE	4000
#define  MASTER		0

/* structs */
struct vertex{
    float x, y;
};
struct triangle{
    struct vertex r, s, t;
};

/* functions */
void randomInscribedTri(struct triangle *);
void printStruct(struct triangle);
float calculateArea(struct triangle);

/* main */
int main (int argc, char *argv[])
{
    int numtasks, taskid, rc, dest, offset, i, j, tag1, tag2, source, chunksize;
    MPI_Status status;
    /***** Initializations *****/
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &taskid);
    /* mpi struct vertex */
    MPI_Datatype dt_vertex;
    MPI_Type_contiguous(2, MPI_FLOAT, &dt_vertex);
    MPI_Type_commit(&dt_vertex);
    /* mpi struct triangle */
    MPI_Datatype dt_triangle;
    MPI_Type_contiguous(3, dt_vertex, &dt_triangle);
    MPI_Type_commit(&dt_triangle);
    /* array of vertices */
    struct triangle A[ARRAYSIZE];
    /* array for calculated areas */
    float area[ARRAYSIZE];

    if (ARRAYSIZE % numtasks != 0)
    {
        printf("Quitting. Array size of %d must be divisible by world size.\n", ARRAYSIZE);
        MPI_Abort(MPI_COMM_WORLD, rc);
        exit(0);
    }

    printf ("MPI task %d has started...\n", taskid);
    chunksize = (ARRAYSIZE / numtasks);
    tag2 = 1;
    tag1 = 2;
    /***** Master task only ******/
    if (taskid == MASTER) //Initialize the array
    {
        /* Send each task its portion of the array - master keeps 1st part */
        offset = chunksize;
        for (dest=1; dest<numtasks; dest++)
        {
            MPI_Send(&offset, 1, MPI_INT, dest, tag1, MPI_COMM_WORLD); // offset
            MPI_Send(&A[offset], chunksize, dt_triangle, dest, tag2, MPI_COMM_WORLD); // array of randoms
            MPI_Send(&area[offset], chunksize, MPI_FLOAT, dest, tag2, MPI_COMM_WORLD);

            printf("Sent %d elements to task %d offset= %d\n",chunksize,dest,offset);
            offset = offset + chunksize;
        }

        /* Master does its part of the work */
        offset = 0;
        for(i = 0; i < chunksize; i++)
        {
          // Assigning random values for each vertices
          randomInscribedTri(&A[i]);
          // Calculating the area using the coordinates of each vertice
          area[i] = calculateArea(A[i]);
        }

        /* Wait to receive results from each task */
        for (i=1; i<numtasks; i++)
        {
            source = i;
            MPI_Recv(&offset, 1, MPI_INT, source, tag1, MPI_COMM_WORLD, &status); // offset
            MPI_Recv(&A[offset], chunksize, dt_triangle, source, tag2, MPI_COMM_WORLD, &status); // A array
            MPI_Recv(&area[offset], chunksize, MPI_FLOAT, source, tag2, MPI_COMM_WORLD, &status); // area array
        }

        /* print sample results */
        printf("\n");
        printf("-----------------------\n");
        for(i=0; i<ARRAYSIZE; i++)
        {
            printStruct(A[i]);
            printf(" AREA: %0.3f\n", area[i]);
            printf("-----------------------\n");
        }

  }  /* end of master section */

/***** Non-master tasks only *****/
if (taskid > MASTER)
{
    srand(time(NULL));
    /* Receive my portion of array from the master task */
    source = MASTER;
    MPI_Recv(&offset, 1, MPI_INT, source, tag1, MPI_COMM_WORLD, &status);
    MPI_Recv(&A[offset], chunksize, dt_triangle, source, tag2, MPI_COMM_WORLD, &status);
    MPI_Recv(&area[offset], chunksize, MPI_FLOAT, source, tag2, MPI_COMM_WORLD, &status);

    for(i = offset; i < offset+chunksize; i++)
    {
      // Assigning random values for each vertices
      randomInscribedTri(&A[i]);
      // Calculating the area using the coordinates of each vertice
      area[i] = calculateArea(A[i]);
    }

    dest = MASTER;
    MPI_Send(&offset, 1, MPI_INT, dest, tag1, MPI_COMM_WORLD);
    MPI_Send(&A[offset], chunksize, dt_triangle, MASTER, tag2, MPI_COMM_WORLD);
    MPI_Send(&area[offset], chunksize, MPI_FLOAT, MASTER, tag2, MPI_COMM_WORLD);

} /* end of non-master */

MPI_Finalize();
}   /* end of main */

void randomInscribedTri(struct triangle *A)
{
  A->r.x = 0;
  A->r.y = 2 * ((float)rand()/(float)RAND_MAX);

  A->t.x = (float)rand()/(float)RAND_MAX;
  A->t.y = 0;

  A->s.x = (float)rand()/(float)RAND_MAX;
  A->s.y = ((-2 * A->s.x) + 2);
}

void printStruct(struct triangle A)
{
  printf("(%0.3f,%0.3f)\n",A.r.x, A.r.y);
  printf("(%0.3f,%0.3f)\n",A.s.x, A.s.y);
  printf("(%0.3f,%0.3f)\n",A.t.x, A.t.y);
}

float calculateArea(struct triangle A)
{
  float area;
  area = fabsf((A.r.x * (A.s.y -A.t.y)) + (A.s.x * (A.t.y - A.r.y)) + (A.t.x * (A.r.y - A.s.y)));

  return area/2;
}
