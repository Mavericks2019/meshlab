#ifndef INTERACTIVE_WIDGET_H
#define INTERACTIVE_WIDGET_H

#include "MeshViewerWidget.h"

#include "ANN.h"
class InteractiveWidget : public MeshViewerWidget
{
	Q_OBJECT
public:
	InteractiveWidget(QWidget* parent = 0);
	InteractiveWidget(QGLFormat& _fmt, QWidget* _parent);
	~InteractiveWidget();

public:
	enum { TRANS, VolumePICK, SHEETPICK, CELLPICK, VERTEXPICK, POINTPICK, FACEPICK, EDGEPICK, VERTEXMOVE};
	void setMouseMode(int mm)
	{
		mouse_mode_ = mm;
		if(mouse_mode_ != TRANS)
		{
			buildIndex();
		}
		emit setMouseMode_signal(mm);
	};
	int mouseMode() const { return mouse_mode_; }

signals:
	void pick_Column_signal(OpenVolumeMesh::HalfFaceHandle desired_HalffaceHnadle);
	void pick_Sheet_signal(OpenVolumeMesh::HalfFaceHandle desired_HalffaceHnadle);
	void pick_vertex_column_split_signal(OpenVolumeMesh::CellHandle desired_cellHandle, OpenVolumeMesh::HalfFaceHandle desired_HalffaceHnadle,OpenVolumeMesh::VertexHandle desired_VertexHandle);

	void draw_selected_Sheet_signal();
	void draw_selected_Column_signal();
	void draw_Doublet_signal();
	void draw_adjColumn_signal();
	void draw_Column_split_signal();
	void draw_scene_hexSim_signal();

	void draw_for_GP();
	void draw_for_TS();
	void draw_for_TM();
	void draw_for_GMD();
	void draw_for_MLD();

	void pick_vertex_as_handle_DE_signal(OpenVolumeMesh::VertexHandle vh);
	void pick_handle_for_move_DE_signal(OpenVolumeMesh::VertexHandle vh);
	void handle_move_DE_signal(OpenVolumeMesh::Geometry::Vec3d p);
	void handle_release_DE_signal(OpenVolumeMesh::Geometry::Vec3d p);
	void pick_edge_as_feature_signal(OpenVolumeMesh::EdgeHandle eh);

	void pick_vertex_as_handle_GMD_signal(OpenVolumeMesh::VertexHandle vh);
	void pick_handle_for_move_GMD_signal(OpenVolumeMesh::VertexHandle vh);
	void handle_move_GMD_signal(OpenVolumeMesh::Geometry::Vec3d p);
	void handle_release_GMD_signal(OpenVolumeMesh::Geometry::Vec3d p);

	void pick_vertex_as_handle_MLD_signal(OpenVolumeMesh::Geometry::Vec3d p);
	void pick_handle_for_move_MLD_signal(OpenVolumeMesh::Geometry::Vec3d p);
	void handle_move_MLD_signal(OpenVolumeMesh::Geometry::Vec3d p);
	void handle_release_MLD_signal(OpenVolumeMesh::Geometry::Vec3d p);

	void pick_point_for_polycube(OpenVolumeMesh::Geometry::Vec3d);
	void pick_vertex_for_polycube(OpenVolumeMesh::VertexHandle);

	void pick_face_handle_for_trace_sheet(OpenVolumeMesh::FaceHandle);

	void setMouseMode_signal(int);

	void surface_mesh_changed_signal(SurfaceMesh* mesh_);

public slots:
	void finish_one_topology_operation_slot()
	{
		constructBoundaryMesh();
		updateMesh(false);
		constructShowMesh();
		constructCellCenter();
		//setMouseMode(TRANS);

		//emit surface_mesh_changed_signal(&boundaryMesh);
	}

	void uppdate_vertex_position_slot()
	{
		int b_size = BoundaryFaceOnShowMesh.size();
		for (int i = 0; i < b_size; ++i)
		{
			std::vector<OpenVolumeMesh::Geometry::Vec3d>& bfv  = BoundaryFaceOnShowMesh[i];
			std::vector<int>& bfv_id = BoundaryFaceOnShowMesh_VID[i];
			for (int j = 0; j < bfv.size(); ++j)
			{
				bfv[j] = mesh_.vertex(OpenVolumeMesh::VertexHandle(bfv_id[j]));
			}
			NormalOfBoundaryFFaceOnShowMesh[i] = calculateNormalForQuad(bfv);
		}

		int i_size = InnerFaceOnShowMesh.size();
		for (int i = 0; i < i_size; ++i)
		{
			std::vector<OpenVolumeMesh::Geometry::Vec3d>& ifv = InnerFaceOnShowMesh[i];
			std::vector<int>& ifv_id = InnerFaceOnShowMesh_VID[i];
			for (int j = 0; j < ifv.size(); ++j)
			{
				ifv[j] = mesh_.vertex(OpenVolumeMesh::VertexHandle(ifv_id[j]));
			}
		}
		updateGL();
	}

	void finish_GP_BD_Mapping_slot()
	{
		constructBoundaryMesh();
		updateMesh(false);
		constructShowMesh();
		constructCellCenter();
		setMouseMode(TRANS);
	}

public slots:
	void set_draw_main_scene(int state)
	{
		draw_main_scene = (state == Qt::Checked);
		updateGL();
	}
	void set_draw_flipped_tet(int s)
	{
		draw_flipped_tet_ok = (s == Qt::Checked);
		updateGL();
	}
	void render_iso_d_text_slot(QString str)
	{
		iso_d_str = str;
	}
protected:
	virtual void mousePressEvent(QMouseEvent *_event);
	virtual void mouseMoveEvent(QMouseEvent *_event);
	virtual void mouseReleaseEvent(QMouseEvent *_event);
	int mouse_mode_;

	void dragEnterEvent(QDragEnterEvent *event);
	void dropEvent(QDropEvent *event);

	void pick_Column(double p[3]);
	void pick_Sheet(double p[3]);
	void pick_vertex_column_split(double p[3]);
	void pick_cell(double p[3]);
	void pick_vertex(double p[3]);
	void pick_point(int x,int y,double p[3]);
	void pick_point_get_depth(int x, int y, double p[3], double& d); //assign depth
	void pick_point_with_depth(int x, int y, double p[3], double d); //use depth
	void pick_face(double p[3]);
	void pick_edge(double p[3]);

	double pick_point_depth;
	void buildIndex();
	ANNkd_tree* kdTree;
	/*flann_index_t index_id;
	struct FLANNParameters fp;*/

	virtual void draw_scene(int drawmode);
	void draw_flipped_tet();

	bool draw_main_scene;
	bool draw_flipped_tet_ok;

private:
	OpenVolumeMesh::VertexHandle picked_vertex;
	OpenVolumeMesh::EdgeHandle picked_edge;
	OpenVolumeMesh::FaceHandle picked_face;

public:
	void setFunction_TO(bool fto)
	{
		HaveFunction_TO = fto;
	}
	bool haveFunction_TO()
	{
		return HaveFunction_TO;
	}
	void setFunction_GP(bool fgp)
	{
		HaveFunction_GP = fgp;
	}
	bool haveFunction_GP()
	{
		return HaveFunction_GP;
	}
	void setFunction_TS(bool fhp)
	{
		HaveFunction_TS = fhp;
	}
	bool haveFunction_TS()
	{
		return HaveFunction_TS;
	}
	void setFunction_TM(bool ftm)
	{
		HaveFunction_TM = ftm;
		if(ftm && boundaryMesh.n_vertices() > 0)
		{
			emit surface_mesh_changed_signal(&boundaryMesh);
		}
	}
	bool haveFunction_TM()
	{
		return HaveFunction_TM;
	}
	void setFunction_MCC(bool mcc_)
	{
		HaveFunction_MCC = mcc_;
	}
	bool haveFunction_MCC()
	{
		return HaveFunction_MCC;
	}
	void setFunction_GMD(bool gmd_)
	{
		HaveFunction_GMD = gmd_;
	}
	bool haveFunction_GMD()
	{
		return HaveFunction_GMD;
	}
	void setFunction_MLD(bool mld_)
	{
		HaveFunction_MLD = mld_;
	}
	bool haveFunction_MLD()
	{
		return HaveFunction_MLD;
	}
	void setAllFunction(bool d)
	{
		HaveFunction_TO = d;
		HaveFunction_GP = d;
		HaveFunction_TS = d;
		HaveFunction_TM = d; 
		HaveFunction_MCC = d;
		HaveFunction_GMD = d;
		HaveFunction_MLD = d;
	}
private:
	bool HaveFunction_TO;
	bool HaveFunction_GP;
	bool HaveFunction_TS;
	bool HaveFunction_TM;//tetrahedrization meshing
	bool HaveFunction_MCC;
	bool HaveFunction_GMD;
	bool HaveFunction_MLD;
	QString iso_d_str; QFont tempfont;

#pragma region auxiliary_function
public:
	void subdivide_Tet_TO_Hex();//only for tet mesh
	void transform_mesh(std::vector<double>& tm);
	void create_grid_all_hex_mesh(int x_grid_size, int y_grid_size, int z_grid_size, double length = 1.0);

	void create_grid_mesh_mesh();
	void create_slice_mesh();

#pragma endregion

};

#endif