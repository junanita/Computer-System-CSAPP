 void transpose_64by64_Mtx(int M, int N, int A[N][M], int B[M][N]) {

    int bs = 8; // block size = 8
    int k;
    int tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8;

    for (int row = 0; row < N; row += bs) {
        for (int col = 0; col < M; col += bs) {
            
            // traverse each block
            for (k = col; k < col + 8; k++){
                // In A, we store the current row in local variables
                tmp1 = A[k][row];
                tmp2 = A[k][row + 1];
                tmp3 = A[k][row + 2];
                tmp4 = A[k][row + 3];

                // for each block, save later 4 elements of a row
                if (k == col) {
                    tmp5 = A[k][row + 4];
                    tmp6 = A[k][row + 5];
                    tmp7 = A[k][row + 6];
                    tmp8 = A[k][row + 7];
                }

                // do swap after saving to reduce misses
                B[row][k] = tmp1;
                B[row+1][k] = tmp2; 
                B[row+2][k] = tmp3;
                B[row+3][k] = tmp4;
            }

            for (k = col + 7; k > col; k--) {
                tmp1 = A[k][row + 4];
                tmp2 = A[k][row + 5];
                tmp3 = A[k][row + 6];
                tmp4 = A[k][row + 7];
                B[row+4][k] = tmp1;
                B[row+4+1][k] = tmp2;
                B[row+4+2][k] = tmp3;
                B[row+4+3][k] = tmp4;
            }

            B[row+4][col] = tmp5;
            B[row+4+1][col] = tmp6;
            B[row+4+2][col] = tmp7;
            B[row+4+3][col] = tmp8;

        }
    } 
 }