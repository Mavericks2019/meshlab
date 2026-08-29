#ifndef PARAQUADCUTTING_H
#define PARAQUADCUTTING_H

#include "MeshViewer/MeshDefinition.h"
#include "ParaQuadChartDecomposition.h"

#include <Eigen/Eigen>

#include <memory>

class ParaQuadCutting
{
public:
	ParaQuadCutting(Mesh& _mesh, const char* path, const char* file);
	~ParaQuadCutting() = default;

	void set_path(const char* str) { str_path = std::string(str); }
	void set_file(const char* str) { str_file = std::string(str); }
	const std::string& get_path() { return str_path; }
	bool is_valid() const { return valid_; }

	void cutting(double thres);
	void save_para(const char* filename);

	double get_split_thres() { return split_thres; };

	void set_seleted_vertices(const std::vector<int>& mesh_v);
	void set_seleted_edges(const std::vector<int>& mesh_e);
	void set_seleted_faces(const std::vector<int>& mesh_f);
	void set_seleted(const std::vector<int>& mesh_v, const std::vector<int>& mesh_e, const std::vector<int>& mesh_f);

	void get_textured_mesh(Mesh& tar);
	void trans_textured(Mesh& tar);
	void update_textured_mesh(Mesh& tar, bool use_new = false);
	void split_edges(const std::vector<int>& mesh_e);

	Mesh& get_origin_para() { return origin_para; };
	void update_para(double factor = 1.0);
	void flip_neg_charts();

	const OpenMesh::Vec3d& get_BB_Max() { return BB_Max; };
	const OpenMesh::Vec3d& get_BB_Min() { return BB_Min; };

	const std::set<int>& get_flipped_faces() { return flipped_faces; };

	std::map<int, OpenMesh::Vec3d> old_point;

	void calc_distortion(bool silence);
	double get_distortion() { return para_distortion; };

	void get_scaf_info(Eigen::MatrixXd& v_pos, Eigen::MatrixXd& uv_v_pos, Eigen::MatrixXi& fv_id, Eigen::MatrixXi& uv_fv_id);
	void load_from_scaf(const Eigen::MatrixXd& uv_v_pos);

private:
	Mesh& origin_mesh;
	Mesh origin_para;
	Mesh new_mesh, new_para;
	Mesh mesh_unpack;
	std::vector<Mesh> para_atlas;

	OpenMesh::Vec3d BB_Max, BB_Min;

	double para_distortion = 0.0;

	double cut_length = 0.0;
	double split_thres = 0.02;
	bool valid_ = false;

	std::set<int> flipped_faces;

	std::vector<std::vector<int>> v_para2layer;
	std::vector<int> face_layer;

	bool get_para_mesh();

	OpenMesh::EPropHandleT<int> e_segment;
	OpenMesh::HPropHandleT<std::pair<int, int>> h_mesh2para, h_para2mesh;

	std::vector<int> origin_h_mesh2para;

	OpenMesh::HalfedgeHandle get_mesh2para(OpenMesh::HalfedgeHandle h_mesh)
	{
		const auto& h_pair = new_mesh.property(h_mesh2para, h_mesh);
		return new_para.find_halfedge(new_para.vertex_handle(h_pair.first), new_para.vertex_handle(h_pair.second));
	};
	OpenMesh::HalfedgeHandle get_para2mesh(OpenMesh::HalfedgeHandle h_para)
	{
		const auto& h_pair = new_para.property(h_para2mesh, h_para);
		return new_mesh.find_halfedge(new_mesh.vertex_handle(h_pair.first), new_mesh.vertex_handle(h_pair.second));
	};

	void cut_on_boundary();
	void cut_segments();
	void decomposition();
	void get_packing_result();

	std::vector<int> get_seed_triangles();

	void init();
	std::string str_path, str_file;
	std::unique_ptr<ParaQuadChartDecomposition> charts_decomposition;
	void save_tri_charts();
	void save_quad_charts();

	std::vector<int> v_quad2para;

	void find_cut_path(OpenMesh::HalfedgeHandle& h_prev, int k, int dst_quad_v, int seg_id);

	OpenMesh::VertexHandle split_edge(OpenMesh::HalfedgeHandle h_para, double t);
	OpenMesh::VertexHandle split_face(OpenMesh::FaceHandle f_para, double u, double v);

	double find_split_t(OpenMesh::HalfedgeHandle h, int val_u, int val_v);
	OpenMesh::HalfedgeHandle find_split_uv(OpenMesh::FaceHandle f, int val_u, int val_v, double& u, double& v);

	void find_replace(OpenMesh::HalfedgeHandle& h0_p, OpenMesh::VertexHandle v_hf_p, OpenMesh::VertexHandle v_ht_p, OpenMesh::HalfedgeHandle& h0_m, OpenMesh::VertexHandle v_hf_m, OpenMesh::VertexHandle v_ht_m);

	inline uint get_tag(const OpenMesh::Vec3d& vec);

public:
	double calc_signal_error_3D();
};

#endif // PARAQUADCUTTING_H
