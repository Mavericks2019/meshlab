#include "ConvexQuadOptimization.h"

#if 0 // Legacy MOSEK 7.1 implementation retained for reference.



bool solveConvexQuadPorgramming_mosek(std::vector<MSKboundkeye>& bkc, std::vector<double>& blc, std::vector<double>& buc,/* Bounds on constraints. */
	std::vector<MSKboundkeye>& bkx, std::vector<double>& blx, std::vector<double>& bux,/* Bounds on variables. */
	std::vector<MSKlidxt>& aptrb, std::vector<MSKidxt>& asub, std::vector<double>& aval, 
	std::vector<MSKidxt>& qsubi, std::vector<MSKidxt>& qsubj, std::vector<double>& qval, std::vector<double>& c,
	std::vector<double>& XX, bool show_success)
{
	int NUMCON = bkc.size(); int NUMVAR, NUMANZ;
	if (NUMCON == 0)
	{
		NUMVAR = c.size();
		NUMANZ = 0;
	}
	else
	{
		NUMVAR = aptrb.size() - 1;
		NUMANZ = aptrb.back();
	}
	int NUMQNZ = qval.size();

	MSKidxt       i,j;
	XX.resize(NUMVAR);

	MSKenv_t      env;
	MSKtask_t     task;
	MSKrescodee   r;

	r = MSK_makeenv(&env,NULL);
	if ( r==MSK_RES_OK )
	{
		/* Directs the log stream to the 'printstr' function. */
		MSK_linkfunctoenvstream(env,
			MSK_STREAM_LOG,
			NULL,
			printstr);
	}

	bool s = false;
	/* Initialize the environment. */   
	r = MSK_initenv(env);
	if ( r == MSK_RES_OK )
	{ 
		/* Create the optimization task. */
		r = MSK_maketask(env,NUMCON,NUMVAR,&task);

		if ( r==MSK_RES_OK )
		{
			//r = MSK_linkfunctotaskstream(task,MSK_STREAM_LOG,NULL,printstr);
      
			/* Give MOSEK an estimate of the size of the input data. 
			 This is done to increase the speed of inputting data. 
			 However, it is optional. */
			if (r == MSK_RES_OK)
				r = MSK_putmaxnumvar(task,NUMVAR);
    
			if (r == MSK_RES_OK)
				r = MSK_putmaxnumcon(task,NUMCON);
      
			if (r == MSK_RES_OK)
				r = MSK_putmaxnumanz(task,NUMANZ);
  
			/* Append 'NUMCON' empty constraints.
			The constraints will initially have no bounds. */
			if ( r == MSK_RES_OK )
				r = MSK_appendcons(task, NUMCON);
  
			/* Append 'NUMVAR' variables.
			The variables will initially be fixed at zero (x=0). */
			if ( r == MSK_RES_OK )
				r = MSK_appendvars(task, NUMVAR);
  
			/* Optionally add a constant term to the objective. */
			if ( r ==MSK_RES_OK )
				r = MSK_putcfix(task,0.0);

			if (r == MSK_RES_OK)
				r = MSK_putintparam(task, MSK_IPAR_NUM_THREADS, 8);

			/*if (r == MSK_RES_OK)
			r = MSK_putdouparam(task,MSK_DPAR_INTPNT_CO_TOL_INFEAS ,1e-8);

			if (r == MSK_RES_OK)
			r = MSK_putdouparam(task,MSK_DPAR_INTPNT_CO_TOL_MU_RED ,1e-20);*/
			for(j=0; j<NUMVAR && r == MSK_RES_OK; ++j)
			{
				/* Set the linear term c_j in the objective.*/  
				if(r == MSK_RES_OK)
					r = MSK_putcj(task,j,c[j]);

				/* Set the bounds on variable j.
				blx[j] <= x_j <= bux[j] */
				if(r == MSK_RES_OK)
					r = MSK_putvarbound(task,
										j,           /* Index of variable.*/
										bkx[j],      /* Bound key.*/
										blx[j],      /* Numerical value of lower bound.*/
										bux[j]);     /* Numerical value of upper bound.*/
  
				if (NUMCON > 0)
				{
					/* Input column j of A */
					if (r == MSK_RES_OK)
						r = MSK_putacol(task,
						j,                 /* Variable (column) index.*/
						aptrb[j + 1] - aptrb[j], /* Number of non-zeros in column j.*/
						&asub[0] + aptrb[j],     /* Pointer to row indexes of column j.*/
						&aval[0] + aptrb[j]);    /* Pointer to Values of column j.*/
				}
			}
  
			/* Set the bounds on constraints.
			for i=1, ...,NUMCON : blc[i] <= constraint i <= buc[i] */
			for(i=0; i<NUMCON && r==MSK_RES_OK; ++i)
			{
				r = MSK_putconbound(task,
									i,           /* Index of constraint.*/
									bkc[i],      /* Bound key.*/
									blc[i],      /* Numerical value of lower bound.*/
									buc[i]);     /* Numerical value of upper bound.*/
			}

			if ( r==MSK_RES_OK )
			{
				/*
					* The lower triangular part of the Q
					* matrix in the objective is specified.
					*/

				/* Input the Q for the objective. */

				r = MSK_putqobj(task,NUMQNZ,&qsubi[0],&qsubj[0],&qval[0]);
			}

		
			//MSK_IPAR_INTPNT_NUM_THREADS

			if ( r==MSK_RES_OK )
			{
				MSKrescodee trmcode;

				/* Run optimizer */
				r = MSK_optimizetrm(task,&trmcode);

				/* Print a summary containing information
					about the solution for debugging purposes*/
				MSK_solutionsummary (task,MSK_STREAM_LOG);
        
				if ( r==MSK_RES_OK )
				{
					MSKsolstae solsta;
					int j;
          
					MSK_getsolsta(task,MSK_SOL_ITR,&solsta);

					switch (solsta)
					{
					case MSK_SOL_STA_OPTIMAL:
					case MSK_SOL_STA_NEAR_OPTIMAL:
						MSK_getxx(task,
							MSK_SOL_ITR,    /* Request the interior solution. */
							&XX[0]);

						if (show_success) printf("Optimal primal solution\n");
						s = true;
						break;
					case MSK_SOL_STA_DUAL_INFEAS_CER:
					case MSK_SOL_STA_PRIM_INFEAS_CER:
					case MSK_SOL_STA_NEAR_DUAL_INFEAS_CER:
					case MSK_SOL_STA_NEAR_PRIM_INFEAS_CER:
						printf("Primal or dual infeasibility certificate found.\n");
						break;

					case MSK_SOL_STA_UNKNOWN:
						printf("The status of the solution could not be determined.\n");
						break;
					default:
						printf("Other solution status.");
						break;
					}
				}
				else
				{
					printf("Error while optimizing.\n");
				}
			}

			if (r != MSK_RES_OK)
			{
				/* In case of an error print error code and description. */      
				char symname[MSK_MAX_STR_LEN];
				char desc[MSK_MAX_STR_LEN];

				printf("An error occurred while optimizing.\n");     
				MSK_getcodedesc (r,
									symname,
									desc);
				printf("Error %s - '%s'\n",symname,desc);
			}
		}
	}
	MSK_deletetask(&task);
	MSK_deleteenv(&env);
	return s;
}
#endif

#include <Eigen/Dense>
#include <algorithm>

namespace
{
struct Constraint
{
	Eigen::VectorXd coefficients;
	double value;
	bool lower;
	bool equality;
};

bool solveKkt(const Eigen::MatrixXd& q, const Eigen::VectorXd& c,
	const std::vector<Constraint>& constraints, const std::vector<int>& workingSet,
	Eigen::VectorXd& x, Eigen::VectorXd& multipliers)
{
	const int n = static_cast<int>(q.rows());
	const int m = static_cast<int>(workingSet.size());
	Eigen::MatrixXd kkt = Eigen::MatrixXd::Zero(n + m, n + m);
	kkt.topLeftCorner(n, n) = q;
	Eigen::VectorXd rhs(n + m);
	rhs.head(n) = -c;

	for (int row = 0; row < m; ++row)
	{
		const Constraint& constraint = constraints[workingSet[row]];
		kkt.block(0, n + row, n, 1) = constraint.coefficients;
		kkt.block(n + row, 0, 1, n) = constraint.coefficients.transpose();
		rhs[n + row] = constraint.value;
	}

	Eigen::FullPivLU<Eigen::MatrixXd> decomposition(kkt);
	if (!decomposition.isInvertible()) return false;
	const Eigen::VectorXd solution = decomposition.solve(rhs);
	if (!solution.allFinite()) return false;
	x = solution.head(n);
	multipliers = solution.tail(m);
	return true;
}
}

bool solveConvexQuadPorgramming_mosek(std::vector<MSKboundkeye>& bkc, std::vector<double>& blc,
	std::vector<double>& buc, std::vector<MSKboundkeye>& bkx, std::vector<double>& blx,
	std::vector<double>& bux, std::vector<MSKlidxt>& aptrb, std::vector<MSKidxt>& asub,
	std::vector<double>& aval, std::vector<MSKidxt>& qsubi, std::vector<MSKidxt>& qsubj,
	std::vector<double>& qval, std::vector<double>& c, std::vector<double>& xx, bool)
{
	const int n = static_cast<int>(c.size());
	Eigen::MatrixXd q = Eigen::MatrixXd::Zero(n, n);
	for (size_t k = 0; k < qval.size(); ++k)
	{
		q(qsubi[k], qsubj[k]) += qval[k];
		if (qsubi[k] != qsubj[k]) q(qsubj[k], qsubi[k]) += qval[k];
	}
	q.diagonal().array() += 1.0e-10;

	Eigen::MatrixXd a = Eigen::MatrixXd::Zero(static_cast<int>(bkc.size()), n);
	for (int column = 0; column < n; ++column)
		for (int k = aptrb[column]; k < aptrb[column + 1]; ++k)
			a(asub[k], column) = aval[k];

	std::vector<Constraint> constraints;
	std::vector<int> workingSet;
	auto addConstraint = [&](const Eigen::VectorXd& coefficients, double value, bool lower, bool equality)
	{
		constraints.push_back({coefficients, value, lower, equality});
		if (equality) workingSet.push_back(static_cast<int>(constraints.size()) - 1);
	};

	for (int row = 0; row < static_cast<int>(bkc.size()); ++row)
	{
		if (bkc[row] == MSK_BK_FX) addConstraint(a.row(row), blc[row], true, true);
		else
		{
			if (bkc[row] == MSK_BK_LO || bkc[row] == MSK_BK_RA) addConstraint(a.row(row), blc[row], true, false);
			if (bkc[row] == MSK_BK_UP || bkc[row] == MSK_BK_RA) addConstraint(a.row(row), buc[row], false, false);
		}
	}
	for (int column = 0; column < n; ++column)
	{
		Eigen::VectorXd unit = Eigen::VectorXd::Zero(n);
		unit[column] = 1.0;
		if (bkx[column] == MSK_BK_FX) addConstraint(unit, blx[column], true, true);
		else
		{
			if (bkx[column] == MSK_BK_LO || bkx[column] == MSK_BK_RA) addConstraint(unit, blx[column], true, false);
			if (bkx[column] == MSK_BK_UP || bkx[column] == MSK_BK_RA) addConstraint(unit, bux[column], false, false);
		}
	}

	Eigen::VectorXd x(n), multipliers;
	const Eigen::VectorXd linear = Eigen::Map<const Eigen::VectorXd>(c.data(), n);
	constexpr double tolerance = 1.0e-8;
	for (int iteration = 0; iteration < 4 * n + 100; ++iteration)
	{
		if (!solveKkt(q, linear, constraints, workingSet, x, multipliers)) return false;

		int violated = -1;
		double largestViolation = tolerance;
		for (int index = 0; index < static_cast<int>(constraints.size()); ++index)
		{
			if (constraints[index].equality ||
				std::find(workingSet.begin(), workingSet.end(), index) != workingSet.end()) continue;
			const double residual = constraints[index].coefficients.dot(x) - constraints[index].value;
			const double violation = constraints[index].lower ? -residual : residual;
			if (violation > largestViolation) { largestViolation = violation; violated = index; }
		}
		if (violated >= 0) { workingSet.push_back(violated); continue; }

		int removePosition = -1;
		double worstSign = tolerance;
		for (int position = 0; position < static_cast<int>(workingSet.size()); ++position)
		{
			const Constraint& constraint = constraints[workingSet[position]];
			if (constraint.equality) continue;
			const double badSign = constraint.lower ? multipliers[position] : -multipliers[position];
			if (badSign > worstSign) { worstSign = badSign; removePosition = position; }
		}
		if (removePosition >= 0) { workingSet.erase(workingSet.begin() + removePosition); continue; }

		xx.assign(x.data(), x.data() + x.size());
		return true;
	}
	return false;
}

#if 0 // Experimental MOSEK restoration retained for reference.
#ifdef AAATLAS_USE_MOSEK
namespace
{
bool solveWithMosek(std::vector<MSKboundkeye>& bkc, std::vector<double>& blc,
	std::vector<double>& buc, std::vector<MSKboundkeye>& bkx, std::vector<double>& blx,
	std::vector<double>& bux, std::vector<MSKlidxt>& aptrb, std::vector<MSKidxt>& asub,
	std::vector<double>& aval, std::vector<MSKidxt>& qsubi, std::vector<MSKidxt>& qsubj,
	std::vector<double>& qval, std::vector<double>& c, std::vector<double>& xx, bool showSuccess)
{
	const MSKint32t numCon = static_cast<MSKint32t>(bkc.size());
	const MSKint32t numVar = static_cast<MSKint32t>(c.size());
	MSKenv_t env = nullptr;
	MSKtask_t task = nullptr;
	MSKrescodee result = MSK_makeenv(&env, nullptr);
	if (result == MSK_RES_OK) result = MSK_maketask(env, numCon, numVar, &task);
	if (result == MSK_RES_OK) result = MSK_appendcons(task, numCon);
	if (result == MSK_RES_OK) result = MSK_appendvars(task, numVar);

	for (MSKint32t column = 0; column < numVar && result == MSK_RES_OK; ++column)
	{
		result = MSK_putcj(task, column, c[column]);
		if (result == MSK_RES_OK)
			result = MSK_putvarbound(task, column, bkx[column], blx[column], bux[column]);
		if (result == MSK_RES_OK && numCon > 0)
			result = MSK_putacol(task, column, aptrb[column + 1] - aptrb[column],
				asub.data() + aptrb[column], aval.data() + aptrb[column]);
	}
	for (MSKint32t row = 0; row < numCon && result == MSK_RES_OK; ++row)
		result = MSK_putconbound(task, row, bkc[row], blc[row], buc[row]);

	if (result == MSK_RES_OK && !qval.empty())
		result = MSK_putqobj(task, static_cast<MSKint32t>(qval.size()),
			qsubi.data(), qsubj.data(), qval.data());
	if (result == MSK_RES_OK)
	{
		MSKrescodee termination = MSK_RES_OK;
		result = MSK_optimizetrm(task, &termination);
	}

	bool solved = false;
	if (result == MSK_RES_OK)
	{
		MSKsolstae status = MSK_SOL_STA_UNKNOWN;
		result = MSK_getsolsta(task, MSK_SOL_ITR, &status);
		if (result == MSK_RES_OK && status == MSK_SOL_STA_OPTIMAL)
		{
			xx.resize(numVar);
			result = MSK_getxx(task, MSK_SOL_ITR, xx.data());
			solved = result == MSK_RES_OK;
			if (solved && showSuccess) std::printf("Optimal primal solution (MOSEK)\n");
		}
	}

	if (result != MSK_RES_OK)
	{
		char symbol[MSK_MAX_STR_LEN]{};
		char description[MSK_MAX_STR_LEN]{};
		MSK_getcodedesc(result, symbol, description);
		std::fprintf(stderr, "MOSEK unavailable (%s: %s); using Eigen fallback.\n", symbol, description);
	}
	if (task) MSK_deletetask(&task);
	if (env) MSK_deleteenv(&env);
	return solved;
}
}
#endif

bool solveConvexQuadPorgramming_mosek(std::vector<MSKboundkeye>& bkc, std::vector<double>& blc,
	std::vector<double>& buc, std::vector<MSKboundkeye>& bkx, std::vector<double>& blx,
	std::vector<double>& bux, std::vector<MSKlidxt>& aptrb, std::vector<MSKidxt>& asub,
	std::vector<double>& aval, std::vector<MSKidxt>& qsubi, std::vector<MSKidxt>& qsubj,
	std::vector<double>& qval, std::vector<double>& c, std::vector<double>& xx, bool showSuccess)
{
#ifdef AAATLAS_USE_MOSEK
	if (solveWithMosek(bkc, blc, buc, bkx, blx, bux, aptrb, asub, aval,
		qsubi, qsubj, qval, c, xx, showSuccess)) return true;
#endif
	return solveConvexQuadProgrammingEigen(bkc, blc, buc, bkx, blx, bux, aptrb,
		asub, aval, qsubi, qsubj, qval, c, xx, showSuccess);
}
#endif
