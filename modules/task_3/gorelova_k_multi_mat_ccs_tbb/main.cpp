//  Copyright 2019 Gorelova Ksenia

#include <tbb/tbb.h>
#include <iostream>
#include <vector>
#include <random>
#include <cstring>
#include <ctime>

#include "tbb/task_scheduler_init.h"
#include "tbb/parallel_for.h"
#include "tbb/blocked_range.h"

using std::vector;
struct ccsMatrix {
    int N;
    int NZ;
    double* val;
    int* row;
    int* ColIndex;
};

class multiplicate {
    ccsMatrix A, B;
    vector<int>* rows;
    vector<double>* value;
    int * columnIndex;
 public :
    multiplicate(ccsMatrix a, ccsMatrix b,
        vector<int>* row, vector<double>* val,
        int *colIndex) : A(a), B(b), rows(row),
        value(val), columnIndex(colIndex) {}
    void operator() (const tbb::blocked_range<int> &r) const {
        int begin = r.begin();
        int end = r.end();
        int N = A.N;
        int i, j, k;
        int *temp = new int[N];
        for (i = begin; i < end; i++) {
            memset(temp, -1, N * sizeof(int));
            int ind1 = A.ColIndex[i], ind2 = A.ColIndex[i + 1];
            for (j = ind1; j < ind2; j++) {
                int row = A.row[j];
                temp[row] = j;
            }
            for (j = 0; j < N; j++) {
                double sum = 0;
                int ind3 = B.ColIndex[j], ind4 = B.ColIndex[j + 1];
                for (k = ind3; k < ind4; k++) {
                    int brow = B.row[k];
                    int aind = temp[brow];
                    if (aind != -1)
                        sum += A.val[aind] * B.val[k];
                }
                if (fabs(sum) > 0) {
                    rows[i].push_back(j);
                    value[i].push_back(sum);
                    columnIndex[i]++;
                }
            }
        }
        delete[] temp;
    }
};

void initMatrix(ccsMatrix *mtx, int nz, int n) {
    mtx->NZ = nz;
    mtx->N = n;
    mtx->val = new double[nz];
    mtx->row = new int[nz];
    mtx->ColIndex = new int[n + 1];
}

void freeMatrix(ccsMatrix *mtx) {
    delete[] mtx->val;
    delete[] mtx->row;
    delete[] mtx->ColIndex;
}

void transMatrix(ccsMatrix A, ccsMatrix *AT) {
    int sum = 0;
    int index, colIn;
    double v;
    memset(AT->ColIndex, 0, (AT->N + 1) * sizeof(int));
    for (int i = 0; i < A.NZ; i++)
        AT->ColIndex[A.row[i] + 1]++;
    for (int i = 1; i <= A.N; i++) {
        int tmp = AT->ColIndex[i];
        AT->ColIndex[i] = sum;
        sum += tmp;
    }
    for (int i = 0; i < A.N; i++) {
        int row = i;
        for (int j = A.ColIndex[i]; j < A.ColIndex[i + 1]; j++) {
            v = A.val[j];
            colIn = A.row[j];
            index = AT->ColIndex[colIn + 1];
            AT->val[index] = v;
            AT->row[index] = row;
            AT->ColIndex[colIn + 1]++;
        }
    }
}

void generateMatrix(ccsMatrix *mtx, int n, int cntInCol) {
    int rows;
    bool exist;
    int nz = cntInCol * n;
    initMatrix(mtx, nz, n);
    for (int col = 0; col < n; col++) {
        for (int row = 0; row < cntInCol; row++) {
            do {
                rows = std::rand() % n;
                exist = false;
                for (int i = 0; i < row; i++)
                    if (rows == mtx->row[col*cntInCol + i])
                        exist = true;
                break;
            } while (exist == true);
            mtx->row[col*cntInCol + row] = rows;
        }
        for (int row = 0; row < cntInCol - 1; row++)
            for (int val = 0; val < cntInCol - 1; val++)
                if (mtx->row[col*cntInCol + val] >
                    mtx->row[col*cntInCol + val + 1]) {
                    int tmp = mtx->row[col*cntInCol + val];
                    mtx->row[col*cntInCol + val] =
                        mtx->row[col*cntInCol + val + 1];
                    mtx->row[col*cntInCol + val + 1] = tmp;
                }
    }
    for (int val = 0; val < nz; val++)
        mtx->val[val] = (std::rand() % 10000) / 1000.0f;
    int c = 0;
    for (int col = 0; col <= n; col++) {
        mtx->ColIndex[col] = c;
        c += cntInCol;
    }
}

void printMatrix(int n, ccsMatrix *mtx) {
    int i;
    int k = mtx[0].NZ;
    std::cout << "\n Value: ";
    for (i = 0; i < k; i++)
        std::cout << " " << mtx[0].val[i];
    std::cout << "\n Row: ";
    for (i = 0; i < k; i++)
        std::cout << " " << mtx[0].row[i];
    std::cout << "\n ColIndex: ";
    for (i = 0; i < n + 1; i++)
        std::cout << " " << mtx[0].ColIndex[i];
    std::cout << std::endl;
}

int multiply(ccsMatrix A, ccsMatrix B, ccsMatrix *C) {
    if (A.N != B.N)
        return 1;
    int N = A.N;
    vector<int> rows;
    vector<double> value;
    vector<int> columnIndex;
    int nz = 0;
    int *temp = new int[N];
    columnIndex.push_back(0);
    for (int i = 0; i < N; i++) {
        memset(temp, -1, N * sizeof(int));
        int ind1 = A.ColIndex[i], ind2 = A.ColIndex[i + 1];
        for (int j = ind1; j < ind2; j++) {
            int row = A.row[j];
            temp[row] = j;
        }
        for (int j = 0; j < N; j++) {
            double sum = 0;
            int ind3 = B.ColIndex[i], ind4 = B.ColIndex[i + 1];
            for (int k = ind3; k < ind4; k++) {
                int brow = B.row[k];
                int aind = temp[brow];
                if (aind != -1)
                    sum += A.val[aind] * B.val[k];
            }
            if (fabs(sum) > 0) {
                rows.push_back(j);
                value.push_back(sum);
                nz++;
            }
        }
        columnIndex.push_back(nz);
    }
    initMatrix(C, nz, N);
    for (int j = 0; j < nz; j++) {
        C->row[j] = rows[j];
        C->val[j] = value[j];
    }
    for (int i = 0; i <= N; i++)
        C->ColIndex[i] = columnIndex[i];
    delete[] temp;
    return 0;
}

int multiplyTbb(ccsMatrix A, ccsMatrix B, ccsMatrix *C) {
    if (A.N != B.N)
        return 1;
    int N = A.N;
    tbb::task_scheduler_init init(4);
    vector<int>* rows = new vector<int>[N];
    vector<double>* value = new vector<double>[N];
    int* columnIndex = new int[N + 1];
    memset(columnIndex, 0, sizeof(int) * N);
    int grainsize = 10;
    tbb::parallel_for(tbb::blocked_range<int>(0, A.N, grainsize),
        multiplicate(A, B, rows, value, columnIndex));
    int NZ = 0;
    for (int i = 0; i < N; i++) {
        int tmp = columnIndex[i];
        columnIndex[i] = NZ;
        NZ += tmp;
    }
    columnIndex[N] = NZ;
    initMatrix(C, NZ, N);
    int count = 0;
    for (int i = 0; i < N; i++) {
        int size = rows[i].size();
        memcpy(&C->row[count], &rows[i][0],
            size * sizeof(int));
        memcpy(&C->val[count], &value[i][0],
            size * sizeof(double));
        count += size;
    }
    memcpy(C->ColIndex, &columnIndex[0], (N + 1) * sizeof(int));

    return 0;
}

int main(int argc, char** argv) {
    int N = (argc != 1) ? atoi(argv[1]) : 10;
    int cntInCol = (argc != 1) ? atoi(argv[2]) : 5;
    srand((unsigned int)time(0));
    ccsMatrix A, B, AT, C;
    generateMatrix(&A, N, cntInCol);
    initMatrix(&AT, A.NZ, A.N);
    transMatrix(A, &AT);
    freeMatrix(&A);
    generateMatrix(&B, N, cntInCol);
    initMatrix(&C, cntInCol, N);
    tbb::tick_count t1 = tbb::tick_count::now();
    multiply(AT, B, &C);
    tbb::tick_count t2 = tbb::tick_count::now();
    std::cout << "Time usual is " << (t2 - t1).seconds() << std::endl;
    tbb::tick_count tbbT1 = tbb::tick_count::now();
    multiplyTbb(AT, B, &C);
    tbb::tick_count tbbT2 = tbb::tick_count::now();
    std::cout << "Time tbb is " << (tbbT2 - tbbT1).seconds() << std::endl;

    freeMatrix(&AT);
    freeMatrix(&B);
    freeMatrix(&C);
    return 0;
}
