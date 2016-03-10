/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 *
 *
 *
 * Name: Ningna Wang
 * Andrew ID: ningnaw
 *
 */
  
#include <stdio.h>
#include "cachelab.h"
#include "contracts.h"


#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <math.h>
#include <strings.h>
#include <string.h>

int is_transpose(int M, int N, int A[N][M], int B[M][N]);
void transpose_32by32_Mtx(int M, int N, int A[N][M], int B[M][N]);
void transpose_64by64_Mtx(int M, int N, int A[N][M], int B[M][N]);
void transpose_61by67_Mtx(int M, int N, int A[N][M], int B[M][N]);


/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. The REQUIRES and ENSURES from 15-122 are included
 *     for your convenience. They can be removed if you like.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    REQUIRES(M > 0);
    REQUIRES(N > 0);

    if (M == 32) {
        transpose_32by32_Mtx(M, N, A, B);
    }
    else if (M == 64) {
        transpose_64by64_Mtx(M, N, A, B);
    }
    else if (M == 61 && N == 67) {
        transpose_61by67_Mtx(M, N, A, B);
    }


    ENSURES(is_transpose(M, N, A, B));
}


/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 


/*
 * Helper function that transpose 32x32 matrix
 */
void transpose_32by32_Mtx(int M, int N, int A[N][M], int B[M][N]) {
     
    int bs = 8; // block size 8x8
    int dia_val;
    int dia_idx;

    // i is row index of block
    // j is col index of block
    for (int row = 0; row < N; row += bs) {
        for (int col = 0; col < M; col += bs) {
            for (int i = row; i < row + bs; i++) {
                for (int j = col; j < col + bs; j++) {
                    // For diagonal element: 
                    // B[j][i] will kick A[i][j] out from 
                    // cache, since they are in same row
                    if (i == j && row == col) { 
                        dia_val = A[i][j];
                        dia_idx = i;
                    }
                    else {
                        B[j][i] = A[i][j];
                    }
                }
                if (row == col) {
                    B[dia_idx][dia_idx] = dia_val;
                }
            }
        }
    }
}

/*
 * Helper function that transpose 64x64 matrix
 */
 void transpose_64by64_Mtx(int M, int N, int A[N][M], int B[M][N]) {
     int b1, b2, b3, b4, b5, b6, b7, b8;
     for (int row = 0; row < N; row += 8) {
         for (int col = 0; col < M; col += 8) {
             // traverse A's 8x8 block row by row
             for (int k = 0; k < 8; k++) {

                 // handle B's a,b,c,d
                 // handle a along with b
                 if (k < 4) {
                    // handle b: 

                    // store 8 elements of A'b in to temp
                    b1 = A[row+k][col];
                    b2 = A[row+k][col+1];
                    b3 = A[row+k][col+2];
                    b4 = A[row+k][col+3];

                    b5 = A[row+k][col+4];
                    b6 = A[row+k][col+5];
                    b7 = A[row+k][col+6];
                    b8 = A[row+k][col+7];

                    // swap 4 elements in B'a
                    B[col][row+k] = b1;
                    B[col+1][row+k] = b2;
                    B[col+2][row+k] = b3;
                    B[col+3][row+k] = b4;

                    // store B's c transpose into B'b
                    // based on temp
                    B[col][row+k+4] = b5;
                    B[col+1][row+k+4] = b6;
                    B[col+2][row+k+4] = b7;
                    B[col+3][row+k+4] = b8;

                 }
                 // scan A'c and A'd, handle B'c with B'b
                 else if (k >= 4) {
                     
                     // find a row in A'c to fill the col in B'b

                     // store a row of A'c to fill the col in B'b and B'd
                     b1 = A[row+k][col];
                     b2 = A[row+k][col+1];
                     b3 = A[row+k][col+2];
                     b4 = A[row+k][col+3];

                     // scan a col of B's b to local variables
                     // right now B's b stores B's c transpose
                     b5 = B[col][row+k];
                     b6 = B[col+1][row+k];
                     b7 = B[col+2][row+k];
                     b8 = B[col+3][row+k];

                     B[col][row+k] = b1;
                     B[col+1][row+k] = b2;
                     B[col+2][row+k] = b3;
                     B[col+3][row+k] = b4;
                     

                     // use local variables to fill a col of B'c
                     B[col+4][row+k-4] = b5;
                     B[col+4+1][row+k-4] = b6;
                     B[col+4+2][row+k-4] = b7;
                     B[col+4+3][row+k-4] = b8;

                     b5 = A[row+k][col+4];
                     b6 = A[row+k][col+5];
                     b7 = A[row+k][col+6];
                     b8 = A[row+k][col+7];
                     
                     // handle B's d, 
                     B[col+4][row+k] = b5;
                     B[col+4+1][row+k] = b6;
                     B[col+4+2][row+k] = b7;
                     B[col+4+3][row+k] = b8;

                 }
             } // for k
         }
     }
 }


/*
 * Helper function that transpose 61x67 matrix
 */
void transpose_61by67_Mtx(int M, int N, int A[N][M], int B[M][N]) {

    int dia_val;
    int dia_idx;
    int b_c = 16; // block col
    int b_r = 4; // block row, block: 4x16

    // A[i][j] = B[j][i]
    // i is row index of A block
    // j is col index of A block
    for (int col = 0; col < M; col += b_c) {
        for (int row = 0; row < N; row += b_r) {
            for (int i = row; i < row + b_r && i < N; i++) {
                for (int j = col; j < col + b_c && j < M; j++) {
                    // For diagonal element: 
                    // B[j][i] will kick A[i][j] out from 
                    // cache, since they are in same row
                    if (i == j && row == col) { 
                        dia_val = A[i][j];
                        dia_idx = i;
                    }
                    else {
                        B[j][i] = A[i][j];
                    }
                }
                if (row == col) {
                    B[dia_idx][dia_idx] = dia_val;
                }
            }
        }
    }
}


/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    REQUIRES(M > 0);
    REQUIRES(N > 0);

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

    ENSURES(is_transpose(M, N, A, B));
}



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
    // registerTransFunction(trans, trans_desc); 

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

