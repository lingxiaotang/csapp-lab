/*
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/*
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    int a1;
    int a2;
    int a3;
    int a4;
    int a5;
    int a6;
    int a7;
    int a8;
    int i, j, ii, jj;
    if (M==N&&M==32)
    {
        
        for (i=0;i<N/8;i++)
        {
            for (j=0;j<M/8;j++)
            {
                for (ii=0;ii<8;ii++)
                {
                    a1=A[i*8+ii][j*8+0];
                    a2=A[i*8+ii][j*8+1];
                    a3=A[i*8+ii][j*8+2];
                    a4=A[i*8+ii][j*8+3];
                    a5=A[i*8+ii][j*8+4];
                    a6=A[i*8+ii][j*8+5];
                    a7=A[i*8+ii][j*8+6];
                    a8=A[i*8+ii][j*8+7];
                    B[j*8+0][i*8+ii]=a1;
                    B[j*8+1][i*8+ii]=a2;
                    B[j*8+2][i*8+ii]=a3;
                    B[j*8+3][i*8+ii]=a4;
                    B[j*8+4][i*8+ii]=a5;
                    B[j*8+5][i*8+ii]=a6;
                    B[j*8+6][i*8+ii]=a7;
                    B[j*8+7][i*8+ii]=a8;
                }
            }
        }
    }

    if (M==N&&M==64)
    {
        for (i=0;i<N;i+=4)
        {
            for (j=0;j<M;j+=4)
            {
                for(ii=i;ii<i+4;ii+=2)
                {
                    a1=A[ii][j];
                    a2=A[ii][j+1];
                    a3=A[ii][j+2];
                    a4=A[ii][j+3];
                    a5=A[ii+1][j];
                    a6=A[ii+1][j+1];
                    a7=A[ii+1][j+2];
                    a8=A[ii+1][j+3];

                    B[j][ii]=a1;
                    B[j+1][ii]=a2;
                    B[j+2][ii]=a3;
                    B[j+3][ii]=a4;
                    B[j][ii+1]=a5;
                    B[j+1][ii+1]=a6;
                    B[j+2][ii+1]=a7;
                    B[j+3][ii+1]=a8;
                }
            }
        }
    }

    if (N==67&&M==61)
    {
        for(i=0;i<N;i+=16){
            for(j=0;j<M;j+=16){
                for(ii=i;ii<i+16&&ii<N;ii++){
                    for(jj=j;jj<j+16&&jj<M;jj++){
                        B[jj][ii]=A[ii][jj];
                    }
                }
            }
        }
    }

}

/*
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started.
 */



 /*
  * trans - A simple baseline transpose function, not optimized for the cache.
  */

  /*
   * registerFunctions - This function registers your transpose
   *     functions with the driver.  At runtime, the driver will
   *     evaluate each of the registered functions and summarize their
   *     performance. This is a handy way to experiment with different
   *     transpose strategies.
   */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc);

    /* Register any additional transpose functions */
    //registerTransFunction(trans, trans_desc); 

}

/*
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

