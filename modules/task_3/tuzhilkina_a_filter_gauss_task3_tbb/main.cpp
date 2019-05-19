// Copyright 2019 Tuzhilkina Anastasia

#include <tbb/tbb.h>
#include <tbb/task_scheduler_init.h>
#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>

void InitMatr(int rows, int cols, int** m) {
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            m[i][j] = static_cast<int>(std::rand() % 255);
}


void InitKern(double kernel[3][3], int radius, double sigma) {
    double norm = 0;

    for (int i = -radius; i <= radius; i++)
        for (int j = -radius; j <= radius; j++) {
            kernel[i + radius][j + radius] = (exp(-(i * i + j * j) / (2 * sigma * sigma)));
            norm += kernel[i + radius][j + radius];
        }

    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            kernel[i][j] /= norm;
}

inline int Clamp(int value, int min, int max) {
    if (value < min)
        return min;

    if (value > max)
        return max;

    return value;
}

void par_mat(double kernel[3][3], int **picture1, int **picture2, int j, size_t size_rows, size_t size_cols) {
    for (size_t i = 0; i < size_rows; i++) {
        double temp = 0;
        for (int q = -1; q <= 1; q++)
            for (int l = -1; l <= 1; l++) {
                int idX = Clamp(i + q, 0, size_rows - 1);
                int idY = Clamp(j + l, 0, size_cols - 1);

                temp += picture1[idX][idY] * kernel[q + 1][l + 1];
            }
        picture2[i][j] = static_cast<int>(temp);
    }
}

void parallel_matrix_multiply(double kernel[3][3], int **picture1, int **picture2, size_t size_rows, size_t size_cols) {
    tbb::parallel_for(tbb::blocked_range<size_t>(0, size_cols), [=](const tbb::blocked_range<size_t>& r) {
        for (size_t j = r.begin(); j != r.end(); ++j) {
            par_mat(kernel, picture1, picture2, j, size_rows, size_cols);
        }
    });
}

int main() {
    int rows, cols;
    double kernel[3][3];

    rows = 5000, cols = 5000;

    int** picture = new int*[rows];
    int** res_tbb = new int*[rows];

    for (int i = 0; i < rows; i++) {
        picture[i] = new int[cols];
        res_tbb[i] = new int[cols];
    }

    InitMatr(rows, cols, picture);
    InitKern(kernel, 1, 1.0);

    tbb::tick_count t0 = tbb::tick_count::now();
    tbb::task_scheduler_init init(4);
    parallel_matrix_multiply(kernel, picture, res_tbb, rows, cols);
    tbb::tick_count t1 = tbb::tick_count::now();
    std::cout << "Time par TBB: " << (t1 - t0).seconds() << std::endl;

    for (int i = 0; i < rows; i++) {
        delete[] picture[i];
        delete[] res_tbb[i];
    }

    delete[] picture;
    delete[] res_tbb;

    return 0;
}

