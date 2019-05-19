// Copyright 2019 Tipanov Daniil
#include <tbb/tbb.h>
#include <iostream>
#include <cmath>
#include <ctime>

int** matrCreate(int N) {
    int** matr = new int*[N];
    for (int i = 0; i < N; i++)
        matr[i] = new int[N];

    return matr;
}

void delMatr(int** matr, int N) {
    for (int i = 0; i < N; i++)
        delete[] matr[i];
    delete[] matr;
}

void printMatr(int** matr, int N) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++)
            std::cout << matr[i][j] << " ";
        std::cout << std::endl;
    }

    std::cout << std::endl;
}

void genRandMatr(int** matr1, int** matr2, int N) {
    std::srand(static_cast<unsigned int>(time(0)));
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            matr1[i][j] = std::rand() % 10;
            matr2[i][j] = std::rand() % 10;
        }
    }
}

void simple_alg(int** A, int** B, int** C, int N) {
    int sum;
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++) {
            sum = 0;
            for (int k = 0; k < N; k++)
                sum += A[i][k] * B[k][j];
            C[i][j] = sum;
        }
}

void Add(int** A, int** B, int** C, int N) {
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            C[i][j] = A[i][j] + B[i][j];
}

void Add(int** A, int** B, int** C, int** D, int** E, int N) {
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            E[i][j] = A[i][j] + B[i][j] + C[i][j] + D[i][j];
}

void Sub(int** A, int** B, int** C, int N) {
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            C[i][j] = A[i][j] - B[i][j];
}

void Sub(int** A, int** B, int** C, int** D, int** E, int N) {
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            E[i][j] = A[i][j] + B[i][j] + C[i][j] - D[i][j];
}

bool isEqual(int** matr1, int** matr2, int N) {
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            if (matr1[i][j] != matr2[i][j])
                return false;
        }
    }
    return true;
}

void str_alg(int** matr1, int** matr2, int** matr3, int N, int threshold) {
    if (N <= threshold) {
        simple_alg(matr1, matr2, matr3, N);
    } else {
        N = N / 2;

        int** A[4]; int** B[4]; int** C[4]; int** P[7];

        int** TMP1 = matrCreate(N); int** TMP2 = matrCreate(N);
        int** TMP3 = matrCreate(N); int** TMP4 = matrCreate(N);
        int** TMP5 = matrCreate(N); int** TMP6 = matrCreate(N);
        int** TMP7 = matrCreate(N); int** TMP8 = matrCreate(N);
        int** TMP9 = matrCreate(N); int** TMP10 = matrCreate(N);

        /* Videlenie pamyati pod vspomogatelnie matrici */
        for (int i = 0; i < 4; i++) {
            A[i] = matrCreate(N);
            B[i] = matrCreate(N);
            C[i] = matrCreate(N);
        }

        for (int i = 0; i < 7; i++)
            P[i] = matrCreate(N);

        for (int i = 0; i < N; i++)
            for (int j = 0; j < N; j++) {
                A[0][i][j] = matr1[i][j];
                A[1][i][j] = matr1[i][j + N];
                A[2][i][j] = matr1[i + N][j];
                A[3][i][j] = matr1[i + N][j + N];

                B[0][i][j] = matr2[i][j];
                B[1][i][j] = matr2[i][j + N];
                B[2][i][j] = matr2[i + N][j];
                B[3][i][j] = matr2[i + N][j + N];
            }

        Add(A[0], A[3], TMP1, N);
        Add(B[0], B[3], TMP2, N);
        str_alg(TMP1, TMP2, P[0], N, threshold);  // (A11 + A22)*(B11 + B22)

        Add(A[2], A[3], TMP3, N);
        str_alg(TMP3, B[0], P[1], N, threshold);  // (A21 + A22)*B11

        Sub(B[1], B[3], TMP4, N);
        str_alg(A[0], TMP4, P[2], N, threshold);  // A11*(B12 - B22)

        Sub(B[2], B[0], TMP5, N);
        str_alg(A[3], TMP5, P[3], N, threshold);  // A22*(B21 - B11)

        Add(A[0], A[1], TMP6, N);
        str_alg(TMP6, B[3], P[4], N, threshold);  // (A11 + A12)*B22

        Sub(A[2], A[0], TMP7, N);
        Add(B[0], B[1], TMP8, N);
        str_alg(TMP7, TMP8, P[5], N, threshold);  // (A21 - A11)*(B11 + B12)

        Sub(A[1], A[3], TMP9, N);
        Add(B[2], B[3], TMP10, N);
        str_alg(TMP9, TMP10, P[6], N, threshold);  // (A12 - A22)*(B21 + B22)

        Sub(P[0], P[3], P[6], P[4], C[0], N);  // P1 + P4 - P5 + P7
        Add(P[2], P[4], C[1], N);  // P3 + P5
        Add(P[1], P[3], C[2], N);  // P2 + P4
        Sub(P[0], P[2], P[5], P[1], C[3], N);  // P1 - P2 + P3 + P6

        for (int i = 0; i < N; i++)
            for (int j = 0; j < N; j++) {
                matr3[i][j] = C[0][i][j];
                matr3[i][j + N] = C[1][i][j];
                matr3[i + N][j] = C[2][i][j];
                matr3[i + N][j + N] = C[3][i][j];
            }

        for (int i = 0; i < 4; i++) {
            delMatr(A[i], N);
            delMatr(B[i], N);
            delMatr(C[i], N);
        }

        for (int i = 0; i < 7; i++) {
            delMatr(P[i], N);
        }

        delMatr(TMP1, N); delMatr(TMP2, N); delMatr(TMP3, N); delMatr(TMP4, N); delMatr(TMP5, N);
        delMatr(TMP6, N); delMatr(TMP7, N); delMatr(TMP8, N); delMatr(TMP9, N); delMatr(TMP10, N);
    }
}

class StrTask : public tbb::task {
 public:
    int** matr1; int** matr2; int** matr3; int N; int threshold;

    StrTask(int** matr1_, int**  matr2_, int** matr3_, int N_, int threshold_) :
        matr1(matr1_), matr2(matr2_), matr3(matr3_), N(N_), threshold(threshold_)
    {}
    tbb::task* execute() {
        if (N <= threshold) {
            simple_alg(matr1, matr2, matr3, N);
        } else {
            tbb::task_list list;
            int count = 1;

            N = N / 2;

            int** A[4]; int** B[4]; int** C[4]; int** P[7];

            int** TMP1 = matrCreate(N); int** TMP2 = matrCreate(N); int** TMP3 = matrCreate(N);
            int** TMP4 = matrCreate(N); int** TMP5 = matrCreate(N); int** TMP6 = matrCreate(N);
            int** TMP7 = matrCreate(N); int** TMP8 = matrCreate(N); int** TMP9 = matrCreate(N);
            int** TMP10 = matrCreate(N);

            for (int i = 0; i < 4; i++) {
                A[i] = matrCreate(N);
                B[i] = matrCreate(N);
                C[i] = matrCreate(N);
            }

            for (int i = 0; i < 7; i++)
                P[i] = matrCreate(N);

            for (int i = 0; i < N; i++)
                for (int j = 0; j < N; j++) {
                    A[0][i][j] = matr1[i][j];
                    A[1][i][j] = matr1[i][j + N];
                    A[2][i][j] = matr1[i + N][j];
                    A[3][i][j] = matr1[i + N][j + N];

                    B[0][i][j] = matr2[i][j];
                    B[1][i][j] = matr2[i][j + N];
                    B[2][i][j] = matr2[i + N][j];
                    B[3][i][j] = matr2[i + N][j + N];
                }

            Add(A[0], A[3], TMP1, N);
            Add(B[0], B[3], TMP2, N);
            count++;
            list.push_back(*new(allocate_child()) StrTask(TMP1, TMP2, P[0], N, threshold));

            Add(A[2], A[3], TMP3, N);
            count++;
            list.push_back(*new(allocate_child()) StrTask(TMP3, B[0], P[1], N, threshold));

            Sub(B[1], B[3], TMP4, N);
            count++;
            list.push_back(*new(allocate_child()) StrTask(A[0], TMP4, P[2], N, threshold));

            Sub(B[2], B[0], TMP5, N);
            count++;
            list.push_back(*new(allocate_child()) StrTask(A[3], TMP5, P[3], N, threshold));

            Add(A[0], A[1], TMP6, N);
            count++;
            list.push_back(*new(allocate_child()) StrTask(TMP6, B[3], P[4], N, threshold));

            Sub(A[2], A[0], TMP7, N);
            Add(B[0], B[1], TMP8, N);
            count++;
            list.push_back(*new(allocate_child()) StrTask(TMP7, TMP8, P[5], N, threshold));

            Sub(A[1], A[3], TMP9, N);
            Add(B[2], B[3], TMP10, N);
            count++;
            list.push_back(*new(allocate_child()) StrTask(TMP9, TMP10, P[6], N, threshold));

            set_ref_count(count);
            spawn_and_wait_for_all(list);

            Sub(P[0], P[3], P[6], P[4], C[0], N);
            Add(P[2], P[4], C[1], N);
            Add(P[1], P[3], C[2], N);
            Sub(P[0], P[2], P[5], P[1], C[3], N);

            for (int i = 0; i < N; i++)
                for (int j = 0; j < N; j++) {
                    matr3[i][j] = C[0][i][j];
                    matr3[i][j + N] = C[1][i][j];
                    matr3[i + N][j] = C[2][i][j];
                    matr3[i + N][j + N] = C[3][i][j];
                }

            for (int i = 0; i < 4; i++) {
                delMatr(A[i], N);
                delMatr(B[i], N);
                delMatr(C[i], N);
            }

            for (int i = 0; i < 7; i++) {
                delMatr(P[i], N);
            }

            delMatr(TMP1, N); delMatr(TMP2, N); delMatr(TMP3, N); delMatr(TMP4, N);
            delMatr(TMP5, N); delMatr(TMP6, N); delMatr(TMP7, N); delMatr(TMP8, N);
            delMatr(TMP9, N); delMatr(TMP10, N);
        }
        return nullptr;
    }
};

void par_str_alg(int** matr1, int** matr2, int** matr3, int N, int threshold) {
    StrTask& a = *new(tbb::task::allocate_root()) StrTask(matr1, matr2, matr3, N, threshold);
    tbb::task::spawn_root_and_wait(a);
}

int main(int argc, char** argv) {
    int** matr_A = nullptr;
    int** matr_B = nullptr;
    int** matr_Rez_Str = nullptr;
    int** matr_Rez_Par_Str = nullptr;
    int** matr_Rez_Check = nullptr;

    int  N, thr = 64;

    double StartStrAlg = 0;
    double TimeStrAlg = 0;
    int k;
    int threads_num;

    /* std::cout << "Enter the degree of two for size: " << std::endl;
    std::cin >> k; */
    k = 10;
    /* std::cout << "Enter the number of threads: " << std::endl;
    std::cin >> threads_num; */
    threads_num = 4;

    N = static_cast<int>(pow(2.0, k));
    tbb::task_scheduler_init init(threads_num);

    std::cout << "Size of matrix: " << N << " x " << N << std::endl;

    /* Creating and filling matrix */
    matr_A = matrCreate(N);
    matr_B = matrCreate(N);
    matr_Rez_Str = matrCreate(N);
    matr_Rez_Par_Str = matrCreate(N);
    matr_Rez_Check = matrCreate(N);
    genRandMatr(matr_A, matr_B, N);

    /* Strassen alg */
    StartStrAlg = clock() / static_cast<double>(CLOCKS_PER_SEC);
    str_alg(matr_A, matr_B, matr_Rez_Str, N, thr);
    TimeStrAlg = (clock() / static_cast<double>(CLOCKS_PER_SEC)) - StartStrAlg;

    /* Parallel Strassen alg */
    tbb::tick_count tP1 = tbb::tick_count::now();
    par_str_alg(matr_A, matr_B, matr_Rez_Par_Str, N, thr);
    tbb::tick_count tP2 = tbb::tick_count::now();


    /* Printing matrix */
    /*  std::cout << "Matrix A: " << std::endl;
    printMatr(matr_A, N);
    std::cout << std::endl;
    std::cout << "Matrix B: " << std::endl;
    printMatr(matr_B, N);
    std::cout << std::endl;
    std::cout << "Matrix C: " << std::endl;
    printMatr(matr_Rez_Str, N);
    std::cout << std::endl;  */


    /* Check results Strassen and parallel Str alg */
    if (isEqual(matr_Rez_Str, matr_Rez_Par_Str, N))
        std::cout << "Check: Maxtrix matr_Rez_Str and matr_Rez_Par_Str are equal";
    else
        std::cout << "Check: Maxtrix matr_Rez_Str and matr_Rez_Par_Str are not equal";
    std::cout << std::endl;

    /* Printing time */
    std::cout << "Strassen algorithm time = " << TimeStrAlg << std::endl;
    std::cout << "Strassen parallel algorithm time = " << (tP2 - tP1).seconds() << std::endl;
    std::cout << std::endl;

    delMatr(matr_Rez_Check, N);
    delMatr(matr_Rez_Str, N);
    delMatr(matr_Rez_Par_Str, N);
    delMatr(matr_B, N);
    delMatr(matr_A, N);
    return 0;
}
