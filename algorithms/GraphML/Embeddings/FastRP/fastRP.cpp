#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <gle/engine/cpplib/headers.hpp>
#include <iostream>
#include <fstream>
#include <random>
#include <vector>  

inline void fastRP(ListAccum<int> degree_diagonal, int m, int n, int k, int s, int d, double beta, string weights_filepath) {
  // parameters
  std::ofstream foutput("/home/tigergraph/parameters.txt");
  foutput << "|E|:" << m << std::endl;
  foutput << "|V|:" << n << std::endl;
  foutput << "K:" << k << std::endl;
  foutput << "Random Projection S:" << s << std::endl;
  foutput << "Embedding Dimension:" << d << std::endl;
  foutput << "Normalization Strength:" << beta << std::endl;
  foutput << "Weights filepath:" << weights_filepath << std::endl;
  foutput << "Weights:" << std::endl;
  
  // get weights
  // weights should be formatted in a file, where there is one weight per line, in order
  std::vector<double> weights;
  std::ifstream finput(weights_filepath);
  string current_weight;
  while (getline(finput, current_weight)) {
    foutput << "\t" << current_weight << std::endl;
    weights.push_back(std::stod(current_weight));
  }
  finput.close();
  
  // random number generation
  std::random_device rd;  
  std::mt19937 gen(rd()); 
  std::uniform_real_distribution<double> distribution(0.0, 1.0);

  // number of non zeros for matrix R
  size_t nnz_R = (size_t) (n * d * 1.0/s);

  // R matrix fill version 2
  std::vector<Eigen::Triplet<double>> triplets_R;
  triplets_R.reserve(nnz_R);
  double p1 = 0.5/s, p2 = p1, p3 = 1- 1.0/s;
  double v1 = sqrt(s), v2 = -v1, v3 = 0.0;
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < d; j++) {
      double random_value = distribution(gen);
      double triplet_value;
      if (random_value <= p1)
          triplet_value = v1;
      else if (random_value <= p1 + p2) 
          triplet_value = v2;
      else
          triplet_value = v3;

      triplets_R.push_back(Eigen::Triplet<double>(i,j, triplet_value));
    }
  }
  Eigen::SparseMatrix<double> R(n,d);
  R.setFromTriplets(begin(triplets_R), end(triplets_R));

  // foutput << "R\n" << R << std::endl;

  // create D, L and similar matrices
  std::vector<Eigen::Triplet<double>> triplets_A;
  triplets_A.reserve(n);
  std::vector<Eigen::Triplet<double>> triplets_L;
  triplets_L.reserve(n);
  for (int i = 0; i < degree_diagonal.size(); i++) {
      int value = degree_diagonal.get(i);
      triplets_A.push_back(Eigen::Triplet<double>(i,i, pow((.5/m)* value, beta)));
      triplets_L.push_back(Eigen::Triplet<double>(i,i, 1.0/value ));
  }

  Eigen::SparseMatrix<double> A(n,n);
  A.setFromTriplets(begin(triplets_A), end(triplets_A));
  Eigen::SparseMatrix<double> L(n,n);
  L.setFromTriplets(begin(triplets_L), end(triplets_L));
  // foutput << "A\n" << A.outerSize() << std::endl;
  // foutput << "L\n" << L.outerSize() << std::endl;
  
  //embeddings vector
  std::vector<Eigen::SparseMatrix<double>> N_i;

  // store embeddings
  Eigen::SparseMatrix<double> N_1 = A * L * R;
  N_i.push_back(N_1);
  // foutput << "N_1\n" << N_1 << std::endl;

  for(int i = 1; i < k; i++) {
    // foutput << "N_" << i << std::endl << A * N_i[i-1] << std::endl; 
    N_i.push_back(A * N_i[i-1]);
  }
  // apply weights and compute N
  Eigen::SparseMatrix<double>  N(n,d);
  for (int i = 0; i < k; i++)
      N += weights[i] * N_i[i];
  
  // Output N
  foutput << "Final Embedding N\n" << N<< std::endl;
  foutput.close();
}
