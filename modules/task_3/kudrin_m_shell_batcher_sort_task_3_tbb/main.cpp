// Copyright 2019 Kudrin Matvey

#include <tbb/tbb.h>
#include <math.h>
#include <stdio.h>
#include <time.h>
#include <utility>
#include <string>
#include <iostream>
#include <vector>
#include <algorithm>
#include "tbb/concurrent_vector.h"

using std::vector;
using tbb::blocked_range;
using tbb::concurrent_vector;



class OddEvenSorter {
    concurrent_vector<int> *arr;
    const int p, r, d;

 public:
    OddEvenSorter(concurrent_vector<int> *arr, const int p, const int r,
            const int  d) : arr(arr), p(p), r(r), d(d) {}
    void operator()(const blocked_range<int>& range) const {
        int begin = range.begin(),  end = range.end();
        for (int i = begin; i != end; i++) {
            if ((p & i) == r) {
                if (arr->at(i) > arr->at(i + d)) {
                    int tmp = arr->at(i);
                    arr->at(i) = arr->at(i + d);
                    arr->at(i + d) = tmp;
                }
            }
        }
    }
};

class BatcherSorter {
    concurrent_vector<int> *a;
    int step;

 public:
    BatcherSorter(concurrent_vector<int> *a, const int step) : a(a),
    step(step) {}
    void operator()(const blocked_range<int>& range) const {
        int begin = range.begin(),  end = range.end();
        vector<int> *tmp = new vector<int>(a->size() / step);
        for (int start = begin; start != end; start++) {
            for (unsigned int i = start, j = 0; i < a->size(); i += step, j++) {
                tmp->push_back(a->at(i));
            }
            const int length = tmp->size();
            int t = static_cast<int>(ceil(log2(length)));
            int p = static_cast<int>(pow(2, t - 1));
            while (p > 0) {
                int q = static_cast<int>(pow(2, t - 1));
                int r = 0;
                int d = p;
                int i;
                while (d > 0) {
                    for (i = 0; i < length - d; ++i) {
                        if ((i & p) == r) {
                            if (tmp->at(i) > tmp->at(i + d)) {
                                std::iter_swap(tmp->begin() + i,
                                        tmp->begin() + i + d);
                            }
                        }
                    }
                    d = q - p;
                    q /= 2;
                    r = p;
                }
                p /= 2;
            }

            unsigned int i = start, j = 0;
            for (; i < a->size() - start; i += step, j++) {
                a->at(i) = tmp->at(j);
            }
            tmp->clear();
        }
        delete tmp;
    }
};

void oddEvenMergeSortLinear(std::vector<int> *arr) {
    const int length = arr->size();
    int t = static_cast<int>(ceil(log2(length)));
    int p = static_cast<int>(pow(2, t - 1));
    while (p > 0) {
        int q = static_cast<int>(pow(2, t - 1));
        int r = 0;
        int d = p;
        int i;
        while (d > 0) {
            for (i = 0; i < length - d; ++i) {
                if ((i & p) == r) {
                    if (arr->at(i) > arr->at(i + d)) {
                        std::iter_swap(arr->begin() + i, arr->begin() + i + d);
                    }
                }
            }
            d = q - p;
            q /= 2;
            r = p;
        }
        p /= 2;
    }
}

void oddEvenMergeSortTbb(concurrent_vector<int> *arr) {
    const int length = arr->size();
    int t = static_cast<int>(ceil(log2(length)));
    int p = static_cast<int>(pow(2, t - 1));
    while (p > 0) {
        int q = static_cast<int>(pow(2, t - 1));
        int r = 0;
        int d = p;
        while (d > 0) {
            parallel_for(blocked_range<int>(1, length - d - 1, 40000),
                    OddEvenSorter(arr, p, r, d));
            d = q - p;
            q /= 2;
            r = p;
        }
        p /= 2;
    }
}

char* getCmdOption(char **begin, char **end, const std::string& option) {
    char **itr = std::find(begin, end, option);
    if (itr != end && ++itr != end)
        return *itr;
    return 0;
}

bool cmdOptionExists(char** begin, char** end, const std::string& option) {
    return std::find(begin, end, option) != end;
}

vector<int>* generateRandomArray(const int en, int min, int max) {
    if (min >= max) {
        max = min + max;
    }
    vector<int> *arr = new vector<int>(en);
    srand(static_cast<unsigned int>(time(NULL)));
    for (int j = 0; j < en; j++)
        arr->at(j) = min + (std::rand() % (max - min));
    return arr;
}

bool check(concurrent_vector<int> *arr, int elementsNumber) {
    bool flag = true;
    int min = arr->at(0);
    for (int i = 1; i < elementsNumber; i++) {
        if (arr->at(i) < min) {
            flag = false;
        }
    }
    return flag;
}

int calculateStep(int iter) {
    int step = 0;
    if (iter % 2) {
        step = static_cast<int>(8 * pow(2, iter)
                                - 6 * pow(2, (iter + 1) / 2) + 1);
    } else {
        step = static_cast<int>(9 * pow(2, iter) - 9 * pow(2, iter / 2) + 1);
    }
    return step;
}

void batcherLinear(vector<int> *a, const int step) {
    vector<int> *tmp = new vector<int>;
    int start;
    for (start = 0; start < step; start++) {
        for (unsigned int i = start, j = 0; i < a->size(); i += step, j++) {
            tmp->push_back(a->at(i));
        }
        oddEvenMergeSortLinear(tmp);
        unsigned int i = start, j = 0;
        for (; i < a->size() - start; i += step, j++) {
            a->at(i) = tmp->at(j);
        }
        tmp->clear();
    }  // end of parallel section
    delete tmp;
}

void batcherTbb(concurrent_vector<int> *a, const int step) {
    concurrent_vector<int> *tmp = new concurrent_vector<int>;
    int start;
    for (start = 0; start < step; start++) {
        for (unsigned int i = start, j = 0; i < a->size(); i += step, j++) {
            tmp->push_back(a->at(i));
        }
        oddEvenMergeSortTbb(tmp);
        unsigned int i = start, j = 0;
        for (; i < a->size() - start; i += step, j++) {
            a->at(i) = tmp->at(j);
        }
        tmp->clear();
    }  // end of parallel section
    delete tmp;
}


void shellSortLinear(vector<int> *a, int size) {
    int step = 0;
    int iter = 0;
    while (calculateStep(iter++) < size / 3) {
        step = calculateStep(iter);
    }
    while (--iter >= 0) {
        step = calculateStep(iter);
        batcherLinear(a, step);
    }
}

void shellSortTbb(concurrent_vector<int> *a, int size) {
    int step = 0;
    int iter = 0;
    while (calculateStep(iter++) < size / 3) {
        step = calculateStep(iter);
    }
    while (--iter >= 0) {
        step = calculateStep(iter);
        if (step < 10) {
            batcherTbb(a, step);
        } else {
            parallel_for(blocked_range<int>(0, step, 100),
                    BatcherSorter(a, step));
        }
    }
}

int main(int argc, char *argv[]) {
    int elementsNumber = 1000000;
    int a = 0;
    int b = 10000000;
    vector<int> *arr_linear;
    concurrent_vector<int> *arr_tbb;

    if (cmdOptionExists(argv, argv + argc, "-n")) {
        char *wcount = getCmdOption(argv, argv + argc, "-n");
        elementsNumber = atoi(wcount);
    }

    if (cmdOptionExists(argv, argv + argc, "-a")) {
        char *wcount = getCmdOption(argv, argv + argc, "-a");
        a = atoi(wcount);
    }

    if (cmdOptionExists(argv, argv + argc, "-b")) {
        char *wcount = getCmdOption(argv, argv + argc, "-b");
        b = atoi(wcount);
    }

    arr_linear = generateRandomArray(elementsNumber, a, b);
    arr_tbb = new concurrent_vector<int>(elementsNumber);

    for (int i = 0; i < elementsNumber; ++i) {
        arr_tbb->at(i) =  arr_linear->at(i);
    }

    shellSortLinear(arr_linear, elementsNumber);

    tbb::task_scheduler_init init;
    shellSortTbb(arr_tbb, elementsNumber);

    if (check(arr_tbb, elementsNumber)) {
        printf("\nOK: array is lineary sorted");
    } else {
        printf("\n ERROR: array is not sorted");
    }
    arr_linear->clear();
    arr_tbb->clear();
    return 0;
}
