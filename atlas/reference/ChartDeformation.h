#pragma once

#include <set>
#include "PolySquareDeformation.h"

#include "Eigen\Eigen"
#include "Eigen\Sparse"

inline std::vector<double>& unusedGradient()
{
	static std::vector<double> value;
	return value;
}

inline Eigen::SparseMatrix<double>& unusedHessian()
{
	static Eigen::SparseMatrix<double> value;
	return value;
}

class ChartUntangle;
class ChartDeformation
{
public:
	ChartDeformation(PolySquareDeformation& _parent);
	~ChartDeformation();

	void build_chart();
	void build_vertices();
	void build_faces();

	void deformation();
	void begin_deformation();
	void rotation_step();
	void align_step(int iteration);
	bool inner_step();
	void flattening_step();
	void untangle_step();
	void final_step();

	int n_vertices, n_edges, n_faces;
	int n_boundary_edges;
	double total_area, total_boundary_length;

	double kernel_width, energy_lambda;
	double align_alpha, amips_alpha;
	double energy_align, energy_amips, max_angle_align;
	double goal_length;

	bool z_pos;

	std::vector<int> boundary_h_meshid;
	std::vector<int> boundary_h_tag;
	std::vector<std::pair<int, int>> boundary_h_vert;
	std::vector<double> boundary_h_len0;
	std::vector<OpenMesh::Vec2d> boundary_h_svec;

	std::vector<int> boundary_v_k;
	std::vector<double> boundary_v_sangle;
	std::vector<double> boundary_v_angle0;

	std::vector<int> mesh_faces;
	std::vector<PolySquareDeformation::face_info> chart_face_info;
	std::set<int> fixed_uv_x;

	std::vector<double> uv_x;
	std::vector<std::vector<double>> uv_x_vec;

	std::vector<std::vector<double>> lbfgs_vec;

	void rotating_angle_evalfunc(const std::vector<double>& x, double& f, std::vector<double>& g = unusedGradient());

	template <bool GRADIENT>
	double align_energy_evalfunc(const std::vector<double>& x, double& f, std::vector<double>& g = unusedGradient());
	template <bool EXP_ENERGY, bool GRADIENT, bool FIX_POINTS>
	double amips_energy_evalfunc(const std::vector<double>& x, double& f, std::vector<double>& g = unusedGradient());
	template <bool GRADIENT, bool HESSIAN_P, bool FIX_POINTS>
	double dirichlet_energy_evalfunc(const std::vector<double>& x, double& f, std::vector<double>& g = unusedGradient(), Eigen::SparseMatrix<double>& h = unusedHessian());

private:
	PolySquareDeformation& parent;

	inline int boundary(int bid);

	template <bool EXP_ENERGY>
	void calc_align_energy();

	void calc_boundary_directions();

	void calc_global_rotation();
	template <bool EXP_ENERGY>
	void calc_align_deformation(int max_iter_times);
	template <bool EXP_ENERGY>
	void calc_inner_deformation(int max_iter_times);
	template <bool EXP_ENERGY>
	void calc_final_deformation(int max_iter_times);

	bool CM_inner_deformation(int max_iter_times);
	double backtracking_line_search(const std::vector<double>& uv, const std::vector<double>& g, const std::vector<double>& p, double& alpha);

	double calc_max_step(const std::vector<double>& uv, const std::vector<double>& p);

	void find_corners();
	void move_corners();
	void segment_flattening();
	bool mosek_flattening(std::vector<double>& sol_x);

	void polysquare_post_deformation();

	std::vector<int> uv2kkt;
	Eigen::Matrix<double, 6, 6> H_det;
	void hessian_preparation();
	void update_uv2kkt();

	void update_face_info();

	void scale_finfo(double scale);
	void check_vk();

	struct segment_info
	{
		int begin, end;
		int tag;
		int size;
		double coordinate;
		double length0;
	};
	std::vector<segment_info> segments;

	double interior_angle(int next_boundary_h, const std::vector<double>& uv);

	inline OpenMesh::Vec2d get_vec(int v0, int v1, const std::vector<double>& uv) { return OpenMesh::Vec2d(uv[2 * v1 + 0] - uv[2 * v0 + 0], uv[2 * v1 + 1] - uv[2 * v0 + 1]); };
	inline double tag_dot(const OpenMesh::Vec2d& p, int t) { return (t & 2) ? -p[t & 1] : p[t & 1]; };

	std::vector<std::pair<int, int>> aligned_uv;

private:
	struct corner_info
	{
		int bvid;
		int status;//bit01: tag, bit2: deleted
		double seg_length;

		corner_info() : bvid(-1), status(0), seg_length(0.0) {};
	};
	std::list<corner_info> boundary_corners;
	using corner = std::list<corner_info>::iterator;
	corner next_corner(corner it);
	corner prev_corner(corner it);
	void calc_seglength(corner it, const std::vector<double>& bh_len);
	void update_segs_len(int uv_id = -1);

 	bool modify_short_segments(double thres_factor = 1.0);
 	bool conflicting_segments();
	void tag_from_segments();
	void built_segments();

	friend class ChartUntangle;

	double max_sigma = 20.0;
	double min_sigma = 0.0;
};
