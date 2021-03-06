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


void printParameters(const Mrpt_Parameters &op) {
  std::cerr << "n_trees:                      " << op.n_trees << "\n";
  std::cerr << "depth:                        " << op.depth << "\n";
  std::cerr << "votes:                        " << op.votes << "\n";
  std::cerr << "k:                            " << op.k << "\n";
  std::cerr << "estimated query time:         " << op.estimated_qtime * 1000.0 << " ms.\n";
  std::cerr << "estimated recall:             " << op.estimated_recall << "\n";
}

double mean(const std::vector<int> &x) {
  int n = x.size();
  int xsum = 0;
  for(int i = 0; i < n; ++i)
    xsum += x[i];
  return xsum / static_cast<double>(n);
}

int less_than(const std::vector<int> &x, int val) {
  int c = 0;
  for(const auto &i : x)
    if(i < val)
      c++;
  return c;
}

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
    std::string result_path(argv[10]);
    if (!result_path.empty() && result_path.back() != '/')
      result_path += '/';

    std::string infile_path(argv[11]);
    if (!infile_path.empty() && infile_path.back() != '/')
      infile_path += '/';

    float density = atof(argv[12]);
    bool parallel = atoi(argv[13]);

    size_t n_points = n - n_test;
    bool verbose = false;

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


    const Map<const MatrixXf> M(train, dim, n_points);
    MatrixXf M_norm(dim, n_points);
    for(int i = 0; i < n_points; ++i)
      M_norm.col(i) = M.col(i).normalized();
    // const Map<const MatrixXf> test_queries(test, dim, n_test);
    Map<MatrixXf> Q(test, dim, n_test);

    if(!parallel) omp_set_num_threads(1);
    int seed_mrpt = 12345;

    std::vector<int> ks{1, 10, 100};
    double build_time;

    for (int j = 0; j < ks.size(); ++j) {
      int k = ks[j];

      double build_start = omp_get_wtime();
      Mrpt mrpt(M_norm);
      mrpt.grow_autotune(k, trees_max, depth_max, depth_min, votes_max, density, seed_mrpt);
      double build_end = omp_get_wtime();

      std::vector<double> target_recalls {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.65, 0.7, 0.75, 0.8, 0.825, 0.85, 0.875, 0.9, 0.91, 0.92,
                                          0.93, 0.94, 0.95, 0.96, 0.96, 0.97, 0.98, 0.98, 0.99, 0.995};

      for(const auto &tr : target_recalls) {
        Mrpt mrpt_new(mrpt.subset(tr));
        Mrpt_Parameters par(mrpt_new.parameters());

        if(mrpt_new.empty()) {
          continue;
        }

        std::vector<double> times;
        std::vector<std::set<int>> idx;
        std::vector<int> cs_sizes;

        for (int i = 0; i < n_test; ++i) {
          std::vector<int> result(k);
          std::vector<float> distances(k);
          int n_elected = 0;

          double start = omp_get_wtime();
          VectorXf q_norm = Q.col(i).normalized();
          mrpt_new.query(q_norm, &result[0], &distances[0], &n_elected);
          double end = omp_get_wtime();

          times.push_back(end - start);
          idx.push_back(std::set<int>(result.begin(), result.begin() + k));
          cs_sizes.push_back(n_elected);
        }

        if(verbose)
          std::cout << "k: " << k << ", # of trees: " << par.n_trees << ", depth: " << par.depth << ", density: " << density << ", votes: " << par.votes << "\n";
        else
          std::cout << k << " " << par.n_trees << " " << par.depth << " " << density << " " << par.votes << " ";

        results(k, times, idx, (result_path + "truth_" + std::to_string(k)).c_str(), verbose);
        std::cout << build_end - build_start << std::endl;
        // std::cout << build_end - build_start << " ";
        // std::cout << mean(cs_sizes) << " ";
        // std::cout << less_than(cs_sizes, k) << std::endl;
      }
    }



    delete[] test;
    if(!mmap) delete[] train;

    return 0;
}
