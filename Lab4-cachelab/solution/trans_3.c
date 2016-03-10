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