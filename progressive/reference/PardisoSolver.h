#pragma once

#include <Eigen/Sparse>
#include <Eigen/SparseLU>

#include <vector>

class PardisoSolver
{
public:
	PardisoSolver();
	~PardisoSolver();

	void pardiso_init();
	bool factorize();
	void pardiso_solver();
	void free_numerical_factorization_memory();

	std::vector<double> result;
	std::vector<int> ia;
	std::vector<int> ja;
	std::vector<double> a;
	std::vector<double> rhs;
	int nnz;
	int num;

private:
	Eigen::SparseMatrix<double> matrix_;
	Eigen::SparseLU<Eigen::SparseMatrix<double> > solver_;
	bool factorized_;
};
