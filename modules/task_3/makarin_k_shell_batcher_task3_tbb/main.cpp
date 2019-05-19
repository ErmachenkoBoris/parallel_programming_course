#define _SCL_SECURE_NO_WARNINGS
// Copyright 2019 Kirill Makarin
#include <tbb/tbb.h>
#include <algorithm>
#include <ctime>
#include <functional>
#include <iomanip>
#include <iostream>
#include <random>
#include <utility>

void shellSort(int* array, int size) {
    for (int k = size / 2; k > 0; k /= 2) {
        for (int i = k; i < size; i++) {
            int j;
            int t = array[i];
            for (j = i; j >= k; j -= k) {
                if (t < array[j - k]) {
                    array[j] = array[j - k];
                } else {
                    break;
                }
            }
            array[j] = t;
        }
    }
}

class TBBEvenSplitter : public tbb::task {
 private:
    int* array;
    int* tmp;
    int size1;
    int size2;

 public:
    TBBEvenSplitter(int* data, int* temp, int size1_, int size2_)
        : array(data), tmp(temp), size1(size1_), size2(size2_) {}

    tbb::task* execute() {
        for (int i = 0; i < size1; i += 2) {
            tmp[i] = array[i];
        }

        int* array2 = array + size1;
        int a = 0, b = 0, i = 0;

        while ((a < size1) && (b < size2)) {
            if (tmp[a] <= array2[b]) {
                array[i] = tmp[a];
                a += 2;
            } else {
                array[i] = array2[b];
                b += 2;
            }

            i += 2;
        }

        if (a == size1) {
            for (int j = b; j < size2; j += 2, i += 2) {
                array[i] = array2[j];
            }
        } else {
            for (int j = a; j < size1; j += 2, i += 2) {
                array[i] = tmp[j];
            }
        }

        return NULL;
    }
};

class TBBOddSplitter : public tbb::task {
 private:
    int* array;
    int* tmp;
    int size1;
    int size2;

 public:
    TBBOddSplitter(int* data, int* temp, int size1_, int size2_)
        : array(data), tmp(temp), size1(size1_), size2(size2_) {}

    tbb::task* execute() {
        for (int i = 1; i < size1; i += 2) {
            tmp[i] = array[i];
        }

        int* array2 = array + size1;

        int a = 1, b = 1, i = 1;

        while ((a < size1) && (b < size2)) {
            if (tmp[a] <= array2[b]) {
                array[i] = tmp[a];
                a += 2;
            } else {
                array[i] = array2[b];
                b += 2;
            }

            i += 2;
        }

        if (a == size1) {
            for (int j = b; j < size2; j += 2, i += 2) {
                array[i] = array2[j];
            }
        } else {
            for (int j = a; j < size1; j += 2, i += 2) {
                array[i] = tmp[j];
            }
        }

        return NULL;
    }
};

class TBBSimpleComparator {
 private:
    int* array;

 public:
    explicit TBBSimpleComparator(int* data) : array(data) {}

    void operator()(const tbb::blocked_range<int>& r) const {
        int begin = r.begin(), end = r.end();

        for (int i = begin; i < end; i++) {
            if (array[2 * i] < array[2 * i - 1]) {
                int _tmp = array[2 * i - 1];
                array[2 * i - 1] = array[2 * i];
                array[2 * i] = _tmp;
            }
        }
    }
};

class TBBParallelSorter : public tbb::task {
 private:
    int* array;
    int* tmp;
    int size;
    int portion;

 public:
    TBBParallelSorter(int* data, int* temp, int size_, int portion_)
        : array(data), tmp(temp), size(size_), portion(portion_) {}

    tbb::task* execute() {
        if (size <= portion) {
            shellSort(array, size);
        } else {
            int s = size / 2 + (size / 2) % 2;

            TBBParallelSorter& sorter1 = *new (
                allocate_child()) TBBParallelSorter(array, tmp, s, portion);
            TBBParallelSorter& sorter2 =
                *new (allocate_child())
                    TBBParallelSorter(array + s, tmp + s, size - s, portion);

            tbb::task::set_ref_count(3);

            tbb::task::spawn(sorter1);
            tbb::task::spawn_and_wait_for_all(sorter2);

            TBBEvenSplitter& splitter1 = *new (
                allocate_child()) TBBEvenSplitter(array, tmp, s, size - s);
            TBBOddSplitter& splitter2 =
                *new (allocate_child()) TBBOddSplitter(array, tmp, s, size - s);

            tbb::task::set_ref_count(3);

            tbb::task::spawn(splitter1);
            tbb::task::spawn_and_wait_for_all(splitter2);

            parallel_for(tbb::blocked_range<int>(1, (size + 1) / 2),
                         TBBSimpleComparator(array));
        }

        return NULL;
    }
};

void TBBParallelSort(int* input, int size, int nThreads) {
    int* temp = new int[size];

    tbb::task_scheduler_init init(nThreads);

    int portion = size / nThreads;

    if (size % nThreads != 0) {
        portion++;
    }

    TBBParallelSorter& sorter =
        *new (tbb::task::allocate_root())
            TBBParallelSorter(input, temp, size, portion);
    tbb::task::spawn_root_and_wait(sorter);

    delete[] temp;
}

bool IsCorrectlySorted(int* linear_sorted_array, int* parallel_sorted_array,
                       const int size) {
    for (int i = 0; i < size; i++) {
        if (linear_sorted_array[i] != parallel_sorted_array[i]) {
            return false;
        }
    }

    return true;
}

void PrintArray(int* array, const int size) {
    for (int i = 0; i < size; i++) {
        std::cout << array[i] << " ";
    }
    std::cout << std::endl;
}

int main(int argc, char** argv) {
    int SIZE = 100000;
    int THREADS = 4;

    if (argc == 2) {
        SIZE = atoi(argv[1]);
    } else if (argc == 3) {
        SIZE = atoi(argv[1]);
        THREADS = atoi(argv[2]);
    }

    int* array = new int[SIZE];
    int* linear_sorted_array = new int[SIZE];
    int* stl_sorted_array = new int[SIZE];

    srand(static_cast<unsigned int>(time(NULL)));
    std::generate(array, array + SIZE, []() { return std::rand() % 100 + 1; });
    std::copy(array, array + SIZE, linear_sorted_array);
    std::copy(array, array + SIZE, stl_sorted_array);
    if (SIZE < 50) {
        std::cout << "Initial array: ";
        PrintArray(array, SIZE);
    }

    tbb::tick_count start_time = tbb::tick_count::now();
    TBBParallelSort(array, SIZE, THREADS);
    tbb::tick_count finish_time = tbb::tick_count::now();
    double parallel_time = (finish_time - start_time).seconds();

    start_time = tbb::tick_count::now();
    shellSort(linear_sorted_array, SIZE);
    finish_time = tbb::tick_count::now();
    double linear_time = (finish_time - start_time).seconds();

    // Sort array using stl sort to check custom algorithms
    std::sort(stl_sorted_array, stl_sorted_array + SIZE);

    if (IsCorrectlySorted(stl_sorted_array, array, SIZE) &&
        IsCorrectlySorted(stl_sorted_array, linear_sorted_array, SIZE)) {
        if (SIZE < 50) {
            std::cout << "Sorted array:  ";
            PrintArray(array, SIZE);
        }
    } else {
        std::cout << "Failed." << std::endl;
    }
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "Parallel time: " << parallel_time << std::endl;
    std::cout << "Linear time:   " << linear_time << std::endl;
    std::cout << "Boost: " << linear_time / parallel_time << std::endl;

    delete[] array;
    delete[] linear_sorted_array;
    delete[] stl_sorted_array;

    return 0;
}
