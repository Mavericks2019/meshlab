#include "ParaQuadCutting.h"
#include "MeshViewer/Mesh_doubleIO.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <set>
#include <queue>

#include <Eigen/Eigen>
#include <Eigen/SparseCholesky>

#include <QElapsedTimer>

#ifdef MY_DEBUG
#define MY_DOUBT(cond, msg) if (cond) std::cout << msg << std::endl;
#else
#define MY_DOUBT(cond, msg)
#endif

static OpenMesh::Vec3uc boundary_edge_color = { 229, 156,  59 };
static OpenMesh::Vec3uc interior_edge_color = { 100, 100, 100 };

ParaQuadCutting::ParaQuadCutting(Mesh& _mesh, const char* path, const char* file)
	: origin_mesh(_mesh)
{
	str_path = std::string(path);
	str_file = std::string(file);
	init();
}

void ParaQuadCutting::init()
{
	Mesh_doubleIO::copy_mesh(origin_mesh, new_mesh);

	if (!get_para_mesh()) return;
	valid_ = true;
	
	new_para.add_property(e_segment);
	for (auto e_h : new_para.edges())
	{
		new_para.property(e_segment, e_h) = -1;
	}
}

uint ParaQuadCutting::get_tag(const OpenMesh::Vec3d& vec)
{
	uint b = (abs(vec[0]) <= abs(vec[1]));
	uint a = (vec[b] <= 0.0);

	return (a << 1) + b;
}

double ParaQuadCutting::calc_signal_error_3D()
{
	constexpr int split_n = 8;
	OpenMesh::Vec3d mesh_BB_max, mesh_BB_min;
	mesh_BB_max = mesh_BB_min = origin_mesh.point(origin_mesh.vertex_handle(0));
	for (auto v_h : origin_mesh.vertices())
	{
		const auto& p = origin_mesh.point(v_h);
		mesh_BB_max = mesh_BB_max.maximize(p);
		mesh_BB_min = mesh_BB_min.minimize(p);
	}

	OpenMesh::Vec3d grid_length = (mesh_BB_max - mesh_BB_min) / 100.0;

	auto get_color = [&](const OpenMesh::Vec3d& pt)
	{
		auto pt1 = (pt - mesh_BB_min) / grid_length;
		bool block = (((std::lround(std::floor(pt1[0])) + std::lround(std::floor(pt1[1])) + std::lround(std::floor(pt1[2]))) & 1) == 0);

		return block ? OpenMesh::Vec3d(1.0, 1.0, 1.0) : OpenMesh::Vec3d(0.0, 0.0, 0.0);
	};

	auto get_interior_point = [](const std::vector<OpenMesh::Vec3d>& v_list, double u, double v)
	{
		return u * v_list[0] + v * v_list[1] + (1.0 - u - v) * v_list[2];
	};

	double delta_u = (BB_Max - BB_Min).max() / 4096.0;
	double delta_v = (BB_Max - BB_Min).max() / 4096.0;
	auto signal_error = [&](const std::vector<OpenMesh::Vec3d>& v_list, const std::vector<OpenMesh::Vec3d>& uv_list, int i0, int j0, int i1, int j1, int i2, int j2)
	{
		std::vector<OpenMesh::Vec3d> pt(3), pt_color(3);
		double divisor = 1.0 / split_n;

		pt[0] = get_interior_point(uv_list, i0 * divisor, j0 * divisor);
		pt[1] = get_interior_point(uv_list, i1 * divisor, j1 * divisor);
		pt[2] = get_interior_point(uv_list, i2 * divisor, j2 * divisor);

		pt_color[0] = get_color(get_interior_point(v_list, i0 * divisor, j0 * divisor));
		pt_color[1] = get_color(get_interior_point(v_list, i1 * divisor, j1 * divisor));
		pt_color[2] = get_color(get_interior_point(v_list, i2 * divisor, j2 * divisor));

		Eigen::Matrix3d para_mat;
		Eigen::Matrix3d signal_mat;

		for (int i = 0; i < 3; i++)
		{
			para_mat(0, i) = pt[i][0] / delta_u;
			para_mat(1, i) = pt[i][1] / delta_v;
			para_mat(2, i) = 1.0;

			signal_mat(0, i) = pt_color[i][0];
			signal_mat(1, i) = pt_color[i][1];
			signal_mat(2, i) = pt_color[i][2];
		}

		Eigen::Matrix3d J_mat = signal_mat * para_mat.inverse();
		return J_mat.col(0).squaredNorm() + J_mat.col(1).squaredNorm();
	};

	double error = 0.0;
	double total_area = 0.0;
	for (auto f_h : new_mesh.faces())
	{
		double s_area = new_mesh.calc_sector_area(*new_mesh.fh_begin(f_h));
		total_area += s_area;
		s_area /= split_n * split_n;

		std::map<int, int> v_mesh2uv;

		std::vector<OpenMesh::Vec3d> fv_p, fuv_p;
		fv_p.reserve(3);
		fuv_p.reserve(3);
		for (auto fh_h : new_mesh.fh_range(f_h))
		{
			fv_p.push_back(new_mesh.point(new_mesh.to_vertex_handle(fh_h)));
			fuv_p.push_back(new_para.point(new_para.to_vertex_handle(get_mesh2para(fh_h))));
		}

		double f_error = 0.0;
		for (int j = 0; j <= split_n; j++)
		{
			for (int k = 0; k <= split_n - j; k++)
			{
				if (j + k < split_n) f_error += s_area * signal_error(fv_p, fuv_p, j, k, j + 1, k, j, k + 1);

				if (j > 0 && k > 0) f_error += s_area * signal_error(fv_p, fuv_p, j, k, j - 1, k, j, k - 1);
			}
		}

		error += f_error;
	}

	error /= 12.0 * total_area;

	std::cout << "3D Checkerboard Signal Error " << error << std::endl;

	return error;
}

bool ParaQuadCutting::get_para_mesh()
{
	origin_para.clear();
	new_para.clear();

	OpenMesh::MPropHandleT<std::vector<Mesh::TexCoord2D>> mvt_list;
	OpenMesh::HPropHandleT<int> hvt_index;

	if (!origin_mesh.get_property_handle(mvt_list, "mvt_list") || !origin_mesh.get_property_handle(hvt_index, "hvt_index"))
	{
		std::cout << "Texture data is invalid." << std::endl;
		return false;
	}

	for (int i = 0; i < origin_mesh.property(mvt_list).size(); i++)
	{
		origin_para.add_vertex(Mesh::Point(origin_mesh.property(mvt_list)[i][0], origin_mesh.property(mvt_list)[i][1], 0.0));
		new_para.add_vertex(Mesh::Point(origin_mesh.property(mvt_list)[i][0], origin_mesh.property(mvt_list)[i][1], 0.0));
	}

	for (int i = 0; i < origin_mesh.n_faces(); i++)
	{
		auto f_h = origin_mesh.face_handle(i);
		std::vector<OpenMesh::VertexHandle> para_f;
		para_f.reserve(3);

		for (auto fh_h = origin_mesh.cfh_begin(f_h); fh_h != origin_mesh.cfh_end(f_h); fh_h++)
		{
			para_f.push_back(origin_para.vertex_handle(origin_mesh.property(hvt_index, *fh_h)));
		}

		origin_para.add_face(para_f);
		new_para.add_face(para_f);
	}

	origin_h_mesh2para.resize(origin_mesh.n_halfedges(), -1);
	new_mesh.add_property(h_mesh2para);
	new_para.add_property(h_para2mesh);
	for (int i = 0; i < origin_mesh.n_faces(); i++)
	{
		auto f0 = origin_mesh.face_handle(i);
		auto f1 = new_mesh.face_handle(i);
		auto f2 = new_para.face_handle(i);
		
		std::map<int, int> v_para2mesh;
		for (auto f0_h : origin_mesh.fh_range(f0))
		{
			v_para2mesh[origin_mesh.property(hvt_index, f0_h)] = origin_mesh.to_vertex_handle(f0_h).idx();
		}

		std::map<int, int> mesh_v2h;
		for (auto f1_h : new_mesh.fh_range(f1))
		{
			mesh_v2h[new_mesh.to_vertex_handle(f1_h).idx()] = f1_h.idx();
		}

		for (auto f2_h : new_para.fh_range(f2))
		{
			auto f1_h = new_mesh.halfedge_handle(mesh_v2h[v_para2mesh[new_para.to_vertex_handle(f2_h).idx()]]);

			new_mesh.property(h_mesh2para, f1_h) = std::make_pair(new_para.from_vertex_handle(f2_h).idx(), new_para.to_vertex_handle(f2_h).idx());
			new_para.property(h_para2mesh, f2_h) = std::make_pair(new_mesh.from_vertex_handle(f1_h).idx(), new_mesh.to_vertex_handle(f1_h).idx());

			origin_h_mesh2para[f1_h.idx()] = f2_h.idx();
		}
	}

	cut_length = 0.0;
	OpenMesh::EPropHandleT<bool> e_oncut;
	origin_mesh.get_property_handle(e_oncut, "e_oncut");
	for (auto e_h : origin_mesh.edges())
	{
		if (origin_mesh.is_boundary(e_h))
		{
			cut_length += origin_mesh.calc_edge_length(e_h);
			continue;
		}
		
		int to0 = origin_mesh.property(hvt_index, origin_mesh.halfedge_handle(e_h, 0));
		int to1 = origin_mesh.property(hvt_index, origin_mesh.halfedge_handle(e_h, 1));
		int from0 = origin_mesh.property(hvt_index, origin_mesh.prev_halfedge_handle(origin_mesh.halfedge_handle(e_h, 0)));
		int from1 = origin_mesh.property(hvt_index, origin_mesh.prev_halfedge_handle(origin_mesh.halfedge_handle(e_h, 1)));

		bool is_cut = ((to0 != from1) || (to1 != from0));
		origin_mesh.property(e_oncut, e_h) = is_cut;
		if (is_cut) cut_length += origin_mesh.calc_edge_length(e_h) * 2.0;
	}

	origin_mesh.request_vertex_normals();

	new_para.request_edge_status();
	new_para.request_face_status();

	new_mesh.request_edge_status();
	new_mesh.request_face_status();

	origin_para.request_face_normals();
	origin_para.update_face_normals();

	origin_para.request_edge_colors();
	for (auto e_h : origin_para.edges())
	{
		origin_para.set_color(e_h, origin_para.is_boundary(e_h) ? boundary_edge_color : interior_edge_color);
	}
//	std::cout << "Euler of Para : " << origin_para.n_vertices() + origin_para.n_faces() - origin_para.n_edges() << std::endl;

	BB_Max = BB_Min = origin_para.point(origin_para.vertex_handle(0));
	for (auto v_h : origin_para.vertices())
	{
		const auto& p = origin_para.point(v_h);
		BB_Max = BB_Max.maximize(p);
		BB_Min = BB_Min.minimize(p);
	}
//	std::cout << "BB of Para : X [" << BB_Min[0] << ", " << BB_Max[0] << "] Y [" << BB_Min[1] << ", " << BB_Max[1] << "]" << std::endl;
	return true;
}

void ParaQuadCutting::calc_distortion(bool silence)
{
	int nf = origin_mesh.n_faces();

	int n_pos = 0;
	int n_neg = 0;
	double total_area = 0.0;
	double total_uv_area = 0.0;

	OpenMesh::Vec3d mesh_BB_max, mesh_BB_min;
	std::vector<double> face_area(nf);
	mesh_BB_max = mesh_BB_min = origin_mesh.point(origin_mesh.vertex_handle(0));
	for (auto f_h : origin_mesh.faces())
	{
		OpenMesh::Vec3d mesh_p[3];
		OpenMesh::Vec3d para_p[3];

		auto fh_iter = origin_mesh.cfh_begin(f_h);
		for (int i = 0; i < 3; i++, fh_iter++)
		{
			mesh_p[i] = origin_mesh.point(origin_mesh.to_vertex_handle(*fh_iter));
			para_p[i] = origin_para.point(origin_para.to_vertex_handle(get_mesh2para(*fh_iter)));

			mesh_BB_max = mesh_BB_max.maximize(mesh_p[i]);
			mesh_BB_min = mesh_BB_min.minimize(mesh_p[i]);
		}

		mesh_p[1] -= mesh_p[0];
		mesh_p[2] -= mesh_p[0];
		para_p[1] -= para_p[0];
		para_p[2] -= para_p[0];

		face_area[f_h.idx()] = OpenMesh::cross(mesh_p[1], mesh_p[2]).norm() / 2.0;

		total_area += face_area[f_h.idx()];
		total_uv_area += OpenMesh::cross(para_p[1], para_p[2]).norm() / 2.0;
	}

	double factor = std::sqrt(total_uv_area / total_area);
	std::vector<double> distor(nf);
	for (auto f_h : origin_mesh.faces())
	{
		OpenMesh::Vec3d mesh_p[3];
		OpenMesh::Vec3d para_p[3];

		auto fh_iter = origin_mesh.cfh_begin(f_h);
		for (int i = 0; i < 3; i++, fh_iter++)
		{
			mesh_p[i] = origin_mesh.point(origin_mesh.to_vertex_handle(*fh_iter));
			para_p[i] = origin_para.point(origin_para.to_vertex_handle(get_mesh2para(*fh_iter)));
		}

		Eigen::Matrix2d mesh_M, para_M;

		mesh_p[1] -= mesh_p[0];
		mesh_p[2] -= mesh_p[0];
		para_p[1] -= para_p[0];
		para_p[2] -= para_p[0];

		mesh_p[1] *= factor;
		mesh_p[2] *= factor;

		OpenMesh::Vec3d e1 = mesh_p[1].normalized();
		OpenMesh::Vec3d e2 = OpenMesh::cross(origin_mesh.normal(f_h), e1);

		mesh_M(0, 0) = mesh_p[1].norm();
		mesh_M(1, 0) = 0.0;
		mesh_M(0, 1) = OpenMesh::dot(mesh_p[2], e1);
		mesh_M(1, 1) = OpenMesh::dot(mesh_p[2], e2);

		para_M(0, 0) = para_p[1][0];
		para_M(1, 0) = para_p[1][1];
		para_M(0, 1) = para_p[2][0];
		para_M(1, 1) = para_p[2][1];

		double det_p = para_M.determinant();

		(det_p > 0 ? n_pos : n_neg)++;

		if (det_p <= 0)
		{
			para_M.row(0) = -para_M.row(0);
			flipped_faces.insert(f_h.idx());
		}

		Eigen::Matrix2d J = para_M * mesh_M.inverse();
		Eigen::JacobiSVD<Eigen::Matrix2d> SVD_solver;

		SVD_solver.compute(J);
		Eigen::Vector2d singulars = SVD_solver.singularValues();

		double det_J = J.determinant();
		double s_max = singulars.maxCoeff();
		double s_min = singulars.minCoeff();

		distor[f_h.idx()] = (s_max * s_max + s_min * s_min + 1.0 / s_max / s_max + 1.0 / s_min / s_min) * 0.25;
	}

	double x_avg_w = 0.0;
	for (int i = 0; i < distor.size(); i++) x_avg_w += distor[i] * face_area[i];
	x_avg_w /= total_area;

	para_distortion = x_avg_w;

	if (!silence)
	{
		const double bb_area = (BB_Max[0] - BB_Min[0]) * (BB_Max[1] - BB_Min[1]);
		std::cout << "PE " << (bb_area > 0.0 ? total_uv_area / bb_area : 0.0) << std::endl;
		std::cout << "BL " << cut_length / (mesh_BB_max - mesh_BB_min).norm() << std::endl;
		std::cout << "ED " << x_avg_w << std::endl;
	}
}

void ParaQuadCutting::get_scaf_info(Eigen::MatrixXd& v_pos, Eigen::MatrixXd& uv_v_pos, Eigen::MatrixXi& fv_id, Eigen::MatrixXi& uv_fv_id)
{
	v_pos.resize(new_mesh.n_vertices(), 3);
	uv_v_pos.resize(new_para.n_vertices(), 2);
	fv_id.resize(new_mesh.n_faces(), 3);
	uv_fv_id.resize(new_para.n_faces(), 3);

	for (int i = 0; i < new_mesh.n_vertices(); i++)
	{
		const auto& pt = new_mesh.point(new_mesh.vertex_handle(i));
		v_pos(i, 0) = pt[0];
		v_pos(i, 1) = pt[1];
		v_pos(i, 2) = pt[2];
	}

	for (int i = 0; i < new_para.n_vertices(); i++)
	{
		const auto& pt = new_para.point(new_para.vertex_handle(i));
		uv_v_pos(i, 0) = pt[0];
		uv_v_pos(i, 1) = pt[1];
	}

	for (int i = 0; i < new_mesh.n_faces(); i++)
	{
		int j = 0;
		for (auto vf : new_mesh.fv_range(new_mesh.face_handle(i))) fv_id(i, j++) = vf.idx();
	}

	for (int i = 0; i < new_para.n_faces(); i++)
	{
		int j = 0;
		for (auto vf : new_para.fv_range(new_para.face_handle(i))) uv_fv_id(i, j++) = vf.idx();
	}
}

void ParaQuadCutting::load_from_scaf(const Eigen::MatrixXd& uv_v_pos)
{
	for (int i = 0; i < new_para.n_vertices(); i++)
	{
		auto& pt = new_para.point(new_para.vertex_handle(i));
		pt[0] = uv_v_pos(i, 0);
		pt[1] = uv_v_pos(i, 1);
	}
}

void ParaQuadCutting::flip_neg_charts()
{
	std::vector<bool> f_visited(origin_para.n_faces(), false);
	for (auto f_h : origin_para.faces())
	{
		if (f_visited[f_h.idx()] || origin_para.normal(f_h)[2] > 0) continue;

		std::set<int> comp_v;
		std::queue<int> queue_f;

		queue_f.push(f_h.idx());
		while (!queue_f.empty())
		{
			int cur_f = queue_f.front();
			queue_f.pop();

			if (f_visited[cur_f]) continue;
			auto cur_fh = origin_para.face_handle(cur_f);
			f_visited[cur_f] = true;

			for (auto fv : origin_para.fv_range(cur_fh))
			{
				comp_v.insert(fv.idx());
			}

			for (auto fh : origin_para.fh_range(cur_fh))
			{
				auto next_fh = origin_para.opposite_face_handle(fh);
				if (next_fh.is_valid() && !f_visited[next_fh.idx()])
				{
					queue_f.push(next_fh.idx());
				}
			}
		}

		double x_max = -std::numeric_limits<double>::max();
		double x_min = std::numeric_limits<double>::max();
		for (int vid : comp_v)
		{
			double v_x = origin_para.point(origin_para.vertex_handle(vid))[0];
			x_max = std::max(v_x, x_max);
			x_min = std::min(v_x, x_min);
		}

		for (int vid : comp_v)
		{
			double& v_x = new_para.point(new_para.vertex_handle(vid))[0];
			v_x = x_max + x_min - v_x;
		}
	}
}

void ParaQuadCutting::set_seleted_vertices(const std::vector<int>& mesh_v)
{
	(void)mesh_v;
}

void ParaQuadCutting::set_seleted_edges(const std::vector<int>& mesh_e)
{
	(void)mesh_e;
}

void ParaQuadCutting::set_seleted_faces(const std::vector<int>& mesh_f)
{
	(void)mesh_f;
}

void ParaQuadCutting::set_seleted(const std::vector<int>& mesh_v, const std::vector<int>& mesh_e, const std::vector<int>& mesh_f)
{
	set_seleted_vertices(mesh_v);
	set_seleted_edges(mesh_e);
	set_seleted_faces(mesh_f);
}

void ParaQuadCutting::get_textured_mesh(Mesh& tar)
{
	Mesh_doubleIO::copy_mesh(new_mesh, tar);

	OpenMesh::MPropHandleT<std::vector<Mesh::TexCoord2D>> mvt_list;
	OpenMesh::HPropHandleT<int> hvt_index;

	if (!tar.get_property_handle(mvt_list, "mvt_list") || !tar.get_property_handle(hvt_index, "hvt_index"))
	{
		std::cout << "Texture data is invalid." << std::endl;
		return;
	}

	tar.property(mvt_list).resize(new_para.n_vertices());
	for (auto v_h : new_para.vertices())
	{
		auto p = new_para.point(v_h);
		tar.property(mvt_list)[v_h.idx()][0] = p[0];
		tar.property(mvt_list)[v_h.idx()][1] = p[1];
	}

	for (auto f_h : new_mesh.faces())
	{
		std::map<int, int> v_mesh2para;
		for (auto fh_h : new_mesh.fh_range(f_h))
		{
			int v0 = new_mesh.to_vertex_handle(fh_h).idx();
			int v1 = new_mesh.property(h_mesh2para, fh_h).second;

			v_mesh2para[v0] = v1;
		}

		for (auto fh_h : tar.fh_range(tar.face_handle(f_h.idx())))
		{
			tar.property(hvt_index, fh_h) = v_mesh2para.at(tar.to_vertex_handle(fh_h).idx());
		}
	}
}

void ParaQuadCutting::trans_textured(Mesh& tar)
{
	Mesh_doubleIO::copy_mesh(new_mesh, tar);

	OpenMesh::MPropHandleT<std::vector<Mesh::TexCoord2D>> mvt_list0, mvt_list1;
	OpenMesh::HPropHandleT<int> hvt_index0, hvt_index1;

	tar.get_property_handle(mvt_list0, "mvt_list");
	tar.get_property_handle(hvt_index0, "hvt_index");
	new_mesh.get_property_handle(mvt_list1, "mvt_list");
	new_mesh.get_property_handle(hvt_index1, "hvt_index");

	tar.property(mvt_list0) = std::move(new_mesh.property(mvt_list1));
	for (auto h1 : new_mesh.halfedges())
	{
		auto h0 = tar.find_halfedge(new_mesh.from_vertex_handle(h1), new_mesh.to_vertex_handle(h1));
		tar.property(hvt_index0, h0) = new_mesh.property(hvt_index1, h1);
	}
}

void ParaQuadCutting::update_textured_mesh(Mesh& tar, bool use_new /*= false*/)
{
	OpenMesh::MPropHandleT<std::vector<Mesh::TexCoord2D>> mvt_list;
	OpenMesh::HPropHandleT<int> hvt_index;

	if (!tar.get_property_handle(mvt_list, "mvt_list") || !tar.get_property_handle(hvt_index, "hvt_index"))
	{
		std::cout << "Texture data is invalid." << std::endl;
		return;
	}

	const Mesh& src = use_new ? new_para : origin_para;

	if (tar.property(mvt_list).size() != src.n_vertices())
	{
		std::cout << "Para vertices do not match." << std::endl;
	}

	for (auto v_h : src.vertices())
	{
		auto p = src.point(v_h);
		tar.property(mvt_list)[v_h.idx()][0] = p[0];
		tar.property(mvt_list)[v_h.idx()][1] = p[1];
	}
}

void ParaQuadCutting::split_edges(const std::vector<int>& mesh_e)
{
	for (int i : mesh_e)
	{
		split_edge(get_mesh2para(new_mesh.halfedge_handle(2 * i)), 0.5);
	}

	new_mesh.garbage_collection();
	new_para.garbage_collection();
}

void ParaQuadCutting::update_para(double factor /*= 1.0*/)
{
	(void)factor;

	if (new_para.n_vertices() != origin_para.n_vertices())
	{
		std::cout << "Para vertices do not match." << std::endl;
	}

	for (auto v_h : new_para.vertices())
	{
		new_para.point(v_h) = origin_para.point(origin_para.vertex_handle(v_h.idx()));
	}
}

void ParaQuadCutting::cutting(double thres)
{
	QElapsedTimer timer;
	split_thres = thres;

	charts_decomposition = std::make_unique<ParaQuadChartDecomposition>(new_para);

	timer.start();
	charts_decomposition->decomposition();
	
	cut_segments();
	decomposition();
	std::cout << "Time: " << timer.elapsed() / 1000.0 << "s" << std::endl;

	get_packing_result();

// 	save_quad_charts();
// 	save_tri_charts();
}

OpenMesh::VertexHandle ParaQuadCutting::split_edge(OpenMesh::HalfedgeHandle h_para, double t)
{
//	MY_DOUBT(t < split_thres || t > 1.0 - split_thres, "Split Edge Error");

	if (new_para.status(h_para).deleted())
	{
		return OpenMesh::VertexHandle();
	}

	auto h0_mesh = get_para2mesh(h_para);

	bool is_bh_mesh[2];
	OpenMesh::VertexHandle v_new[2];
	OpenMesh::HalfedgeHandle h_split[2];
	OpenMesh::HalfedgeHandle h_split_mesh[2] = { h0_mesh, new_mesh.opposite_halfedge_handle(h0_mesh) };

	//boundary may change after a face is splited, so boundary info need to save
	is_bh_mesh[0] = new_mesh.is_boundary(h_split_mesh[0]);
	is_bh_mesh[1] = new_mesh.is_boundary(h_split_mesh[1]);

	auto insert_p_mesh = (1 - t) * new_mesh.point(new_mesh.to_vertex_handle(h_split_mesh[1])) + t * new_mesh.point(new_mesh.to_vertex_handle(h_split_mesh[0]));
	auto v_new_mesh = new_mesh.add_vertex(insert_p_mesh);
	
	for (int i = 0; i < 2; i++)
	{
		if (is_bh_mesh[i]) continue;

		h_split[i] = get_mesh2para(h_split_mesh[i]);

		int es_seg = new_para.property(e_segment, new_para.edge_handle(h_split[i]));

		OpenMesh::HalfedgeHandle h_face[2], h_new[2];
		OpenMesh::VertexHandle v_hf[2], v_ht[2];

		OpenMesh::HalfedgeHandle h_face_mesh[2], h_new_mesh[2];
		OpenMesh::VertexHandle v_hf_mesh[2], v_ht_mesh[2];

		if (i == 0)
		{
			auto v_hsf = new_para.from_vertex_handle(h_split[i]);
			auto v_hst = new_para.to_vertex_handle(h_split[i]);

			auto insert_p = (1.0 - t) * new_para.point(v_hsf) + t * new_para.point(v_hst);
			v_new[0] = new_para.add_vertex(insert_p);
		}
		else if (i == 1)
		{
			if (new_para.edge_handle(h_split[0]) != new_para.edge_handle(h_split[1]))
			{
				auto v_hsf = new_para.from_vertex_handle(h_split[i]);
				auto v_hst = new_para.to_vertex_handle(h_split[i]);

				auto insert_p = t * new_para.point(v_hsf) + (1.0 - t) * new_para.point(v_hst);
				v_new[1] = new_para.add_vertex(insert_p);
			}
			else
			{
				v_new[1] = v_new[0];
			}
		}

		h_face[0] = new_para.next_halfedge_handle(h_split[i]);
		h_face[1] = new_para.prev_halfedge_handle(h_split[i]);
		auto f_h = new_para.face_handle(h_split[i]);

		h_face_mesh[0] = new_mesh.next_halfedge_handle(h_split_mesh[i]);
		h_face_mesh[1] = new_mesh.prev_halfedge_handle(h_split_mesh[i]);
		auto f_h_mesh = new_mesh.face_handle(h_split_mesh[i]);

		for (int j = 0; j < 2; j++)
		{
			v_hf[j] = new_para.from_vertex_handle(h_face[j]);
			v_ht[j] = new_para.to_vertex_handle(h_face[j]);

			v_hf_mesh[j] = new_mesh.from_vertex_handle(h_face_mesh[j]);
			v_ht_mesh[j] = new_mesh.to_vertex_handle(h_face_mesh[j]);
		}

		new_para.delete_face(f_h, false);
		new_mesh.delete_face(f_h_mesh, false);

		new_para.add_face(v_hf[0], v_ht[0], v_new[i]);
		new_mesh.add_face(v_hf_mesh[0], v_ht_mesh[0], v_new_mesh);
		new_para.add_face(v_hf[1], v_ht[1], v_new[i]);
		new_mesh.add_face(v_hf_mesh[1], v_ht_mesh[1], v_new_mesh);

		for (int j = 0; j < 2; j++)
		{
			find_replace(h_face[j], v_hf[j], v_ht[j], h_face_mesh[j], v_hf_mesh[j], v_ht_mesh[j]);
		}

		new_para.property(e_segment, new_para.edge_handle(new_para.next_halfedge_handle(h_face[0]))) = -1;

		new_para.property(h_para2mesh, new_para.next_halfedge_handle(h_face[0])) = std::make_pair(v_ht_mesh[0].idx(), v_new_mesh.idx());
		new_para.property(h_para2mesh, new_para.prev_halfedge_handle(h_face[1])) = std::make_pair(v_new_mesh.idx(), v_hf_mesh[1].idx());

		new_mesh.property(h_mesh2para, new_mesh.next_halfedge_handle(h_face_mesh[0])) = std::make_pair(v_ht[0].idx(), v_new[i].idx());
		new_mesh.property(h_mesh2para, new_mesh.prev_halfedge_handle(h_face_mesh[1])) = std::make_pair(v_new[i].idx(), v_hf[1].idx());

		new_para.property(e_segment, new_para.edge_handle(new_para.prev_halfedge_handle(h_face[0]))) = es_seg;
		new_para.property(e_segment, new_para.edge_handle(new_para.next_halfedge_handle(h_face[1]))) = es_seg;

		new_para.property(h_para2mesh, new_para.prev_halfedge_handle(h_face[0])) = std::make_pair(v_new_mesh.idx(), v_hf_mesh[0].idx());
		new_para.property(h_para2mesh, new_para.next_halfedge_handle(h_face[1])) = std::make_pair(v_ht_mesh[1].idx(), v_new_mesh.idx());

		new_mesh.property(h_mesh2para, new_mesh.prev_halfedge_handle(h_face_mesh[0])) = std::make_pair(v_new[i].idx(), v_hf[0].idx());
		new_mesh.property(h_mesh2para, new_mesh.next_halfedge_handle(h_face_mesh[1])) = std::make_pair(v_ht[1].idx(), v_new[i].idx());
	}

	return v_new[0];
}

OpenMesh::VertexHandle ParaQuadCutting::split_face(OpenMesh::FaceHandle f_para, double u, double v)
{
	MY_DOUBT(u < split_thres || v < split_thres || u + v > 1.0 - split_thres, "Split Face Error");

	std::vector<OpenMesh::HalfedgeHandle> h_face, h_face_mesh;
	std::vector<OpenMesh::VertexHandle> v_ht, v_hf, v_ht_mesh, v_hf_mesh;
	double w = 1.0 - u - v;

	v_ht.reserve(3);
	v_hf.reserve(3);
	h_face.reserve(3);

	v_ht_mesh.reserve(3);
	v_hf_mesh.reserve(3);
	h_face_mesh.reserve(3);

	bool skip_f = new_para.status(f_para).deleted();
	for (auto fh_h : new_para.fh_range(f_para))
	{
		if (skip_f || new_para.status(fh_h).deleted())
		{
			skip_f = true;
			break;
		}
		h_face.push_back(fh_h);
		v_ht.push_back(new_para.to_vertex_handle(fh_h));
		v_hf.push_back(new_para.from_vertex_handle(fh_h));

		const auto& fh_h_pair = new_para.property(h_para2mesh, fh_h);
		auto vf_mesh = new_mesh.vertex_handle(fh_h_pair.first);
		auto vt_mesh = new_mesh.vertex_handle(fh_h_pair.second);

		h_face_mesh.push_back(new_mesh.find_halfedge(vf_mesh, vt_mesh));
		v_ht_mesh.push_back(vt_mesh);
		v_hf_mesh.push_back(vf_mesh);
	}

	if (skip_f)
	{
		return OpenMesh::VertexHandle();
	}

	auto insert_p = u * new_para.point(v_ht[0]) + v * new_para.point(v_ht[1]) + w * new_para.point(v_ht[2]);
	auto insert_p_mesh = u * new_mesh.point(v_ht_mesh[0]) + v * new_mesh.point(v_ht_mesh[1]) + w * new_mesh.point(v_ht_mesh[2]);

	auto v_new = new_para.add_vertex(insert_p);
	auto v_new_mesh = new_mesh.add_vertex(insert_p_mesh);

	new_para.delete_face(f_para, false);
	new_mesh.delete_face(new_mesh.face_handle(f_para.idx()), false);
	for (int i = 0; i < 3; i++)
	{
		new_para.add_face(v_hf[i], v_ht[i], v_new);
		new_mesh.add_face(v_hf_mesh[i], v_ht_mesh[i], v_new_mesh);

		find_replace(h_face[i], v_hf[i], v_ht[i], h_face_mesh[i], v_hf_mesh[i], v_ht_mesh[i]);
	}

	for (auto ve : new_para.ve_range(v_new))
	{
		new_para.property(e_segment, ve) = -1;
	}

	for (int i = 0; i < 3; i++)
	{
		auto h0_para = new_para.find_halfedge(v_new, v_ht[i]);
		auto h1_para = new_para.opposite_halfedge_handle(h0_para);

		new_para.property(h_para2mesh, h0_para) = std::make_pair(v_new_mesh.idx(), v_ht_mesh[i].idx());
		new_para.property(h_para2mesh, h1_para) = std::make_pair(v_ht_mesh[i].idx(), v_new_mesh.idx());

		auto h0_mesh = new_mesh.find_halfedge(v_new_mesh, v_ht_mesh[i]);
		auto h1_mesh = new_mesh.opposite_halfedge_handle(h0_mesh);

		new_mesh.property(h_mesh2para, h0_mesh) = std::make_pair(v_new.idx(), v_ht[i].idx());
		new_mesh.property(h_mesh2para, h1_mesh) = std::make_pair(v_ht[i].idx(), v_new.idx());
	}

	return v_new;
}

void ParaQuadCutting::find_replace(OpenMesh::HalfedgeHandle& h0_p, OpenMesh::VertexHandle v_hf_p, OpenMesh::VertexHandle v_ht_p, OpenMesh::HalfedgeHandle& h0_m, OpenMesh::VertexHandle v_hf_m, OpenMesh::VertexHandle v_ht_m)
{
	if (!new_para.status(h0_p).deleted()) return;

	auto new_h0_p = new_para.find_halfedge(v_hf_p, v_ht_p);
	auto new_h0_m = new_mesh.find_halfedge(v_hf_m, v_ht_m);

	new_para.property(e_segment, new_para.edge_handle(new_h0_p)) = new_para.property(e_segment, new_para.edge_handle(h0_p));

	new_para.property(h_para2mesh, new_h0_p) = new_para.property(h_para2mesh, h0_p);
	new_mesh.property(h_mesh2para, new_h0_m) = new_mesh.property(h_mesh2para, h0_m);

	h0_p = new_h0_p;
	h0_m = new_h0_m;
}

double ParaQuadCutting::find_split_t(OpenMesh::HalfedgeHandle h, int val_u, int val_v)
{
	const auto& uv0 = new_para.point(new_para.from_vertex_handle(h));
	const auto& uv1 = new_para.point(new_para.to_vertex_handle(h));
	OpenMesh::Vec3d p(val_u, val_v, 0.0);

	auto v_e = uv1 - uv0;
	auto v_p = p - uv0;

	return OpenMesh::dot(v_e, v_p) / v_e.sqrnorm();
}

OpenMesh::HalfedgeHandle ParaQuadCutting::find_split_uv(OpenMesh::FaceHandle f, int val_u, int val_v, double& u, double& v)
{
	OpenMesh::Vec3d tar = { (double)val_u, (double)val_v, 0.0 };
	std::vector<OpenMesh::HalfedgeHandle> fh_h;
	std::vector<OpenMesh::Vec3d> v_ht;
	fh_h.reserve(3);
	v_ht.reserve(3);

	for (auto fh : new_para.fh_range(f))
	{
		fh_h.push_back(fh);
		v_ht.push_back(new_para.point(new_para.to_vertex_handle(fh)));
	}

	tar -= v_ht[2];
	v_ht[0] -= v_ht[2];
	v_ht[1] -= v_ht[2];

	u =  tar[0] * v_ht[1][1] - tar[1] * v_ht[1][0];
	v = -tar[0] * v_ht[0][1] + tar[1] * v_ht[0][0];

	double det_f = v_ht[0][0] * v_ht[1][1] - v_ht[0][1] * v_ht[1][0];
	u /= det_f;
	v /= det_f;

	if (u <= split_thres) return fh_h[2];
	else if (v <= split_thres) return fh_h[0];
	else if (u + v >= 1.0 - split_thres) return fh_h[1];
	else return OpenMesh::HalfedgeHandle();
}

void ParaQuadCutting::find_cut_path(OpenMesh::HalfedgeHandle& h_prev, int k, int dst_quad_v, int seg_id)
{
	const auto& quad_mesh = charts_decomposition->get_quad_mesh();
	MY_DOUBT(k < 1 || k > 4, "K Error");

	const int tag_step = 1;
	int tag_k = (get_tag(-new_para.calc_edge_vector(h_prev)) - k * tag_step) & 3;
	int gtag = (tag_k - tag_step) & 3;
	
	auto& uv0 = new_para.point(new_para.to_vertex_handle(h_prev));
	double tracing_val = std::round(uv0[gtag & 1]);
	double dst_val = std::round(quad_mesh.point(quad_mesh.vertex_handle(dst_quad_v))[tag_k & 1]);

	struct path_node
	{
		path_node(OpenMesh::HalfedgeHandle _h, double _t, double _u, double _v)
			:h(_h), t(_t), tar_p(_u, _v, 0.0) {};

		double t;
		OpenMesh::HalfedgeHandle h;
		OpenMesh::Vec3d tar_p;
	};
	bool spilt_end = false;
	std::vector<path_node> pass_by;
	OpenMesh::VertexHandle dst_v;

	std::set<int> dst_tris; 
	if (v_quad2para[dst_quad_v] != -1)
	{
		for (auto f_h : new_para.vf_range(new_para.vertex_handle(v_quad2para[dst_quad_v])))
		{
			dst_tris.insert(f_h.idx());
		}

		for (auto f_h : new_para.vf_range(new_para.to_vertex_handle(h_prev)))
		{
			if (dst_tris.count(f_h.idx()) > 0)
			{
				dst_v = new_para.vertex_handle(v_quad2para[dst_quad_v]);
				break;
			}
		}
	}

	auto h_iter = h_prev;

	while (true)
	{
		if (dst_v.is_valid()) break;

		if (k == 4 && h_iter == h_prev)
		{
			h_iter = new_para.opposite_halfedge_handle(new_para.next_halfedge_handle(h_iter));
			continue;
		}

		auto v1 = new_para.point(new_para.from_vertex_handle(h_iter)) - uv0;
		auto v2 = new_para.point(new_para.to_vertex_handle(new_para.next_halfedge_handle(h_iter))) - uv0;

		if ((!(gtag & 2) && (v1[gtag & 1] <= 0) && (v2[gtag & 1] >= 0)) || ((gtag & 2) && (v1[gtag & 1] >= 0) && (v2[gtag & 1] <= 0)))
		{
			//tracing
			auto tracing_h = new_para.prev_halfedge_handle(h_iter);

			MY_DOUBT((new_para.point(new_para.from_vertex_handle(tracing_h))[gtag & 1] - tracing_val) * (new_para.point(new_para.to_vertex_handle(tracing_h))[gtag & 1] - tracing_val) > 0,
				"Tracing Error 1, v " << new_para.to_vertex_handle(h_prev).idx() << ", k " << k);

			while (true)
			{
				tracing_h = new_para.opposite_halfedge_handle(tracing_h);
				auto tracing_f = new_para.face_handle(tracing_h);

				const auto& p1 = new_para.point(new_para.from_vertex_handle(tracing_h));
				const auto& p2 = new_para.point(new_para.to_vertex_handle(tracing_h));
				double c1 = p1[gtag & 1];
				double c2 = p2[gtag & 1];

				double split_t = (tracing_val - c1) / (c2 - c1);
				OpenMesh::Vec3d split_p;
				split_p[tag_k & 1] = p1[tag_k & 1] + split_t * (p2[tag_k & 1] - p1[tag_k & 1]);
				split_p[gtag & 1] = tracing_val;
				split_p[2] = 0.0;

				if (v_quad2para[dst_quad_v] == -1 && (((tag_k & 2) && split_p[tag_k & 1] <= dst_val) || (!(tag_k & 2) && split_p[tag_k & 1] >= dst_val)))
				{
					auto split_f = new_para.opposite_face_handle(tracing_h);
					OpenMesh::Vec3d dst_i;
					dst_i[tag_k & 1] = dst_val;
					dst_i[gtag & 1] = tracing_val;
					dst_i[2] = 0.0;

					double split_u, split_v;
					auto t_h = find_split_uv(split_f, dst_i[0], dst_i[1], split_u, split_v);

					if (!t_h.is_valid())
					{
						dst_v = split_face(split_f, split_u, split_v);
						v_quad2para[dst_quad_v] = dst_v.idx();
						new_para.point(dst_v) = dst_i;
					}
					else 
					{
						if (pass_by.empty() || t_h != pass_by.back().h)
						{
							double t_t = find_split_t(t_h, dst_i[0], dst_i[1]);
							pass_by.emplace_back(t_h, t_t, dst_i[0], dst_i[1]);
						}
						spilt_end = true;
					}

					break;
				}

				pass_by.emplace_back(tracing_h, split_t, split_p[0], split_p[1]);
				if (dst_tris.count(tracing_f.idx()) > 0)
				{
					dst_v = new_para.vertex_handle(v_quad2para[dst_quad_v]);
					break;
				}

				double c0 = new_para.point(new_para.to_vertex_handle(new_para.next_halfedge_handle(tracing_h)))[gtag & 1];

				MY_DOUBT((c1 - tracing_val) * (c2 - tracing_val) > 0, "Tracing Error 2, v " << new_para.to_vertex_handle(h_prev).idx() << ", k " << k);

				auto t1t0 = new_para.prev_halfedge_handle(tracing_h);
				auto t2t0 = new_para.next_halfedge_handle(tracing_h);
				if (c0 != tracing_val && c1 != tracing_val && c2 != tracing_val)
				{
					tracing_h = ((c0 < tracing_val) == (c1 < tracing_val)) ? t2t0 : t1t0;
				}
				else if (c0 != tracing_val && c1 == tracing_val && c2 != tracing_val)
				{
					tracing_h = ((c0 < tracing_val) == (c2 < tracing_val)) ? t1t0 : t2t0;
				}
				else if (c0 != tracing_val && c1 != tracing_val && c2 == tracing_val)
				{
					tracing_h = ((c0 < tracing_val) == (c1 < tracing_val)) ? t2t0 : t1t0;
				}
				else if (c0 == tracing_val && c1 == tracing_val && c2 != tracing_val)
				{
					tracing_h = t2t0;
				}
				else if (c0 == tracing_val && c1 != tracing_val && c2 == tracing_val)
				{
					tracing_h = t1t0;
				}
				else //FTT or TFF
				{
					double d_t1 = std::abs(new_para.point(new_para.from_vertex_handle(tracing_h))[(gtag & 1) ^ 1] - uv0[(gtag & 1) ^ 1]);
					double d_t2 = std::abs(new_para.point(new_para.to_vertex_handle(tracing_h))[(gtag & 1) ^ 1] - uv0[(gtag & 1) ^ 1]);

					tracing_h = (d_t1 <= d_t2) ? t2t0 : t1t0;
				}
			}

			break;
		}
 	
		h_iter = new_para.opposite_halfedge_handle(new_para.next_halfedge_handle(h_iter));
	}

	std::vector<OpenMesh::VertexHandle> path_v = { new_para.to_vertex_handle(h_prev) };

	std::set<int> pass_by_v;
	std::vector<int> split_status(pass_by.size());
	for (int i = 0; i < pass_by.size(); i++)
	{
		const auto& split_info = pass_by[i];
		split_status[i] = -1;

		if (split_info.t <= split_thres)
		{
			auto already_v = new_para.from_vertex_handle(split_info.h);
			if (!new_para.is_boundary(already_v))
			{
				split_status[i] = already_v.idx();
				pass_by_v.insert(split_status[i]);
			}
		}
		else if (split_info.t >= 1.0 - split_thres)
		{
			auto already_v = new_para.to_vertex_handle(split_info.h);
			if (!new_para.is_boundary(already_v))
			{
				split_status[i] = already_v.idx();
				pass_by_v.insert(split_status[i]);
			}
		}
	}

	for (int i = 0; i < pass_by.size(); i++)
	{
		if (split_status[i] == -1)
		{
			int v_from = new_para.from_vertex_handle(pass_by[i].h).idx();
			int v_to = new_para.to_vertex_handle(pass_by[i].h).idx();

			if (!(i == pass_by.size() - 1 && spilt_end) && (pass_by_v.count(v_from) == 1 || pass_by_v.count(v_to) == 1))
			{
				split_status[i] = -2;
			}
		}
	}

	for (int kkk = 0; kkk < 0; kkk++)
	{
		for (int i = 1; i < pass_by.size(); i++)
		{
			if (split_status[i] < 0) continue;
			auto v1 = new_para.vertex_handle(split_status[i]);

			int iter_i;

			iter_i = i - 1;
			while (iter_i >= 0 && (split_status[iter_i] == -2 || split_status[iter_i] == v1.idx())) iter_i--;
			if (iter_i < 0 || split_status[iter_i] < 0) continue;
			auto v0 = new_para.vertex_handle(split_status[iter_i]);

			iter_i = i + 1;
			while (iter_i < pass_by.size() && (split_status[iter_i] == -2 || split_status[iter_i] == v1.idx())) iter_i++;
			if (iter_i < pass_by.size() && split_status[iter_i] < 0) continue;
			if (iter_i == pass_by.size() && !dst_v.is_valid()) continue;
			auto v2 = (iter_i < pass_by.size()) ? new_para.vertex_handle(split_status[iter_i]) : dst_v;

			std::set<int> f_v012;
			auto h0 = new_para.find_halfedge(v0, v1);
			int aaa = new_para.face_handle(h0).idx();
			f_v012.insert(new_para.face_handle(h0).idx());
			aaa = new_para.opposite_face_handle(h0).idx();
			f_v012.insert(new_para.opposite_face_handle(h0).idx());

			bool flat_triangle = false;
			auto h1 = new_para.find_halfedge(v1, v2);
			if (f_v012.count(new_para.face_handle(h1).idx()) == 1) flat_triangle = true;
			if (f_v012.count(new_para.opposite_face_handle(h1).idx()) == 1) flat_triangle = true;

			if (!flat_triangle) continue;

			for (int j = i; j < iter_i; j++)
			{
				split_status[j] = -2;
				pass_by[j].h = new_para.find_halfedge(v0, v2);
				pass_by[j].t = 0.0;
			}
		}
	}

	for (int i = 0; i < pass_by.size(); i++)
	{
		const auto& split_info = pass_by[i];

		OpenMesh::VertexHandle new_v;
		if (split_status[i] >= 0)
		{
			new_v = new_para.vertex_handle(split_status[i]);
		}
		else if (split_status[i] == -1)
		{
			new_v = split_edge(split_info.h, split_info.t);
		}

		if (new_v.is_valid() && new_v != path_v.back())
		{
			old_point[new_v.idx()] = new_para.point(new_v);
			for (int k : {0, 1})
			{
				double val = split_info.tar_p[k];
				if (val - std::round(val) > 1e-4) continue;

				new_para.point(new_v)[k] = val;
			}
			
			path_v.push_back(new_v);
		}
	}

	if (spilt_end)
	{
		new_para.point(path_v.back()) = quad_mesh.point(quad_mesh.vertex_handle(dst_quad_v));
		v_quad2para[dst_quad_v] = path_v.back().idx();
	}

	if (dst_v.is_valid() && dst_v != path_v.back())
	{
		path_v.push_back(dst_v);
	}

	OpenMesh::HalfedgeHandle path_h;
	for (int i = 0; i < path_v.size() - 1; i++)
	{
		path_h = new_para.find_halfedge(path_v[i], path_v[i + 1]);
		new_para.property(e_segment, new_para.edge_handle(path_h)) = seg_id;
	}

	h_prev = path_h;
}

void ParaQuadCutting::save_quad_charts()
{
	if (!str_path.empty())
	{
		Mesh_doubleIO::save_mesh(charts_decomposition->get_quad_mesh(), (str_path + "/quad_comp/quad_comp.obj").c_str(), true);
		Mesh_doubleIO::save_mesh(charts_decomposition->get_packing_mesh(), (str_path + "/quad_comp/packing.obj").c_str());

		for (int i = 0; i < charts_decomposition->get_atlas().size(); i++)
		{
			std::stringstream comp_filename;
			comp_filename << str_path << "/quad_comp/quad_comp" << i << ".obj";
			Mesh_doubleIO::save_mesh(charts_decomposition->get_atlas()[i], comp_filename.str().c_str());
		}
	}
}

void ParaQuadCutting::save_tri_charts()
{
// 	Mesh_doubleIO::save_mesh(new_mesh, (str_path + "/tri_comp/mesh.obj").c_str());
// 	Mesh_doubleIO::save_mesh(new_para, (str_path + "/tri_comp/para.obj").c_str());

	if (!str_path.empty())
	{
		Mesh_doubleIO::save_mesh(new_para, (str_path + "/tri_comp/tri_comp.obj").c_str(), true);
		Mesh_doubleIO::save_mesh(new_mesh, (str_path + "/mesh_result.obj").c_str(), true);
		Mesh_doubleIO::save_mesh(mesh_unpack, (str_path + "/tri_comp/unpack.obj").c_str(), true);
//		Mesh_doubleIO::save_uv_mesh(new_mesh, (str_path + "/para_result.obj").c_str());

// 		for (int i = 0; i < para_atlas.size(); i++)
// 		{
// 			std::stringstream comp_filename;
// 			comp_filename << str_path << "/tri_comp/tri_comp" << i << ".obj";
// 			Mesh_doubleIO::save_mesh(para_atlas[i], comp_filename.str().c_str());
// 		}
	}

// 	std::ofstream result_log((str_path + "/result_log.txt").c_str());
// 	result_log << "#Charts : " << para_atlas.size() << std::endl;
// 	result_log << "#Charts : " << para_atlas.size() << std::endl;
// 	result_log.close();
}

void ParaQuadCutting::save_para(const char* filename)
{
	if (!str_path.empty())
	{
		Mesh_doubleIO::save_mesh(new_para, filename);

		std::ofstream para_log(str_path + "/para_log.txt");
		para_log << "Euler of Para : " << origin_para.n_vertices() + origin_para.n_faces() - origin_para.n_edges() << std::endl;
		para_log.close();
	}
}

void ParaQuadCutting::cut_on_boundary()
{
	const auto& quad_mesh = charts_decomposition->get_quad_mesh();

	const auto& boundary_h = charts_decomposition->get_boundary_h();
	const auto& bh_seg = charts_decomposition->get_bh_segment();
	for (int i = 0; i < boundary_h.size(); i++)
	{
		auto e_h = new_para.edge_handle(boundary_h[i] / 2);
		new_para.property(e_segment, e_h) = bh_seg[i];
	}

	v_quad2para.assign(quad_mesh.n_vertices(), -1); 
	const auto& vert_corner = charts_decomposition->get_vert_corner();
	for (int i = 0; i < vert_corner.size(); i++)
	{
		v_quad2para[i] = vert_corner[i];
	}

	const auto& boundary_cutting_nodes = charts_decomposition->get_boundary_cutting_nodes();
	for (const auto& bc_cutting : boundary_cutting_nodes)
	{
		if (v_quad2para[bc_cutting.first] != -1) continue;

		int seg_id = bc_cutting.second;
		int vert0 = charts_decomposition->get_segments()[seg_id].vert0;
		int vert1 = charts_decomposition->get_segments()[seg_id].vert1;
		int tag = charts_decomposition->get_segments()[seg_id].tag;

		double x_tar = std::round(quad_mesh.point(quad_mesh.vertex_handle(bc_cutting.first))[tag & 1]);

		OpenMesh::HalfedgeHandle h_iter;
		for (auto voh : new_para.voh_range(new_para.vertex_handle(vert0)))
		{
			if (new_para.property(e_segment, new_para.edge_handle(voh)) == seg_id)
			{
				h_iter = voh;
				break;
			}
		}

		MY_DOUBT(!new_para.is_boundary(h_iter), "Boundary Error");

		double split_t;
		while (true)
		{
			double x0 = new_para.point(new_para.from_vertex_handle(h_iter))[tag & 1];
			double x1 = new_para.point(new_para.to_vertex_handle(h_iter))[tag & 1];

			if ((!(tag & 2) && x0 <= x_tar && x1 > x_tar) || ((tag & 2) && x1 <= x_tar && x0 > x_tar))
			{
				split_t = (x_tar - x0) / (x1 - x0);
				break;
			}

			if (new_para.to_vertex_handle(h_iter).idx() == vert1)
			{
				std::cout << "Cant find h in seg." << std::endl;
				break;
			}
			h_iter = new_para.next_halfedge_handle(h_iter);
		}

		OpenMesh::VertexHandle v_new;
		if (split_t <= split_thres)
		{
			v_new = new_para.from_vertex_handle(h_iter);
		}
		else if (split_t >= 1.0 - split_thres)
		{
			v_new = new_para.to_vertex_handle(h_iter);
		}
		else
		{
			auto h_inside = new_para.opposite_halfedge_handle(h_iter);
			auto vf = new_para.from_vertex_handle(h_inside);
			auto vt = new_para.to_vertex_handle(h_inside);

			v_new = split_edge(h_inside, 1.0 - split_t);

			auto h0 = new_para.find_halfedge(vf, v_new);
			auto h0_mesh = get_para2mesh(h0);
			auto h1_mesh = new_mesh.opposite_halfedge_handle(h0_mesh);
			if (!new_mesh.is_boundary(h1_mesh))
			{
				auto h1 = get_mesh2para(h1_mesh);
				auto v_new2 = new_para.from_vertex_handle(h1);

				int seg_id = new_para.property(e_segment, new_para.edge_handle(h1));
				const auto& seg2 = charts_decomposition->get_segments()[seg_id];
				const auto& seg2_nodes = charts_decomposition->get_seg_nodes()[seg_id];

				const auto& quad_vertices = charts_decomposition->get_quad_vertices();

				MY_DOUBT(std::lround(new_para.point(v_new2)[(seg2.tag & 1) ^ 1]) != seg2.coord, "Seg2 Error1");
			}
		}

		v_quad2para[bc_cutting.first] = v_new.idx();
		new_para.point(v_new)[tag & 1] = x_tar;
		new_para.point(v_new)[(tag & 1) ^ 1] = charts_decomposition->get_segments()[seg_id].coord;
		new_para.point(v_new)[2] = 0.0;
	}
}

void ParaQuadCutting::cut_segments()
{
	cut_on_boundary();
	
	const auto& quad_mesh = charts_decomposition->get_quad_mesh();
	const auto& cut_path_h = charts_decomposition->get_cut_path_h();
	const auto& cut_path_v = charts_decomposition->get_cut_path_v();
	const auto& cut_path_c = charts_decomposition->get_cut_path_seg();
	int n_cut_path = cut_path_v.size();

	for (int i = 0; i < n_cut_path; i++)
	{
		int h_quad_prev = cut_path_h[i][0];
		int v_quad = cut_path_v[i][0].first;
	
		MY_DOUBT(v_quad2para[v_quad] == -1, "Cut Path Error");

		OpenMesh::HalfedgeHandle h_prev;
		OpenMesh::VertexHandle v0 = new_para.vertex_handle(v_quad2para[v_quad]);
		for (auto vih : new_para.vih_range(v0))
		{
			int vih_seg = new_para.property(e_segment, new_para.edge_handle(vih));
			auto h_quad_seg = charts_decomposition->get_cutting_edge_seg().find(h_quad_prev / 2);

			if (h_quad_seg != charts_decomposition->get_cutting_edge_seg().end() && vih_seg == h_quad_seg->second 
				&& get_tag(new_para.calc_edge_vector(vih)) == get_tag(quad_mesh.calc_edge_vector(quad_mesh.halfedge_handle(h_quad_prev))))
			{
				h_prev = vih;
				break;
			}
		}
		
		for (int j = 0; j < cut_path_v[i].size() - 1; j++)
		{
			find_cut_path(h_prev, cut_path_v[i][j].second, cut_path_v[i][j + 1].first, cut_path_c[i][j]);
		}
	}

	new_mesh.garbage_collection();
	new_para.garbage_collection();
}

void ParaQuadCutting::decomposition()
{
	para_atlas.clear();

	face_layer.assign(new_para.n_faces(), -1);

	std::vector<int> seed_triangles = get_seed_triangles();

	for (int i = 0; i < seed_triangles.size(); i++)
	{
		auto f_h = new_para.face_handle(seed_triangles[i]);
		int seed_test = face_layer[f_h.idx()];
		bool aaaa = face_layer[f_h.idx()] >= 0;
		MY_DOUBT(face_layer[f_h.idx()] >= 0, "SEED TRIANGLE ERROR");

		para_atlas.emplace_back();
		Mesh& comp = para_atlas.back();
		std::vector<int> vertex_index(new_para.n_vertices(), -1);

		std::queue<int> face_queue;
		face_queue.push(f_h.idx());

		while (!face_queue.empty())
		{
			int fid_cur = face_queue.front();
			face_queue.pop();
			if (face_layer[fid_cur] >= 0) continue;

			face_layer[fid_cur] = para_atlas.size() - 1;
			auto f_cur = new_para.face_handle(fid_cur);

			std::vector<OpenMesh::VertexHandle> v_fcur;
			for (auto fv : new_para.fv_range(f_cur))
			{
				if (vertex_index[fv.idx()] == -1)
				{
					vertex_index[fv.idx()] = comp.n_vertices();
					comp.add_vertex(new_para.point(fv));
				}
				v_fcur.emplace_back(vertex_index[fv.idx()]);
			}
			comp.add_face(v_fcur);

			for (auto fh : new_para.fh_range(f_cur))
			{
				if (new_para.property(e_segment, new_para.edge_handle(fh)) == -1 && face_layer[new_para.opposite_face_handle(fh).idx()] == -1)
				{
					face_queue.push(new_para.opposite_face_handle(fh).idx());
				}
			}
		}

		v_para2layer.emplace_back(std::move(vertex_index));
	}
 
 	std::cout << "Components " << para_atlas.size() << std::endl;
}

void ParaQuadCutting::get_packing_result()
{
	std::vector<int> nv_offset(para_atlas.size());

	nv_offset[0] = 0;
	for (int i = 0; i < para_atlas.size() - 1; i++)
	{
		nv_offset[i + 1] = nv_offset[i] + para_atlas[i].n_vertices();
	}

	OpenMesh::MPropHandleT<std::vector<Mesh::TexCoord2D>> mvt_list;
	OpenMesh::HPropHandleT<int> hvt_index;

	new_mesh.add_property(mvt_list, "mvt_list");
	new_mesh.add_property(hvt_index, "hvt_index");

	new_mesh.property(mvt_list).resize(nv_offset.back() + para_atlas.back().n_vertices());

	const auto& chart_translation = charts_decomposition->get_chart_translation();
	const auto& chart_flipped = charts_decomposition->get_chart_flipped();
	for (int i = 0; i < para_atlas.size(); i++)
	{
		double offset_u = chart_translation[i][0];
		double offset_v = chart_translation[i][1];
		for (int j = 0; j < para_atlas[i].n_vertices(); j++)
		{
			const auto& point_v = para_atlas[i].point(para_atlas[i].vertex_handle(j));
			if (chart_flipped[i])
			{
				new_mesh.property(mvt_list)[nv_offset[i] + j][0] = -point_v[1] + offset_u;
				new_mesh.property(mvt_list)[nv_offset[i] + j][1] = point_v[0] + offset_v;
			}
			else
			{
				new_mesh.property(mvt_list)[nv_offset[i] + j][0] = point_v[0] + offset_u;
				new_mesh.property(mvt_list)[nv_offset[i] + j][1] = point_v[1] + offset_v;
			}
		}
	}

	for (int i = 0; i < new_mesh.n_faces(); i++)
	{
		auto f_h = new_mesh.face_handle(i);
		int layer_id = face_layer[i];

		for (auto fh_h = new_mesh.cfh_begin(f_h); fh_h != new_mesh.cfh_end(f_h); fh_h++)
		{
			auto h_para = get_mesh2para(*fh_h);
			if (!h_para.is_valid())
			{
				std::cout << new_mesh.from_vertex_handle(*fh_h).idx() << " " << new_mesh.to_vertex_handle(*fh_h).idx() << " " << i << std::endl;
				continue;
			}

			new_mesh.property(hvt_index, *fh_h) = v_para2layer[layer_id][new_para.to_vertex_handle(h_para).idx()] + nv_offset[layer_id];
		}
	}

	OpenMesh::MPropHandleT<std::vector<Mesh::TexCoord2D>> mvt_list_para;
	OpenMesh::HPropHandleT<int> hvt_index_para;

	new_para.add_property(mvt_list_para, "mvt_list");
	new_para.add_property(hvt_index_para, "hvt_index");

	new_para.property(mvt_list_para) = new_mesh.property(mvt_list);
	for (int i = 0; i < new_para.n_faces(); i++)
	{
		auto f_h = new_para.face_handle(i);
		int layer_id = face_layer[i];

		for (auto fh_h : new_para.fh_range(f_h))
		{
			new_para.property(hvt_index_para, fh_h) = v_para2layer[layer_id][new_para.to_vertex_handle(fh_h).idx()] + nv_offset[layer_id];
		}
	}

	Mesh_doubleIO::copy_mesh(new_mesh, mesh_unpack);
	OpenMesh::MPropHandleT<std::vector<Mesh::TexCoord2D>> mvt_list_unpack;
	OpenMesh::HPropHandleT<int> hvt_index_unpack;

	mesh_unpack.add_property(mvt_list_unpack, "mvt_list");
	mesh_unpack.add_property(hvt_index_unpack, "hvt_index");

	mesh_unpack.property(mvt_list_unpack).resize(nv_offset.back() + para_atlas.back().n_vertices());

	for (int i = 0; i < para_atlas.size(); i++)
	{
		for (int j = 0; j < para_atlas[i].n_vertices(); j++)
		{
			const auto& point_v = para_atlas[i].point(para_atlas[i].vertex_handle(j));
			mesh_unpack.property(mvt_list_unpack)[nv_offset[i] + j][0] = point_v[0];
			mesh_unpack.property(mvt_list_unpack)[nv_offset[i] + j][1] = point_v[1];
		}
	}

	for (auto h_h : mesh_unpack.halfedges())
	{
		auto v0 = mesh_unpack.from_vertex_handle(h_h);
		auto v1 = mesh_unpack.to_vertex_handle(h_h);

		auto h_mesh = new_mesh.find_halfedge(v0, v1);
		mesh_unpack.property(hvt_index_unpack, h_h) = new_mesh.property(hvt_index, h_mesh);
	}
}

std::vector<int> ParaQuadCutting::get_seed_triangles()
{
	const auto& quad_mesh = charts_decomposition->get_quad_mesh();
	const auto& quad_atlas = charts_decomposition->get_atlas();
	const auto& v_chart2quad = charts_decomposition->get_v_chart2quad();

	std::vector<int> seed_triangles(quad_atlas.size(), -1);
	for (int i = 0; i < quad_atlas.size(); i++)
	{
		const auto& quad_chart = quad_atlas[i];

		OpenMesh::VertexHandle corner;
		for (auto v_h : quad_chart.vertices())
		{
			if (quad_chart.is_boundary(v_h) && quad_chart.valence(v_h) != 3)
			{
				corner = v_h;
				break;
			}
		}

		OpenMesh::HalfedgeHandle inner_out_h;
		for (auto vih : quad_chart.vih_range(corner))
		{
			if (quad_chart.is_boundary(vih))
			{
				inner_out_h = quad_chart.opposite_halfedge_handle(vih);
				break;
			}
		}

		int tag0 = get_tag(quad_chart.calc_edge_vector(inner_out_h));
		int tag1 = get_tag(-quad_chart.calc_edge_vector(quad_chart.prev_halfedge_handle(inner_out_h)));

		auto vf_quad = quad_mesh.vertex_handle(v_chart2quad[i][quad_chart.from_vertex_handle(inner_out_h).idx()]);
		auto vt_quad = quad_mesh.vertex_handle(v_chart2quad[i][quad_chart.to_vertex_handle(inner_out_h).idx()]);
		auto h_quad = quad_mesh.find_halfedge(vf_quad, vt_quad);
		int seg_id = charts_decomposition->get_cutting_edge_seg().at(h_quad.idx() / 2);

		OpenMesh::HalfedgeHandle h_tri;
		OpenMesh::VertexHandle v_tri = new_para.vertex_handle(v_quad2para[v_chart2quad[i][corner.idx()]]);
		MY_DOUBT(!v_tri.is_valid(), "Cut Path Error 2");

		for (auto voh : new_para.voh_range(v_tri))
		{
			if (get_tag(new_para.calc_edge_vector(voh)) == tag0 && new_para.property(e_segment, new_para.edge_handle(voh)) == seg_id)
			{
				h_tri = voh;
				break;
			}
		}

		OpenMesh::Vec3d h_vec;

		if (!new_para.is_boundary(h_tri))
		{
			h_vec = -new_para.calc_edge_vector(new_para.prev_halfedge_handle(h_tri));
			h_vec[tag0 & 1] = 0.0;
			if (get_tag(h_vec) == tag1) seed_triangles[i] = new_para.face_handle(h_tri).idx();
		}

		if (!new_para.is_boundary(new_para.opposite_halfedge_handle(h_tri)))
		{
			h_vec = new_para.calc_edge_vector(new_para.next_halfedge_handle(new_para.opposite_halfedge_handle(h_tri)));
			h_vec[tag0 & 1] = 0.0;
			if (get_tag(h_vec) == tag1) seed_triangles[i] = new_para.opposite_face_handle(h_tri).idx();
		}
	}

	return seed_triangles;
}
