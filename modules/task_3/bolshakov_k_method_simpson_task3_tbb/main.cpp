// Copyright 2019 Bolshakov Konstantin
#include <tbb/tbb.h>
#include <stdio.h>
#include <math.h>
#include <iostream>
#include <functional>

double func(double x, double y) {
    return x+y;
}
double func_simpson(double f(double, double),
    double xp, double yp, double x, double y,
    double xn, double yn) {
    return (f(xp, yp) + 4 * f(x, y) + f(xn, yn));
}
double integrate_linear(double f(double, double),
    double lower_bound_x, double lower_bound_y, double upper_bound_x, double upper_bound_y, int y_n, int x_n) {
    double h_x = (upper_bound_x - lower_bound_x) / x_n;
    double h_y = (upper_bound_y - lower_bound_x) / y_n;
    double* x = new double[x_n + 1];
    double* y = new double[y_n + 1];
    double sum = 0;
    for (int i = 0; i <= x_n; i++) {
        x[i] = lower_bound_x + i * h_x;
    }
    for (int i = 0; i <= y_n; i++) {
        y[i] = lower_bound_y + i * h_y;
    }
    for (int i = 1; i < x_n; i += 2) {
        for (int j = 1; j < y_n; j += 2) {
            sum += func_simpson(f,
                x[i - 1], y[j - 1], x[i - 1], y[j], x[i - 1], y[j + 1]) +
                4 * func_simpson(f,
                    x[i], y[j - 1], x[i], y[j], x[i], y[j + 1]) +
                func_simpson(f,
                    x[i + 1], y[j - 1], x[i + 1], y[j], x[i + 1], y[j + 1]);
        }
    }
    delete[] x;
    delete[] y;
    return sum * h_x * h_y / 9;
}
double integrate_parallel_tbb(double f(double, double),
    double lower_bound_x, double lower_bound_y, double upper_bound_x, double upper_bound_y, int x_n, int y_n) {
    double h_x = (upper_bound_x - lower_bound_x) / x_n;
    double h_y = (upper_bound_y - lower_bound_x) / y_n;
    double* x = new double[x_n + 1];
    double* y = new double[y_n + 1];
    for (int i = 0; i <= x_n; i++) {
        x[i] = lower_bound_x + i * h_x;
    }
    for (int i = 0; i <= y_n; i++) {
        y[i] = lower_bound_y + i * h_y;
    }
    size_t g_size = x_n / 8;
    double sum = tbb::parallel_reduce(
        tbb::blocked_range<size_t>(0, x_n, g_size), 0.0,
        [&](const tbb::blocked_range<size_t> &range, double partSum) -> double {
        size_t begin = range.begin(), end = range.end();
        for (size_t i = begin + 1; i < end; i += 2) {
            for (int j = 1; j < y_n; j += 2) {
                partSum += func_simpson(f, x[i - 1], y[j - 1], x[i - 1], y[j], x[i - 1], y[j + 1])
                    + func_simpson(f, x[i], y[j - 1], x[i], y[j], x[i], y[j + 1]) * 4
                    + func_simpson(f, x[i + 1], y[j - 1], x[i + 1], y[j], x[i + 1], y[j + 1]);
            }
        }
        return partSum;
    }, std::plus<double>());
    delete[]x;
    delete[]y;
    return sum * h_x * h_y / 9;
}

int main(int argc, char* argv[]) {
    tbb::tick_count linear_t1 = tbb::tick_count::now();
    double linear_result = integrate_linear(func, 0, 0, 2, 2, 2000, 2000);
    tbb::tick_count linear_t2 = tbb::tick_count::now();
    double linear_time = (linear_t2 - linear_t1).seconds();
    std::cout << "linear result is " << std::fixed << linear_result << " and time is " << linear_time<< std::endl;

    tbb::tick_count tbb1 = tbb::tick_count::now();
    double tbb_result = integrate_parallel_tbb(func, 0, 0, 2, 2, 2000, 2000);
    tbb::tick_count tbb2 = tbb::tick_count::now();
    double tbb_time = (tbb2 - tbb1).seconds();
    std::cout << "tbb result is " << std::fixed << tbb_result << " and time is " << tbb_time << std::endl;
    std::cout << "boost is " << linear_time/tbb_time <<std::endl;

    return 0;
}
