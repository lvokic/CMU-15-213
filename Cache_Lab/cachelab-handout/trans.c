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

void block_transpose(int M, int N, int A[N][M], int B[M][N], int *row_start, int *col_start) {
    int tmp;
    if (*row_start + 18 >= N && *col_start + 18 >= M) {
        for (int i = *row_start; i < N; ++i) {
            for (int j = *col_start; j < M; ++j) {
                tmp = A[i][j];
                B[j][i] = tmp;
            }
        }
        *row_start = N;
        *col_start = M;
    } else if (*row_start + 18 >= N) {
        for (int i = *row_start; i < N; ++i) {
            for (int j = *col_start; j < *col_start + 18; ++j) {
                tmp = A[i][j];
                B[j][i] = tmp;
            }
        }
        *col_start += 18;
    } else if (*col_start + 18 >= M) {
        for (int i = *row_start; i < *row_start + 18; ++i) {
            for (int j = *col_start; j < M; ++j) {
                tmp = A[i][j];
                B[j][i] = tmp;
            }
        }
        *row_start += 18;
        *col_start = 0;
    } else {
        for (int i = *row_start; i < *row_start + 18; ++i) {
            for (int j = *col_start; j < *col_start + 18; ++j) {
                tmp = A[i][j];
                B[j][i] = tmp;
            }
        }
        *col_start += 18;
    }
}

/* Read each 32x32 tile row before writing its transposed values. */
static void block_transpose_32(int M, int N, int A[N][M], int B[M][N],
                               int *row_start, int *col_start) {
    int i;
    int a0, a1, a2, a3, a4, a5, a6, a7;

    for (i = *row_start; i < *row_start + 8; ++i) {
        a0 = A[i][*col_start];
        a1 = A[i][*col_start + 1];
        a2 = A[i][*col_start + 2];
        a3 = A[i][*col_start + 3];
        a4 = A[i][*col_start + 4];
        a5 = A[i][*col_start + 5];
        a6 = A[i][*col_start + 6];
        a7 = A[i][*col_start + 7];
        B[*col_start][i] = a0;
        B[*col_start + 1][i] = a1;
        B[*col_start + 2][i] = a2;
        B[*col_start + 3][i] = a3;
        B[*col_start + 4][i] = a4;
        B[*col_start + 5][i] = a5;
        B[*col_start + 6][i] = a6;
        B[*col_start + 7][i] = a7;
    }
    *col_start += 8;
    if (*col_start == M) {
        *col_start = 0;
        *row_start += 8;
    }
}

/* Transpose an 8x8 tile using B to stage its upper-right quadrant. */
static void block_transpose_64(int M, int N, int A[N][M], int B[M][N],
                               int *row_start, int *col_start) {
    int i;
    int a0, a1, a2, a3, a4, a5, a6, a7;

    /* Store A00 in place and stage A01 in B's upper-right quadrant. */
    for (i = *row_start; i < *row_start + 4; ++i) {
        a0 = A[i][*col_start];
        a1 = A[i][*col_start + 1];
        a2 = A[i][*col_start + 2];
        a3 = A[i][*col_start + 3];
        a4 = A[i][*col_start + 4];
        a5 = A[i][*col_start + 5];
        a6 = A[i][*col_start + 6];
        a7 = A[i][*col_start + 7];
        B[*col_start][i] = a0;
        B[*col_start + 1][i] = a1;
        B[*col_start + 2][i] = a2;
        B[*col_start + 3][i] = a3;
        B[*col_start][i + 4] = a4;
        B[*col_start + 1][i + 4] = a5;
        B[*col_start + 2][i + 4] = a6;
        B[*col_start + 3][i + 4] = a7;
    }

    /* Exchange staged A01 with A10, one B row at a time. */
    for (i = *col_start; i < *col_start + 4; ++i) {
        a0 = B[i][*row_start + 4];
        a1 = B[i][*row_start + 5];
        a2 = B[i][*row_start + 6];
        a3 = B[i][*row_start + 7];
        a4 = A[*row_start + 4][i];
        a5 = A[*row_start + 5][i];
        a6 = A[*row_start + 6][i];
        a7 = A[*row_start + 7][i];
        B[i][*row_start + 4] = a4;
        B[i][*row_start + 5] = a5;
        B[i][*row_start + 6] = a6;
        B[i][*row_start + 7] = a7;
        B[i + 4][*row_start] = a0;
        B[i + 4][*row_start + 1] = a1;
        B[i + 4][*row_start + 2] = a2;
        B[i + 4][*row_start + 3] = a3;
    }

    /* Transpose the remaining A11 quadrant. */
    for (i = *row_start + 4; i < *row_start + 8; ++i) {
        a0 = A[i][*col_start + 4];
        a1 = A[i][*col_start + 5];
        a2 = A[i][*col_start + 6];
        a3 = A[i][*col_start + 7];
        B[*col_start + 4][i] = a0;
        B[*col_start + 5][i] = a1;
        B[*col_start + 6][i] = a2;
        B[*col_start + 7][i] = a3;
    }

    *col_start += 8;
    if (*col_start == M) {
        *col_start = 0;
        *row_start += 8;
    }
}

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
    int row_start = 0, col_start = 0;
    while (row_start != N && col_start != M) {
        if (M == 32 && N == 32)
            block_transpose_32(M, N, A, B, &row_start, &col_start);
        else if (M == 64 && N == 64)
            block_transpose_64(M, N, A, B, &row_start, &col_start);
        else
            block_transpose(M, N, A, B, &row_start, &col_start);
    }
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

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
    registerTransFunction(trans, trans_desc); 

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

