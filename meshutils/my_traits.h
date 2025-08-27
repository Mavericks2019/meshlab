#pragma once
#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Geometry/VectorT.hh>
#include <OpenMesh/Core/Mesh/PolyMesh_ArrayKernelT.hh>
#include <Eigen/Dense>
#include <fstream>    // 添加这行
#include <iomanip>    // 添加这行（因为后面使用了 std::setprecision）



struct MyTraits : public OpenMesh::DefaultTraits {
    typedef OpenMesh::Vec3d Point;  // 使用双精度浮点数 (double) 作为点的坐标类型
	typedef OpenMesh::Vec3d Normal;
	typedef OpenMesh::Vec2d TexCoord2D;

	VertexAttributes(OpenMesh::Attributes::Status);
	FaceAttributes(OpenMesh::Attributes::Status);
	EdgeAttributes(OpenMesh::Attributes::Status);
	HalfedgeAttributes(OpenMesh::Attributes::Status);
    VertexTraits {
        float curvature;
    };
};

typedef OpenMesh::PolyMesh_ArrayKernelT<MyTraits> Mesh;
//typedef OpenMesh::PolyMesh_ArrayKernelT<MyTraits> Mesh;

bool is_flip_ok_openmesh(Mesh::EdgeHandle& eh, Mesh& mesh_);
bool flip_openmesh(Mesh::EdgeHandle& eh, Mesh& mesh_);
bool check_in_triangle_face(const std::vector<OpenMesh::Vec3d>& tri, const OpenMesh::Vec3d& p);

class Mesh_doubleIO
{
public:
	static bool load_mesh(Mesh& _mesh, const char* _filename, bool load_texture = false);
	static bool save_mesh(const Mesh& _mesh, const char* _filename, bool save_texture = false);

	static bool save_uv_mesh(const Mesh& _mesh, const char* _filename);

	enum class file_type
	{
		others, obj, off
	};

	static file_type get_file_type(const char* _filename);

	//edge indices may change
	static void copy_mesh(const Mesh& src, Mesh& dst);

private:
	static bool load_obj(Mesh& _mesh, const char* _filename, bool load_texture);
	static bool load_off(Mesh& _mesh, const char* _filename);

	static bool save_obj(const Mesh& _mesh, const char* _filename, bool save_texture);
	static bool save_off(const Mesh& _mesh, const char* _filename);
};