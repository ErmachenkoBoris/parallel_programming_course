//  Copyright 2019 Chvanov Leonid

#define _SCL_SECURE_NO_WARNINGS
#include <tbb/tbb.h>
#include <ctime>
#include <iostream>
#include <algorithm>
#include <utility>


void printArray(int* arr, int size) {
    for (int i = 0; i < size; i++)
        std::cout << arr[i] << " ";
    std::cout << std::endl;
}

bool compareArrays(int* arr0, int* arr1, int size) {
    for (int i = 0; i < size; i++)
        if (arr0[i] != arr1[i])
            return false;
    return true;
}

void copyArray(int* src, int* dest, int size) {
    for (int i = 0; i < size; i++)
        dest[i] = src[i];
}

void insertSort(int* arr, int size, int step) {
    for (int j = step; j < size; j += step) {
        int key = arr[j];
        int i = j - step;
        while (i >= 0 && arr[i] > key) {
            arr[i + step] = arr[i];
            i -= step;
        }
        arr[i + step] = key;
    }
}

void shellsort(int* arr, int size) {
    for (int step = size / 2; step > 0; step /= 2)
        tbb::parallel_for(tbb::blocked_range<int>(0, step),
                            [&](const tbb::blocked_range<int> &r) {
                                for (int i = r.begin(); i < r.end(); i++) {
                                    insertSort(arr + i, size - i, step);
                                }
                            });
}

int main(int argc, char **argv) {
    const int size = ((argc >= 2) && (atoi(argv[1]) > 0)) ? atoi(argv[1]) : 10;
    const int threadNum = ((argc >= 3) && (atoi(argv[2]) > 0)) ? atoi(argv[2]) : 4;
    int *arrSTL, *arrLin, *arrPar;
    tbb::tick_count startTime;
    double stlTime, linearTime, parallelTime;

    arrSTL = new int[size];
    arrLin = new int[size];
    arrPar = new int[size];

    for (int i = 0; i < size; i++)
        arrSTL[i] = i;
    std::random_shuffle(arrSTL, arrSTL + size);

    copyArray(arrSTL, arrLin, size);
    copyArray(arrSTL, arrPar, size);

    if (size < 100) {
        std::cout << "Initial array: ";
        printArray(arrSTL, size);
    }

    startTime =  tbb::tick_count::now();
    std::sort(arrSTL, arrSTL + size);
    stlTime =  (tbb::tick_count::now() - startTime).seconds();

    tbb::task_scheduler_init init(1);
    startTime = tbb::tick_count::now();
    shellsort(arrLin, size);
    linearTime = (tbb::tick_count::now() - startTime).seconds();
    init.terminate();

    init.initialize(threadNum);
    startTime = tbb::tick_count::now();
    shellsort(arrPar, size);
    parallelTime = (tbb::tick_count::now() - startTime).seconds();
    init.terminate();

    if (size < 100) {
        std::cout << "Sorted array: ";
        printArray(arrPar, size);
    }

    if (compareArrays(arrSTL, arrPar, size))
        std::cout << "Parallel shell sort is working" << std::endl;
    else
        std::cout << "Parallel shell sort isn't working!" << std::endl;
    std::cout << "STL time:      " << stlTime << std::endl
        << "Linear time:   " << linearTime << std::endl
        << "Parallel time: " << parallelTime << std::endl
        << "Parallel acceleration = "
        <<  linearTime / parallelTime << std::endl;

    delete[] arrSTL;
    delete[] arrLin;
    delete[] arrPar;
    return 0;
}
