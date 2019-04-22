// Copyright Ermachenko Boris 2019
// 1
#include <omp.h>
#include <math.h>
#include <tbb/tbb.h>
#include <iostream>
#include <ctime>
#define THREADS 12
const double PI = 3.1415;
int blockX = 0;
int blockY = 0;
double** getNewArr(int width, int height) {
    double** new_arrImage = new double*[height];
    for (int i = 0; i < height; i++) {
        new_arrImage[i] = new double[width];
    }
    return new_arrImage;
}
double** LinierFilterGauss(double** arrImage, double** new_arrImage,
    int width, int height) {
    int n = 3;
    double** w = getNewArr(n, n);
    double sigma = 1.0;
    double r;
    double s = 2.0 * sigma * sigma;
    for (int i = -n / 2; i < n / 2; i++) {
        for (int j = -n / 2; j < n / 2; j++) {
            r = sqrt(i * i + j * j);
            w[i + n / 2][j + n / 2] = (exp(-(r*r) / s)) / (PI * s);
        }
    }
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            new_arrImage[i][j] = arrImage[i][j];
        }
    }
    for (int yi = 0; yi < height; yi++) {
        for (int xj = 0; xj < width; xj++) {
            double color = 0;
            double kSum = 0;
            for (int i = -n / 2; i < n / 2; i++) {
                for (int j = -n / 2; j < n / 2; j++) {
                    kSum += w[i + n / 2][j + n / 2];
                    if ((i + yi) >= 0 && (i + yi) < height
                        && (j + xj) >= 0 && (j + xj) < width)
                        color += arrImage[static_cast<int>(i + yi)][static_cast<int>(j + xj)]
                        * w[i + n / 2][j + n / 2];
                }
            }
            if (kSum <= 0) kSum = 1;
            color /= kSum;
            if (color < 0) color = 0;
            if (color > 255) color = 255;
            if ((yi) >= 0 && (yi) < height && (xj) >= 0 && (xj) < width)
                new_arrImage[static_cast<int>(yi)][static_cast<int>(xj)] = color;
        }
    }
    return new_arrImage;
}
void getBlockNum(int throwds) {
    int tmp = sqrt(throwds);
    while (throwds % tmp != 0) {
        tmp--;
    }
    blockX = tmp;
    blockY = throwds / tmp;
}
int* arrayLengthBlocksX(int width) {
    int* rezArrX = new int[blockX];
    int rest = width % blockX;
    for (int i = 0; i < blockX; i++) {
        rezArrX[i] = width / blockX;
    }
    if (rest != 0) {
        int k = 0;
        while (rest != 0) {
            rezArrX[k%rest]++;
            rest--;
            k++;
        }
    }
    return rezArrX;
}
int * arrayLengthBlocksY(int height) {
    int* rezArrY = new int[blockY];
    int rest = height % blockY;
    for (int i = 0; i < blockY; i++) {
        rezArrY[i] = height / blockY;
    }
    if (rest != 0) {
        int k = 0;
        while (rest != 0) {
            rezArrY[k%rest]++;
            rest--;
            k++;
        }
    }
    return rezArrY;
}
double** ParallelFilterGaussOMP(double** arrImage,
    double** new_arrImage, int width, int height) {
    int n = 3;
    double** w = getNewArr(n, n);
    double sigma = 1.0;
    double r;
    double s = 2.0 * sigma * sigma;
    for (int i = -n / 2; i < n / 2; i++) {
        for (int j = -n / 2; j < n / 2; j++) {
            r = sqrt(i * i + j * j);
            w[i + n / 2][j + n / 2] = (exp(-(r*r) / s)) / (PI * s);
        }
    }
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            new_arrImage[i][j] = arrImage[i][j];
        }
    }
    getBlockNum(THREADS);
    int* lengthX = arrayLengthBlocksX(width);
    int* lengthY = arrayLengthBlocksY(height);
#pragma omp parallel
    {
        int numberThroat = omp_get_thread_num();
        int startX = 0;
        int startY = 0;
        // int RestX = 0;
        int row = 0;
        int count = 0;
        int column = 0;
        for (int i = 0; i < blockY; i++) {
            if (i > 0) startY += lengthY[i - 1];
            for (int j = 0; j < blockX; j++) {
                if (j > 0) startX += lengthX[j - 1];
                count++;
                if (count > numberThroat)break;
                column++;
            }
            if (count > numberThroat)break;
            row++;
            column = 0;
            startX = 0;
        }
        for (int yi = startY; yi < startY + lengthY[row]; yi++) {
            for (int xj = startX; xj < startX + lengthX[column]; xj++) {
                double color = 0;
                double kSum = 0;
                for (int i = -n / 2; i < n / 2; i++) {
                    for (int j = -n / 2; j < n / 2; j++) {
                        kSum += w[i + n / 2][j + n / 2];
                        if ((i + yi) >= 0 && (i + yi) < height
                            && (j + xj) >= 0 && (j + xj) < width)
                            color += arrImage[static_cast<int>(i + yi)][static_cast<int>(j + xj)]
                            * w[i + n / 2][j + n / 2];
                    }
                }
                if (kSum <= 0) kSum = 1;
                color /= kSum;
                if (color < 0) color = 0;
                if (color > 255) color = 255;
                if ((yi) >= 0 && (yi) < height &&
                    (xj) >= 0 && (xj) < width)
                    new_arrImage[static_cast<int>(yi)][static_cast<int>(xj)]
                    = color;
            }
        }
    }
    return new_arrImage;
}
double** ParallelFilterGaussTBB(double** arrImage,
    double** new_arrImage, int width, int height) {
    int n = 3;
    double** w = getNewArr(n, n);
    double sigma = 1.0;
    double r;
    double s = 2.0 * sigma * sigma;
    for (int i = -n / 2; i < n / 2; i++) {
        for (int j = -n / 2; j < n / 2; j++) {
            r = sqrt(i * i + j * j);
            w[i + n / 2][j + n / 2] = (exp(-(r*r) / s)) / (PI * s);
        }
    }
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            new_arrImage[i][j] = arrImage[i][j];
        }
    }
    getBlockNum(THREADS);
    int* lengthX = arrayLengthBlocksX(width);
    int* lengthY = arrayLengthBlocksY(height);
    tbb::parallel_for(tbb::blocked_range2d<int>(0, height, lengthY[0], 0, width, lengthX[0]),
        [&](const tbb::blocked_range2d<int, int>& r) {
            printf("1 \n");
            for (int yi = r.rows().begin()+n/2; yi < r.rows().end()-n / 2; yi++) {
                for (int xj = r.cols().begin() + n / 2; xj < r.cols().end() - n / 2; xj++) {
                    double color = 0;
                    double kSum = 0;
                    for (int i = -n / 2; i < n / 2; i++) {
                        for (int j = -n / 2; j < n / 2; j++) {
                            kSum += w[i + n / 2][j + n / 2];
                                color += arrImage[static_cast<int>(i + yi)][static_cast<int>(j + xj)]
                                * w[i + n / 2][j + n / 2];
                        }
                    }
                    if (kSum <= 0) kSum = 1;
                    color /= kSum;
                    if (color < 0) color = 0;
                    if (color > 255) color = 255;
                    new_arrImage[static_cast<int>(yi)][static_cast<int>(xj)]
                        = color;
                }
            }
        });
    return new_arrImage;
}
double** getImage(int width, int height) {
    srand(time(0));
    double** arrImage = getNewArr(width, height);
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            arrImage[i][j] = static_cast<int>(std::rand()) % 256;
        }
    }
    return arrImage;
}
void ShowArr(double** arr, int width, int height) {
    std::cout << "\n";
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            std::cout << arr[i][j] << " ";
        }
        std::cout << "\n";
    }
}
int main() {
    double tStart, tEnd;
    int height = 5000;
    int width = 5000;
    double** arrImage = getImage(width, height);
    double** new_arrImage_Liner = getNewArr(width, height);
    omp_set_num_threads(THREADS);
    // liner
    tStart = omp_get_wtime();
    new_arrImage_Liner = LinierFilterGauss(arrImage,
        new_arrImage_Liner, width, height);
    tEnd = omp_get_wtime();
    float tmp = tEnd - tStart;
    printf("Time linier :  %.4lf \n", tEnd - tStart);
    // OpenMp
    double** new_arrImage_Parallel = getNewArr(width, height);
    tStart = omp_get_wtime();
    new_arrImage_Parallel = ParallelFilterGaussOMP(arrImage,
       new_arrImage_Parallel, width, height);
    tEnd = omp_get_wtime();
    printf("Time parallel OpenMp:  %.4lf \n", tEnd - tStart);
    printf("Koef :  %.4lf \n", tmp / (tEnd - tStart));
    // tbb
    tbb::task_scheduler_init init(tbb::task_scheduler_init::deferred);
    init.initialize(THREADS);
    double** new_arrImage_Parallel_TBB = getNewArr(width, height);
    tbb::tick_count start = tbb::tick_count::now();
    new_arrImage_Parallel_TBB = ParallelFilterGaussTBB(arrImage,
        new_arrImage_Parallel_TBB, width, height);
    tbb::tick_count end = tbb::tick_count::now();
    printf("Time parallel TBB:  %.4lf \n", (end - start).seconds());
    printf("Koef :  %.4lf \n", tmp / (end - start).seconds());
    // ShowArr(arrImage, width, height);
    // printf("\n");
    // ShowArr(new_arrImage_Liner, width,height);
    // printf("\n");
    // ShowArr(new_arrImage_Parallel, width, height);
    // printf(" \n");
    // ShowArr(new_arrImage_Parallel_TBB, width, height);
    return 0;
}