// Copyright 2019 Evdokimova Julia
#define _SCL_SECURE_NO_WARNINGS
#include <tbb/tbb.h>
#include <tbb/task_scheduler_init.h>
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_reduce.h>
#include <iostream>
#include <random>
#include <iomanip>
#include <ctime>
#include <string>
#include <algorithm>
std::default_random_engine generator((unsigned)time(0));
std::uniform_int_distribution <int> dist(0, 10);

void CreateMatrix(double* matrix, int size) {
    for (int i = 0; i < size; i++) {
        for (int j = i; j < size; j++) {
            if (i == j)
                matrix[i * size + j] = dist(generator);
            else
                matrix[j * size + i] = matrix[i * size + j] = dist(generator);
        }
    }
}

void CreateVector(double* vector, int size) {
    for (int i = 0; i < size; i++) {
        vector[i] = dist(generator);
    }
}

void PrintVector(double* vector, int size) {
    for (int i = 0; i < size; i++) {
        std::cout << std::setprecision(3) << vector[i] << "\t";
    }
    std::cout << std::endl;
}
void Print_SLU(double* matrix, double* vector, int size) {
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            std::cout << std::setprecision(3) << matrix[i * size + j] << "\t";
        }
        std::cout << " | " << std::setprecision(3) << vector[i] << "\n";
    }
}
// sequential
void MatrixVector(double* matrix, double* vector, double* result, int size) {
    for (int i = 0; i < size; i++) {
        result[i] = 0;
        for (int j = 0; j < size; j++)
            result[i] += matrix[i * size + j] * vector[j];
    }
}
double ScalVector(double* vec1, double* vec2, int size) {
    double result = 0;
    for (int i = 0; i < size; i++) {
        result += vec1[i] * vec2[i];
    }
    return result;
}
void SoprGradMethod(double* matrix, double* vector, double* x0,
    double eps, double* result, int* count, int maxIter, int size) {
    double* rPrev = new double[size];  // rPrev - neviazki tekuchego priblizhenie
    double* rNext = new double[size];  // rNext - neviazki sleduchego priblizhenie
    double* p = new double[size];  // p vector napravlenia
    double* y = new double[size];  // y vectornoe proizvedenie matrix*x0
    double* Ap = new double[size];  // Ap vectornoe proizvedenie matrix*p
    // check tekuchaia tochnost metoda, norm norma vectora vector
    // beta, alpha koefficient raschetnih formul
    double beta, alpha, check;
    double* swap;
    // initiallization metod
    *count = 0;
    // vicheslenie neviazki s nachalnim priblizhenie
    MatrixVector(matrix, x0, y, size);
    for (int i = 0; i < size; i++)
        rPrev[i] = vector[i] - y[i];
    std::copy(rPrev, rPrev + size, p);
    std::copy(x0, x0 + size, result);

    // iteracii metoda
    do {
        (*count)++;
        MatrixVector(matrix, p, Ap, size);
        alpha = ScalVector(rPrev, rPrev, size) / ScalVector(p, Ap, size);
        for (int i = 0; i < size; i++) {
            result[i] += alpha * p[i];
            rNext[i] = rPrev[i] - alpha * Ap[i];
        }
        beta = ScalVector(rNext, rNext, size) / ScalVector(rPrev, rPrev, size);
        // Norma neviazki
        check = sqrt(ScalVector(rNext, rNext, size));
        for (int j = 0; j < size; j++)
            p[j] = beta * p[j] + rNext[j];
        swap = rNext;
        rNext = rPrev;
        rPrev = swap;
    } while ((check > eps) && (*(count) <= maxIter));

    delete[] rPrev;
    delete[] rNext;
    delete[] p;
    delete[] y;
    delete[] Ap;
}
double VectorsMultiplication(const double *v1, const double *v2, int size) {
    double result = 0.0;
    for (int i = 0; i < size; i++)
        result += v1[i] * v2[i];
    return result;
}
class VectorsMultiplicator {
    double *matrix, *vector;
    double *resultVector;
    int size;
 public:
    VectorsMultiplicator(double *tmatrix, double *tvector, double *tresultVector, int tsize) :
        matrix(tmatrix), vector(tvector), resultVector(tresultVector), size(tsize) {
    }
    void operator()(const tbb::blocked_range<int>& r) const {
        int begin = r.begin(), end = r.end();
        for (int i = begin; i < end; i++) {
            resultVector[i] = VectorsMultiplication(&(matrix[i*size]), vector, size);
        }
    }
};
class ScalarMultiplicator {
 private:
    const double *a, *b;
    double c;
 public:
    explicit ScalarMultiplicator(double *ta, double *tb) : a(ta), b(tb), c(0) {}
    ScalarMultiplicator(const ScalarMultiplicator& m, tbb::split) : a(m.a), b(m.b), c(0) {}
    void operator()(const tbb::blocked_range<int>& r) {
        int begin = r.begin(), end = r.end();
        c += VectorsMultiplication(&(a[begin]), &(b[begin]), end - begin);
    }
    void join(const ScalarMultiplicator& multiplicator) {
        c += multiplicator.c;
    }
    double Result() {
        return c;
    }
};
void TBBMatrixVectorMultiplication(double* matrix, double* vector, double* resultVector, int size, int grainSize) {
    tbb::parallel_for(tbb::blocked_range<int>(0, size, grainSize),
        VectorsMultiplicator(matrix, vector, resultVector, size));
}
double TBBScalarMultiplication(double* vector1, double* vector2, int size, int grainSize) {
    ScalarMultiplicator s(vector1, vector2);
    tbb::parallel_reduce(tbb::blocked_range<int>(0, size, grainSize), s);
    return s.Result();
}
void SoprGradMethod_tbb(double* matrix, double* vector, double* x0,
    double eps, double* result, int* count, int maxIter, int size, int grainSize) {
    double* rPrev = new double[size];  // rPrev - neviazki tekuchego priblizhenie
    double* rNext = new double[size];  // rNext - neviazki sleduchego priblizhenie
    double* p = new double[size];  // p vector napravlenia
    double* y = new double[size];  // y vectornoe proizvedenie matrix*x0
    double* Ap = new double[size];  // Ap vectornoe proizvedenie matrix*p
    // check tekuchaia tochnost metoda, norm norma vectora vector
    // beta, alpha koefficient raschetnih formul
    double beta, alpha, check;
    double* swap;
    // initiallization metod
    *count = 0;
    for (int i = 0; i < size; i++) {
        rPrev[i] = vector[i];
        p[i] = rPrev[i];
        result[i] = 0.0;
    }
    // iteracii metoda
    do {
        (*count)++;
        TBBMatrixVectorMultiplication(matrix, p, Ap, size, grainSize);
        alpha = TBBScalarMultiplication(rPrev, rPrev, size, grainSize) /
TBBScalarMultiplication(p, Ap, size, grainSize);
        for (int i = 0; i < size; i++) {
            result[i] += alpha * p[i];
            rNext[i] = rPrev[i] - alpha * Ap[i];
        }
        beta = TBBScalarMultiplication(rNext, rNext, size, grainSize) /
TBBScalarMultiplication(rPrev, rPrev, size, grainSize);
        check = sqrt(TBBScalarMultiplication(rNext, rNext, size, grainSize));
        for (int j = 0; j < size; j++)
            p[j] = beta * p[j] + rNext[j];
        swap = rNext;
        rNext = rPrev;
        rPrev = swap;
    } while ((check > eps) && (*(count) <= maxIter));

    delete[] rPrev;
    delete[] rNext;
    delete[] p;
    delete[] y;
    delete[] Ap;
}
int main(int argc, char **argv) {
    double eps = 0;  // tochnost
    int grainSize = 5;
    int pNum = 4;
    int size = 0;  // razmer matrica
    int maxIter_seq = 0;  // max kol iteracii
    int maxIter_tbb = 0;
    int count_seq;  // kol iteracii
    int count_tbb;

    if (argc > 1) {
        pNum = atoi(argv[1]);
        size = atoi(argv[2]);
        eps = atoi(argv[3]);
    }
    tbb::task_scheduler_init init(pNum);

    double* matrix = new double[size * size];
    double* vector = new double[size];  // vector pravoi chasti
    double* result_tbb = new double[size];  // vector result
    double* result_seq = new double[size];
    double* x0_seq = new double[size];  // nachal'noe priblizhenie
    double* x0_tbb = new double[size];

    maxIter_seq = size * 10;
    maxIter_tbb = size * 10;

    for (int i = 0; i < size; i++) {
        x0_seq[i] = 0.0;
        x0_tbb[i] = 0.0;
    }

    CreateMatrix(matrix, size);
    CreateVector(vector, size);
    std::cout << "SYSTEM: " << std::endl;
    if (size < 5)
        Print_SLU(matrix, vector, size);
    else
        std::cout << "System size large" << std::endl;


    // Seq
    tbb::tick_count start_seq = tbb::tick_count::now();
    SoprGradMethod(matrix, vector, x0_seq, eps, result_seq, &count_seq, maxIter_seq, size);
    tbb::tick_count finish_seq = tbb::tick_count::now();
    double time_seq = (finish_seq - start_seq).seconds();

    // par
    tbb::tick_count start_tbb = tbb::tick_count::now();
    SoprGradMethod_tbb(matrix, vector, x0_tbb, eps, result_tbb, &count_tbb, maxIter_tbb, size, grainSize);
    tbb::tick_count finish_tbb = tbb::tick_count::now();
    double time_tbb = (finish_tbb - start_tbb).seconds();

    std::cout << "SEQUENTIAL ALGORITHM: " << std::endl;
    std::cout << "Solution: " << std::endl;
    PrintVector(result_seq, size);
    std::cout << "Count: " << count_seq << std::endl;
    std::cout << "Time: " << time_seq << " c" << std::endl;
    std::cout << "PARALLEL ALGORITHM: " << std::endl;
    std::cout << "Solution: " << std::endl;
    PrintVector(result_tbb, size);
    std::cout << "Count: " << count_tbb << std::endl;
    std::cout << "Time: " << time_tbb << " c" << std::endl;
    std::cout << "Acceleration: " << time_seq / time_tbb << std::endl;

    delete[] matrix;
    delete[] vector;
    delete[] result_tbb;
    delete[] result_seq;
    delete[] x0_tbb;
    delete[] x0_seq;
}
