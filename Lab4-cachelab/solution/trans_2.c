void transpose_64by64_Mtx(int M, int N, int A[N][M], int B[M][N]) {
    int temp;
    int b_temp;

    for (int col = 0; col < M; col += 8) {
        for (int row = 0; row < N; row +=8) {
            // A[i][j] = B[j][i]
            // divide A by 8x8 blocks, 
            // and divide each 8x8 block into 4 4x4 blocks 
            // |a|b|
            // |c|d|
            for (int i = row; i < row + 8; i++) {
                for (int j = col; j < col + 8; j++) {
                    if (i < row + 4 && j >= col + 4) {
                        continue;
                    }
                    // handle a along with b
                    if (i < row + 4 && j < col + 4) {
                        B[j][i] = A[i][j];
                        // handle b: store c's transpose into b
                        // for (int k = row; k < row + 4; k++) {
                        //     temp = A[k][j+4];
                        //     B[j][k+4] = temp; 
                        // }

                        temp = A[i][j+4]; // c's transpose
                        B[row+j-col][i+col+4] = temp; 

                        if (i == 0 && j == 0) {
                            printf("-----part1----:\n");
                            printf("row: %d\n", row);
                            printf("col: %d\n", col);
                            printf("i: %d\n", i);
                            printf("j: %d\n", j);
                            printf("A40: %d\n", A[4][0]);
                            printf("A04: %d\n", A[0][4]);
                            printf("B04: %d\n", B[0][4]);
                            printf("temp: %d\n", temp);
                        }

                        if (j >= 8 && B[0][4] != A[4][0]) {
                            printf("------part6-------\n");
                            printf("row: %d\n", row);
                            printf("col: %d\n", col);
                            printf("i: %d\n", i);
                            printf("j: %d\n", j);
                            printf("A40: %d\n", A[4][0]);
                            printf("A04: %d\n", A[0][4]);
                            printf("B04: %d\n", B[0][4]);
                            printf("temp: %d\n", temp);
                        }
                    }
                    // handle c using b
                    else if (i >= row + 4 && j < col + 4) {
                        // scan a row of b to local variables
                        // right now b stores c's transpose
                        b_temp = B[i-4][j+4];
                        // find values in A to fill the row in b
                        B[i-4][j+4] = A[j+4][i-4];
                        // use local variables to fill a row of c
                        B[i][j] = b_temp;

                        if (i == 4 && j==0)  {
                            // if (A[12][0] != B[0][12]) {
                                printf("------part2-------\n");
                                printf("row: %d\n", row);
                                printf("col: %d\n", col);
                                printf("i: %d\n", i);
                                printf("j: %d\n", j);
                                printf("A40: %d\n", A[4][0]);
                                printf("B04: %d\n", B[0][4]);
                            // }
                        }

                        if (B[0][4] != A[4][0]) {
                            printf("------part4-------\n");
                            printf("row: %d\n", row);
                            printf("col: %d\n", col);
                            printf("i: %d\n", i);
                            printf("j: %d\n", j);
                            printf("A40: %d\n", A[4][0]);
                            printf("B04: %d\n", B[0][4]);
                            printf("B[i-4][j+4]: %d\n", B[i-4][j+4]);
                            printf("A[j+4][i-4];: %d\n", A[j+4][i-4]);
                            printf("B[i][j]: %d\n", B[i][j]);
                            // exit(-1);
                        }
                    }
                    // handle d
                    else if (i >= row + 4 && j >= col + 4) {
                        B[j][i] = A[i][j];
                        if (B[0][4] != A[4][0]) {
                            printf("------part5-------\n");
                            printf("row: %d\n", row);
                            printf("col: %d\n", col);
                            printf("i: %d\n", i);
                            printf("j: %d\n", j);
                            printf("A40: %d\n", A[4][0]);
                            printf("B04: %d\n", B[0][4]);
                            // exit(-1);
                        }
                    } 

                    if (i == 0 && j == 4) {
                        printf("-------part3------\n");
                        printf("row: %d\n", row);
                        printf("col: %d\n", col);
                        printf("i: %d\n", i);
                        printf("j: %d\n", j);
                        printf("A40: %d\n", A[4][0]);
                        printf("B04: %d\n", B[0][4]);
                    }

                    // if (B[0][12] == A[12][0]) {
                    //     printf("!!!!!!!!!!!!Equal!!!!!!!\n");
                    // }
                    if (i >= 4 && j >= 0 && B[0][4] != A[4][0]) {
                        printf("!!!!!!!!!!!!Not Equal!!!!!!!\n");
                        printf("row: %d\n", row);
                        printf("col: %d\n", col);
                        printf("i: %d\n", i);
                        printf("j: %d\n", j);
                        printf("A40: %d\n", A[4][0]);
                        printf("B04: %d\n", B[0][4]);
                        exit(-1);
                    }
                }
            }
        }
    }
}