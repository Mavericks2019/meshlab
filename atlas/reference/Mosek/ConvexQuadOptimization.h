#ifndef CONVEX_QUAD_OPRIMIZATION_H
#define CONVEX_QUAD_OPRIMIZATION_H

#include <vector>

using MSKidxt = int;
using MSKlidxt = int;

enum MSKboundkeye
{
	MSK_BK_LO = 0,
	MSK_BK_UP = 1,
	MSK_BK_FX = 2,
	MSK_BK_FR = 3,
	MSK_BK_RA = 4
};

constexpr double MSK_INFINITY = 1.0e30;

bool solveConvexQuadPorgramming_mosek(std::vector<MSKboundkeye>& bkc, std::vector<double>& blc, std::vector<double>& buc,
	std::vector<MSKboundkeye>& bkx, std::vector<double>& blx, std::vector<double>& bux,
	std::vector<MSKlidxt>& aptrb, std::vector<MSKidxt>& asub, std::vector<double>& aval,
	std::vector<MSKidxt>& qsubi, std::vector<MSKidxt>& qsubj, std::vector<double>& qval, std::vector<double>& c,
	std::vector<double>& XX, bool show_success = true);


#endif
