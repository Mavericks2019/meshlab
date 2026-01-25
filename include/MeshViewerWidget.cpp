#include <map>
#include <string>
#include <iostream>
#include <algorithm>
#include <cmath>

#include <qapplication.h>
#include <QLineEdit>
#include <QTime>

#include <Eigen/Dense>
#include <QGLContext>

#include "MeshViewerWidget.h"

MeshViewerWidget::MeshViewerWidget(QWidget* parent)
	:QGLViewerWidget(parent)
{
	showRight = true; showUp = true; showBack = true; 
	x_per = 0.0; y_per = 0.0; z_per = 0.0;
	Verterx_Texture_ID = 0;
}

MeshViewerWidget::MeshViewerWidget(QGLFormat& _fmt, QWidget* _parent)
	:QGLViewerWidget(_fmt, _parent)
{
	showRight = true; showUp = true; showBack = true;
	x_per = 0.0; y_per = 0.0; z_per = 0.0;
	Verterx_Texture_ID = 0;
}


MeshViewerWidget::~MeshViewerWidget()
{

}

void MeshViewerWidget::updateMeshCenter(bool set_scene)
{
#if 0
	SurfaceMesh::VertexIter vIt = boundaryMesh.vertices_begin();
	SurfaceMesh::VertexIter vEnd = boundaryMesh.vertices_end();
	bbMin = bbMax = boundaryMesh.point(vIt.handle());

	size_t count = 0;
	for (; vIt != vEnd; ++vIt, ++count)
	{
		bbMin.minimize(boundaryMesh.point(vIt.handle()));
		bbMax.maximize(boundaryMesh.point(vIt.handle()));
	}
#else
	OpenVolumeMesh::VertexIter vIt = mesh_.vertices_begin();
	OpenVolumeMesh::VertexIter vEnd = mesh_.vertices_end();
	OpenVolumeMesh::Geometry::Vec3d bbMin_ = mesh_.vertex(vIt.cur_handle());
	OpenVolumeMesh::Geometry::Vec3d bbMax_ = mesh_.vertex(vIt.cur_handle());

	size_t count = 0;
	for (; vIt != vEnd; ++vIt, ++count)
	{
		bbMin_.minimize(mesh_.vertex(vIt.cur_handle()));
		bbMax_.maximize(mesh_.vertex(vIt.cur_handle()));
	}
	bbMin = OpenMesh::Vec3d(bbMin_[0], bbMin_[1], bbMin_[2]);
	bbMax = OpenMesh::Vec3d(bbMax_[0], bbMax_[1], bbMax_[2]);
#endif
	
	OpenVolumeMesh::EdgeIter e_it = mesh_.edges_begin();
	OpenVolumeMesh::EdgeIter e_end = mesh_.edges_end();
	double aveLen = 0.0;
	maxEdgeLen = 0.0; minEdgeLen = (bbMin-bbMax).norm();
	double e_len = 0.0;
	for(; e_it != e_end; ++e_it)
	{
		e_len = mesh_.length(*e_it);
		if( e_len > maxEdgeLen )
		{
			maxEdgeLen = e_len;
		}
		else if(e_len < minEdgeLen )
		{
			minEdgeLen = e_len;
		}
		aveLen += e_len;
	}
	aveLen /= mesh_.n_edges();

	// set center and radius and box's radius.
	SurfaceMesh::Point c = (bbMin+bbMax)*0.5;
	OpenVolumeMesh::Geometry::Vec3d center(c[0],c[1],c[2]);
	//if(set_scene) 
		set_scene_pos( center, (bbMin-bbMax).norm()*0.5 );
	mesh_.BBox_min[0] = bbMin[0]; mesh_.BBox_min[1] = bbMin[1]; mesh_.BBox_min[2] = bbMin[2];
	mesh_.BBox_max[0] = bbMax[0]; mesh_.BBox_max[1] = bbMax[1]; mesh_.BBox_max[2] = bbMax[2];
	mesh_.BBox_center = center;
	mesh_.radius = (bbMin-bbMax).norm()*0.5;
	mesh_.avg_edge_length = aveLen;
}

void MeshViewerWidget::updateMeshNormals()
{
}

bool MeshViewerWidget::openMesh(const char* filename)
{
	bool update_view = true;
	if (mesh_.n_vertices() != 0) update_view = false;
#if 1
	if( fileManager.readFile( filename, mesh_ ) )
	{
		mesh_.enable_bottom_up_incidences(true);
		vertex_texture.clear();

		boundaryMesh.clear();
		boundaryMesh.request_face_status();
		boundaryMesh.request_vertex_status();
		boundaryMesh.request_edge_status();
		boundaryMesh.request_face_normals();
		boundaryMesh.request_vertex_normals();
		constructBoundaryMesh();
		updateMesh(update_view);
		printBasicMeshInfo();

		//showRight = true; showUp = true; showBack = true; 
		//x_per = 0.0; y_per = 0.0; z_per = 0.0;

		constructShowMesh();
		//cell center
		constructCellCenter();

		/*FILE* f_b = fopen("E:\\Volume Mesh\\Our_Volume_Models\\Tetgen\\sphere_p.de", "w");
		for (OpenVolumeMesh::VertexIter v_it = mesh_.vertices_begin(); v_it != mesh_.vertices_end(); ++v_it)
		{
			if (mesh_.is_boundary(*v_it))
			{
				OpenVolumeMesh::Geometry::Vec3d p = mesh_.vertex(*v_it);
				fprintf(f_b, "%d %20.19f %20.19f %20.19f\n", v_it->idx(), p[0], p[1], p[2]);
			}
		}
		fclose(f_b);*/

		// loading done
		updateGL();
		return true;
	}
#endif
	return false;
}

//just support the hex mesh in open volume mesh library.
bool MeshViewerWidget::openMesh_VTK(const char* filename)
{
	FILE* input_f = fopen(filename,"r");
	if(!input_f)
	{
		return false;
	}

	const char* input_format = "-v";

	char oneLine[2048];
	char* vertex_header = "POINTS";
	char* cell_header = "CELLS";
	bool vertexInfo = false; bool cellInfo = false;
	int vertexNum =1; char format[1024] = ""; int haveVertexNum =0;
	int cellNum = 1; int faceNum =0;int haveCellNum =0;
	OpenVolumeMesh::Geometry::Vec3d v;
	char v1[128]; char v2[128];char v3[128];
	std::vector<OpenVolumeMesh::VertexHandle> v_Vec;

	int faceVertexNum = 0; 
	int index[8] = {0};
	std::vector<OpenVolumeMesh::VertexHandle> vh_Vec;
	int Correspondences[8] = {0,1,5,4,3,7,6,2};
	while( fgets(oneLine,2048,input_f) )
	{
		//get vertex's information
		if(!vertexInfo)
		{
			int count = 0;
			for(unsigned int i=0;i<6;++i)
			{
				if(oneLine[i] == vertex_header[i])
				{
					++count;
				}
			}
			if( count == 6 )
			{
				vertexInfo = true;
				char vertex[6] = "";
				sscanf(oneLine,"%s %d %s",vertex,&vertexNum,format);
				v_Vec.resize(vertexNum);
			}
		}
		else if(haveVertexNum < vertexNum && vertexInfo && !cellInfo)
		{
			if(oneLine[0] != ' ')
			{
				sscanf(oneLine,"%s %s %s",v1,v2,v3);
				v[0] = atof(v1); v[1] = atof(v2); v[2] = atof(v3);
				v_Vec[haveVertexNum] = mesh_.add_vertex(v); 
				++haveVertexNum;
			}
		}

		if(haveVertexNum == vertexNum && !cellInfo && vertexInfo)
		{
			int count = 0;
			for(unsigned int i=0;i<5;++i)
			{
				if(oneLine[i] == cell_header[i])
				{
					++count;
				}
			}
			if( count == 5 )
			{
				cellInfo = true;
				char cell[5] = "";
				sscanf(oneLine,"%s %d %d",cell,&cellNum,&faceNum);
			}
		}
		else if(haveVertexNum == vertexNum && cellInfo && vertexInfo)
		{
			if(oneLine[0] != ' ')
			{
				sscanf(oneLine,"%d %d %d %d %d %d %d %d %d",&faceVertexNum,&index[0],&index[1],&index[2],&index[3],&index[4],&index[5],&index[6],&index[7]);
				if(faceVertexNum == 8)
				{
					vh_Vec.resize(faceVertexNum);
					if( strcmp(input_format,"-v") == 0)
					{
						vh_Vec[0] = v_Vec[ index[Correspondences[0]] ]; vh_Vec[1] = v_Vec[ index[Correspondences[1]] ];
						vh_Vec[2] = v_Vec[ index[Correspondences[2]] ]; vh_Vec[3] = v_Vec[ index[Correspondences[3]] ];
						vh_Vec[4] = v_Vec[ index[Correspondences[4]] ]; vh_Vec[5] = v_Vec[ index[Correspondences[5]] ];
						vh_Vec[6] = v_Vec[ index[Correspondences[6]] ]; vh_Vec[7] = v_Vec[ index[Correspondences[7]] ];
					}
					/*else if(strcmp(input_format,"-h") == 0)
					{
					vh_Vec[0] = v_Vec[ index[Correspondences_Hex[0]] ]; vh_Vec[1] = v_Vec[ index[Correspondences_Hex[1]] ];
					vh_Vec[2] = v_Vec[ index[Correspondences_Hex[2]] ]; vh_Vec[3] = v_Vec[ index[Correspondences_Hex[3]] ];
					vh_Vec[4] = v_Vec[ index[Correspondences_Hex[4]] ]; vh_Vec[5] = v_Vec[ index[Correspondences_Hex[5]] ];
					vh_Vec[6] = v_Vec[ index[Correspondences_Hex[6]] ]; vh_Vec[7] = v_Vec[ index[Correspondences_Hex[7]] ];
					}*/

					//mesh_.add_cell(vh_Vec);
					++haveCellNum;
				}
			}
		}
		if(haveCellNum == cellNum)
		{
			break;
		}
	}
}

void MeshViewerWidget::printBasicMeshInfo()
{
	checkMeshMode();

	printf("Information of the input mesh:\nVertices : %d;\nFaces : %d; HalfFaces : %d\nEdges : %d; HalfEdges : %d;\nCells : %d\n", 
		mesh_.n_vertices(),mesh_.n_faces(),mesh_.n_halffaces(),mesh_.n_edges(),mesh_.n_halfedges(),mesh_.n_cells());

	printf("Information of the boundary mesh:\nVertices : %d;\nFaces : %d;\nEdges : %d; HalfEdges : %d\n", 
		boundaryMesh.n_vertices(),boundaryMesh.n_faces(),boundaryMesh.n_edges(),boundaryMesh.n_halfedges());

	printf("BoundingBox Of the Mesh:\nX : [ %f , %f ]\nY : [ %f , %f ]\nZ : [ %f , %f ]\n",
		bbMin[0],bbMax[0],bbMin[1],bbMax[1],bbMin[2],bbMax[2]);
}

//bug: when the volume mesh has no cells
void MeshViewerWidget::constructBoundaryMesh()
{
	//OpenVolumeMesh::BoundaryFaceIter bf_it(mesh_.bf_iter());
	//OpenVolumeMesh::HalfFaceVertexIter hfv_it = mesh_.hfv_iter( mesh_.halfface_handle(*bf_it,0) );
	
	std::vector<SurfaceMesh::VertexHandle> vertexHandleVec;
	std::vector<SurfaceMesh::VertexHandle> faceVertexHandle;
	std::map<int,int> VolumeSurfaceVertex;
	std::map<int,int>::iterator map_it;
	int indexOnSurface = 0;
	int InvalidHF =0;
	BoundaryFaceFlag.clear();
	BoundaryFaceFlag.resize(mesh_.n_faces(),false);
	boundaryMesh.clear();
	//for (; bf_it.valid(); ++bf_it)
	for (OpenVolumeMesh::FaceIter f_it = mesh_.faces_begin(); f_it != mesh_.faces_end();++f_it)
	{
		if ( !mesh_.is_boundary(*f_it) ) continue;

		OpenVolumeMesh::HalfFaceHandle hfHandle = mesh_.halfface_handle(*f_it, 0);
		OpenVolumeMesh::HalfFaceHandle hfHandle2 = mesh_.halfface_handle(*f_it, 1);
		int cell_id = mesh_.incident_cell(hfHandle2).idx();
		if( cell_id < 0 )
		{
			hfHandle2 = hfHandle;
			cell_id = mesh_.incident_cell(hfHandle2).idx();
			hfHandle = mesh_.halfface_handle(*f_it, 1);
		}
		BoundaryFaceFlag[mesh_.face_handle(hfHandle).idx()] = true;
		++InvalidHF;
		OpenVolumeMesh::HalfFaceVertexIter hfv_it = mesh_.hfv_iter(hfHandle);
		faceVertexHandle.clear();
		for( hfv_it; hfv_it; ++hfv_it)
		{
			map_it = VolumeSurfaceVertex.find(hfv_it->idx()) ;
			if( map_it == VolumeSurfaceVertex.end() )
			{
				VolumeSurfaceVertex.insert(std::pair<int,int>( hfv_it->idx(), indexOnSurface ));
				OpenVolumeMesh::Geometry::Vec3d v1 = mesh_.vertex(*hfv_it);
				SurfaceMesh::Point v2( v1[0], v1[1], v1[2] );
				vertexHandleVec.push_back( boundaryMesh.add_vertex( v2 ) );
				boundaryMesh.data(vertexHandleVec.back()).set_tet_vertex_id(hfv_it->idx());
				faceVertexHandle.push_back( vertexHandleVec.back() );
				++indexOnSurface;
			}
			else
			{
				faceVertexHandle.push_back(vertexHandleVec[map_it->second]);
			}
		}

		std::reverse(faceVertexHandle.begin(), faceVertexHandle.end());
		OpenMesh::FaceHandle fh = boundaryMesh.add_face(faceVertexHandle);
		boundaryMesh.data(fh).set_cell_id( cell_id );
	}

	boundaryMesh.update_normals();
	//OpenMesh::IO::write_mesh(boundaryMesh, "bm.off");
}

void MeshViewerWidget::constructShowMesh()
{
	//QTime time;
	//std::cout << "Begin to construct the Showing Mesh" <<std::endl;
	//time.start();
	if(mesh_.n_vertices() == 0 )
		return;

	if(vertex_flag.size() != mesh_.n_vertices())
	{
		vertex_flag.resize(mesh_.n_vertices(),false);
	}

	VolumeMesh temp_VM = mesh_;
	double offset = minEdgeLen*0.1;
	//VertexOnShowMesh.clear();
	if(showRight && showUp && showBack)
	{
		//x,y,z
		double x_plane = bbMax[0] - x_per*(bbMax[0] - bbMin[0]) + offset;
		double y_plane = bbMax[1] - y_per*(bbMax[1] - bbMin[1]) + offset;
		//double y_plane = y_per*(bbMax[1] - bbMin[1]) - offset;
		//double z_plane = bbMax[2] - z_per*(bbMax[2] - bbMin[2]) + offset;
		double z_plane = bbMin[2] + z_per*(bbMax[2] - bbMin[2]) - offset;

		OpenVolumeMesh::VertexIter v_it = temp_VM.vertices_begin();
		int vertexCount =0; int temp_mesh_vertexCount =0;
		for( v_it; v_it != temp_VM.vertices_end(); ++v_it )
		{
			OpenVolumeMesh::Geometry::Vec3d v = temp_VM.vertex(*v_it);
			//if( v[0] <= x_plane && v[1] <= y_plane && v[2] <= z_plane )//in
			//if (v[0] <= x_plane && v[1] > y_plane && v[2] <= z_plane)//in
			if (v[0] <= x_plane && v[1] <= y_plane && v[2] >= z_plane)//in
			{
				vertex_flag[vertexCount] = true;
				//VertexOnShowMesh.push_back(v);
			}
			else
			{
				vertex_flag[vertexCount] = false;
			}
			++vertexCount;
		}
	}
	//int t_ms = time.elapsed();
	//std::cout << "End setting vertex flag..... using time: " << t_ms <<std::endl;
	//time.start();

	OpenVolumeMesh::CellIter valid_c_it = temp_VM.cells_begin();
	OpenVolumeMesh::CellVertexIter valid_cv_it = temp_VM.cv_iter(*valid_c_it);
	if(ValidCell.size() != temp_VM.n_cells())
	{
		ValidCell.resize( temp_VM.n_cells() );
	}
	for( valid_c_it; valid_c_it != temp_VM.cells_end(); ++valid_c_it )
	{
		valid_cv_it = temp_VM.cv_iter(*valid_c_it);
		bool valid_cell_OK = true;
		for(valid_cv_it; valid_cv_it; ++valid_cv_it)
		{
			if( !vertex_flag[valid_cv_it->idx()] )
			{
				valid_cell_OK = false;
				ValidCell[valid_c_it->idx()] = false;
				break;
			}
		}
		if( valid_cell_OK )
		{
			ValidCell[valid_c_it->idx()] = true;
		}
	}

	/*for(valid_c_it = temp_VM.cells_begin();valid_c_it != temp_VM.cells_end(); )
	{
		if( !ValidCell[valid_c_it->idx()] )
		{
			temp_VM.delete_cell(*valid_c_it);
		}
		else
		{
			++valid_c_it;
		}
	}*/

	//t_ms = time.elapsed();
	//std::cout << "End deleting cell..... using time: " << t_ms <<std::endl;
	//time.start();

	OpenVolumeMesh::HalfFaceIter bf_it(temp_VM.halffaces_begin());
	OpenVolumeMesh::HalfFaceVertexIter hfv_it = temp_VM.hfv_iter( *bf_it );
	BoundaryFaceOnShowMesh.clear(); InnerFaceOnShowMesh.clear();
	BoundaryFaceOnShowMesh_VID.clear(); InnerFaceOnShowMesh_VID.clear();
	mesh_.showing_face_id.clear();
	NormalOfBoundaryFFaceOnShowMesh.clear(); NormalOfInnerFFaceOnShowMesh.clear();
	std::vector<OpenVolumeMesh::Geometry::Vec3d> one_face; std::vector<int> one_face_vid;
	OpenVolumeMesh::Geometry::Vec3d v1;
	bool face_ok = true;
	int TwoHalfFaceInValid = 0;
	for (; bf_it != temp_VM.halffaces_end(); ++bf_it)
	{
		OpenVolumeMesh::HalfFaceHandle hfHandle = *bf_it;
		int cell_ID = temp_VM.incident_cell(hfHandle).idx();
		/*if(cell_ID == -1)
		{
			int fuxm = 1;
		}*/
		if( cell_ID>=0 && ValidCell[cell_ID] )
		{
			continue;
		}
	
		if(temp_VM.incident_cell(hfHandle).idx() == -1 && temp_VM.incident_cell(mesh_.opposite_halfface_handle(hfHandle)).idx() == -1)
		{
			++TwoHalfFaceInValid;
			//continue;
		}

		/*if (cell_ID < 0)
		{
			hfHandle = mesh_.opposite_halfface_handle(hfHandle);
		}*/

		hfv_it = temp_VM.hfv_iter( hfHandle );
		one_face.clear(); face_ok = true; one_face_vid.clear();
		for( hfv_it; hfv_it.valid(); ++hfv_it)
		{
			if( !vertex_flag[hfv_it->idx()] )
			{
				face_ok = false;
				break;
			}
			v1 = temp_VM.vertex(*hfv_it);
			one_face.push_back(v1); one_face_vid.push_back(hfv_it->idx());
		}
		if(face_ok)
		{
			if( BoundaryFaceFlag[mesh_.face_handle(*bf_it).idx()] )//if the face is at the boundary
			{
				BoundaryFaceOnShowMesh.push_back(one_face);
				BoundaryFaceOnShowMesh_VID.push_back(one_face_vid);
				NormalOfBoundaryFFaceOnShowMesh.push_back( calculateNormalForQuad(one_face) );
			}
			else
			{
				InnerFaceOnShowMesh.push_back(one_face);
				InnerFaceOnShowMesh_VID.push_back(one_face_vid);
				NormalOfInnerFFaceOnShowMesh.push_back( calculateNormalForQuad(one_face) );
			}
			mesh_.showing_face_id.push_back(bf_it->idx());
		}
	}

	//std::cout << "End constructing the Showing Mesh" <<std::endl;
	//std::cout << "The elapsed time is: " << time.elapsed() << " ms." <<std::endl;
	constructShowMeshFaceCenter();
	//updateIndices();
	//setDrawMode(FLAT_LINES);
	updateGL();
}

OpenVolumeMesh::Geometry::Vec3d MeshViewerWidget::calculateNormalForQuad(std::vector< OpenVolumeMesh::Geometry::Vec3d> one_face)
{
	OpenVolumeMesh::Geometry::Vec3d normal(0.0,0.0,0.0);
	OpenVolumeMesh::Geometry::Vec3d temp_normal(0.0,0.0,0.0);
	OpenVolumeMesh::Geometry::Vec3d v1;OpenVolumeMesh::Geometry::Vec3d v2;
#if 0
	if( one_face.size() == 4)
	{
		int i1 =0; int i2 =0; int i3 =0;
		for( int i=0; i<4; ++i )
		{
			i1 = i; i2 = (i+1)%4; i3 = (i+2)%4;
			v1 = one_face[i1] - one_face[i2];
			v2 = one_face[i3] - one_face[i2];
			temp_normal = (v2 % v1);
			temp_normal /= temp_normal.norm();
			normal += temp_normal;
		}

	}
	else if(one_face.size() == 3)
	{
		v1 = one_face[0] - one_face[1];
		v2 = one_face[2] - one_face[1];
		normal = (v2 % v1);
	}
#endif
	v1 = one_face[0] - one_face[1];
	v2 = one_face[2] - one_face[1];
	normal = (v2 % v1);
	
	normal /= normal.norm();
	return normal;
}


void MeshViewerWidget::constructShowMeshFaceCenter()
{
	FaceCenterOnShowMesh.clear();
	OpenVolumeMesh::Geometry::Vec3d fc;
	for(unsigned int i=0; i<BoundaryFaceOnShowMesh.size(); ++i)
	{
		fc = OpenVolumeMesh::Geometry::Vec3d(0,0,0);
		for(unsigned int j=0;j<BoundaryFaceOnShowMesh[i].size();++j)
		{
			fc += BoundaryFaceOnShowMesh[i][j];
		}
		fc /= BoundaryFaceOnShowMesh[i].size();
		FaceCenterOnShowMesh.push_back(fc);
	}
	for(unsigned int i=0; i<InnerFaceOnShowMesh.size(); ++i)
	{
		fc = OpenVolumeMesh::Geometry::Vec3d(0,0,0);
		for(unsigned int j=0;j<InnerFaceOnShowMesh[i].size();++j)
		{
			fc += InnerFaceOnShowMesh[i][j];
		}
		fc /= InnerFaceOnShowMesh[i].size();
		FaceCenterOnShowMesh.push_back(fc);
	}
}

bool MeshViewerWidget::saveMesh(const char* filename)
{
	//OpenVolumeMesh::IO::FileManager fileManager;
	return fileManager.writeFile(filename , mesh_);
	//return true;
}

bool MeshViewerWidget::saveBoundaryMesh(const char* filename)
{
	return OpenMesh::IO::write_mesh(boundaryMesh, filename);
}

bool MeshViewerWidget::saveMesh_VTK(const char* filename)
{
	FILE* vtk_file = fopen(filename,"w");
	if( !vtk_file )
	{
		std::cout << "can't write file" <<std::endl;
		return false;
	}

	fprintf(vtk_file,"# vtk DataFile Version 3.0\n");
	fprintf(vtk_file,"Volume mesh\n");
	fprintf(vtk_file,"ASCII\n");
	fprintf(vtk_file,"DATASET UNSTRUCTURED_GRID\n");
	fprintf(vtk_file,"POINTS %d double\n",mesh_.n_vertices());
	OpenVolumeMesh::VertexIter v_it = mesh_.vertices_begin();
	OpenVolumeMesh::Geometry::Vec3d v_p;
	for( v_it; v_it != mesh_.vertices_end(); ++v_it )
	{
		v_p = mesh_.vertex(*v_it);
		fprintf(vtk_file,"%20.19f %20.19f %20.19f\n",v_p[0],v_p[1],v_p[2]);
	}
	fprintf(vtk_file,"CELLS %d %d\n",mesh_.n_cells(),mesh_.n_cells()*9);
	std::vector<int> one_cell(8);
	OpenVolumeMesh::CellIter c_it = mesh_.cells_begin();
	OpenVolumeMesh::OpenVolumeMeshCell cell = mesh_.cell(*c_it);
	std::vector<OpenVolumeMesh::HalfFaceHandle> hff_vec;
	for( c_it; c_it != mesh_.cells_end() ; ++c_it)
	{
		cell = mesh_.cell(*c_it);
		hff_vec = cell.halffaces();
		OpenVolumeMesh::HalfFaceVertexIter hfv_it = mesh_.hfv_iter(hff_vec[0]);
		unsigned int i = 0;
		for(hfv_it;hfv_it;++hfv_it)
		{
			one_cell[i] = hfv_it->idx();
			++i;
		}
		for(i=1;i<hff_vec.size();++i)
		{
			OpenVolumeMesh::OpenVolumeMeshFace face = mesh_.face(mesh_.face_handle(hff_vec[i]));
			std::vector< OpenVolumeMesh::HalfEdgeHandle > hef_Vec = face.halfedges();
			for(unsigned int j=0;j<hef_Vec.size();++j)
			{
				OpenVolumeMesh::OpenVolumeMeshEdge edge = mesh_.edge(mesh_.edge_handle(hef_Vec[j]));
				int count = 0; int from = -1; int to = -1; bool edge_ok = false;
				for(unsigned int k=0;k<4;++k)
				{
					if( one_cell[k] == edge.from_vertex().idx())
					{
						from = k;
						++count;
					}
					if( one_cell[k] == edge.to_vertex().idx() )
					{
						to = k;
						++count;
					}
				}
				if(count == 1)
				{
					if( from >= 0 )
					{
						one_cell[from + 4] = edge.to_vertex().idx();
					}
					else if( to >= 0 )
					{
						one_cell[to + 4] = edge.from_vertex().idx();
					}
				}
			}
		}

		fprintf(vtk_file,"8 %d %d %d %d %d %d %d %d\n",one_cell[0],one_cell[1],one_cell[2],one_cell[3],one_cell[4],one_cell[5],one_cell[6],one_cell[7]);
		//fprintf(vtk_file, "8 %d %d %d %d %d %d %d %d\n", one_cell[0], one_cell[3], one_cell[2], one_cell[1], one_cell[4], one_cell[7], one_cell[6], one_cell[5]);
	}//end for all cell

	fprintf(vtk_file,"CELL_TYPES %d\n",mesh_.n_cells());
	c_it = mesh_.cells_begin();
	for(c_it; c_it != mesh_.cells_end(); ++c_it)
	{
		fprintf(vtk_file,"12\n");
	}

	fclose(vtk_file);
	return true;
}

bool MeshViewerWidget::saveMesh_Tet(const char* filename)
{
	FILE* tet_file = fopen(filename,"w");
	if( !tet_file )
	{
		std::cout << "can't write file" <<std::endl;
		return false;
	}

	int nv = mesh_.n_vertices();
	int nc = mesh_.n_cells();
	fprintf(tet_file, "%d vertices\n", nv);
	fprintf(tet_file, "%d cells\n", nc);
	OpenVolumeMesh::VertexIter v_it = mesh_.vertices_begin();
	OpenVolumeMesh::Geometry::Vec3d P;
	for( v_it; v_it != mesh_.vertices_end(); ++v_it )
	{
		P = mesh_.vertex(*v_it);
		fprintf(tet_file, "%20.19f %20.19f %20.19f\n", P[0], P[1], P[2]);
	}

	OpenVolumeMesh::CellIter c_it = mesh_.cells_begin();
	for(c_it; c_it != mesh_.cells_end(); ++c_it)
	{
		OpenVolumeMesh::CellVertexIter cv_it = mesh_.cv_iter(*c_it);
		fprintf(tet_file, "4");
		for(cv_it;cv_it.valid();++cv_it)
		{
			fprintf(tet_file, " %d", cv_it->idx());
		}
		fprintf(tet_file, "\n");
	}
	fclose(tet_file);
	return true;
}

bool MeshViewerWidget::saveMesh_VOFF(const char* filename)
{
	FILE* voff_file = fopen(filename,"w");
	if( !voff_file ) {std::cout << "can't write file" <<std::endl; return false;}

	int nv = mesh_.n_vertices();
	int nc = mesh_.n_cells();
	fprintf(voff_file, "VOFF\n", nv);
	fprintf(voff_file, "%d %d 0 0\n",nv, nc);
	OpenVolumeMesh::VertexIter v_it = mesh_.vertices_begin();
	OpenVolumeMesh::Geometry::Vec3d P;
	for( v_it; v_it != mesh_.vertices_end(); ++v_it )
	{
		P = mesh_.vertex(*v_it);
		fprintf(voff_file, "%f %f %f\n", P[0], P[1], P[2]);
	}

	OpenVolumeMesh::CellIter c_it = mesh_.cells_begin();
	for(c_it; c_it != mesh_.cells_end(); ++c_it)
	{
		if(mesh_mode_ == TETRAHEDRAL)
		{
			OpenVolumeMesh::CellVertexIter cv_it = mesh_.cv_iter(*c_it);
			std::vector<int> vertices;
			for(cv_it;cv_it.valid();++cv_it)
			{
				vertices.push_back(cv_it->idx());
			}
			fprintf(voff_file, "%d", vertices.size());
			for(unsigned int i=0;i<vertices.size();++i)
			{
				fprintf(voff_file, " %d", vertices[i]);
			}
			fprintf(voff_file, "\n");
		}
		else if(mesh_mode_ == HEXAHEDRA)
		{
			std::vector<int> vertices(8);
			OpenVolumeMesh::OpenVolumeMeshCell cell = mesh_.cell(*c_it);
			std::vector<OpenVolumeMesh::HalfFaceHandle> hfh_vec = cell.halffaces();
			OpenVolumeMesh::HalfFaceHandle opp_hf = oppositeHalffaceInCell( hfh_vec[0], &mesh_);
			OpenVolumeMesh::HalfFaceVertexIter hfv_it = mesh_.hfv_iter(hfh_vec[0]); vertices[3] = hfv_it->idx();
			OpenVolumeMesh::VertexHandle vh = adjVertexBetweenTwoFace(hfh_vec[0], *hfv_it, opp_hf, &mesh_); vertices[5] = vh.idx();
			++hfv_it;  vertices[2] = hfv_it->idx();
			vh = adjVertexBetweenTwoFace(hfh_vec[0], *hfv_it, opp_hf, &mesh_); vertices[6] = vh.idx();
			++hfv_it;  vertices[1] = hfv_it->idx();
			vh = adjVertexBetweenTwoFace(hfh_vec[0], *hfv_it, opp_hf, &mesh_); vertices[7] = vh.idx();
			++hfv_it;  vertices[0] = hfv_it->idx();
			vh = adjVertexBetweenTwoFace(hfh_vec[0], *hfv_it, opp_hf, &mesh_); vertices[4] = vh.idx();

			fprintf(voff_file, "%d", vertices.size());
			for(unsigned int i=0;i<vertices.size();++i)
			{
				fprintf(voff_file, " %d", vertices[i]);
			}
			fprintf(voff_file, "\n");
		}
	}
	fclose(voff_file);
	return true;
}

void MeshViewerWidget::clearAllMesh()
{
	clearALLVolumeMesh();
	clearALLBoundaryMesh();
	clearALLShowMesh();
	updateGL();
}

//check the original mesh type
void MeshViewerWidget::checkMeshMode()
{
	OpenVolumeMesh::CellIter cIt = mesh_.cells_begin();
	OpenVolumeMesh::CellIter cEnd = mesh_.cells_end();
	int count = 1;
	int meshType[3] = {0};
	for(cIt; cIt != cEnd; ++cIt)
	{
		count = mesh_.valence(*cIt);
		if(count == 6)
		{
			meshType[1]++;
		}
		else if(count == 4)
		{
			meshType[0]++;
		}
		else
		{
			meshType[2]++;
		}
	}
	int cellNum = mesh_.n_cells();
	if(meshType[1] == cellNum)//hex
	{
		setMeshMode(HEXAHEDRA);
		//compute the scaled jacobian
		printf("Hex Mesh\n");
		compute_scaled_jacobian_hex();
	}
	else if(meshType[0] == cellNum)//tetr
	{
		setMeshMode(TETRAHEDRAL);
		printf("Tet Mesh\n");
	}
	else
	{
		setMeshMode(N_MESH_MODES);
		printf("General Mesh\n");
	}
}

void MeshViewerWidget::compute_scaled_jacobian_hex()
{
	double ave_sj = 0; double count = 0;
	double min_sj = 2.0; Eigen::Matrix3d min_J;
	Eigen::Matrix3d J; 
	std::vector<OpenVolumeMesh::Geometry::Vec3d> three_edge(3);
	std::vector<OpenVolumeMesh::VertexHandle> eight_vertex(8);
	std::vector<OpenVolumeMesh::Geometry::Vec3d> eight_vertex_pos(8);
	OpenVolumeMesh::Geometry::Vec3d temp_pos;
	OpenVolumeMesh::CellIter c_it = mesh_.cells_begin();
	for(c_it; c_it != mesh_.cells_end(); ++c_it)
	{
		OpenVolumeMesh::OpenVolumeMeshCell cell = mesh_.cell(*c_it);
		std::vector<OpenVolumeMesh::HalfFaceHandle> hff_Vec = cell.halffaces();
		OpenVolumeMesh::HalfFaceVertexIter hfv_it = mesh_.hfv_iter(hff_Vec[0]);
		int j = 3;
		for(hfv_it; hfv_it; ++hfv_it)
		{
			eight_vertex[j] = *hfv_it; --j;
		}
		eight_vertex[4] = VertexHandleOnOppositeFace(hff_Vec[0],eight_vertex[0], &mesh_);
		eight_vertex[7] = VertexHandleOnOppositeFace(hff_Vec[0],eight_vertex[1], &mesh_);
		eight_vertex[6] = VertexHandleOnOppositeFace(hff_Vec[0],eight_vertex[2], &mesh_);
		eight_vertex[5] = VertexHandleOnOppositeFace(hff_Vec[0],eight_vertex[3], &mesh_);
		for(unsigned int i=0; i<8; ++i)
		{
			eight_vertex_pos[i] = mesh_.vertex(eight_vertex[i]);
		}
		temp_pos = eight_vertex_pos[1] - eight_vertex_pos[0];
		three_edge[0] = temp_pos.normalize_cond();
		temp_pos = eight_vertex_pos[4] - eight_vertex_pos[0];
		three_edge[1] = temp_pos.normalize_cond();
		temp_pos = eight_vertex_pos[3] - eight_vertex_pos[0];
		three_edge[2] = temp_pos.normalize_cond();
		for(int i=0;i<3;++i)
		{
			J(0,i) = three_edge[i][0]; J(1,i) = three_edge[i][1]; J(2,i) = three_edge[i][2];
		}
		double det_val = J.determinant();
		if( det_val < min_sj )
		{
			min_sj = det_val;
		}
		ave_sj += det_val; count += 1;
		
		three_edge[0] = (eight_vertex_pos[7] - eight_vertex_pos[1]).normalize();
		three_edge[1] = (eight_vertex_pos[0] - eight_vertex_pos[1]).normalize();
		three_edge[2] = (eight_vertex_pos[2] - eight_vertex_pos[1]).normalize();
		for(int i=0;i<3;++i)
		{
			J(0,i) = three_edge[i][0]; J(1,i) = three_edge[i][1]; J(2,i) = three_edge[i][2];
		}
		det_val = J.determinant();
		if( det_val < min_sj )
		{
			min_sj = det_val;
		}
		ave_sj += det_val; count += 1;

		three_edge[0] = (eight_vertex_pos[4] - eight_vertex_pos[7]).normalize();
		three_edge[1] = (eight_vertex_pos[1] - eight_vertex_pos[7]).normalize();
		three_edge[2] = (eight_vertex_pos[6] - eight_vertex_pos[7]).normalize();
		for(int i=0;i<3;++i)
		{
			J(0,i) = three_edge[i][0]; J(1,i) = three_edge[i][1]; J(2,i) = three_edge[i][2];
		}
		det_val = J.determinant();
		if(det_val < min_sj)
		{
			min_sj = det_val;
		}
		ave_sj += det_val; count += 1;

		three_edge[0] = (eight_vertex_pos[0] - eight_vertex_pos[4]).normalize();
		three_edge[1] = (eight_vertex_pos[7] - eight_vertex_pos[4]).normalize();
		three_edge[2] = (eight_vertex_pos[5] - eight_vertex_pos[4]).normalize();
		for(int i=0;i<3;++i)
		{
			J(0,i) = three_edge[i][0]; J(1,i) = three_edge[i][1]; J(2,i) = three_edge[i][2];
		}
		det_val = J.determinant();
		if(det_val < min_sj)
		{
			min_sj = det_val;
		}
		ave_sj += det_val; count += 1;

		three_edge[0] = (eight_vertex_pos[5] - eight_vertex_pos[3]).normalize();
		three_edge[1] = (eight_vertex_pos[2] - eight_vertex_pos[3]).normalize();
		three_edge[2] = (eight_vertex_pos[0] - eight_vertex_pos[3]).normalize();
		for(int i=0;i<3;++i)
		{
			J(0,i) = three_edge[i][0]; J(1,i) = three_edge[i][1]; J(2,i) = three_edge[i][2];
		}
		det_val = J.determinant();
		if(det_val < min_sj)
		{
			min_sj = det_val;
		}
		ave_sj += det_val; count += 1;

		three_edge[0] = (eight_vertex_pos[3] - eight_vertex_pos[2]).normalize();
		three_edge[1] = (eight_vertex_pos[6] - eight_vertex_pos[2]).normalize();
		three_edge[2] = (eight_vertex_pos[1] - eight_vertex_pos[2]).normalize();
		for(int i=0;i<3;++i)
		{
			J(0,i) = three_edge[i][0]; J(1,i) = three_edge[i][1]; J(2,i) = three_edge[i][2];
		}
		det_val = J.determinant();
		if(det_val < min_sj)
		{
			min_sj = det_val;
		}
		ave_sj += det_val; count += 1;

		three_edge[0] = (eight_vertex_pos[2] - eight_vertex_pos[6]).normalize();
		three_edge[1] = (eight_vertex_pos[5] - eight_vertex_pos[6]).normalize();
		three_edge[2] = (eight_vertex_pos[7] - eight_vertex_pos[6]).normalize();
		for(int i=0;i<3;++i)
		{
			J(0,i) = three_edge[i][0]; J(1,i) = three_edge[i][1]; J(2,i) = three_edge[i][2];
		}
		det_val = J.determinant();
		if(det_val < min_sj)
		{
			min_sj = det_val;
		}
		ave_sj += det_val; count += 1;

		three_edge[0] = (eight_vertex_pos[6] - eight_vertex_pos[5]).normalize();
		three_edge[1] = (eight_vertex_pos[3] - eight_vertex_pos[5]).normalize();
		three_edge[2] = (eight_vertex_pos[4] - eight_vertex_pos[5]).normalize();
		for(int i=0;i<3;++i)
		{
			J(0,i) = three_edge[i][0]; J(1,i) = three_edge[i][1]; J(2,i) = three_edge[i][2];
		}
		det_val = J.determinant();
		if(det_val < min_sj)
		{
			min_sj = det_val;
		}
		ave_sj += det_val; count += 1;
	}

	printf("Average Scaled Jacobian: %f \n", ave_sj/count);
	printf("Min Scaled Jacobian: %f \n", min_sj);
	//std::cout << min_J << "\n" << min_J.determinant() << "\n";
}

void MeshViewerWidget::updateIndices()
{
}

void MeshViewerWidget::constructCellCenter()
{
	OpenVolumeMesh::CellIter c_it = mesh_.cells_begin();
	OpenVolumeMesh::CellVertexIter cv_it = mesh_.cv_iter(*c_it);
	OpenVolumeMesh::Geometry::Vec3d center;
	CellCenter.resize(mesh_.n_cells());
	for(; c_it != mesh_.cells_end(); ++c_it)
	{
		cv_it = mesh_.cv_iter(*c_it);
		center = OpenVolumeMesh::Geometry::Vec3d(0,0,0);
		int vertexCount = 0;
		for(cv_it; cv_it; ++cv_it)
		{
			center +=  mesh_.vertex(*cv_it);
			++vertexCount;
		}
		center /= vertexCount;

		CellCenter[c_it->idx()] = center;
	}
}

bool MeshViewerWidget::SaveScreen(const char* filePath)
{
	QImage image = grabFrameBuffer();
	return image.save(filePath);
}

void MeshViewerWidget::save_mesh_graph(const char* filename)
{
	FILE* f_g = fopen(filename, "w");

	unsigned nv = mesh_.n_vertices();
	fprintf(f_g, "%d", nv);

	OpenVolumeMesh::VertexHandle tvh;
	for (OpenVolumeMesh::VertexIter v_it = mesh_.vertices_begin(); v_it != mesh_.vertices_end(); ++v_it)
	{
		OpenVolumeMesh::VertexHandle vh = v_it.cur_handle();
		fprintf(f_g, "\n%d", mesh_.valence(vh));
		for (OpenVolumeMesh::VertexOHalfEdgeIter voh_it = mesh_.voh_iter(vh); voh_it; ++voh_it)
		{
			OpenVolumeMesh::OpenVolumeMeshEdge e = mesh_.edge(mesh_.edge_handle(voh_it.cur_handle()));
			if (e.from_vertex() == vh) tvh = e.to_vertex();
			else tvh = e.from_vertex();
			fprintf(f_g, " %d", tvh.idx());
		}
	}

	fclose(f_g);
}

void MeshViewerWidget::save_hex_mesh_graph(const char* filename)
{
	FILE* f_g = fopen(filename, "w");

	unsigned nv = mesh_.n_vertices();
	fprintf(f_g, "%d", nv);

	OpenVolumeMesh::VertexHandle tvh;
	std::vector<int> face_flag(mesh_.n_faces(), -1); std::vector<int> vertex_face;
	std::vector<int> vertex_flag(mesh_.n_vertices(), -1); std::vector<int> vertex_vertex;
	for (OpenVolumeMesh::VertexIter v_it = mesh_.vertices_begin(); v_it != mesh_.vertices_end(); ++v_it)
	{
		OpenVolumeMesh::VertexHandle vh = v_it.cur_handle();
		vertex_face.clear(); vertex_vertex.clear();
		for (OpenVolumeMesh::VertexOHalfEdgeIter voh_it = mesh_.voh_iter(vh); voh_it; ++voh_it)
		{
			for (OpenVolumeMesh::HalfEdgeHalfFaceIter hehf_it = mesh_.hehf_iter(*voh_it); hehf_it; ++hehf_it)
			{
				int face_id = mesh_.face_handle(*hehf_it).idx();
				if (face_flag[face_id] == -1)
				{
					vertex_face.push_back(face_id);
					face_flag[face_id] = 1;
				}
			}
		}

		for (int i = 0; i < vertex_face.size(); ++i)
		{
			OpenVolumeMesh::HalfFaceHandle hfh = mesh_.halfface_handle(OpenVolumeMesh::FaceHandle(vertex_face[i]), 0);
			for (OpenVolumeMesh::HalfFaceVertexIter hfv_it = mesh_.hfv_iter(hfh); hfv_it; ++hfv_it)
			{
				int v_id = hfv_it->idx();
				if (vertex_flag[v_id] == -1 && v_id != vh.idx())
				{
					vertex_vertex.push_back(v_id);
					vertex_flag[v_id] = 1;
				}
			}
		}

		for (int i = 0; i < vertex_face.size(); ++i)
		{
			face_flag[vertex_face[i]] = -1;
		}
		fprintf(f_g, "\n%d", vertex_vertex.size());
		for (int i = 0; i < vertex_vertex.size(); ++i)
		{
			fprintf(f_g, " %d", vertex_vertex[i]);
			vertex_flag[vertex_vertex[i]] = -1;
		}
	}
	fclose(f_g);
}

void MeshViewerWidget::construct_vertex_texture()
{
	int nv = mesh_.n_vertices();
	if (nv == 0) return;

	vertex_texture.resize(nv);
	OpenVolumeMesh::Geometry::Vec3d box_width = mesh_.BBox_max - mesh_.BBox_min;
	double width = box_width.max() + 1e-14; double inv_width = 1.0 / width;

	for (OpenVolumeMesh::VertexIter v_it = mesh_.vertices_begin(); v_it != mesh_.vertices_end(); ++v_it)
	{
		int v_id = v_it->idx();
		OpenVolumeMesh::Geometry::Vec3d p = mesh_.vertex(*v_it);
		vertex_texture[v_id] = (p - mesh_.BBox_min)*inv_width;
	}

	PFNGLTEXIMAGE3DPROC glTexImage3D;
	//simple texture
	glEnable(GL_TEXTURE_3D);
	glTexImage3D = (PFNGLTEXIMAGE3DPROC)context()->getProcAddress("glTexImage3D");

	int square_length = 256; int interval = 16;
	glBindTexture(GL_TEXTURE_3D, Verterx_Texture_ID);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	unsigned char* RGBA = new unsigned char[4 * square_length*square_length* square_length];
	for (int i = 0; i < square_length; i++)
	{
		for (int j = 0; j < square_length; j++)
		{
			for (int k = 0; k < square_length; ++k)
			{
				int id = i*square_length*square_length + j * square_length + k;

				if ((i / interval + j / interval + k / interval) % 2 == 1)
				{
					RGBA[id * 4 + 0] = 200; RGBA[id * 4 + 1] = 200;
					RGBA[id * 4 + 2] = 200; RGBA[id * 4 + 3] = 255;
				}
				else
				{
					RGBA[id * 4 + 0] = 0; RGBA[id * 4 + 1] = 125;
					RGBA[id * 4 + 2] = 65; RGBA[id * 4 + 3] = 255;
				}
			}
		}
	}

	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, square_length, square_length, square_length, 0, GL_RGBA, GL_UNSIGNED_BYTE, RGBA);

	glDisable(GL_TEXTURE_3D);
}
