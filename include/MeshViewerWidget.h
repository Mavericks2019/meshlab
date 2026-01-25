#ifndef MESHVIEWER_WIDGET_H
#define MESHVIEWER_WIDGET_H

#include <QString>
#include <QMessageBox>
#include <QFileDialog>

#include "QGLViewerWidget.h"
#include "MeshDefinition.h"

#include <OpenVolumeMesh/FileManager/FileManager.hh>

class MeshViewerWidget : public QGLViewerWidget 
{
	Q_OBJECT
public:
	MeshViewerWidget(QWidget* parent = 0);
	MeshViewerWidget(QGLFormat& _fmt, QWidget* _parent);
	~MeshViewerWidget();
public:
	bool openMesh(const char* filename);
	bool openMesh_VTK(const char* filename);
	bool saveMesh(const char* filename);
	bool saveBoundaryMesh(const char* filename);
	bool saveMesh_VTK(const char* filename);
	bool saveMesh_Tet(const char* filename);
	bool saveMesh_VOFF(const char* filename);
	bool SaveScreen(const char* filePath);
	virtual void clearAllMesh();

	VolumeMesh* mesh_ptr() { return &mesh_; };
	VolumeMesh& mesh() { return mesh_; };
	const VolumeMesh& mesh() const { return mesh_; };
	SurfaceMesh* surface_mesh_ptr() { return &boundaryMesh; };
	SurfaceMesh& surface_mesh() { return boundaryMesh; };
	const SurfaceMesh& surface_mesh() const { return boundaryMesh; };
	void updateMesh(bool set_scene = true)
	{
		updateMeshNormals();
		updateMeshCenter(set_scene);
	};
	void save_mesh_graph(const char* filename);
	void save_hex_mesh_graph(const char* filename);
	void construct_vertex_texture();
	int get_vertex_texture_ID(){ return Verterx_Texture_ID; };
public slots:
	void showLeftRight(int state);
	void showUpDown(int state);
	void showBackFront(int state);
	void xSliderShow(int value);
	void ySliderShow(int value);
	void zSliderShow(int value);
	void save_show_mesh();

signals:
	//void printDebugInfo(QString debuginfo);
	void loadMeshOK(bool OK,QString);

protected:
	void constructBoundaryMesh();
	void constructShowMesh();
	void constructShowMeshFaceCenter();
	std::vector<bool> vertex_flag;
	std::vector<bool> ValidCell;
	std::vector<bool> ValidFace;

	void updateMeshCenter(bool set_scene = true); // used by update_mesh().
	void updateMeshNormals(); // used by update_mesh().

protected:
	virtual void draw_scene(int drawmode);

private:
	void draw_mesh_wireframe() const;
	//void draw_mesh_hidden_lines() const;
	void draw_mesh_solidflat() const;
	void draw_mesh_all_lines() const;
	void draw_mesh_all_polys() const;
	void draw_mesh_all_polys_lines() const;
	void draw_mesh_singularity();
	void draw_mesh_all_points();
	void draw_mesh_texture();

protected:
	OpenVolumeMesh::IO::FileManager fileManager;
	VolumeMesh mesh_;
	SurfaceMesh boundaryMesh;
	
	std::vector<OpenVolumeMesh::Geometry::Vec3d> vertex_texture;
	int Verterx_Texture_ID;

	std::vector<OpenVolumeMesh::Geometry::Vec3d> VertexOnShowMesh;
	std::vector< std::vector<OpenVolumeMesh::Geometry::Vec3d> > BoundaryFaceOnShowMesh;
	std::vector< std::vector<OpenVolumeMesh::Geometry::Vec3d> > InnerFaceOnShowMesh;
	std::vector< std::vector<int> > BoundaryFaceOnShowMesh_VID;
	std::vector< std::vector<int> > InnerFaceOnShowMesh_VID;
	std::vector< OpenVolumeMesh::Geometry::Vec3d> NormalOfBoundaryFFaceOnShowMesh;
	std::vector< OpenVolumeMesh::Geometry::Vec3d> FaceCenterOnShowMesh;
	std::vector< OpenVolumeMesh::Geometry::Vec3d> NormalOfInnerFFaceOnShowMesh;
	OpenVolumeMesh::Geometry::Vec3d calculateNormalForQuad(std::vector< OpenVolumeMesh::Geometry::Vec3d> one_face);
	std::vector< bool > BoundaryFaceFlag;

	double minEdgeLen;
	double maxEdgeLen;

	bool showRight; double x_per;
	bool showUp;    double y_per;
	bool showBack;  double z_per;
	void constructCellCenter();
	std::vector<OpenVolumeMesh::Geometry::Vec3d> CellCenter;
private:
	void updateIndices();

public:
	// mesh modes.
	enum { TETRAHEDRAL = 0, HEXAHEDRA, N_MESH_MODES };
	void setMeshMode(int mm) { mesh_mode_ = mm;}
	int meshMode() const { return mesh_mode_; }
	void checkMeshMode();

private:
	int mesh_mode_;
	std::vector<unsigned int>  indices_;
	std::vector<unsigned int> VIndices;
	
	void compute_scaled_jacobian_hex();
private://clear data
	void clearALLVolumeMesh()
	{
		mesh_.clear(true);
		CellCenter.clear();
	}
	void clearALLBoundaryMesh()
	{
		//boundary mesh
		boundaryMesh.clear();
	}
	void clearALLShowMesh()
	{
		VertexOnShowMesh.clear();
		BoundaryFaceOnShowMesh.clear();
		InnerFaceOnShowMesh.clear();
		NormalOfBoundaryFFaceOnShowMesh.clear();
		FaceCenterOnShowMesh.clear();
		BoundaryFaceFlag.clear();
	}
protected:
	void printBasicMeshInfo();

};

#endif // MESHPROCESSING_MESHVIEWERWIDGET_H
