#include <iostream>
#include <fstream>
#include <Eigen/Dense>
#include <Eigen/SparseCore>
#include <typeinfo>

#include <vector>
#include <cstdio>
#include <stdint.h>
#include <omp.h>

#include <algorithm>
#include <functional>
#include <numeric>
#include <queue>
#include <string>
#include <memory>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "Mrpt.h"
#include "common.h"




using namespace Eigen;

int main(int argc, char **argv) {
    size_t n = atoi(argv[1]);
    size_t n_test = atoi(argv[2]);
    int k = atoi(argv[3]);
    int trees_max = atoi(argv[4]);
    int depth_min = atoi(argv[5]);
    int depth_max = atoi(argv[6]);
    int votes_max = atoi(argv[7]);

    size_t dim = atoi(argv[8]);
    int mmap = atoi(argv[9]);
    std::string result_file(argv[10]);

    std::string infile_path(argv[11]);
    if (!infile_path.empty() && infile_path.back() != '/')
      infile_path += '/';

    float density = atof(argv[12]);
    bool parallel = atoi(argv[13]);

    size_t n_points = n - n_test;
    bool verbose = false;

    std::string result_path = "results/mnist/";

    /////////////////////////////////////////////////////////////////////////////////////////
    // test mrpt
    float *train, *test;

    test = read_memory((infile_path + "test.bin").c_str(), n_test, dim);
    if(!test) {
        std::cerr << "in mrpt_comparison: test data " << infile_path + "test.bin" << " could not be read\n";
        return -1;
    }

    if(mmap) {
        train = read_mmap((infile_path + "train.bin").c_str(), n_points, dim);
    } else {
        train = read_memory((infile_path + "train.bin").c_str(), n_points, dim);
    }

    if(!train) {
        std::cerr << "in mrpt_comparison: training data " << infile_path + "train.bin" << " could not be read\n";
        return -1;
    }


    const Map<const MatrixXf> *M = new Map<const MatrixXf>(train, dim, n_points);
    Map<MatrixXf> *test_queries = new Map<MatrixXf>(test, dim, n_test);

    if(!parallel) omp_set_num_threads(1);
    int seed_mrpt = 12345;

    std::vector<int> ks{1, 10, 100};
    std::vector<double> build_times;
    std::vector<int> target_recalls(99);
    std::iota(target_recalls.begin(), target_recalls.end(), 1);

    Mrpt index(M);
    index.grow(trees_max, depth_max, density, seed_mrpt);

    for (int j = 0; j < ks.size(); ++j) {
      int k = ks[j];

      double build_start = omp_get_wtime();
      Autotuning at(M, test_queries);
      at.tune(trees_max, depth_min, depth_max, votes_max, density, k, seed_mrpt);
      double build_time = omp_get_wtime() - build_start;
      build_times.push_back(build_time);

      bool add = j ? true : false;
      at.write_results(result_file, add);

      for(const auto &tr : target_recalls) {

        double start_subset = omp_get_wtime();
        Mrpt index2(M);
        at.subset_trees(tr, index, index2);
        double end_subset = omp_get_wtime();

        if(index2.is_empty()) {
          continue;
        }

        std::vector<double> times;
        std::vector<std::set<int>> idx;

        for (int i = 0; i < n_test; ++i) {
                std::vector<int> result(k);
                Map<VectorXf> q(&test[i * dim], dim);

                double start = omp_get_wtime();
                at.query(q, &result[0], index2);
                double end = omp_get_wtime();

                times.push_back(end - start);
                idx.push_back(std::set<int>(result.begin(), result.begin() + k));
        }

        if(verbose)
            std::cout << "k: " << k << ", # of trees: " << index2.get_n_trees() << ", depth: " << index2.get_depth() << ", density: " << density << ", votes: " << index2.get_votes() << "\n";
        else
            std::cout << k << " " << index2.get_n_trees() << " " << index2.get_depth() << " " << density << " " << index2.get_votes() << " ";

        results(k, times, idx, (result_path + "truth_" + std::to_string(k)).c_str(), verbose);
        std::cout << end_subset - start_subset << std::endl;


      }

    }



    delete[] test;
    if(!mmap) delete[] train;

    return 0;
}
