#include "uvparamwidget_extended.h"

#ifdef MY_DEBUG
#define MY_DOUBT(cond, msg) if (cond) std::cout << msg << std::endl;
#else
#define MY_DOUBT(cond, msg)
#endif

OpenMesh::Vec3uc boundary_edge_color = { 229, 156,  59 };
OpenMesh::Vec3uc interior_edge_color = { 100, 100, 100 };

UVParamWidgetExtended::UVParamWidgetExtended(QWidget *parent)
    : UVParamWidget(parent)
{
    // 初始化任何额外的成员变量
}

void UVParamWidgetExtended::init()
{
	Mesh_doubleIO::copy_mesh(mesh, new_mesh);
	if (!get_para_mesh()) return;
    mesh.request_face_normals();
    mesh.update_face_normals();
	new_para.add_property(e_segment);
	for (auto e_h : new_para.edges())
	{
		new_para.property(e_segment, e_h) = -1;
	}
    calc_distortion();
}

bool UVParamWidgetExtended::get_para_mesh()
{
    origin_para.clear();
    new_para.clear();

    OpenMesh::MPropHandleT<std::vector<Mesh::TexCoord2D>> mvt_list;
    OpenMesh::HPropHandleT<int> hvt_index;

    if (!mesh.get_property_handle(mvt_list, "mvt_list") || !mesh.get_property_handle(hvt_index, "hvt_index"))
    {
        std::cout << "Texture data is invalid." << std::endl;
        return false;
    }

    for (int i = 0; i < mesh.property(mvt_list).size(); i++)
    {
        origin_para.add_vertex(Mesh::Point(mesh.property(mvt_list)[i][0], mesh.property(mvt_list)[i][1], 0.0));
        new_para.add_vertex(Mesh::Point(mesh.property(mvt_list)[i][0], mesh.property(mvt_list)[i][1], 0.0));
    }

    for (int i = 0; i < mesh.n_faces(); i++)
    {
        auto f_h = mesh.face_handle(i);
        std::vector<OpenMesh::VertexHandle> para_f;
        para_f.reserve(3);

        for (auto fh_h = mesh.cfh_begin(f_h); fh_h != mesh.cfh_end(f_h); fh_h++)
        {
            para_f.push_back(origin_para.vertex_handle(mesh.property(hvt_index, *fh_h)));
        }

        origin_para.add_face(para_f);
        new_para.add_face(para_f);
    }
    origin_h_mesh2para.resize(mesh.n_halfedges(), -1);
    new_mesh.add_property(h_mesh2para);
    new_para.add_property(h_para2mesh);
    for (int i = 0; i < mesh.n_faces(); i++)
    {
        auto f0 = mesh.face_handle(i);
        auto f1 = new_mesh.face_handle(i);
        auto f2 = new_para.face_handle(i);
        
        std::map<int, int> v_para2mesh;
        for (auto f0_h : mesh.fh_range(f0))
        {
            v_para2mesh[mesh.property(hvt_index, f0_h)] = mesh.to_vertex_handle(f0_h).idx();
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
    if (!mesh.get_property_handle(e_oncut, "e_oncut")) {
        mesh.add_property(e_oncut, "e_oncut");
        // 初始化所有边的属性值
        for (auto e_h : mesh.edges()) {
            mesh.property(e_oncut, e_h) = false;
        }
    }

    for (auto e_h : mesh.edges())
    {
        if (mesh.is_boundary(e_h))
        {
            cut_length += mesh.calc_edge_length(e_h);
            continue;
        }
        
        int to0 = mesh.property(hvt_index, mesh.halfedge_handle(e_h, 0));
        int to1 = mesh.property(hvt_index, mesh.halfedge_handle(e_h, 1));
        int from0 = mesh.property(hvt_index, mesh.prev_halfedge_handle(mesh.halfedge_handle(e_h, 0)));
        int from1 = mesh.property(hvt_index, mesh.prev_halfedge_handle(mesh.halfedge_handle(e_h, 1)));

        bool is_cut = ((to0 != from1) || (to1 != from0));
        mesh.property(e_oncut, e_h) = is_cut;
        if (is_cut) cut_length += mesh.calc_edge_length(e_h) * 2.0;
    }

    mesh.request_vertex_normals();

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

    BB_Max = BB_Min = origin_para.point(origin_para.vertex_handle(0));
    for (auto v_h : origin_para.vertices())
    {
        const auto& p = origin_para.point(v_h);
        BB_Max = BB_Max.maximize(p);
        BB_Min = BB_Min.minimize(p);
    }
    return true;
}

void UVParamWidgetExtended::calc_distortion()
{
	int nf = mesh.n_faces();

	int n_pos = 0;
	int n_neg = 0;
	double total_area = 0.0;
	double total_uv_area = 0.0;

	OpenMesh::Vec3d mesh_BB_max, mesh_BB_min;
	std::vector<double> face_area(nf);
	mesh_BB_max = mesh_BB_min = mesh.point(mesh.vertex_handle(0));
	for (auto f_h : mesh.faces())
	{
		OpenMesh::Vec3d mesh_p[3];
		OpenMesh::Vec3d para_p[3];

		auto fh_iter = mesh.cfh_begin(f_h);
		for (int i = 0; i < 3; i++, fh_iter++)
		{
			mesh_p[i] = mesh.point(mesh.to_vertex_handle(*fh_iter));
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
	for (auto f_h : mesh.faces())
	{
		OpenMesh::Vec3d mesh_p[3];
		OpenMesh::Vec3d para_p[3];

		auto fh_iter = mesh.cfh_begin(f_h);
		for (int i = 0; i < 3; i++, fh_iter++)
		{
			mesh_p[i] = mesh.point(mesh.to_vertex_handle(*fh_iter));
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
		OpenMesh::Vec3d e2 = OpenMesh::cross(mesh.normal(f_h), e1);

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

	// if (!silence)
	// {
	// 	std::cout << "PE " << total_uv_area / viewer->get_bb_area() << std::endl;
    std::cout << "BL " << cut_length / (mesh_BB_max - mesh_BB_min).norm() << std::endl;
    std::cout << "ED " << x_avg_w << std::endl;
	// }
}

void UVParamWidgetExtended::segment_detective()
{
	std::set<int> add_c;
//	std::set<int> add_c = sample_boundary_vertices();

	int n_inner_segs = 0;
	vert_corner.clear();
	quad_vertices.clear();
	accu_bh_chart.assign(1, 0);
	accu_bseg_chart.assign(1, 0);
	std::vector<bool> h_visited(mesh.n_halfedges(), false);
	for (auto h : mesh.halfedges())
	{
		if (!mesh.is_boundary(h) || h_visited[h.idx()]) continue;
		
		int euler = 0;
		double z_normal = 0.0;
		auto bh_iter = h;
		do
		{
			h_visited[bh_iter.idx()] = true;

			double angle_v = 0;
			int to_v = mesh.to_vertex_handle(bh_iter).idx();
			bh_iter = mesh.opposite_halfedge_handle(bh_iter);

			while (!mesh.is_boundary(bh_iter))
			{
				auto vec0 = mesh.calc_edge_vector(bh_iter);
				bh_iter = mesh.opposite_halfedge_handle(mesh.prev_halfedge_handle(bh_iter));
				auto vec1 = mesh.calc_edge_vector(bh_iter);

				z_normal += vec0[0] * vec1[1] - vec0[1] * vec1[0];
				angle_v += CommonFunctions::vec_angle_acos(vec0, vec1);
			}

			int vk = std::lround(angle_v / M_PI_2);
			if (vk != 2 || add_c.count(to_v) == 1)
			{
				vert_corner.push_back(to_v);
				n_inner_segs += vk - 1;

				v_in_quad[to_v] = quad_vertices.size();
				const auto& uv_bv = mesh.point(mesh.vertex_handle(to_v));
				quad_vertices.emplace_back(std::lround(uv_bv[0]), std::lround(uv_bv[1]));

//				std::cout << to_v << " " << angle_v / M_PI_2 << " " << vk << std::endl;
			}
			euler += 2 - vk;
			vert_k[to_v] = vk;
		} while (bh_iter != h);

		accu_bh_chart.push_back(vert_k.size());
		accu_bseg_chart.push_back(vert_corner.size());
		MY_DOUBT(euler != 4, "Euler Error");
		MY_DOUBT(z_normal <= 0, "Normal Error");
	}

	int n_charts = accu_bseg_chart.size() - 1;
	seg_chart.resize(vert_corner.size());
	boundary_v.reserve(vert_k.size());
	boundary_h.reserve(vert_k.size());
	for (int i = 0; i < n_charts; i++)
	{
		auto bv0 = mesh.vertex_handle(vert_corner[accu_bseg_chart[i]]);
		OpenMesh::HalfedgeHandle bh_iter;
		for (auto voh : mesh.voh_range(bv0))
		{
			bh_iter = voh;
			if (mesh.is_boundary(voh)) break;
		}

		for (int j = accu_bh_chart[i]; j < accu_bh_chart[i + 1]; j++)
		{
			auto bv_iter = mesh.from_vertex_handle(bh_iter);
			boundary_v_index[bv_iter.idx()] = boundary_v.size();
			boundary_h_index[bh_iter.idx()] = boundary_h.size();

			boundary_v.push_back(bv_iter.idx());
			boundary_h.push_back(bh_iter.idx());

			bh_iter = mesh.next_halfedge_handle(bh_iter);
		}

		for (int j = accu_bseg_chart[i]; j < accu_bseg_chart[i + 1]; j++)
		{
			seg_chart[j] = i;
		}
	}
	n_bsegs = vert_corner.size();

	MY_DOUBT(n_bsegs - n_inner_segs != 4 * n_charts, "Segs Num Error");

	segments.clear();
	segments.reserve(n_bsegs + n_inner_segs);
	segments.resize(n_bsegs);

	seg_nodes.clear();
	seg_nodes.reserve(n_bsegs + n_inner_segs);
	seg_nodes.resize(n_bsegs);

	triangle_segs.clear();
	triangle_segs.resize(mesh.n_faces());

	bh_segment.resize(boundary_h.size());

	for (int i = 0; i < vert_corner.size(); i++)
	{
		segments[i].vert0 = vert_corner[i];
		segments[i].vert1 = vert_corner[chart_segment_cycle(i, 1)];

		auto vec_seg = mesh.point(mesh.vertex_handle(segments[i].vert1)) - mesh.point(mesh.vertex_handle(segments[i].vert0));
		segments[i].tag = CommonFunctions::get_tag(vec_seg);
		segments[i].coord = std::lround(mesh.point(mesh.vertex_handle(segments[i].vert0))[(segments[i].tag & 1) ^ 1]);

		seg_nodes[i] = { v_in_quad[segments[i].vert0], v_in_quad[segments[i].vert1] };

		for (auto vf_h : mesh.vf_range(mesh.vertex_handle(vert_corner[i])))
		{
			triangle_corner[vf_h.idx()].push_back(v_in_quad[vert_corner[i]]);
		}
	}

	int cur_seg = 0;
	for (int i = 0; i < boundary_h.size(); i++)
	{
		if (boundary_v[i] == segments[cur_seg].vert1 || i == accu_bh_chart[seg_chart[cur_seg] + 1]) cur_seg++;
	
		triangle_segs[mesh.opposite_face_handle(mesh.halfedge_handle(boundary_h[i])).idx()].insert(cur_seg);
		bh_segment[i] = cur_seg;
	}
}

inline int UVParamWidgetExtended::chart_segment_cycle(int cur, int delta)
{
	int cur_chart = seg_chart[cur];
	int n_bseg_chart = accu_bseg_chart[cur_chart + 1] - accu_bseg_chart[cur_chart];
	int bseg0_chart = accu_bseg_chart[cur_chart];
	return CommonFunctions::period_id(cur - bseg0_chart + delta, n_bseg_chart) + bseg0_chart;
}