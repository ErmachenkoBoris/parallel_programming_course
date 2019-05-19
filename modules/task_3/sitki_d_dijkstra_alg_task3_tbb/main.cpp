// Copyright 2019 Sitkin Dmitry
#include <stdlib.h>
#include <stdio.h>
#include <omp.h>
#include <tbb/tbb.h>
#include <utility>
#include <random>
#include <ctime>
#include <queue>
#include <iostream>
#include <string>
#include <map>
#include <vector>

#define INFINITI 10000000
#define WEIGHT 5
#define NUM_OF_VERTEX 2000
// using namespace std;

void print_d(int* d, int size) {
    for (int i = 0; i < size; i++) {
        std::cout << d[i] << " ";
    }
    std::cout << std::endl;
}

void print_graph(int** G, int size) {
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            std::cout << G[i][j] << " ";
        }
        std::cout << std::endl;
    }
}

int* init_round(int countEdge, int countVertex) {
    int* one_graph = new int[countVertex * countVertex];
    int** graph = new int*[countVertex];

    for (int i = 0; i < countVertex; i++) {
        graph[i] = new int[countVertex];
    }
    srand((unsigned int)std::time(NULL));
    for (int i = 0; i < countVertex; i++) {
        for (int j = 0; j < countVertex; j++) {
            if (i == j) {
                graph[i][j] = 0;
            } else {
                if (j == i + 1) {
                    graph[i][j] = 1 + std::rand() % WEIGHT;
                } else {
                    graph[i][j] = INFINITI;
                }
            }
        }
    }
    // print_graph(graph, countVertex);

    graph[countVertex - 1][0] = 1 + std::rand() % WEIGHT;
    for (int i = 0, t = 0; i < countVertex; i++) {
        for (int j = 0; j < countVertex; j++) {
            one_graph[t] = graph[j][i];
            t++;
        }
    }
    return one_graph;
}

int* init_graph(int countEdge, int countVertex) {
    int* one_graph = new int[countVertex * countVertex];
    srand((unsigned int)std::time(NULL));

    for (int i = 0; i < countVertex; i++) {
        for (int j = 0; j < countVertex; j++) {
            if (i == j) {
                one_graph[j*countVertex + i] = 0;
            } else {
                one_graph[j*countVertex + i] = std::rand() % WEIGHT;
                if (one_graph[j*countVertex + i] == 0) {
                    one_graph[j*countVertex + i] = INFINITI;
                }
            }
        }
    }
    return one_graph;
}

int* init_d(int size) {
    int* d = new int[size];
    for (int i = 0; i < size; i++) {
        d[i] = INFINITI;
    }
    return d;
}

int* d_for_round(int* graph, int start, int count_vertex) {
    int* d = init_d(count_vertex);
    d[start] = 0;
    for (int ofset = 0, i = 1 + start; (i < count_vertex); ofset++, i++) {
        d[i] = d[i - 1] + graph[i*count_vertex + start + ofset];
    }
    for (int i = 0; i < start; i++) {
        if (i == 0) {
            d[i] = d[count_vertex - 1] + graph[count_vertex - 1];
        } else {
            d[i] = d[i - 1] + graph[i*count_vertex + (i - 1)];
        }
    }
    return d;
}

void compare(int* d, int* ideal, int size) {
    int mistakes = 0;
    for (int i = 0; i < size; i++)
    if ((d[i] != ideal[i])) {
        mistakes++;
    }
    // std::cout << "MISTAKES: " << mistakes << std::endl;

    if (mistakes == 0) {
        std::cout << "It's OK" << std::endl;
    } else {
        std::cout << "It's NOT OK" << std::endl;
    }
}

int* dijkstra(int* graph, int start, int count_vertex) {
    int* d = init_d(count_vertex);
    d[start] = 0;
    std::priority_queue<std::pair<int, int>>  queue;
    queue.push(std::make_pair(0, start));
    while (!queue.empty()) {
        int v = queue.top().second, cur_d = queue.top().first;
        queue.pop();

        if (cur_d > d[v])  continue;

        for (int i = 0; i < count_vertex; ++i) {
            int to = i, len = graph[i * count_vertex + v];
            if (d[v] + len < d[to]) {
                d[to] = d[v] + len;
                queue.push(std::make_pair(d[to], to));
            }
        }
    }
    return d;
}

int* parallel_dijkstra(int* graph, int start, int count_vertex) {
    int* d = init_d(count_vertex);
    d[start] = 0;
    std::priority_queue<std::pair<int, int>>  queue;
    queue.push(std::make_pair(0, start));
    while (!queue.empty()) {
        int v = queue.top().second, cur_d = queue.top().first;
        queue.pop();

        if (cur_d > d[v])  continue;

#pragma omp parallel for
            for (int i = 0; i < count_vertex; ++i) {
                int to = i, len = graph[i * count_vertex + v];
                if (d[v] + len < d[to]) {
                    d[to] = d[v] + len;
                    #pragma omp critical
                    queue.push(std::make_pair(d[to], to));
                }
            }
    }
    return d;
}

int* tbb_parallel_dijkstra(int* graph, int start, int count_vertex) {
    int* d = init_d(count_vertex);
    d[start] = 0;
    std::priority_queue<std::pair<int, int>>  queue;
    queue.push(std::make_pair(0, start));
    tbb::mutex mutex;
    while (!queue.empty()) {
        int v = queue.top().second, cur_d = queue.top().first;
        queue.pop();

        if (cur_d > d[v])  continue;

        tbb::parallel_for(tbb::blocked_range<int>(0,
            count_vertex, count_vertex / 4),
            [&](const tbb::blocked_range<int> &r) {
            for (int i = r.begin(); i != r.end(); ++i) {
                int to = i, len = graph[i * count_vertex + v];
                if (d[v] + len < d[to]) {
                    d[to] = d[v] + len;
                    mutex.lock();
                    queue.push(std::make_pair(d[to], to));
                    mutex.unlock();
                }
            }
        });
    }
    return d;
}

std::pair<double, double>(math_exp_and_disp(std::map<double, double> m)) {
    std::map<double, double>::iterator it;
    std::pair<double, double> p;
    double result = 0.0;
    double result2 = 0.0;
    for (it = m.begin(); it != m.end(); it++) {
        result += it->first * it->second;
        result2 += it->first * it->first * it->second;
    }
    p.first = result;
    p.second = result2 - result*result;
    return p;
}

void experiment(int begin, int step, int count_step) {
    // int vert = NUM_OF_VERTEX;
    int count_edge;
    int start;

    int *graph = NULL;
    int *result = NULL;
    int *parallel_result = NULL;

    tbb::tick_count  t1, t2;
    double time, ptime, perfomance;
    int size = begin;

    std::vector<double> test;
    std::map<double, double> test_map;
    std::pair<double, double> disp;

    std::map<double, double>::iterator it;

    for (int i = 0; i < count_step; i++) {
        count_edge = (size - 1) + std::rand() % ((size * (size - 1)) / 2);
        start = std::rand() % (size - 1);

        graph = init_graph(count_edge, size);

        t1 = tbb::tick_count::now();
        result = dijkstra(graph, start, size);
        t2 = tbb::tick_count::now();
        time = (t2 - t1).seconds();

        t1 = tbb::tick_count::now();
        parallel_result = parallel_dijkstra(graph, start, size);
        t2 = tbb::tick_count::now();
        ptime = (t2 - t1).seconds();

        perfomance = round((time / ptime) * 100) / 100;
        std::cout << perfomance << std::endl;
        test_map[perfomance] += 1.0;
        test.push_back(perfomance);
        size += step;
        delete[] graph;
        delete[] result;
        delete[] parallel_result;
    }

    std::cout << std::endl;

    for (it = test_map.begin(); it != test_map.end(); it++) {
        it->second /= count_step;
        std::cout << it->first << "  " << it->second << std::endl;
    }

    std::cout << std::endl;

    disp = math_exp_and_disp(test_map);
    std::cout << disp.first << "  " << disp.second << std::endl;
    return;
}

int main(int argc, char *argv[]) {
    srand((unsigned int)std::time(NULL));

    int num_threads = 4;
    tbb::task_scheduler_init init(num_threads);

    int vert = NUM_OF_VERTEX;
    int count_edge = (vert - 1) + std::rand() % ((vert * (vert - 1)) / 2);
    int start = std::rand() % (NUM_OF_VERTEX - 1);

    int *graph = init_graph(count_edge, NUM_OF_VERTEX);
    int *result = NULL;  // = dijkstra(graph, start, NUM_OF_VERTEX);
    int *parallel_result = NULL;

    tbb::tick_count  t1, t2;
    double time, ptime;

    t1 = tbb::tick_count::now();
    result = dijkstra(graph, start, NUM_OF_VERTEX);
    t2 = tbb::tick_count::now();

    // std::cout <<"Dijkstra" <<std::endl;
    // for (int i = NUM_OF_VERTEX-1000; i < NUM_OF_VERTEX-950; i++)
    // {
    //    std::cout << parallel_result[i] << "  ";
    // }
    // std::cout << std::endl;
    // print_d(parallel_result, NUM_OF_VERTEX);
    // std::cout<<"Ideal" << std::endl;
    /*for (int i = NUM_OF_VERTEX - 1000; i < NUM_OF_VERTEX-950; i++)
    {
        std::cout << ideal_d[i] << "  ";
    }
    std::cout << std::endl;*/
    // print_d(ideal_d, NUM_OF_VERTEX);
    time = (t2 - t1).seconds();
    std::cout << "time = " << time << std::endl;
    t1 = tbb::tick_count::now();
    parallel_result = tbb_parallel_dijkstra(graph, start, NUM_OF_VERTEX);
    t2 = tbb::tick_count::now();
    ptime = (t2 - t1).seconds();
    std::cout << "ptime = " << ptime << std::endl;
    std::cout << "Perfomance: " << time / ptime << std::endl;
    compare(parallel_result, result, NUM_OF_VERTEX);
    // experiment(950, 1, 50);
    delete[] graph;
    delete[] result;
    delete[] parallel_result;
    return 0;
}
