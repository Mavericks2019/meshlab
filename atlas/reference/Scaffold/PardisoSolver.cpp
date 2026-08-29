#include "PardisoSolver.h"

#if 0 // Original MKL implementation retained below for historical reference.

//#define PLOTS_PARDISO 

PardisoSolver::PardisoSolver()
{

}


PardisoSolver::~PardisoSolver()
{
	if (mtype == -1)
		return;
	/* -------------------------------------------------------------------- */
	/* ..  Termination and release of memory.                               */
	/* -------------------------------------------------------------------- */
	/*for (i = 0; i < num + 1; i++) {
		ia[i] -= 1;
		}
		for (i = 0; i < nnz; i++) {
		ja[i] -= 1;
	}*/
	//ia.clear(); ja.clear();

	phase = -1;                 /* Release internal memory. */

	pardiso(pt, &maxfct, &mnum, &mtype, &phase,
		&num, &ddum, ia.data(), ja.data(), &idum, &nrhs,
		iparm, &msglvl, &ddum, &ddum, &error);

}

void PardisoSolver::pardiso_init()
{
	mtype = -2;
	if (mtype == -1)
		throw std::runtime_error("Pardiso mtype not set.");

	/* -------------------------------------------------------------------- */
	/* ..  Setup Pardiso control parameters.                                */
	/* -------------------------------------------------------------------- */
	for (i = 0; i < 64; i++)
	{
		pt[i] = 0;
	}

	//printf("Variable number : %d\n", num);

	result.clear();
	result.resize(num, 0);

	nrhs = 1;
	error = 0;
	solver = 0;/* use sparse direct solver */
	pardisoinit(pt, &mtype, iparm);

	//if (error != 0)
	//{
	//	if (error == -10)
	//		throw std::runtime_error("No license file found \n");
	//	if (error == -11)
	//		throw std::runtime_error("License is expired \n");
	//	if (error == -12)
	//		throw std::runtime_error("Wrong username or hostname \n");
	//}
	// else
	//   printf("[PARDISO]: License check was successful ... \n");


	///* Numbers of processors, value of OMP_NUM_THREADS */
	// var = getenv("OMP_NUM_THREADS");
	// if(var != NULL)
	//   sscanf( var, "%d", &num_procs );
	// else 
	//  throw std::runtime_error("Set environment OMP_NUM_THREADS to 1");

	////num_procs =4;
	//iparm[2] = num_procs;

	maxfct = 1;		/* Maximum number of numerical factorizations.  */
	mnum = 1;         /* Which factorization to use. */

	msglvl = 0;         /* Print statistical information  */
	error = 0;         /* Initialize error flag */

	for (i = 0; i < num + 1; i++) {
		ia[i] += 1;
	}
	for (i = 0; i < nnz; i++) {
		ja[i] += 1;
	}

	//  /* -------------------------------------------------------------------- */
	//  /* Initialize the internal solver memory pointer. This is only */
	//  /* necessary for the FIRST call of the PARDISO solver. */
	//  /* -------------------------------------------------------------------- */
	/*for ( i = 0; i < 64; i++ )
	{
	pt[i] = 0;
	}*/
	phase = 11;
	//cout << "err" << endl;
	pardiso(pt, &maxfct, &mnum, &mtype, &phase,
		&num, a.data(), ia.data(), ja.data(), &idum, &nrhs,
		iparm, &msglvl, &ddum, &ddum, &error);

	if (error != 0) {
		printf("\nERROR during symbolic factorization: %d", error);
		exit(1);
	}
}
bool PardisoSolver::factorize()
{
	if (mtype == -1)
		throw std::runtime_error("Pardiso mtype not set.");
	/* -------------------------------------------------------------------- */
	/* ..  Numerical factorization.                                         */
	/* -------------------------------------------------------------------- */
	phase = 22;
	//  iparm[32] = 1; /* compute determinant */

	pardiso(pt, &maxfct, &mnum, &mtype, &phase,
		&num, a.data(), ia.data(), ja.data(), &idum, &nrhs,
		iparm, &msglvl, &ddum, &ddum, &error);

	
#ifdef PLOTS_PARDISO
	printf("\nFactorization completed ... ");
#endif
	return (error == 0);
}


void PardisoSolver::pardiso_solver()
{
	

#ifdef PLOTS_PARDISO
	/* -------------------------------------------------------------------- */
	/* ..  pardiso_chkvec(...)                                              */
	/*     Checks the given vectors for infinite and NaN values             */
	/*     Input parameters (see PARDISO user manual for a description):    */
	/*     Use this functionality only for debugging purposes               */
	/* -------------------------------------------------------------------- */

	pardiso_chkvec(&numRows, &nrhs, rhs.data(), &error);
	if (error != 0) {
		printf("\nERROR  in right hand side: %d", error);
		exit(1);
	}

	/* -------------------------------------------------------------------- */
	/* .. pardiso_printstats(...)                                           */
	/*    prints information on the matrix to STDOUT.                       */
	/*    Use this functionality only for debugging purposes                */
	/* -------------------------------------------------------------------- */

	pardiso_printstats(&mtype, &numRows, a.data(), ia.data(), ja.data(), &nrhs, rhs.data(), &error);
	if (error != 0) {
		printf("\nERROR right hand side: %d", error);
		exit(1);
	}

#endif
	/* -------------------------------------------------------------------- */
	/* ..  Back substitution and iterative refinement.                      */
	/* -------------------------------------------------------------------- */
	phase = 33;

	iparm[7] = 1;       /* Max numbers of iterative refinement steps. */

	pardiso(pt, &maxfct, &mnum, &mtype, &phase,
		&num, a.data(), ia.data(), ja.data(), &idum, &nrhs,
		iparm, &msglvl, rhs.data(), result.data(), &error);

#ifdef PLOTS_PARDISO
	printf("\nSolve completed ... ");
	printf("\nThe solution of the system is: ");
	for (i = 0; i < numRows; i++) {
		printf("\n x [%d] = % f", i, result.data()[i]);
	}
	printf("\n\n");
#endif
}

void PardisoSolver::free_numerical_factorization_memory()
{
	phase = 0;                 /* Release internal memory. */

	pardiso(pt, &maxfct, &mnum, &mtype, &phase,
		&num, &ddum, ia.data(), ja.data(), &idum, &nrhs,
		iparm, &msglvl, &ddum, &ddum, &error);
}
#endif

#if 0 // Experimental oneMKL restoration; the validated build uses Eigen below.

#include <stdexcept>

namespace
{
void callPardiso(void** pt, MKL_INT& maxfct, MKL_INT& mnum, MKL_INT& mtype,
	MKL_INT phase, MKL_INT num, double* values, int* rows, int* columns,
	MKL_INT& nrhs, MKL_INT* iparm, MKL_INT& msglvl, double* rhs,
	double* result, MKL_INT& error)
{
	MKL_INT dummy = 0;
	pardiso(pt, &maxfct, &mnum, &mtype, &phase, &num, values,
		reinterpret_cast<MKL_INT*>(rows), reinterpret_cast<MKL_INT*>(columns),
		&dummy, &nrhs, iparm, &msglvl, rhs, result, &error);
}
}

PardisoSolver::PardisoSolver() = default;

PardisoSolver::~PardisoSolver()
{
	if (!initialized_) return;
	MKL_INT phase = -1;
	double dummy = 0.0;
	callPardiso(pt_, maxfct_, mnum_, mtype_, phase, num, &dummy, ia.data(),
		ja.data(), nrhs_, iparm_, msglvl_, &dummy, &dummy, error_);
}

void PardisoSolver::pardiso_init()
{
	static_assert(sizeof(int) == sizeof(MKL_INT), "AAAtlas requires oneMKL LP64 integers");
	if (num <= 0 || ia.size() != static_cast<size_t>(num + 1))
		throw std::runtime_error("Invalid CSR matrix passed to PARDISO");

	result.assign(num, 0.0);
	std::fill(std::begin(pt_), std::end(pt_), nullptr);
	pardisoinit(pt_, &mtype_, iparm_);
	iparm_[34] = 1; // The AAAtlas CSR arrays use zero-based indexing.

	MKL_INT phase = 11;
	callPardiso(pt_, maxfct_, mnum_, mtype_, phase, num, a.data(), ia.data(),
		ja.data(), nrhs_, iparm_, msglvl_, rhs.data(), result.data(), error_);
	if (error_ != 0)
		throw std::runtime_error("PARDISO symbolic factorization failed: " + std::to_string(error_));
	initialized_ = true;
}

bool PardisoSolver::factorize()
{
	if (!initialized_) return false;
	MKL_INT phase = 22;
	callPardiso(pt_, maxfct_, mnum_, mtype_, phase, num, a.data(), ia.data(),
		ja.data(), nrhs_, iparm_, msglvl_, rhs.data(), result.data(), error_);
	return error_ == 0;
}

void PardisoSolver::pardiso_solver()
{
	if (!initialized_) throw std::runtime_error("PARDISO has not been initialized");
	MKL_INT phase = 33;
	iparm_[7] = 1;
	callPardiso(pt_, maxfct_, mnum_, mtype_, phase, num, a.data(), ia.data(),
		ja.data(), nrhs_, iparm_, msglvl_, rhs.data(), result.data(), error_);
	if (error_ != 0)
		throw std::runtime_error("PARDISO solve failed: " + std::to_string(error_));
}

void PardisoSolver::free_numerical_factorization_memory()
{
	if (!initialized_) return;
	MKL_INT phase = 0;
	double dummy = 0.0;
	callPardiso(pt_, maxfct_, mnum_, mtype_, phase, num, &dummy, ia.data(),
		ja.data(), nrhs_, iparm_, msglvl_, &dummy, &dummy, error_);
}

#else

PardisoSolver::PardisoSolver() = default;

PardisoSolver::~PardisoSolver() = default;

void PardisoSolver::pardiso_init()
{
	result.assign(num, 0.0);
}

bool PardisoSolver::factorize()
{
	std::vector<Eigen::Triplet<double>> entries;
	entries.reserve(static_cast<size_t>(nnz) * 2);
	for (int row = 0; row < num; ++row)
	{
		for (int index = ia[row]; index < ia[row + 1]; ++index)
		{
			const int column = ja[index];
			entries.emplace_back(row, column, a[index]);
			if (row != column) entries.emplace_back(column, row, a[index]);
		}
	}
	matrix_.resize(num, num);
	matrix_.setFromTriplets(entries.begin(), entries.end());
	matrix_.makeCompressed();
	solver_.analyzePattern(matrix_);
	solver_.factorize(matrix_);
	return solver_.info() == Eigen::Success;
}

void PardisoSolver::pardiso_solver()
{
	const Eigen::Map<const Eigen::VectorXd> rightHandSide(rhs.data(), static_cast<Eigen::Index>(rhs.size()));
	const Eigen::VectorXd solution = solver_.solve(rightHandSide);
	result.assign(solution.data(), solution.data() + solution.size());
}

void PardisoSolver::free_numerical_factorization_memory()
{
	matrix_.resize(0, 0);
}

#endif
