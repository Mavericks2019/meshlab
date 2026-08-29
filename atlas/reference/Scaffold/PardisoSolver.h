#pragma once

#include <vector>
#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <Eigen/Sparse>
#include <Eigen/SparseLU>
using namespace std;

class PardisoSolver
{
public:
	PardisoSolver();
	~PardisoSolver();
	
	void pardiso_init();
	bool factorize();
	void pardiso_solver();
	void free_numerical_factorization_memory();

	 vector<double> result;

	 vector<int> ia;
	 vector<int> ja;
	 vector<double> a;
	 vector<double> rhs;

	 int nnz;
	 int num;

protected:
	Eigen::SparseMatrix<double> matrix_;
	Eigen::SparseLU<Eigen::SparseMatrix<double>> solver_;
};

