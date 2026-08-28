#include "PardisoSolver.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

PardisoSolver::PardisoSolver() : nnz(0), num(0), factorized_(false)
{
}

PardisoSolver::~PardisoSolver()
{
}

void PardisoSolver::pardiso_init()
{
	if (num <= 0 || ia.size() != static_cast<size_t>(num + 1))
		throw std::runtime_error("Invalid sparse matrix dimensions.");

	result.assign(num, 0.0);
	factorized_ = false;
}

bool PardisoSolver::factorize()
{
	if (a.size() != ja.size() || ia.size() != static_cast<size_t>(num + 1))
		throw std::runtime_error("Invalid CSR sparse matrix data.");

	std::vector<Eigen::Triplet<double> > entries;
	entries.reserve(a.size() * 2);
	for (int row = 0; row < num; ++row)
	{
		for (int index = ia[row]; index < ia[row + 1]; ++index)
		{
			const int column = ja[index];
			if (!std::isfinite(a[index]))
				throw std::runtime_error("Sparse matrix contains a non-finite value.");
			if (column < row)
				continue;
			if (column < 0 || column >= num)
				throw std::runtime_error("Sparse matrix column is out of range.");

			entries.push_back(Eigen::Triplet<double>(row, column, a[index]));
			if (column != row)
				entries.push_back(Eigen::Triplet<double>(column, row, a[index]));
		}
	}

	matrix_.resize(num, num);
	matrix_.setFromTriplets(entries.begin(), entries.end());
	matrix_.makeCompressed();
	solver_.compute(matrix_);
	factorized_ = solver_.info() == Eigen::Success;

	// Parameterization energies are translation invariant, so their Hessians can
	// contain a two-dimensional null space. PARDISO tolerated those pivots; add a
	// scale-aware diagonal shift when Eigen reports a singular factorization.
	if (!factorized_)
	{
		double diagonal_scale = 1.0;
		for (int index = 0; index < num; ++index)
			diagonal_scale = (std::max)(diagonal_scale, std::abs(matrix_.coeff(index, index)));

		for (int attempt = 0; attempt < 4 && !factorized_; ++attempt)
		{
			const double shift = diagonal_scale * std::pow(10.0, -10.0 + attempt * 2.0);
			for (int index = 0; index < num; ++index)
				matrix_.coeffRef(index, index) += shift;
			matrix_.makeCompressed();
			solver_.compute(matrix_);
			factorized_ = solver_.info() == Eigen::Success;
		}
	}
	return factorized_;
}

void PardisoSolver::pardiso_solver()
{
	if (!factorized_)
		throw std::runtime_error("Sparse matrix factorization failed.");
	if (rhs.size() != static_cast<size_t>(num))
		throw std::runtime_error("Right-hand side has an invalid size.");
	for (size_t index = 0; index < rhs.size(); ++index)
		if (!std::isfinite(rhs[index]))
			throw std::runtime_error("Right-hand side contains a non-finite value.");

	const Eigen::Map<const Eigen::VectorXd> right_hand_side(rhs.data(), num);
	const Eigen::VectorXd solution = solver_.solve(right_hand_side);
	if (solver_.info() != Eigen::Success)
		throw std::runtime_error("Sparse linear solve failed.");

	result.assign(solution.data(), solution.data() + solution.size());
}

void PardisoSolver::free_numerical_factorization_memory()
{
	factorized_ = false;
	matrix_.resize(0, 0);
}
