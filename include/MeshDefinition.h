#ifndef MESHPROCESSING_MESHDEFINITION_H
#define MESHPROCESSING_MESHDEFINITION_H

#include <OpenVolumeMesh/Geometry/VectorT.hh>
#include <OpenVolumeMesh/Mesh/PolyhedralMesh.hh>
#include <OpenVolumeMesh/Mesh/HexahedralMesh.hh>

#include <OpenMesh/Core/Geometry/VectorT.hh>
#include <OpenMesh/Core/Mesh/PolyMesh_ArrayKernelT.hh>
#include <OpenMesh/Core/IO/MeshIO.hh>

typedef OpenVolumeMesh::GeometryKernel<OpenVolumeMesh::Geometry::Vec3d> Src_VolumeMesh;

class VolumeMesh : public Src_VolumeMesh
{
public:
	VolumeMesh() :Src_VolumeMesh()
	{
		showing_face_id.clear();
	}

	OpenVolumeMesh::Geometry::Vec3d BBox_min;
	OpenVolumeMesh::Geometry::Vec3d BBox_max;
	double radius;
	OpenVolumeMesh::Geometry::Vec3d BBox_center;
	double avg_edge_length;

	std::vector<int> showing_face_id;
};

OpenVolumeMesh::HalfFaceHandle oppositeHalffaceInCell(OpenVolumeMesh::HalfFaceHandle hf, VolumeMesh* mesh_);
std::vector<OpenVolumeMesh::VertexHandle> vertexOnEdge(OpenVolumeMesh::EdgeHandle eh, VolumeMesh* mesh_);
OpenVolumeMesh::VertexHandle adjVertexBetweenTwoFace(OpenVolumeMesh::HalfFaceHandle hf1, OpenVolumeMesh::VertexHandle v1, OpenVolumeMesh::HalfFaceHandle hf2, VolumeMesh* mesh_);
OpenVolumeMesh::VertexHandle VertexHandleOnOppositeFace(OpenVolumeMesh::HalfFaceHandle hf, OpenVolumeMesh::VertexHandle v, VolumeMesh* mesh_);

//some basic routines for volume mesh
double compute_average_edge_len(VolumeMesh* mesh_);
double compute_average_face_area(VolumeMesh* mesh_);
double compute_absolute_volume(VolumeMesh* mesh_, OpenVolumeMesh::CellHandle cell_handle);
void compute_all_absolute_volume(VolumeMesh* mesh_,std::vector<double>& cell_vol);
bool check_negative_volume(VolumeMesh* mesh_);
bool check_negative_volume_id(VolumeMesh* mesh_, int& min_cell_id);
void get_negative_volume(VolumeMesh* mesh_, std::vector<OpenVolumeMesh::CellHandle>& negative_cell);
void calc_tet_face_normal(VolumeMesh* mesh_, OpenVolumeMesh::HalfFaceHandle& hf, OpenVolumeMesh::Geometry::Vec3d& n);
void calc_tet_face_normal_anisotropy(VolumeMesh* mesh_, OpenVolumeMesh::HalfFaceHandle& hf,const OpenVolumeMesh::Geometry::Vec6d& Q, OpenVolumeMesh::Geometry::Vec3d& n);
bool check_in_tet(VolumeMesh* mesh_, const OpenVolumeMesh::Geometry::Vec3d& p, OpenVolumeMesh::CellHandle ch);

//for tetrahedral mesh
bool is_flip_face_23_ok(VolumeMesh* mesh_, OpenVolumeMesh::FaceHandle fh);
bool flip_face_23(VolumeMesh* mesh_, OpenVolumeMesh::FaceHandle fh);
bool is_flip_edge_32_ok(VolumeMesh* mesh_, OpenVolumeMesh::EdgeHandle eh);
bool flip_edge_32(VolumeMesh* mesh_, OpenVolumeMesh::EdgeHandle eh);
bool line_intersect_triangle(const OpenVolumeMesh::Geometry::Vec3d& p,
							 const OpenVolumeMesh::Geometry::Vec3d& q,
							 const OpenVolumeMesh::Geometry::Vec3d& r,
							 const OpenVolumeMesh::Geometry::Vec3d& s,
							 const OpenVolumeMesh::Geometry::Vec3d& t);

struct SurfaceMeshTraits : public OpenMesh::DefaultTraits
{
	typedef OpenMesh::Vec3d Point;
	typedef OpenMesh::Vec3d Normal;

	VertexAttributes(OpenMesh::Attributes::Status);
	FaceAttributes(OpenMesh::Attributes::Status);
	EdgeAttributes(OpenMesh::Attributes::Status);
	HalfedgeAttributes(OpenMesh::Attributes::Status);
	
	VertexTraits
	{
		private:
			int tet_vertex_id;
		public:
			VertexT(): tet_vertex_id(-1)
			{};
			void set_tet_vertex_id(int id){tet_vertex_id = id;};
			int get_tet_vertex_id(){return tet_vertex_id;};
	};

	FaceTraits
	{
	private:
		Point faceCenter_;
		int cell_id;
	public:
		FaceT(): faceCenter_( Point(0,0,0) ), cell_id(-1)
		{};

		Point faceCenter()
		{
			return faceCenter_;
		};
		void setFaceCenter(Point c)
		{
			faceCenter_ = c;
		};
		int get_cell_id()
		{
			return cell_id;
		};
		void set_cell_id(int i)
		{
			cell_id = i;
		};
	};
};

typedef OpenMesh::PolyMesh_ArrayKernelT<SurfaceMeshTraits> SurfaceMesh;

void compute_point_area(SurfaceMesh* mesh_, std::vector<std::map<int,double>>& cornerArea, std::vector<double>& pointArea);

void rot_coord_sys(const OpenMesh::Vec3d &old_u, const OpenMesh::Vec3d &old_v,
				   const OpenMesh::Vec3d &new_norm,
				   OpenMesh::Vec3d &new_u, OpenMesh::Vec3d &new_v);

void proj_curv(const OpenMesh::Vec3d &old_u, const OpenMesh::Vec3d &old_v,
			   double old_ku, double old_kuv, double old_kv,
			   const OpenMesh::Vec3d &new_u, const OpenMesh::Vec3d &new_v,
			   double &new_ku, double &new_kuv, double &new_kv);

// Given a curvature tensor, find principal directions and curvatures
// Makes sure that pdir1 and pdir2 are perpendicular to normal
void diagonalize_curv(const OpenMesh::Vec3d &old_u, const OpenMesh::Vec3d &old_v,
					  double ku, double kuv, double kv,
					  const OpenMesh::Vec3d &new_norm,
					  OpenMesh::Vec3d &pdir1, OpenMesh::Vec3d &pdir2, double &vk1, double &vk2);

void compute_principal_curvature(SurfaceMesh* mesh_, 
								 std::vector<double>& K1, std::vector<double>& K2, 
								 std::vector<OpenMesh::Vec3d>& dir1,std::vector<OpenMesh::Vec3d>& dir2);

bool baryCoord( const OpenMesh::Vec3d& _p, const OpenMesh::Vec3d& _u, const OpenMesh::Vec3d& _v, const OpenMesh::Vec3d& _w, OpenMesh::Vec3d&_result );

double distPointTriangleSquared(const OpenVolumeMesh::Geometry::Vec3d& _p,
	const OpenVolumeMesh::Geometry::Vec3d& _v0,
	const OpenVolumeMesh::Geometry::Vec3d& _v1,
	const OpenVolumeMesh::Geometry::Vec3d& _v2,
	OpenVolumeMesh::Geometry::Vec3d& _nearestPoint);

double distPointTriangleSquared2(const double _p[3],
	const double _v0[3],
	const double _v1[3],
	const double _v2[3],
	double _nearestPoint[3]);

#endif