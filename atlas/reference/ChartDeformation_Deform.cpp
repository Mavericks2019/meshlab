#include <iostream>
#include <iomanip>
#include "ChartDeformation.h"

#include "Optimization\Numeric\fmath.hpp"
#include "Common\CommonFunctions.h"
#include "Optimization\HLBFGS\HLBFGS.h"

#include <Eigen/SparseCholesky>
//#define AMIPS

void angle_func(const size_t N, const std::vector<double>& x, double& f, std::vector<double>& g, void* user_supply)
{
	auto ptr_this = static_cast<ChartDeformation*>(user_supply);
	ptr_this->rotating_angle_evalfunc(x, f, g);
};

template <bool EXP_ENERGY>
void align_deformation_func(const size_t N, const std::vector<double>& x, double& f, std::vector<double>& g, void* user_supply)
{
	auto ptr_this = static_cast<ChartDeformation*>(user_supply);
	ptr_this->align_energy_evalfunc<true>(x, f, g);
#ifdef AMIPS
	ptr_this->amips_energy_evalfunc<EXP_ENERGY, true, false>(x, f, g);
#else
	ptr_this->dirichlet_energy_evalfunc<true, false, false>(x, f, g);
#endif
};

template <bool EXP_ENERGY>
void final_deformation_func(const size_t N, const std::vector<double>& x, double& f, std::vector<double>& g, void* user_supply)
{
	auto ptr_this = static_cast<ChartDeformation*>(user_supply);
#ifdef AMIPS
	ptr_this->amips_energy_evalfunc<EXP_ENERGY, true, true>(x, f, g);
#else
	ptr_this->dirichlet_energy_evalfunc<true, false, true>(x, f, g);
#endif
};

void energy_iter(const size_t niter, const size_t call_iter, const size_t n_vars, const std::vector<double>& variables, const double& func_value, const std::vector<double>& gradient, const double& gnorm, void* user_pointer)
{
// 	if ((niter < 10) || (niter < 100 && (niter % 10 == 0)) || (niter % 100 == 0))
// 		std::cout << niter << " " << call_iter << " " << func_value << " " << gnorm << std::endl;
	
//	static_cast<ChartDeformation*>(user_pointer)->lbfgs_vec.push_back(variables);
}

void ChartDeformation::calc_global_rotation()
{
	HLBFGS angle_solver;
	angle_solver.set_number_of_variables(1);
	angle_solver.set_verbose(false);
	angle_solver.set_func_callback(angle_func, 0, 0, 0, 0);

	double global_theta = 0.0;
	double theta[1] = { global_theta };
	angle_solver.optimize_without_constraints(theta, 100, this);
	global_theta = theta[0];

	OpenMesh::Vec2d chart_center(0.0, 0.0);
	for (int i = 0; i < n_vertices; i++)
	{
		chart_center[0] += uv_x[2 * i + 0];
		chart_center[1] += uv_x[2 * i + 1];
	}
	chart_center /= n_vertices;

	double cos_theta = std::cos(global_theta);
	double sin_theta = std::sin(global_theta);

	for (int i = 0; i < n_vertices; i++)
	{
		OpenMesh::Vec2d point(uv_x[2 * i + 0], uv_x[2 * i + 1]);
		point -= chart_center;
		parent.rotate_uv(point, cos_theta, sin_theta);
		point += chart_center;

		uv_x[2 * i + 0] = point[0];
		uv_x[2 * i + 1] = point[1];
	}

	for (int i = 0; i < n_boundary_edges; i++)
	{
		parent.rotate_uv(boundary_h_svec[i], cos_theta, sin_theta);
	}
}

void ChartDeformation::rotating_angle_evalfunc(const std::vector<double>& x, double& f, std::vector<double>& g)
{
	double cos_theta_t = cos(x[0]);
	double sin_theta_t = sin(x[0]);

	f = 0.0; 
	g[0] = 0.0;

	for (int i = 0; i < n_boundary_edges; i++)
	{
		OpenMesh::Vec2d h_vec = boundary_h_svec[i];
		double length = boundary_h_len0[i];
		 
		parent.rotate_uv(h_vec, cos_theta_t, sin_theta_t);

		f += length * h_vec[0] * h_vec[0] * h_vec[1] * h_vec[1];
		g[0] += 2.0 * length * h_vec[0] * h_vec[1] * (h_vec[1] * h_vec[1] - h_vec[0] * h_vec[0]);
	}
}

template<bool EXP_ENERGY>
void ChartDeformation::calc_align_energy()
{
	double f0 = 0.0;
	max_angle_align = 0.0;
	energy_align = align_energy_evalfunc<false>(uv_x, f0);
#ifdef AMIPS
	energy_amips = amips_energy_evalfunc<EXP_ENERGY, false, false>(uv_x, f0);
#else
	energy_amips = dirichlet_energy_evalfunc<false, false, false>(uv_x, f0);
#endif
}

template <bool EXP_ENERGY>
void ChartDeformation::calc_align_deformation(int max_iter_times)
{
	if (max_iter_times <= 0) return;

	HLBFGS align_solver;
	align_solver.set_number_of_variables(n_vertices * 2);
	align_solver.set_verbose(false);
//	align_solver.set_func_callback(align_deformation_func<EXP_ENERGY>, 0, 0, energy_iter, 0);
	align_solver.set_func_callback(align_deformation_func<EXP_ENERGY>, 0, 0, 0, 0);

	align_solver.optimize_without_constraints(uv_x.data(), max_iter_times, this);
}

template <bool EXP_ENERGY>
void ChartDeformation::calc_inner_deformation(int max_iter_times)
{
	if (max_iter_times <= 0) return;

	HLBFGS inner_energy;
	inner_energy.set_number_of_variables(n_vertices * 2);
	inner_energy.set_verbose(false);
//	inner_energy.set_func_callback(final_deformation_func<EXP_ENERGY>, 0, 0, energy_iter, 0);
	inner_energy.set_func_callback(final_deformation_func<EXP_ENERGY>, 0, 0, 0, 0);

	inner_energy.optimize_without_constraints(uv_x.data(), max_iter_times, this);
}

template <bool EXP_ENERGY>
void ChartDeformation::calc_final_deformation(int max_iter_times)
{
	if (max_iter_times <= 0) return;

	auto uv_x0 = uv_x;

	bool invalid_result = false;
	double energy_amips0 = 0.0;
	double energy_amips1 = 0.0;

#ifdef AMIPS
	amips_energy_evalfunc<EXP_ENERGY, false, false>(uv_x, energy_amips0);
#else
	dirichlet_energy_evalfunc<false, false, false>(uv_x, energy_amips0);
#endif

	invalid_result = !CM_inner_deformation(10);
	if (invalid_result) calc_inner_deformation<EXP_ENERGY>(max_iter_times);

#ifdef AMIPS
	amips_energy_evalfunc<EXP_ENERGY, false, false>(uv_x, energy_amips1);
#else
	dirichlet_energy_evalfunc<false, false, false>(uv_x, energy_amips1);
#endif
	invalid_result = (!std::isnormal(energy_amips1) || std::abs(energy_amips1) >= 1.0e+100);

	if (invalid_result)
	{
		std::cout << "Final Deformation Failed";
		uv_x = uv_x0;
	}
	else
	{
		auto default_precision = std::cout.precision();
		std::cout << "Final Energy: ";
		std::cout << std::setprecision(6) << std::scientific << energy_amips0 << " -> " << energy_amips1;
		std::cout << std::resetiosflags(std::ios::floatfield) << std::setprecision(default_precision);
	}
}

template <bool GRADIENT>
double ChartDeformation::align_energy_evalfunc(const std::vector<double>& x, double & f, std::vector<double>& g)
{
	double alpha = align_alpha;
	double align_energy = 0.0;
	const double align_factor = energy_lambda / total_boundary_length;

	int nb_begin = 0;
	std::vector<double> h_angles(n_boundary_edges);
	std::vector<int> h_tags(n_boundary_edges);
	for (int j = 0; j < n_boundary_edges; j++)
	{
		int k = CommonFunctions::period_id(j + 1, n_boundary_edges);

		if (boundary_v_k[j] != 0 || boundary_v_k[k] != 0) continue;

		int v0 = boundary_h_vert[j].first;
		int v1 = boundary_h_vert[j].second;
		auto h_vec = get_vec(v0, v1, x).normalized();
		if (tag_dot(h_vec, boundary_h_tag[j]) <= 0.5) continue;

		nb_begin = j;
		h_angles[j] = std::atan2(h_vec[1], h_vec[0]);
		h_tags[j] = (h_angles[j] >= 0 || boundary_h_tag[j] == 0) ? boundary_h_tag[j] : boundary_h_tag[j] - 4;
		break;
	}
	for (int j = nb_begin; j > 0; j--)
	{
		double int_angle = M_PI - interior_angle(j, x);
		h_angles[j - 1] = h_angles[j] + int_angle;
		h_tags[j - 1] = h_tags[j] + boundary_v_k[j];
	}
	for (int j = nb_begin + 1; j < n_boundary_edges; j++)
	{
		double int_angle = M_PI - interior_angle(j, x);
		h_angles[j] = h_angles[j - 1] - int_angle;
		h_tags[j] = h_tags[j - 1] - boundary_v_k[j];
	}
	for (int j = 0; j < n_boundary_edges; j++)
	{
		int v0 = boundary_h_vert[j].first;
		int v1 = boundary_h_vert[j].second;
		auto h_vec = get_vec(v0, v1, x);

		double length0 = boundary_h_len0[j];
		double length = h_vec.norm();

		double diff_angle = h_angles[j] - h_tags[j] * M_PI_2;
		double diff_length = length / length0 - 1.0;
		align_energy += 0.5 * align_factor * length0 * ((1.0 - alpha) * diff_angle * diff_angle + alpha * diff_length * diff_length);

		if constexpr (GRADIENT)
		{
			double g_angle = align_factor * length0 * (1.0 - alpha) * diff_angle / (h_vec[0] * h_vec[0] + h_vec[1] * h_vec[1]);
			double g_length = align_factor * alpha * diff_length / length;
			g[2 * v0 + 0] += h_vec[1] * g_angle - h_vec[0] * g_length;
			g[2 * v1 + 0] -= h_vec[1] * g_angle - h_vec[0] * g_length;
			g[2 * v0 + 1] -= h_vec[0] * g_angle + h_vec[1] * g_length;
			g[2 * v1 + 1] += h_vec[0] * g_angle + h_vec[1] * g_length;
		}
		else
		{
			max_angle_align = std::max(std::abs(diff_angle / M_PI_2), max_angle_align);
		}
	}

	f += align_energy;
	return align_energy / energy_lambda;
}

template <bool EXP_ENERGY, bool GRADIENT, bool FIX_POINTS>
double ChartDeformation::amips_energy_evalfunc(const std::vector<double>& x, double& f, std::vector<double>& g)
{
	double alpha = amips_alpha;
	double amips_energy = 0.0;
	const double amips_factor = 1.0 / total_area;
	const double exp_factor = parent.energy_exp_factor;
	const double exp2_factor = fmath::expd(exp_factor);

	for (const auto& finfo : chart_face_info)
	{
		int v0 = finfo.uv0;
		int v1 = finfo.uv1;
		int v2 = finfo.uv2;

		double du1 = x[2 * v1 + 0] - x[2 * v0 + 0];
		double du2 = x[2 * v2 + 0] - x[2 * v0 + 0];
		double dv1 = x[2 * v1 + 1] - x[2 * v0 + 1];
		double dv2 = x[2 * v2 + 1] - x[2 * v0 + 1];

		double l2_p1 = finfo.l2_p1;
		double l2_p2 = finfo.l2_p2;
		double dot_p = finfo.dot_p;
		double det_p = finfo.det_p;

		double area = std::abs(det_p) / 2.0;

		double det_u = du1 * dv2 - du2 * dv1;
		double det = det_u / det_p;

		if (det < 1.0e-12 || abs(det_u) < 1.0e-20)
		{
			amips_energy += 1.0e+100;
			continue;
		}

		// xHx = |J|_F * det_p * det_p
		// det_u = det_J * det_p
		double xHx = l2_p2 * (du1 * du1 + dv1 * dv1)
				   + l2_p1 * (du2 * du2 + dv2 * dv2)
			 - 2.0 * dot_p * (du1 * du2 + dv1 * dv2);
		double delta_det = 0.5 * (det + 1.0 / det);
		double delta_conf = 0.5 * (xHx / det_p / det_u);
		double delta = (1.0 - alpha) * delta_det + alpha * delta_conf;

		double f_amips_factor = amips_factor * area;

		if constexpr (EXP_ENERGY)
		{
			double delta_exp = fmath::expd(exp_factor * delta);
			f_amips_factor /= exp2_factor;
			amips_energy += f_amips_factor * delta_exp;
			f_amips_factor *= exp_factor * delta_exp;
		}
		else
		{
			amips_energy += f_amips_factor * delta;
		}

		if constexpr (GRADIENT)
		{              
			double factor_xHx = f_amips_factor * 0.5 * alpha / det_u / det_p;
			double factor_det = f_amips_factor * 0.5 * ((1.0 - alpha) * (1.0 / det_p - det_p / det_u / det_u) - alpha * xHx / det_p / det_u / det_u);

			double d_xHx_u1 = 2.0 * (l2_p2 * du1 - dot_p * du2);
			double d_xHx_u2 = 2.0 * (l2_p1 * du2 - dot_p * du1);
			double d_xHx_v1 = 2.0 * (l2_p2 * dv1 - dot_p * dv2);
			double d_xHx_v2 = 2.0 * (l2_p1 * dv2 - dot_p * dv1);

			double g_u1 = factor_xHx * d_xHx_u1 + factor_det * dv2;
			double g_u2 = factor_xHx * d_xHx_u2 - factor_det * dv1;
			double g_v1 = factor_xHx * d_xHx_v1 - factor_det * du2;
			double g_v2 = factor_xHx * d_xHx_v2 + factor_det * du1;

			g[2 * v0 + 0] -= g_u1 + g_u2;
			g[2 * v0 + 1] -= g_v1 + g_v2;
			g[2 * v1 + 0] += g_u1;
			g[2 * v1 + 1] += g_v1;
			g[2 * v2 + 0] += g_u2;
			g[2 * v2 + 1] += g_v2;
		}
	}

	if constexpr (GRADIENT && FIX_POINTS) for (int i : fixed_uv_x) g[i] = 0.0;

	f += amips_energy;
	return amips_energy;
}

template <bool GRADIENT, bool HESSIAN_P, bool FIX_POINTS>
double ChartDeformation::dirichlet_energy_evalfunc(const std::vector<double>& x, double& f, std::vector<double>& g, Eigen::SparseMatrix<double>& h)
{
	double dirichlet_energy = 0.0;
	const double dirichlet_factor = 1.0 / total_area;

	std::vector<Eigen::Triplet<double>> h_list;
	if constexpr (HESSIAN_P) h_list.reserve(36 * n_faces);

	auto set_triplet = [&](int i, int j, double val)
	{
		int kkt_i = uv2kkt[i];
		int kkt_j = uv2kkt[j];
		if (kkt_i >= kkt_j && kkt_j >= 0)
		{
			h_list.emplace_back(kkt_i, kkt_j, val);
		}
	};

	for (const auto& finfo : chart_face_info)
	{
		int v0 = finfo.uv0;
		int v1 = finfo.uv1;
		int v2 = finfo.uv2;

		double du1 = x[2 * v1 + 0] - x[2 * v0 + 0];
		double du2 = x[2 * v2 + 0] - x[2 * v0 + 0];
		double dv1 = x[2 * v1 + 1] - x[2 * v0 + 1];
		double dv2 = x[2 * v2 + 1] - x[2 * v0 + 1];

		double l2_p1 = finfo.l2_p1;
		double l2_p2 = finfo.l2_p2;
		double dot_p = finfo.dot_p;
		double det_p = finfo.det_p;

		double area = std::abs(det_p) / 2.0;

		double det_u = du1 * dv2 - du2 * dv1;
		double det = det_u / det_p;

		if (det < 1.0e-12 || abs(det_u) < 1.0e-20)
		{
			dirichlet_energy += 1.0e+100;
			continue;
		}

		double xHx = l2_p2 * (du1 * du1 + dv1 * dv1)
				   + l2_p1 * (du2 * du2 + dv2 * dv2)
			 - 2.0 * dot_p * (du1 * du2 + dv1 * dv2);

		double f_dirichlet_factor = dirichlet_factor * area;
		double factor_xHx = 0.25 * (1.0 / det_u / det_u + 1.0 / det_p / det_p);

		dirichlet_energy += f_dirichlet_factor * xHx * factor_xHx;

		if constexpr (GRADIENT || HESSIAN_P)
		{
			double inv_u3 = 1.0 / det_u / det_u / det_u;
			double inv_u4 = inv_u3 / det_u;
			double factor_det = -0.5 * xHx * inv_u3;

			double d_xHx_u1 = 2.0 * (l2_p2 * du1 - dot_p * du2);
			double d_xHx_u2 = 2.0 * (l2_p1 * du2 - dot_p * du1);
			double d_xHx_v1 = 2.0 * (l2_p2 * dv1 - dot_p * dv2);
			double d_xHx_v2 = 2.0 * (l2_p1 * dv2 - dot_p * dv1);
			
			Eigen::Matrix<double, 6, 1> g_xHx, g_det;
			g_xHx << -d_xHx_u1 - d_xHx_u2, d_xHx_u1, d_xHx_u2, -d_xHx_v1 - d_xHx_v2, d_xHx_v1, d_xHx_v2;
			g_det << -dv2 + dv1, dv2, -dv1, du2 - du1, -du2, du1;

			Eigen::Matrix<double, 6, 1> g_facet = f_dirichlet_factor * (factor_xHx * g_xHx + factor_det * g_det);

			g[2 * v0 + 0] += g_facet[0];
			g[2 * v1 + 0] += g_facet[1];
			g[2 * v2 + 0] += g_facet[2];
			g[2 * v0 + 1] += g_facet[3];
			g[2 * v1 + 1] += g_facet[4];
			g[2 * v2 + 1] += g_facet[5];

			if constexpr (HESSIAN_P)
			{
				Eigen::Matrix<double, 6, 6> H_xHx;

				H_xHx.setZero();
				H_xHx(0, 0) = -4.0 * dot_p + 2.0 * l2_p1 + 2.0 * l2_p2;
				H_xHx(0, 1) = 2.0 * dot_p - 2.0 * l2_p2;
				H_xHx(0, 2) = 2.0 * dot_p - 2.0 * l2_p1;
				H_xHx(1, 0) = 2.0 * dot_p - 2.0 * l2_p2;
				H_xHx(1, 1) = 2.0 * l2_p2;
				H_xHx(1, 2) = -2.0 * dot_p;
				H_xHx(2, 0) = 2.0 * dot_p - 2.0 * l2_p1;
				H_xHx(2, 1) = -2.0 * dot_p;
				H_xHx(2, 2) = 2.0 * l2_p1;

				H_xHx(3, 3) = -4.0 * dot_p + 2.0 * l2_p1 + 2.0 * l2_p2;
				H_xHx(3, 4) = 2.0 * dot_p - 2.0 * l2_p2;
				H_xHx(3, 5) = 2.0 * dot_p - 2.0 * l2_p1;
				H_xHx(4, 3) = 2.0 * dot_p - 2.0 * l2_p2;
				H_xHx(4, 4) = 2.0 * l2_p2;
				H_xHx(4, 5) = -2.0 * dot_p;
				H_xHx(5, 3) = 2.0 * dot_p - 2.0 * l2_p1;
				H_xHx(5, 4) = -2.0 * dot_p;
				H_xHx(5, 5) = 2.0 * l2_p1;

				Eigen::Matrix<double, 6, 6> H_facet = factor_xHx * H_xHx + factor_det * H_det;

				double factor_h_uu = 1.5 * xHx * inv_u4;
				double factor_h_uh = -0.5 * inv_u3;

				for (int i = 0; i < 6; i++) for (int j = 0; j < 6; j++)
				{
					H_facet(i, j) += factor_h_uu * (g_det[i] * g_det[j]) + factor_h_uh * (g_det[i] * g_xHx[j] + g_xHx[i] * g_det[j]);
				}
				
				double alpha = 0.5 * std::sqrt(xHx + 2.0 * det_p * det_u) / det_p;
				double beta = 0.5 * std::sqrt(xHx - 2.0 * det_p * det_u) / det_p;
				double s_max = alpha + beta;
				double s_min = alpha - beta;
				double factor_h_aa = 1.0 - (alpha * alpha + 3.0 * beta * beta) / (s_max * s_max * s_max * s_min * s_min * s_min);

				if (factor_h_aa < 0.0)
				{
					Eigen::Matrix<double, 6, 1> g_A = g_xHx + 2.0 * det_p * g_det;
					Eigen::Matrix<double, 6, 6> H_A = H_xHx + 2.0 * det_p * H_det;

					Eigen::Matrix<double, 6, 6> H_alpha = H_A / 8.0 / alpha / det_p / det_p;
					double factor_ha_aa = -1.0 / 64.0 / alpha / alpha / alpha / det_p / det_p / det_p / det_p;
					for (int i = 0; i < 6; i++) for (int j = 0; j < 6; j++)
					{
						H_alpha(i, j) += factor_ha_aa * (g_A[i] * g_A[j]);
					}

					H_facet -= alpha * factor_h_aa * H_alpha;
				}

				H_facet *= f_dirichlet_factor;
				std::vector<int> vars = { 2 * v0 + 0, 2 * v1 + 0 , 2 * v2 + 0, 2 * v0 + 1, 2 * v1 + 1 , 2 * v2 + 1 };
				for (int i = 0; i < 6; i++) for (int j = 0; j < 6; j++)
				{
					set_triplet(vars[i], vars[j], H_facet(i, j));
				}
			}
		}
	}

	if constexpr (GRADIENT && FIX_POINTS) for (int i : fixed_uv_x) g[i] = 0.0;

	if constexpr (GRADIENT)
	{
		Eigen::Map<Eigen::VectorXd> g_vec(g.data(), g.size());
		double g_norm = g_vec.norm();
		if (g_norm > 1.0e+10)
		{
			g_vec *= 1.0e+10 / g_norm;
		}
	}

	if constexpr (HESSIAN_P)
	{
		int n_vars = 2 * n_vertices - fixed_uv_x.size();
		h.resize(n_vars, n_vars);
		h.setFromTriplets(h_list.begin(), h_list.end());
		h.makeCompressed();
	}

	f += dirichlet_energy;
	return dirichlet_energy;
}

bool ChartDeformation::CM_inner_deformation(int max_iter_times)
{
	if (max_iter_times <= 0) return false;
	
	for (int k = 0; k < max_iter_times; k++)
	{
		double e0;
		std::vector<double> g(2 * n_vertices, 0.0);
		std::vector<double> p(2 * n_vertices, 0.0);
		Eigen::Map<Eigen::VectorXd> g_vec(g.data(), g.size());
		Eigen::Map<Eigen::VectorXd> p_vec(p.data(), p.size());
		Eigen::SparseMatrix<double> h;

		int n_vars = 2 * n_vertices - fixed_uv_x.size();
		dirichlet_energy_evalfunc<true, true, false>(uv_x, e0, g, h);
		Eigen::SimplicialLDLT<Eigen::SparseMatrix<double>> solver;

		solver.compute(h);
		if (solver.info() != Eigen::Success) return false;
		Eigen::VectorXd solver_b(n_vars), solver_x(n_vars);

		for (int i = 0; i < 2 * n_vertices; i++) if (uv2kkt[i] >= 0) solver_b[uv2kkt[i]] = -g_vec[i];
		solver_x = solver.solve(solver_b);
		for (int i = 0; i < 2 * n_vertices; i++) if (uv2kkt[i] >= 0) p_vec[i] = solver_x[uv2kkt[i]];

		double alpha = 0.95 * calc_max_step(uv_x, p);
		double e1 = backtracking_line_search(uv_x, g, p, alpha);

		Eigen::Map<Eigen::VectorXd>(uv_x.data(), uv_x.size()) += alpha * p_vec;

		if (std::abs(e1 - e0) / e0 < 1.0e-6) break;
	}

	return true;
}

double ChartDeformation::backtracking_line_search(const std::vector<double>& uv, const std::vector<double>& g, const std::vector<double>& p, double& alpha)
{
	double step_factor = 0.5;

	const Eigen::Map<Eigen::VectorXd> x_vec((double*)uv.data(), uv.size());
	const Eigen::Map<Eigen::VectorXd> g_vec((double*)g.data(), g.size());
	const Eigen::Map<Eigen::VectorXd> p_vec((double*)p.data(), p.size());
	double slope = 0.2 * g_vec.dot(p_vec);
	
	double e0 = 0.0, e1;
	dirichlet_energy_evalfunc<false, false, false>(uv, e0);

	std::vector<double> uv_new(uv.size(), 0.0);
	Eigen::Map<Eigen::VectorXd> x_new(uv_new.data(), uv_new.size());

	while (true)
	{
		x_new = x_vec + alpha * p_vec;

		e1 = 0.0;
		dirichlet_energy_evalfunc<false, false, false>(uv_new, e1);

		if (e1 <= e0 + alpha * slope) break;
		alpha *= step_factor;
	}

	return e1;
}

double ChartDeformation::calc_max_step(const std::vector<double>& uv, const std::vector<double>& p)
{
	auto det2 = [](const OpenMesh::Vec2d& x, const OpenMesh::Vec2d& y)
	{
		return x[0] * y[1] - x[1] * y[0];
	};

	double res = std::numeric_limits<double>::max();
	for (const auto& finfo : chart_face_info)
	{
		int v0 = finfo.uv0;
		int v1 = finfo.uv1;
		int v2 = finfo.uv2;

		auto u1 = get_vec(v1, v0, uv);
		auto u2 = get_vec(v2, v0, uv);
		auto d1 = get_vec(v1, v0, p);
		auto d2 = get_vec(v2, v0, p);

		double a = det2(d1, d2);
		double b = det2(d1, u2) + det2(u1, d2);
		double c = det2(u1, u2);
		double delta = b * b - 4.0 * a * c;

		double t = std::numeric_limits<double>::max();
		if (std::abs(a) < -1.0e-12 * b) t = -c / b;
		else if (delta >= 0.0)
		{
			delta = std::sqrt(delta);
			double t1 = 0.5 / a * (-b + delta);
			double t2 = 0.5 / a * (-b - delta);

			if (t1 > 0 && t2 > 0) t = std::min(t1, t2);
			else if (t1 > 0) t = t1;
			else if (t2 > 0) t = t2;
		}

		if (t > 0.0) res = std::min(res, t);
	}

	return res;
}

template void ChartDeformation::calc_align_energy<true>();
template void ChartDeformation::calc_align_energy<false>();
template void ChartDeformation::calc_align_deformation<true>(int max_iter_times);
template void ChartDeformation::calc_align_deformation<false>(int max_iter_times);
template void ChartDeformation::calc_inner_deformation<true>(int max_iter_times);
template void ChartDeformation::calc_inner_deformation<false>(int max_iter_times);
template void ChartDeformation::calc_final_deformation<true>(int max_iter_times);
template void ChartDeformation::calc_final_deformation<false>(int max_iter_times);
