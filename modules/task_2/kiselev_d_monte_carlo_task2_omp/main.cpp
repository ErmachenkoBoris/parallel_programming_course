// Copyright Kiselev Denis 2019

#include <omp.h>

#include <functional>
#include <iostream>
#include <random>
#include <utility>
#include <vector>

#define DEFAULT_NPOINTS 100000

// Ellipsoid options
#define ELLPS_A 1.0
#define ELLPS_B 2.0
#define ELLPS_C 3.0

// Limits of integration
#define X1 -1.0
#define X2 1.0
#define Y1 -2.0
#define Y2 2.0
#define Z1 -3.0
#define Z2 3.0

double ellipsoid(const std::vector<double>& args) {
    return -1.0 + pow(args[0] / ELLPS_A, 2)
                + pow(args[1] / ELLPS_B, 2)
                + pow(args[2] / ELLPS_C, 2);
}

std::vector<std::vector<double> > generateRandomPointsByArea(
        int nPoints, const std::vector<std::pair<double, double> >& limits) {
    int nDimensions = limits.size();

    std::vector<std::uniform_real_distribution<> > distrs;
    distrs.reserve(nDimensions);
    for (int i = 0; i < nDimensions; i++) {
        distrs.emplace_back(limits[i].first, limits[i].second);
    }

    std::random_device r;
    std::default_random_engine generator(r());
    std::vector<std::vector<double> > points(nPoints, std::vector<double>(nDimensions));
    for (int i = 0; i < nPoints; i++) {
        for (int j = 0; j < nDimensions; j++) {
            points[i][j] = distrs[j](generator);
        }
    }

    return points;
}

double integrateByMonteCarlo(
        std::function<double(const std::vector<double>&)> func,
        const std::vector<std::pair<double, double> >& limits,
        const std::vector<std::vector<double> >& points) {
    int nDimensions = limits.size();
    int nPoints = points.size();

    int nPointsInEllipsoid = 0;
    for (int i = 0; i < nPoints; i++) {
        double value = func(points[i]);
        if (value <= 0) nPointsInEllipsoid++;
    }

    double measure = 1.0;
    for (int i = 0; i < nDimensions; i++) {
        measure *= limits[i].second - limits[i].first;
    }

    double hitProbability = static_cast<double>(nPointsInEllipsoid) / nPoints;
    return measure * hitProbability;
}

double integrateByMonteCarloParallel(
        std::function<double(const std::vector<double>&)> func,
        const std::vector<std::pair<double, double> >& limits,
        const std::vector<std::vector<double> >& points) {
    int nDimensions = limits.size();
    int nPoints = points.size();

    int nPointsInEllipsoid = 0;
    #pragma omp parallel reduction(+: nPointsInEllipsoid)
    {
        #pragma omp for schedule(guided)
        for (int i = 0; i < nPoints; i++) {
            double value = func(points[i]);
            if (value <= 0) nPointsInEllipsoid++;
        }
    }

    double measure = 1.0;
    for (int i = 0; i < nDimensions; i++) {
        measure *= limits[i].second - limits[i].first;
    }

    double hitProbability = static_cast<double>(nPointsInEllipsoid) / nPoints;
    return measure * hitProbability;
}

int main(int argc, char *argv[]) {
    int nPoints = (argc > 1) ? atoi(argv[1]) : DEFAULT_NPOINTS;

    std::vector<std::pair<double, double> > limits = { { X1, X2 }, { Y1, Y2 }, { Z1, Z2 } };
    auto points = generateRandomPointsByArea(nPoints, limits);

    // Sequential
    double t1 = omp_get_wtime();
    double seqResult = integrateByMonteCarlo(ellipsoid, limits, points);
    double seqTime = omp_get_wtime() - t1;

    // Parallel
    t1 = omp_get_wtime();
    double parResult = integrateByMonteCarloParallel(ellipsoid, limits, points);
    double parTime = omp_get_wtime() - t1;

    double realRes = 4.0 / 3.0 * std::acos(-1) * ELLPS_A * ELLPS_B * ELLPS_C;
    double speedUp = seqTime / parTime;
    std::cout << "Sequential alg:\n"
                 "\tResult: " << seqResult << "\n"
                 "\tTime: " << seqTime << " sec\n"
                 "Parallel alg:\n"
                 "\tResult: " << parResult << "\n"
                 "\tTime: " << parTime << " sec\n"
                 "Real result: " << realRes << "\n"
                 "Speed up: " << speedUp << "\n";
    return 0;
}
